/**
 *
 * Microvisor Weather Device Demo
 * Version 1.3.5
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


/**
 *  GLOBALS
 */

// This is the FreeRTOS thread task that flashed the USER LED
// and operates the display
osThreadId_t LEDTask;
const osThreadAttr_t led_task_attributes = {
    .name = "LEDTask",
    .stack_size = 2048,
    .priority = (osPriority_t) osPriorityNormal
};

// This is the FreeRTOS thread task that reads the sensor
// and displays the temperature on the LED
osThreadId_t IOTTask;
const osThreadAttr_t iot_task_attributes = {
    .name = "IOTTask",
    .stack_size = 4096,
    .priority = (osPriority_t) osPriorityNormal
};

// I2C-related values
I2C_HandleTypeDef i2c;
char forecast[32] = "None";

/**
 *  Theses variables may be changed by interrupt handler code,
 *  so we mark them as `volatile` to ensure compiler optimization
 *  doesn't render them immutable at runtime
 */
volatile bool       use_i2c = false;
volatile bool       request_recv = false;
volatile bool       new_forecast = false;
volatile bool       is_connected = false;
volatile bool       net_changed = false;
volatile uint8_t    icon_code = 12;

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

    // Initialize the peripherals
    GPIO_init();
    I2C_init();

    // Set up the display if it's available
    if (use_i2c) {
        HT16K33_init(2);
        char* title = malloc(42);
        sprintf(title, "    %s %s (%02i)    ", APP_NAME, APP_VERSION, BUILD_NUM);
        HT16K33_print(title, 75);
        free(title);

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
    }

    // Init scheduler
    osKernelInitialize();

    // Create the thread(s)
    IOTTask = osThreadNew(start_iot_task, NULL, &iot_task_attributes);
    LEDTask = osThreadNew(start_led_task, NULL, &led_task_attributes);

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
 * @retval The clock value.
 */
uint32_t SECURE_SystemCoreClockUpdate() {
    uint32_t clock = 0;
    mvGetHClk(&clock);
    return clock;
}


/**
 * @brief System clock configuration.
 */
void system_clock_config(void) {
    SystemCoreClockUpdate();
    HAL_InitTick(TICK_INT_PRIORITY);
}


/**
 * @brief Initialize the MCU GPIO
 *
 * Used to flash the Nucleo's USER LED, which is on GPIO Pin PA5.
 */
void GPIO_init(void) {
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
void start_led_task(void *unused_arg) {
    uint32_t last_tick = 0;
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
            HAL_GPIO_TogglePin(LED_GPIO_BANK, LED_GPIO_PIN);

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

        // End of cycle delay
        osDelay(10);
    }
}


/**
 * @brief Function implementing the Debug Task thread.
 *
 * @param *unused_arg: Not used.
 */
void start_iot_task(void *unused_arg) {
    // Get the Device ID and build number
    log_device_info();

    // Set up channel notifications
    http_channel_center_setup();

    // Configure OpenWeather
    // NOTE These values derived from env vars -- see README.md
    OW_init(API_KEY, LATITUDE, LONGITUDE);

    // Time trackers
    uint32_t read_tick = HAL_GetTick() - WEATHER_READ_PERIOD_MS;
    uint32_t kill_time = 0;
    bool close_channel = false;

    // Run the thread's main loop
    while (1) {
        uint32_t tick = HAL_GetTick();
        if (tick - read_tick > WEATHER_READ_PERIOD_MS) {
            read_tick = tick;

            // No channel open? Try and send the temperature
            if (http_handles.channel == 0) {
                http_open_channel();
                bool result = OW_request_forecast();
                if (!result) close_channel = true;
                kill_time = tick;
            } else {
                server_error("Channel handle not zero");
            }
        }

        // Process a request's response if indicated by the ISR
        if (request_recv) {
            http_process_response();
        }

        // Use 'kill_time' to force-close an open HTTP channel
        // if it's been left open too long
        if (kill_time > 0 && tick - kill_time > CHANNEL_KILL_PERIOD_MS) {
            close_channel = true;
        }

        // Close the channel if asked to do so or
        // a request yielded a response
        if (close_channel || request_recv) {
            close_channel = false;
            request_recv = false;
            kill_time = 0;
            http_close_channel();
        }

        // End of cycle delay
        osDelay(10);
    }
}


/**
 * @brief Show basic device info.
 */
void log_device_info(void) {
    uint8_t buffer[35] = { 0 };
    mvGetDeviceId(buffer, 34);
    printf("Device: %s\n   App: %s\n Build: %i\n", buffer, APP_NAME, BUILD_NUM);
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
 * @brief Issue debug message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void server_log(char* format_string, ...) {
    if (LOG_DEBUG_MESSAGES) {
        va_list args;
        char buffer[512] = "[DEBUG] ";
        va_start(args, format_string);
        vsprintf(&buffer[8], format_string, args);
        va_end(args);
        printf("%s\n", buffer);
    }
}


/**
 * @brief Issue error message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void server_error(char* format_string, ...) {
    va_list args;
    char buffer[512] = "[ERROR] ";
    va_start(args, format_string);
    vsprintf(&buffer[8], format_string, args);
    va_end(args);
    printf("%s\n", buffer);
}
