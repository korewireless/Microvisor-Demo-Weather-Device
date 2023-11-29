/**
 *
 * Microvisor Weather Device Demo
 *
 * Copyright © 2023, KORE Wireless
 * Licence: MIT
 *
 */
#include "main.h"


/*
 * GLOBALS
 */
// Store for Microvisor resource handles used in this code.
// See `https://www.twilio.com/docs/iot/microvisor/syscalls#config_handles`
static struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
    MvChannelHandle      channel;
} config_handles;

volatile bool received_config = false;


/**
 * @brief Request the value of a secret.
 *
 * @param value_buffer - Whether the value will be written back to.
 * @param key          - The key name we're targeting.
 *
 * @returns `true` if the value was retrieved successfully,
 *          otherwise `false`
 */
bool config_get_secret(char *value_buffer, char key[]) {

    // Check for a valid channel handle
    if (config_handles.channel == 0) {
        // There's no open channel, so open open one now
        if (!config_open_channel()) return false;
    }

    // Set up the request parameters
    struct MvConfigKeyToFetch key_one = {
        .scope = MV_CONFIGKEYFETCHSCOPE_ACCOUNT,    // An account-level value
        .store = MV_CONFIGKEYFETCHSTORE_SECRET,     // A secret value
        .key = {
            .data = (uint8_t*)key,
            .length = strlen(key)
        }
    };

    uint32_t item_count = 1;
    struct MvConfigKeyToFetch keys[item_count];
    keys[0] = key_one;

    struct MvConfigKeyFetchParams request = {
        .num_items = item_count,
        .keys_to_fetch = keys
    };

    // Request the value of the key specified above
    server_log("Requesting value for key '%s'", key);
    enum MvStatus status = mvSendConfigFetchRequest(config_handles.channel, &request);
    if (status != MV_STATUS_OKAY) {
        server_error("Could not issue config fetch request");
        config_close_channel();
        return false;
    }

    // Wait for the data to arrive
    received_config = false;
    uint32_t last_tick = HAL_GetTick();
    while(HAL_GetTick() - last_tick < CONFIG_WAIT_PERIOD_MS) {
        if (received_config) break;
        __asm("nop");
    }

    if (!received_config) {
        server_error("Config fetch request timed out");
        config_close_channel();
        return false;
    }

    // Parse the received data record
    server_log("Received value for key '%s'", key);
    struct MvConfigResponseData response = {
        .result = 0,
        .num_items = 0
    };

    status = mvReadConfigFetchResponseData(config_handles.channel, &response);
    if (status != MV_STATUS_OKAY || response.result != MV_CONFIGFETCHRESULT_OK || response.num_items != item_count) {
        server_error("Could not get config item (status: %i; result: %i)", status, response.result);
        config_close_channel();
        return false;
    }

    uint8_t value[65] = {0};
    uint32_t value_length = 0;
    enum MvConfigKeyFetchResult result = 0;

    struct MvConfigResponseReadItemParams item = {
        .result = &result,
        .item_index = 0,
        .buf = {
            .data = &value[0],
            .size = 64,
            .length = &value_length
        }
    };

    // Get the value itself
    status = mvReadConfigResponseItem(config_handles.channel, &item);
    if (status != MV_STATUS_OKAY || result != MV_CONFIGKEYFETCHRESULT_OK) {
        server_error("Could not get config item (status: %i; result: %i)", status, result);
        config_close_channel();
        return false;
    }

    // Copy the value data to the requested location
    strncpy(value_buffer, (char*)value, value_length + 1);
    config_close_channel();
    return true;
}


/**
 * @brief Open a new config fetch channel.
 *
 * @returns `true` if the channel is open, otherwise `false`.
 */
bool config_open_channel(void) {

    // Set up the HTTP channel's multi-use send and receive buffers
    static volatile uint8_t config_rx_buffer[CONFIG_RX_BUFFER_SIZE_B] __attribute__((aligned(512)));
    static volatile uint8_t config_tx_buffer[CONFIG_RX_BUFFER_SIZE_B] __attribute__((aligned(512)));

    // Get the network channel handle.
    // NOTE This is set in `logging.c` which puts the network in place
    //      (ie. so the network handle != 0) well in advance of this being called
    config_handles.network = net_get_handle();
    if (config_handles.network == 0) return false;
    server_log("Network handle: %lu", (uint32_t)config_handles.network);

    // FROM 3.1.0
    // Set up shared notification center
    config_handles.notification = shared_get_handle();
    if (config_handles.notification == 0) return false;

    // Get the network channel handle.
    // NOTE This is set in `logging.c` which puts the network in place
    //      (ie. so the network handle != 0) well in advance of this being called
    // Configure the required data channel
    struct MvOpenChannelParams channel_config = {
        .version = 1,
        .v1 = {
            .notification_handle = config_handles.notification,
            .notification_tag    = TAG_CHANNEL_CONFIG,
            .network_handle      = config_handles.network,
            .receive_buffer      = (uint8_t*)config_rx_buffer,
            .receive_buffer_len  = sizeof(config_rx_buffer),
            .send_buffer         = (uint8_t*)config_tx_buffer,
            .send_buffer_len     = sizeof(config_tx_buffer),
            .channel_type        = MV_CHANNELTYPE_CONFIGFETCH,
            .endpoint            = {
                .data = (uint8_t*)"",
                .length = 0
            }
        }
    };

    // Ask Microvisor to open the channel
    // and confirm that it has accepted the request
    enum MvStatus status = mvOpenChannel(&channel_config, &config_handles.channel);
    if (status == MV_STATUS_OKAY) {
        server_log("Config channel handle: %lu", (uint32_t)config_handles.channel);
        return true;
    }

    server_error("Could not open config channel. Status: %i", status);
    return false;
}


/**
 * @brief Close the currently open HTTP channel.
 */
void config_close_channel(void) {

    // If we have a valid channel handle -- ie. it is non-zero --
    // then ask Microvisor to close it and confirm acceptance of
    // the closure request.
    if (config_handles.channel != 0) {
        MvChannelHandle old = config_handles.channel;
        enum MvStatus status = mvCloseChannel(&config_handles.channel);
        do_assert((status == MV_STATUS_OKAY || status == MV_STATUS_CHANNELCLOSED), "Channel closure");
        server_log("Config channel %lu closed (status code: %i)", (uint32_t)old, status);
    }

    // Confirm the channel handle has been invalidated by Microvisor
    do_assert(config_handles.channel == 0, "Channel handle not zero");
}

