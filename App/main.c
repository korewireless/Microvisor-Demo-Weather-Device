/**
 *
 * Microvisor Weather Device Demo
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#include "main.h"
#include "app_version.h"


/*
 * STATIC PROTOTYPES
 */
static void system_clock_config(void);
static void GPIO_init(void);
static void task_led(void *unused_arg);
static void task_iot(void *unused_arg);
static void process_http_response(void);
static void log_device_info(void);
static void do_polite_deploy(void* arg);


/*
 *  GLOBALS
 */
// This is the FreeRTOS thread task that flashed the USER LED
// and operates the display
static osThreadId_t thread_led;
static const osThreadAttr_t led_task_attributes = {
    .name = "LEDTask",
    .stack_size = 5120,
    .priority = (osPriority_t)osPriorityNormal
};

// This is the FreeRTOS thread task that reads the sensor
// and displays the temperature on the LED
static osThreadId_t thread_iot;
static const osThreadAttr_t iot_task_attributes = {
    .name = "IOTTask",
    .stack_size = 5120,
    .priority = (osPriority_t)osPriorityNormal
};

// I2C-related values
I2C_HandleTypeDef i2c;
char forecast[48] = "None";

/**
 *  Theses variables may be changed by interrupt handler code,
 *  so we mark them as `volatile` to ensure compiler optimization
 *  doesn't render them immutable at runtime
 */
volatile bool           use_i2c = false;
volatile uint8_t        icon_code = 12;
volatile bool           new_forecast = false;
volatile bool           received_request = false;
volatile bool           channel_was_closed = false;
volatile bool           polite_deploy = false;

static volatile bool    is_connected = false;
static volatile bool    net_changed = false;
static bool             flash_led = false;

/**
 * These variables are defined in `http.c`
 */
extern struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
    MvChannelHandle      channel;
} http_handles;


/**
 * @brief The application entry point.
 */
int main(void) {

    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Configure the system clock
    system_clock_config();

    // Get the Device ID and build number
    log_device_info();

    // Start the network
    net_open_network();
    shared_setup_notification_center();

    // Initialize the peripherals
    GPIO_init();
    I2C_init();

    // Set up the display if it's available
    if (use_i2c) {
        HT16K33_init(2);

        // Set the weather icons
        HT16K33_define_character("\x91\x42\x18\x3d\xbc\x18\x42\x89", CLEAR_DAY);
        HT16K33_define_character("\x31\x7A\x78\xFA\xFC\xF9\x7A\x30", RAIN);
        HT16K33_define_character("\x31\x7A\x78\xFA\xFC\xF9\x7A\x30", DRIZZLE);
        HT16K33_define_character("\x28\x92\x54\x38\x38\x54\x92\x28", SNOW);
        HT16K33_define_character("\x32\x7D\x7A\xFD\xFA\xFD\x7A\x35", SLEET);
        HT16K33_define_character("\x28\x28\x28\x28\x28\xAA\xAA\x44", WIND);
        HT16K33_define_character("\xAA\x55\xAA\x55\xAA\x55\xAA\x55", FOG);
        HT16K33_define_character("\x30\x78\x78\xF8\xF8\xF8\x78\x30", CLOUDY);
        HT16K33_define_character("\x30\x48\x48\x88\x88\x88\x48\x30", PARTLY_CLOUDY);
        HT16K33_define_character("\x00\x00\x00\x0F\x38\xE0\x00\x00", THUNDERSTORM);
        HT16K33_define_character("\x00\x40\x6C\xBE\xBB\xB1\x60\x40", TORNADO);
        HT16K33_define_character("\x3C\x42\x81\xC3\xFF\xFF\x7E\x3C", CLEAR_NIGHT);
        HT16K33_define_character("\x00\x00\x40\x9D\x90\x60\x00\x00", NONE);

        char* title = malloc(42);
        sprintf(title, "    %s %s    ", APP_NAME, APP_VERSION);
        HT16K33_print(title, 75);
        free(title);
    }

    // Init scheduler
    osKernelInitialize();

    // Create the thread(s)
    thread_iot = osThreadNew(task_iot, NULL, &iot_task_attributes);
    thread_led = osThreadNew(task_led, NULL, &led_task_attributes);

    // Start the scheduler
    osKernelStart();

    // We should never get here as control is now taken by the scheduler,
    // but just in case...
    while (1) {
        // NOP
    }
}


