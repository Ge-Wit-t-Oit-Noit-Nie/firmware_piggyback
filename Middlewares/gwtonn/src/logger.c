/**
 ******************************************************************************
 * @file   logger.c
 * @brief  Implementation of the logger
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

#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "internal_sensors.h"
#include "sd_card.h"


#define BUFFER_SIZE 64
#define CLEAR_BUFFER(buffer) memset(buffer, '\0', sizeof(buffer))

/**
 * @brief Function implementing the logTask thread.
 *
 * This function will run a log task that will write to the SD card.
 * It will wait for messages from the loggerQueue and write them to the SD card.
 *
 * In case the card cannot be mounted, the task will clear the buffer and
 * disable the card.
 *
 * @param argument: Not used
 * @retval None
 */
void logger_task(void *argument) {
    UNUSED(argument); // Mark variable as 'UNUSED' to suppress 'unused-variable'
                      // warning

    telemetry_t telemetry;
    char string[BUFFER_SIZE]; // to store strings..
    RTC_DateTypeDef gDate;
    RTC_TimeTypeDef gTime;

    while (1) {

        if (osOK == osMessageQueueGet(loggerQueueHandle, &telemetry, NULL,
                                      osWaitForever)) {
            is_get_date_time(&gDate, &gTime); // get the time from the RTC

            CLEAR_BUFFER(string);
            snprintf(
                string, BUFFER_SIZE, "[%02d:%02d:%02d],%d,%d,%d,%d,%d\n\r",
                gTime.Hours, gTime.Minutes, gTime.Seconds,
                telemetry.instruction_pointer,
                telemetry.shutdown_index_register,
                (uint16_t)(is_get_temperature ()* 100), (uint16_t)(is_get_vref()*100),
                telemetry.trigger); // format the string to write to the file

            write_file(LOG_FILENAME "\0", string);
        }
    }
}