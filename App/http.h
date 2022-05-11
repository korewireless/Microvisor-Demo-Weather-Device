/**
 *
 * Microvisor Weather Device Demo
 * Version 1.1.0
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _HTTP_H_
#define _HTTP_H_


/*
 * CONSTANTS
 */
#define     HTTP_RX_BUFFER_SIZE_B       2560
#define     HTTP_TX_BUFFER_SIZE_B       512


/*
 * PROTOTYPES
 */
void        http_channel_center_setup(void);
bool        http_open_channel(void);
void        http_close_channel(void);
bool        http_send_request(const char* url);
void        http_process_response(void);



#endif      // _HTTP_H_
