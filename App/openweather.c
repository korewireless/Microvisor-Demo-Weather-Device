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
static char request_url[1024] = { 0 };
static char api_key[33] = { 0 };
static bool got_key = false;

/**
 * @brief Initialise the OpenWeather access data.
 *
 *
 */
void OW_init(double lat, double lng) {
    
    // Request the 'secret' API key
    if (config_get_secret(api_key, API_KEY_SECRET_NAME)) {
        // Create the access URL using sprintf()
        sprintf(request_url, "%s?lat=%.6f&lon=%.6f&appid=%s&exclude=minutely,hourly,daily,alerts&units=metric", FORECAST_BASE_URL, lat, lng, api_key);
        got_key = true;
    }
}


/**
 * @brief Issue an HTTP request to OpenWeather.
 *
 * @returns Whether the request was issued (`true`) or not (`false`)
 */
bool OW_request_forecast(void) {
    
    if (!got_key) return false;
    return (http_send_request(request_url) == MV_STATUS_OKAY);
}

