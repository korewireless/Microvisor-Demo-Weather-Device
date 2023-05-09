/**
 *
 * Microvisor Weather Device Demo
 * Version 3.1.1
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _SHARED_H_
#define _SHARED_H_


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define                 SHARED_NC_BUFFER_SIZE_R                     16


/*
 * PROTOTYPES
 */
MvNotificationHandle    shared_get_handle(void);
bool                    shared_setup_notification_center(void);


#ifdef __cplusplus
}
#endif


#endif      // _SHARED_H_
