/**
 *
 * Microvisor Weather Device Demo
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#ifndef _SHARED_H_
#define _SHARED_H_


/*
 * CONSTANTS
 */
#define             SHARED_NC_BUFFER_SIZE_R                     16
#define             TAG_CHANNEL_HTTP                            1
#define             TAG_CHANNEL_CONFIG                          2
#define             TAG_CHANNEL_SYSTEM                          3

#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
MvNotificationHandle    shared_get_handle(void);
bool                    shared_setup_notification_center(void);


#ifdef __cplusplus
}
#endif


#endif      // _SHARED_H_
