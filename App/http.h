/**
 *
 * Microvisor Weather Device Demo
 * Version 3.1.1
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
#define         TAG_CHANNEL_HTTP                1
#define         HTTP_RX_BUFFER_SIZE_B           2560
#define         HTTP_TX_BUFFER_SIZE_B           512
#define         HTTP_NC_BUFFER_SIZE_R           8       // NOTE Size in records, not bytes


/*
 * PROTOTYPES
 */
bool            http_open_channel(void);
void            http_close_channel(void);
enum MvStatus   http_send_request(const char* url);


#ifdef __cplusplus
}
#endif


#endif      // _HTTP_H_