/**
 * @brief Get the MV clock value.
 *
 * @returns The clock value.
 */
uint32_t SECURE_SystemCoreClockUpdate() {

    uint32_t clock = 0;
    mvGetHClk(&clock);
    return clock;
}


/**
 * @brief System clock configuration.
 */
static void system_clock_config(void) {

    SystemCoreClockUpdate();
    HAL_InitTick(TICK_INT_PRIORITY);
}


/**
 * @brief Initialize the MCU GPIO
 *
 * Used to flash the Nucleo's USER LED, which is on GPIO Pin PA5.
 */
static void GPIO_init(void) {

    // Enable GPIO port clock
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure GPIO pin output Level
    HAL_GPIO_WritePin(LED_GPIO_BANK, LED_GPIO_PIN, GPIO_PIN_RESET);

    // Configure GPIO pin PA5
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin   = LED_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(LED_GPIO_BANK, &GPIO_InitStruct);
}


/**
 * @brief Function implementing the display task thread.
 *
 * @param *unused_arg: Not used.
 */
static void task_led(void *unused_arg) {

    uint32_t last_tick = 0;
    osTimerId_t polite_timer;
    bool connection_pixel_state = false;

    // The task's main loop
    while (1) {
        // Check connection state
        is_connected = false;
        if (http_handles.network != 0) {
            enum MvNetworkStatus net_state = MV_NETWORKSTATUS_DELIBERATELYOFFLINE;
            enum MvStatus status = mvGetNetworkStatus(http_handles.network, &net_state);

            if (status == MV_STATUS_OKAY) {
                is_connected = (net_state == MV_NETWORKSTATUS_CONNECTED);
            }
        }

        // Periodically update the display and flash the USER LED
        uint32_t tick = HAL_GetTick();
        if (tick - last_tick > DEFAULT_TASK_PAUSE_MS) {
            last_tick = tick;

            if (flash_led) {
                HAL_GPIO_TogglePin(LED_GPIO_BANK, LED_GPIO_PIN);
            }

            if (use_i2c) {
                if (new_forecast) {
                    // Display the new forecast as a string
                    HT16K33_print(forecast, 100);

                    // Wait before showing the icon
                    sleep_ms(1500);
                    new_forecast = false;
                }

                // Set the top right pixel to flash when
                // the device is disconnected.
                if (!is_connected) {
                    connection_pixel_state = !connection_pixel_state;
                } else {
                    connection_pixel_state = false;
                }

                // Draw the weather icon
                HT16K33_draw_def_char(icon_code);
                HT16K33_plot(7, 7, connection_pixel_state);
                HT16K33_draw();
            }
        }

        // FROM 3.3.0
        // Check if the polite deployment flag has been set
        // via a Microvisor system notification
        if (polite_deploy) {
            server_log("Polite deployment notification issued");
            polite_deploy = false;
            flash_led = true;

            // Set up a 30s timer to trigger the update
            // NOTE In a real-world application, you would apply the update
            //      see `do_polite_deploy()` as soon as any current critical task
            //      completes. Here we just demo the process using a HAL timer.
            polite_timer = osTimerNew(do_polite_deploy, osTimerOnce, NULL, NULL);
            const uint32_t timer_delay_s = 30;
            if (polite_timer != NULL && osTimerStart(polite_timer, timer_delay_s * 1000) == osOK) {
                server_log("Update will install in %lu seconds", timer_delay_s);
            }
        }

        // End of cycle delay
        osDelay(10);
    }
}


/**
 * @brief Function implementing the periodic weather conditions thread.
 *
 * @param *unused_arg: Not used.
 */
