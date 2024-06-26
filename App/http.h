/**
 *
 * Microvisor Weather Device Demo
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#ifndef _HTTP_H_
#define _HTTP_H_


/*
 * CONSTANTS
 */
#define         HTTP_RX_BUFFER_SIZE_B           2560
#define         HTTP_TX_BUFFER_SIZE_B           512
#define         HTTP_NC_BUFFER_SIZE_R           8       // NOTE Size in records, not bytes


#ifdef __cplusplus
extern "C" {
#endif


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
