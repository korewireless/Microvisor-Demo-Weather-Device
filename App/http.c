/**
 *
 * Microvisor Weather Device Demo
 * Version 3.1.1
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


/*
 * GLOBALS
 */
// Store for Microvisor resource handles used in this code.
// See `https://www.twilio.com/docs/iot/microvisor/syscalls#http_handles`
struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
    MvChannelHandle      channel;
} http_handles = { 0, 0, 0 };

// Defined in `main.c`
extern volatile bool        received_request;
extern volatile bool        channel_was_closed;
extern volatile bool        new_forecast;
extern volatile uint32_t    icon_code;
extern          char        forecast[32];


/**
 * @brief Open a new HTTP channel.
 *
 * @returns `true` if the channel is open, otherwise `false`.
 */
bool http_open_channel(void) {
    
    // Set up the HTTP channel's multi-use send and receive buffers
    static volatile uint8_t http_rx_buffer[HTTP_RX_BUFFER_SIZE_B] __attribute__((aligned(512)));
    static volatile uint8_t http_tx_buffer[HTTP_TX_BUFFER_SIZE_B] __attribute__((aligned(512)));

    // Get the network channel handle.
    // NOTE This is set in `logging.c` which puts the network in place
    //      (ie. so the network handle != 0) well in advance of this being called
    http_handles.network = net_get_handle();
    if (http_handles.network == 0) return false;
    server_log("Network handle: %lu", (uint32_t)http_handles.network);

    // FROM 3.1.0
    // Set up shared notification center
    http_handles.notification = shared_get_handle();
    if (http_handles.notification == 0) return false;
    server_log("Shared NC handle: %lu", (uint32_t)http_handles.notification);
    
    // Configure the required data channel
    struct MvOpenChannelParams channel_config = {
        .version = 1,
        .v1 = {
            .notification_handle = http_handles.notification,
            .notification_tag    = TAG_CHANNEL_HTTP,
            .network_handle      = http_handles.network,
            .receive_buffer      = (uint8_t*)http_rx_buffer,
            .receive_buffer_len  = sizeof(http_rx_buffer),
            .send_buffer         = (uint8_t*)http_tx_buffer,
            .send_buffer_len     = sizeof(http_tx_buffer),
            .channel_type        = MV_CHANNELTYPE_HTTP,
            .endpoint            = {
                .data = (uint8_t*)"",
                .length = 0
            }
        }
    };

    // Ask Microvisor to open the channel
    // and confirm that it has accepted the request
    enum MvStatus status = mvOpenChannel(&channel_config, &http_handles.channel);
    if (status == MV_STATUS_OKAY) {
        server_log("HTTP channel handle: %lu", (uint32_t)http_handles.channel);
        return true;
    }
    
    server_error("Could not open HTTP channel. Status: %i", status);
    return false;
}


/**
 * @brief Close the currently open HTTP channel.
 */
void http_close_channel(void) {
    
    // If we have a valid channel handle -- ie. it is non-zero --
    // then ask Microvisor to close it and confirm acceptance of
    // the closure request.
    if (http_handles.channel != 0) {
        MvChannelHandle old = http_handles.channel;
        enum MvStatus status = mvCloseChannel(&http_handles.channel);
        do_assert((status == MV_STATUS_OKAY || status == MV_STATUS_CHANNELCLOSED), "Channel closure");
        server_log("HTTP channel %lu closed (status code: %i)", (uint32_t)old, status);
    }

    // Confirm the channel handle has been invalidated by Microvisor
    do_assert(http_handles.channel == 0, "Channel handle not zero");
}


/**
 * @brief Send an HTTP request.
 *
 * @params url - The URL of the target resource.
 *
 * @returns `true` if the request was accepted by Microvisor, otherwise `false`
 */
enum MvStatus http_send_request(const char* url) {
    
    // Check for a valid channel handle
    if (http_handles.channel == 0) {
        // There's no open channel, so open open one now and
        // try to send again
        http_open_channel();
        return http_send_request(url);
    }
    
    server_log("Sending HTTP request");

    // Set up the request
    const char verb[] = "GET";
    const char body[] = "";
    struct MvHttpHeader hdrs[] = {};
    struct MvHttpRequest request_config = {
        .method = {
            .data = (uint8_t *)verb,
            .length = strlen(verb)
        },
        .url = {
            .data = (uint8_t *)url,
            .length = strlen(url)
        },
        .num_headers = 0,
        .headers = hdrs,
        .body = {
            .data = (uint8_t *)body,
            .length = strlen(body)
        },
        .timeout_ms = 10000
    };

    // Issue the request -- and check its status
    enum MvStatus status = mvSendHttpRequest(http_handles.channel, &request_config);
    if (status == MV_STATUS_OKAY) {
        server_log("Request sent to Twilio");
    } else if (status == MV_STATUS_CHANNELCLOSED) {
        server_error("HTTP channel %lu already closed", (uint32_t)http_handles.channel);
    } else {
        server_error("Could not issue request. Status: %i", status);
    }
    
    return status;
}

