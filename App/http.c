/**
 *
 * Microvisor Weather Device Demo
 * Version 2.0.1
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


// Central store for Microvisor resource handles used in this code.
// See `https://www.twilio.com/docs/iot/microvisor/syscalls#http_handles`
struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
    MvChannelHandle      channel;
} http_handles = { 0, 0, 0 };

// Central store for notification records.
// Holds one record at a time -- each record is 16 bytes.
volatile struct MvNotification http_notification_center[16];

// Defined in `main.c`
extern volatile bool        request_recv;
extern volatile bool        new_forecast;
extern volatile uint32_t    icon_code;
extern          char        forecast[32];


/**
 *  @brief Open a new HTTP channel.
 *
 *  @retval `true` if the channel is open, otherwise `false`.
 */
bool http_open_channel(void) {
    // Set up the HTTP channel's multi-use send and receive buffers
    static volatile uint8_t http_rx_buffer[HTTP_RX_BUFFER_SIZE_B] __attribute__((aligned(512)));
    static volatile uint8_t http_tx_buffer[HTTP_TX_BUFFER_SIZE_B] __attribute__((aligned(512)));
    static const char endpoint[] = "";

    // Get the network channel handle.
    // NOTE This is set in `logging.c` which puts the network in place
    //      (ie. so the network handle != 0) well in advance of this being called
    http_handles.network = get_net_handle();
    if (http_handles.network == 0) return false;
    server_log("Network handle: %lu", (uint32_t)http_handles.network);

    // Configure the required data channel
    struct MvOpenChannelParams channel_config = {
        .version = 1,
        .v1 = {
            .notification_handle = http_handles.notification,
            .notification_tag    = USER_TAG_HTTP_OPEN_CHANNEL,
            .network_handle      = http_handles.network,
            .receive_buffer      = (uint8_t*)http_rx_buffer,
            .receive_buffer_len  = sizeof(http_rx_buffer),
            .send_buffer         = (uint8_t*)http_tx_buffer,
            .send_buffer_len     = sizeof(http_tx_buffer),
            .channel_type        = MV_CHANNELTYPE_HTTP,
            .endpoint            = (uint8_t*)endpoint,
            .endpoint_len        = 0
        }
    };

    // Ask Microvisor to open the channel
    // and confirm that it has accepted the request
    enum MvStatus status = mvOpenChannel(&channel_config, &http_handles.channel);
    if (status == MV_STATUS_OKAY) {
        server_log("HTTP channel open. Handle: %lu", (uint32_t)http_handles.channel);
        return true;
    } else {
        server_error("HTTP channel closed. Status: %i", status);
    }

    return false;
}


/**
 *  @brief Close the currently open HTTP channel.
 */
void http_close_channel(void) {
    // If we have a valid channel handle -- ie. it is non-zero --
    // then ask Microvisor to close it and confirm acceptance of
    // the closure request.
    if (http_handles.channel != 0) {
        enum MvStatus status = mvCloseChannel(&http_handles.channel);
        assert((status == MV_STATUS_OKAY || status == MV_STATUS_CHANNELCLOSED) && "[ERROR] Channel closure");
        server_log("HTTP channel closed");
    }

    // Confirm the channel handle has been invalidated by Microvisor
    assert(http_handles.channel == 0 && "[ERROR] Channel handle not zero");
}


/**
 * @brief Configure the channel Notification Center.
 */
void http_channel_center_setup(void) {
    // Clear the notification store
    memset((void *)http_notification_center, 0xFF, sizeof(http_notification_center));

    // Configure a notification center for network-centric notifications
    static struct MvNotificationSetup http_notification_setup = {
        .irq = TIM8_BRK_IRQn,
        .buffer = (struct MvNotification *)http_notification_center,
        .buffer_size = sizeof(http_notification_center)
    };

    // Ask Microvisor to establish the notification center
    // and confirm that it has accepted the request
    enum MvStatus status = mvSetupNotifications(&http_notification_setup, &http_handles.notification);
    assert(status == MV_STATUS_OKAY && "[ERROR] Could not set up HTTP channel NC");

    // Start the notification IRQ
    NVIC_ClearPendingIRQ(TIM8_BRK_IRQn);
    NVIC_EnableIRQ(TIM8_BRK_IRQn);
    server_log("Notification center handle: %lu", (uint32_t)http_handles.notification);
}


/**
 * @brief Send a stock HTTP request.
 *
 * @retval `true` if the request was accepted by Microvisor, otherwise `false`
 */
bool http_send_request(const char* url) {
    // Check for a valid channel handle
    if (http_handles.channel != 0) {
        server_log("Sending HTTP request");

        // Set up the request
        const char verb[] = "GET";
        const char body[] = "";
        struct MvHttpHeader hdrs[] = {};
        struct MvHttpRequest request_config = {
            .method = (uint8_t *)verb,
            .method_len = strlen(verb),
            .url = (uint8_t *)url,
            .url_len = strlen(url),
            .num_headers = 0,
            .headers = hdrs,
            .body = (uint8_t *)body,
            .body_len = strlen(body),
            .timeout_ms = 10000
        };

        // Issue the request -- and check its status
        enum MvStatus status = mvSendHttpRequest(http_handles.channel, &request_config);
        if (status == MV_STATUS_OKAY) {
            server_log("Request sent to Twilio");
            return true;
        }

        // Report send failure
        server_error("Could not issue request. Status: %i", status);
        return false;
    }

    // There's no open channel, so open open one now and
    // try to send again
    http_open_channel();
    return http_send_request(url);
}


/**
 * @brief Process HTTP response data.
 */
void http_process_response(void) {
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

                        sprintf(forecast, "    %s Out: %.1fc    ", cast, temp);
                        icon_code = code;
                        new_forecast = true;
                    }

                    // Free the JSON parser
                    cJSON_Delete(json);
                    server_log("Forecast: %s (code: %lu) Feels Like %.1fc", cast, code, temp);
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
 * @brief The HTTP channel notification interrupt handler.
 *
 * This is called by Microvisor -- we need to check for key events
 * and extract HTTP response data when it is available.
 */
void TIM8_BRK_IRQHandler(void) {
    // Get the event type
    enum MvEventType event_kind = http_notification_center->event_type;

    if (event_kind == MV_EVENTTYPE_CHANNELDATAREADABLE) {
        // Flag we need to access received data and to close the HTTP channel
        // when we're back in the main loop. This lets us exit the ISR quickly.
        // We should not make Microvisor System Calls in the ISR.
        request_recv = true;
    }
}
