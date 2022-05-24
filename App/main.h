/**
 *
 * Microvisor Weather Device Demo
 * Version 1.3.0
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _MAIN_H_
#define _MAIN_H_


/*
 * INCLUDES
 */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// Microvisor includes
#include "stm32u5xx_hal.h"
#include "cmsis_os.h"
#include "mv_syscalls.h"

// App includes
#include "logging.h"
#include "ht16k33.h"
#include "i2c.h"
#include "http.h"
#include "openweather.h"
#include "cJson.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define     LED_GPIO_BANK           GPIOA
#define     LED_GPIO_PIN            GPIO_PIN_5

#define     BUTTON_GPIO_BANK        GPIOF
#define     BUTTON_GPIO_PIN         GPIO_PIN_6

#define     DEBUG_TASK_PAUSE_MS     1000
#define     DEFAULT_TASK_PAUSE_MS   500

#define     WEATHER_READ_PERIOD_MS  300000
#define     CHANNEL_KILL_PERIOD_MS  15000

#define     CLEAR_DAY               0
#define     RAIN                    1
#define     DRIZZLE                 2
#define     SNOW                    3
#define     SLEET                   4
#define     WIND                    5
#define     FOG                     6
#define     CLOUDY                  7
#define     PARTLY_CLOUDY           8
#define     THUNDERSTORM            9
#define     TORNADO                 10
#define     CLEAR_NIGHT             11
#define     NONE                    12


/*
 * PROTOTYPES
 */
void        system_clock_config(void);
void        GPIO_init(void);
void        start_led_task(void *unused_arg);
void        start_iot_task(void *unused_arg);

void        log_device_info(void);
void        server_log(char* format_string, ...);
void        server_error(char* format_string, ...);
void        sleep_ms(uint32_t ms);


#ifdef __cplusplus
}
#endif


#endif      // _MAIN_H_
