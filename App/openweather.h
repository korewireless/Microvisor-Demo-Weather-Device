/**
 *
 * Microvisor Weather Device Demo
 * Version 2.0.7
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
#define FORECAST_BASE_URL "https://api.openweathermap.org/data/2.5/onecall"


/*
 * PROTOTYPES
 */
void    OW_init(const char* api_key, double lat, double lng);
bool    OW_request_forecast(void);


#ifdef __cplusplus
}
#endif


#endif  // OPENWEATHER_H
