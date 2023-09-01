/**
 *
 * Microvisor Weather Device Demo
 *
 * Copyright © 2023, KORE Wireless
 * Licence: MIT
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
#include <assert.h>

// Microvisor includes
#include "stm32u5xx_hal.h"
#include "cmsis_os.h"
#include "mv_syscalls.h"

// App includes
#include "logging.h"
#include "uart_logging.h"
#include "ht16k33-matrix.h"
#include "i2c.h"
#include "http.h"
#include "network.h"
#include "openweather.h"
#include "cJSON.h"
#include "config.h"
#include "shared.h"


/*
 * CONSTANTS
 */
#define     LED_GPIO_BANK               GPIOA
#define     LED_GPIO_PIN                GPIO_PIN_5

#define     BUTTON_GPIO_BANK            GPIOF
#define     BUTTON_GPIO_PIN             GPIO_PIN_6

#define     DEBUG_TASK_PAUSE_MS         1000
#define     DEFAULT_TASK_PAUSE_MS       500

#define     WEATHER_READ_PERIOD_MS      300000
#define     CHANNEL_KILL_PERIOD_MS      15000


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void sleep_ms(uint32_t ms);


#ifdef __cplusplus
}
#endif


#endif      // _MAIN_H_
