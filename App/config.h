/**
 *
 * Microvisor Weather Device Demo
 * Version 3.1.1
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define     TAG_CHANNEL_CONFIG              2

#define     CONFIG_RX_BUFFER_SIZE_B         2560
#define     CONFIG_TX_BUFFER_SIZE_B         512

#define     CONFIG_WAIT_PERIOD_MS           5000


/*
 * PROTOTYPES
 */
bool        config_open_channel(void);
void        config_close_channel(void);
bool        config_get_secret(char *value_buffer, char key[]);


#ifdef __cplusplus
}
#endif


#endif      // _CONFIG_H_
