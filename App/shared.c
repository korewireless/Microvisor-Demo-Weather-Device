/**
 *
 * Microvisor Weather Device Demo
 * Version 3.1.0
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


/*
 * GLOBALS
 */
// The shared Notification Center's handle.
// This is zero when the NC is not yet established.
// See `https://www.twilio.com/docs/iot/microvisor/syscalls#http_handles`
static MvNotificationHandle notification_handle = 0;

// Central store for notification records.
// Holds SHARED_NC_BUFFER_SIZE_R records at a time -- each record is 16 bytes in size.
static volatile struct MvNotification shared_notification_center[SHARED_NC_BUFFER_SIZE_R] __attribute__((aligned(8)));
static volatile uint32_t notification_index = 0;

// Defined in `http.c` and `config.c`
extern volatile bool received_request;
extern volatile bool received_config;
extern volatile bool channel_was_closed;



/**
 * @brief Provide the shared Notification Center's handle.
 *
 * @returns The shared Notification Center's handle.
 */
MvNotificationHandle shared_get_handle(void) {
    
    return notification_handle;
}


/**
 * @brief Configure the shared Notification Center.
 *
 * @returns `true` of the center was established, otherwise `false`.
 */
bool shared_setup_notification_center(void) {
    
    // Clear the notification store
    memset((void *)shared_notification_center, 0xFF, sizeof(shared_notification_center));

    // Configure a notification center shared across HTTP and Config
    static struct MvNotificationSetup shared_notification_setup = {
        .irq = TIM8_BRK_IRQn,
        .buffer = (struct MvNotification *)shared_notification_center,
        .buffer_size = sizeof(shared_notification_center)
    };

    // Ask Microvisor to establish the notification center
    // and confirm that it has accepted the request
    enum MvStatus status = mvSetupNotifications(&shared_notification_setup, &notification_handle);
    if (status != MV_STATUS_OKAY) {
        server_error("Could not set up shared NC");
        return false;
    }

    // Start the notification IRQ
    NVIC_ClearPendingIRQ(TIM8_BRK_IRQn);
    NVIC_EnableIRQ(TIM8_BRK_IRQn);
    server_log("Shared NC handle: %lu", (uint32_t)notification_handle);
    return true;
}


/**
 * @brief The shared channel notification interrupt handler.
 *
 * This is called by Microvisor. We need to check for key events,
 * and signal the app via flags that data is available. This center
 * handles notifications from two channels, for config fetches and
 * HTTP requests, so we use tags to determine the source channel.
 */
void TIM8_BRK_IRQHandler(void) {
    
    // Check for readable data in the HTTP channel
    bool got_notification = false;
    volatile struct MvNotification notification = shared_notification_center[notification_index];
    switch(notification.tag) {
        // Config fetch channel notifications
        case TAG_CHANNEL_CONFIG:
            if (notification.event_type == MV_EVENTTYPE_CHANNELDATAREADABLE) {
                // Flag we need to access received data and to close the channel
                // when we're back in the main loop. This lets us exit the ISR quickly.
                // Do NOT make Microvisor System Calls in the ISR!
                received_config = true;
                got_notification = true;
            }
            
            break;
        // HTTP channel notifications
        case TAG_CHANNEL_HTTP:
            if (notification.event_type == MV_EVENTTYPE_CHANNELDATAREADABLE) {
                received_request = true;
                got_notification = true;
            }
            
            if (notification.event_type == MV_EVENTTYPE_CHANNELNOTCONNECTED) {
                channel_was_closed = true;
                got_notification = true;
            }
            
            break;
        // Others
        default:
            break;
    }
    
    // We had a relevant notification
    if (got_notification) {
        // Point to the next record to be written
        notification_index = (notification_index + 1) % SHARED_NC_BUFFER_SIZE_R;

        // Clear the current notifications event
        // See https://www.twilio.com/docs/iot/microvisor/microvisor-notifications#buffer-overruns
        notification.event_type = 0;
    }
 }
