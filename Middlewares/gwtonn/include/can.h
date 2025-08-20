/**
 ******************************************************************************
 * @file   can.h
 * @brief  Headerfile for the can.c file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 Ge Wit't Oit Noit Nie.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#ifndef __MIDDLEWARES_GWTONN_CAN_H__
#define __MIDDLEWARES_GWTONN_CAN_H__

#ifdef __cplusplus
extern "C"
{
#endif // End of extern "C" block

typedef enum {
    MESSAGE_CODE_TIME = 0x01,
    MESSAGE_LOG_EVENT = 0x02,
} MESSAGE_CODE;

void can_write(MESSAGE_CODE message, uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif // End of extern "C" block

#endif // __MIDDLEWARES_GWTONN_CAN_H__
