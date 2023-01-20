/**
 *
 * Microvisor Weather Device Demo
 * Version 2.0.7
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _HTTP_H_
#define _HTTP_H_


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define     HTTP_RX_BUFFER_SIZE_B       2560
#define     HTTP_TX_BUFFER_SIZE_B       512
#define     HTTP_NT_BUFFER_SIZE_R       8


/*
 * PROTOTYPES
 */
void            http_channel_center_setup(void);
bool            http_open_channel(void);
void            http_close_channel(void);
enum MvStatus   http_send_request(const char* url);
void            http_process_response(void);


#ifdef __cplusplus
}
#endif


#endif      // _HTTP_H_
