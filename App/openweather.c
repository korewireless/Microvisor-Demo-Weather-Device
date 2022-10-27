/**
 *
 * Microvisor Weather Device Demo
 * Version 2.0.4
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


/*
 * GLOBALS
 */
char request_url[165] = { 0 };


/**
 * @brief Initialise the OpenWeather access data.
 *
 *
 */
void OW_init(const char* api_key, double lat, double lng) {
    // Create the access URL using sprintf()
    sprintf(request_url, "%s?lat=%.6f&lon=%.6f&appid=%s&exclude=minutely,hourly,daily,alerts&units=metric", FORECAST_BASE_URL, lat, lng, api_key);
}


/**
 * @brief Issue an HTTP request to OpenWeather.
 *
 * @retval Whether the request was issued (`true`) or not (`false`)
 */
bool OW_request_forecast(void) {
    return http_send_request(request_url);
}
