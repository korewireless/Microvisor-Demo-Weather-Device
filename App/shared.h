/**
 *
 * Microvisor Weather Device Demo
 *
 * Copyright © 2023, KORE Wireless
 * Licence: MIT
 *
 */
#ifndef _SHARED_H_
#define _SHARED_H_


/*
 * CONSTANTS
 */
#define                 SHARED_NC_BUFFER_SIZE_R                     16


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
