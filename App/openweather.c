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
 * STATIC PROTOTYPES
 */
static bool OW_get_key(void);


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
    if (!got_key) got_key = config_get_secret(api_key, API_KEY_SECRET_NAME);
    
    if (got_key) sprintf(request_url,
                         "%s?lat=%.6f&lon=%.6f&appid=%s&exclude=minutely,hourly,daily,alerts&units=metric",
                         FORECAST_BASE_URL,
                         lat,
                         lng,
                         api_key);
}


/**
 * @brief Issue an HTTP request to OpenWeather.
 *
 * @returns Whether the request was issued (`true`) or not (`false`)
 */
bool OW_request_forecast(void) {
    
    if (!got_key) got_key = OW_get_key();
    if (got_key)  return (http_send_request(request_url) == MV_STATUS_OKAY);
    return false;
}


/**
 * @brief Request the API key.
 *
 * @returns Whether the key was received (`true`) or not (`false`)
 */
static bool OW_get_key(void) {
    
    return config_get_secret(api_key, API_KEY_SECRET_NAME);
}