static void task_iot(void *unused_arg) {

    // Configure OpenWeather
    // NOTE These values derived from env vars -- see README.md
    OW_init(LATITUDE, LONGITUDE);

    // Time trackers
    uint32_t read_tick = HAL_GetTick() - WEATHER_READ_PERIOD_MS;
    uint32_t kill_time = 0;
    bool do_close_channel = false;

    // Run the thread's main loop
    while (1) {
        uint32_t tick = HAL_GetTick();
        if (tick - read_tick > WEATHER_READ_PERIOD_MS) {
            read_tick = tick;

            // No channel open? Try and send the temperature
            if (http_handles.channel == 0) {
                http_open_channel();
                bool result = OW_request_forecast();
                if (!result) do_close_channel = true;
                kill_time = tick;
            } else {
                server_error("Channel handle not zero");
            }
        }

        // Process a request's response if indicated by the ISR
        if (received_request) process_http_response();


        // FROM 2.0.7
        // Was the channel closed unexpectedly?
        // `channel_was_closed` set in IRS
        if (channel_was_closed) do_close_channel = true;

        // Use 'kill_time' to force-close an open HTTP channel
        // if it's been left open too long
        if (kill_time > 0 && tick - kill_time > CHANNEL_KILL_PERIOD_MS) {
            do_close_channel = true;
            server_error("HTTP request timed out");
        }

        // Close the channel if asked to do so or
        // a request yielded a response
        if (do_close_channel || received_request) {
            do_close_channel = false;
            received_request = false;
            kill_time = 0;
            http_close_channel();
        }

        // End of cycle delay
        osDelay(10);
    }
}


/**
 * @brief Process HTTP response data.
 */
