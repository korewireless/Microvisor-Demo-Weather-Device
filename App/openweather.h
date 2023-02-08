/**
 *
 * Microvisor Weather Device Demo
 * Version 3.1.0
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef OPENWEATHER_H
#define OPENWEATHER_H


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define FORECAST_BASE_URL   "https://api.openweathermap.org/data/2.5/onecall"
#define API_KEY_SECRET_NAME "secret-ow-api-key"


/*
 * PROTOTYPES
 */
void    OW_init(double lat, double lng);
bool    OW_request_forecast(void);


#ifdef __cplusplus
}
#endif


#endif  // OPENWEATHER_H
