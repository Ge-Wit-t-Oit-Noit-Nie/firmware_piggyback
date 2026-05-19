/**
 ******************************************************************************
 * @file   internal_sensors.c
 * @brief  Implementation of the internal sensors
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

#include "internal_sensors.h"
#include "main.h"
#include <stdint.h>

uint16_t adc_buffer[2];

volatile float temperature = 0, vref = 0;

/***************************************
 * Public functions
 **************************************/

/*
 * The temperature is constantly updated using the HAL_ADC_ConvCpltCallback function. 
 * The value is stored in the global variable temperature.
 * 
 * This function returns just returns the value of the temperature variable.
 */
float is_get_temperature(void)
{
    return temperature;
}
/*
 * The vref is constantly updated using the HAL_ADC_ConvCpltCallback function.
 * The value is stored in the global variable vref.
 *
 * This function returns just returns the value of the vref variable.
 */
float is_get_vref(void)
{
    return vref;
}
/*
 * Return the value of the RTC date and time.
 */
void is_get_date_time(RTC_DateTypeDef *date, RTC_TimeTypeDef *time)
{
    HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN);
}
/*
 * Set the RTC time to the given values.
 */
void is_set_time(uint8_t hr, uint8_t min, uint8_t sec)
{
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours = hr;
    sTime.Minutes = min;
    sTime.Seconds = sec;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
 * @brief  Set the RTC date to the given values.
 */
void is_set_date(uint8_t year, uint8_t month, uint8_t day)
{
    RTC_DateTypeDef sDate = {0};
    sDate.Year = year;
    sDate.Month = month;
    sDate.Date = day;
    sDate.WeekDay = RTC_WEEKDAY_MONDAY; // Set to a default value, can be changed later
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
}

/***************************************
 * Callback functions
 **************************************/

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    UNUSED(hadc);
    if (adc_buffer[0] == 0)
    {
        return;
    }
    vref = (V_REF_INT * 4095.0F) / adc_buffer[0];
    float vsense = (adc_buffer[1] * vref) / 4095.0F;
    temperature = ((V_AT_25C - vsense) * 1000.0F / AVG_SLOPE) + 25.0F;
}