static void process_http_response(void) {

    // We have received data via the active HTTP channel so establish
    // an `MvHttpResponseData` record to hold response metadata
    struct MvHttpResponseData resp_data;
    enum MvStatus status = mvReadHttpResponseData(http_handles.channel, &resp_data);
    if (status == MV_STATUS_OKAY) {
        // Check we successfully issued the request (`result` is OK) and
        // the request was successful (status code 200)
        if (resp_data.result == MV_HTTPRESULT_OK) {
            if (resp_data.status_code == 200) {
                server_log("HTTP response body length: %lu", resp_data.body_length);

                // Set up a buffer that we'll get Microvisor
                // to write the response body into
                static uint8_t body_buffer[1500];
                memset((void *)body_buffer, 0x00, 1500);
                status = mvReadHttpResponseBody(http_handles.channel, 0, body_buffer, 1500);
                if (status == MV_STATUS_OKAY) {
                    uint32_t wid = 0;
                    uint32_t code = NONE;
                    double temp = 0.0;
                    char cast[14] = "None";

                    // Parse the incoming JSON using cJSON
                    // (https://github.com/DaveGamble/cJSON)
                    cJSON *json = cJSON_Parse((char *)body_buffer);
                    if (json == NULL) {
                        // Parsing failed -- log an error and bail
                        const char *error_ptr = cJSON_GetErrorPtr();
                        if (error_ptr != NULL) {
                            server_error("Cant parse JSON, before %s", error_ptr);
                        }
                        cJSON_Delete(json);
                        return;
                    }

                    // Extract current weather conditions from parsed JSON
                    const cJSON *current = cJSON_GetObjectItemCaseSensitive(json, "current");
                    const cJSON *weather = cJSON_GetObjectItemCaseSensitive(current, "weather");
                    const cJSON *feels_like;

                    if (weather != NULL) {
                        cJSON *item = NULL;
                        cJSON_ArrayForEach(item, weather) {
                            // Get the info we're interested in
                            const cJSON *icon = cJSON_GetObjectItemCaseSensitive(item, "icon");
                            const cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
                            const cJSON *main= cJSON_GetObjectItemCaseSensitive(item, "main");
                            feels_like = cJSON_GetObjectItemCaseSensitive(current, "feels_like");

                            // Set working values
                            if (cJSON_IsNumber(id)) wid = (int)id->valuedouble;
                            if (cJSON_IsString(main) && (main->valuestring != NULL)) {
                                strcpy(cast, main->valuestring);
                            }

                            // Set standard icon values by weather condition
                            if (strcmp(cast, "Rain") == 0) {
                                code = RAIN;
                            } else if (strcmp(cast, "Snow") == 0) {
                                code = SNOW;
                            } else if (strcmp(cast, "Thun") == 0) {
                                code = THUNDERSTORM;
                            }

                            // Update icons and/or condition text for certain
                            // quirky ID values
                            if (wid == 771) {
                                strcpy(cast, "Windy");
                                code = WIND;
                            }

                            if (wid == 871) {
                                strcpy(cast, "Tornado");
                                code = TORNADO;
                            }

                            if (wid > 699 && wid < 770) {
                                strcpy(cast, "Foggy");
                                code = FOG;
                            }

                            if (strcmp(cast, "Clouds") == 0) {
                                if (wid < 804) {
                                    strcpy(cast, "Partly Cloudy");
                                    code = PARTLY_CLOUDY;
                                } else {
                                    strcpy(cast, "Cloudy");
                                    code = CLOUDY;
                                }
                            }

                            if (wid > 602 && wid < 620) {
                                strcpy(cast, "Sleet");
                                code = SLEET;
                            }

                            if (strcmp(cast, "Drizzle") == 0) {
                                code = DRIZZLE;
                            }

                            if (strcmp(cast, "Clear") == 0) {
                                if (cJSON_IsString(icon) && (icon->valuestring != NULL)) {
                                    if (icon->valuestring[2] == 'd') {
                                        code = CLEAR_DAY;
                                    } else {
                                        code = CLEAR_NIGHT;
                                    }
                                }
                            }
                        }
                    }

                    // Did we get updated weather info?
                    if (wid > 0) {
                        // Yes! So update the forecast string, and tell the main loop
                        // to refresh the display
                        if (cJSON_IsNumber(feels_like)) {
                            temp = feels_like->valuedouble;
                        }

                        sprintf(forecast, "    %s Out: %.1f", cast, temp);
                        sprintf(&forecast[strlen(forecast)], "\x7F\x63\x20\x20\x20\x20");
                        icon_code = code;
                        new_forecast = true;
                    }

                    // Free the JSON parser
                    cJSON_Delete(json);
                    server_log("Forecast: %s (code: %lu) Feels Like %.1f°C", cast, code, temp);
                } else {
                    server_error("HTTP response body read status %i", status);
                }
            } else {
                server_error("HTTP status code: %lu", resp_data.status_code);
            }
        } else {
            server_error("Request failed. Status: %i", resp_data.result);
        }
    } else {
        server_error("Response data read failed. Status: %i", status);
    }
}


/**
 * @brief Show basic device info.
 */
static void log_device_info(void) {

    uint8_t buffer[35] = { 0 };
    mvGetDeviceId(buffer, 34);
    server_log("Device: %s", buffer);
    server_log("   App: %s %s-%u", APP_NAME, APP_VERSION, BUILD_NUM);
}


/**
 * @brief Sleep for a fixed period. Blocks
 *
 * @param ms: A sleep period in ms.
 */
void sleep_ms(uint32_t ms) {

    uint32_t tick = HAL_GetTick();
    while (1) {
        if (HAL_GetTick() - tick > ms) break;
    }
}


/**
 * @brief A CMSIS/FreeRTOS timer callback function.
 *
 * This is called when the polite deployment timer (see `task_led()`) fires.
 * It tells Microvisor to apply the application update that has previously
 * been signalled as ready to be deployed.
 *
 * `mvRestart()` should cause the application to be torn down and restarted,
 * but it's important to check the returned value in case Microvisor was not
 * able to perform the restart for some reason.
 *
 * @param arg: Pointer to and argument value passed by the timer controller.
 *             Unused here.
 */
static void do_polite_deploy(void* arg) {

    enum MvStatus status = mvRestart(MV_RESTARTMODE_AUTOAPPLYUPDATE);
    if (status != MV_STATUS_OKAY) {
        server_error("Could not apply update (%lu)", (uint32_t)status);
        flash_led = false;
    }
}