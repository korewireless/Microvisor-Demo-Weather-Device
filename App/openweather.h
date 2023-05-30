/**
 *
 * Microvisor Weather Device Demo
 * Version 3.1.2
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
#define     FORECAST_BASE_URL           "https://api.openweathermap.org/data/2.5/onecall"
#define     API_KEY_SECRET_NAME         "secret-ow-api-key"

// OpenWeather condition codes
#define     CLEAR_DAY                   0
#define     RAIN                        1
#define     DRIZZLE                     2
#define     SNOW                        3
#define     SLEET                       4
#define     WIND                        5
#define     FOG                         6
#define     CLOUDY                      7
#define     PARTLY_CLOUDY               8
#define     THUNDERSTORM                9
#define     TORNADO                     10
#define     CLEAR_NIGHT                 11
#define     NONE                        12


/*
 * PROTOTYPES
 */
void OW_init(double lat, double lng);
bool OW_request_forecast(void);


#ifdef __cplusplus
}
#endif


#endif  // OPENWEATHER_H
