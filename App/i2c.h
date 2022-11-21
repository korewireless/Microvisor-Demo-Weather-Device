/**
 *
 * Microvisor Weather Device Demo
 * Version 2.0.6
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _I2C_HEADER_
#define _I2C_HEADER_


/*
 * CONSTANTS
 */
#define     I2C_GPIO_BANK           GPIOB


/*
 * PROTOTYPES
 */
void I2C_init(void);
void I2C_Scan(void);


#endif  // _I2C_HEADER_
