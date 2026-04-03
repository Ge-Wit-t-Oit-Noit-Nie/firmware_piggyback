/**
 ******************************************************************************
 * @file   can.c
 * @brief  Implementation of the CAN interface
 ******************************************************************************
 *
 * The CAN Interface is used to communicate with other devices.
 * The CAN interface is implemented as an UART interface, however, for the
 * purporse of clarity, we call it CAN.
 *
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

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "usart.h"

#include "bool.h"
#include "can.h"
#include "datetime.h"
#include "internal_sensors.h"

#define UART_RX_BUF_SIZE 12
#define CAN_EVENT_MESSAGE_RECEIVED 0x01

uint8_t uartRxBuf[UART_RX_BUF_SIZE];
uint8_t uart_rx_received;

/**
 * @brief  FREERTOS thread handler for the CAN interface.
 *
 * @param argument: Not used.
 */
void can_thread_handler(void *argument) {

    UNUSED(argument); // Mark variable as 'UNUSED' to suppress 'unused-variable'
                      // warning

    // Initialize the event flags for CAN messages
    uart_rx_received = false;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,
                      GPIO_PIN_SET); // Set a pin to indicate activity

    // Start the UART receive process
    HAL_UART_Receive_DMA(&huart1, uartRxBuf, UART_RX_BUF_SIZE);
    dt_dense_time encoded_time;
    for (;;) {
        // suspend the task until a message is received
        while (!uart_rx_received) {
            osDelay(10); // Sleep for 10 ms to avoid busy waiting
        }
        uart_rx_received = false; // Reset the flag for the next message

        switch (uartRxBuf[0]) {
        case MESSAGE_CODE_TIME:
            // cast 8 bytes from the buffer to a uint64_t
            encoded_time = (dt_dense_time)uartRxBuf[1] << 56 |
                           (dt_dense_time)uartRxBuf[2] << 48 |
                           (dt_dense_time)uartRxBuf[3] << 40 |
                           (dt_dense_time)uartRxBuf[4] << 32 |
                           (dt_dense_time)uartRxBuf[5] << 24 |
                           (dt_dense_time)uartRxBuf[6] << 16 |
                           (dt_dense_time)uartRxBuf[7] << 8 |
                           (dt_dense_time)uartRxBuf[8];

            datetime_t decoded_time = {0};
            dt_decode(encoded_time, &decoded_time);
            is_set_time(
                decoded_time.hour, decoded_time.minute,
                decoded_time
                    .second); // Set the time in the internal sensors module
            is_set_date(
                decoded_time.year, decoded_time.month,
                decoded_time
                    .day); // Set the date in the internal sensors module
            break;

        default:
            break;
        }
    }
}

/**
 * @brief  Function to write data to the CAN interface.
 *
 * @param  code: Code to be sent
 * @param  data: Pointer to the data to be written.
 * @param  length: Length of the data to be written.
 */
void can_write(MESSAGE_CODE code, uint8_t *data, uint16_t length) {

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,
                      GPIO_PIN_SET); // Set a pin to indicate activity

    uint8_t message[12] = {code, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0xFF, 0xFF, 0xFF}; // Initialize message buffer

    if (data != NULL && length > 0) {
        for(int i = 0; i < length && i < (UART_RX_BUF_SIZE - 5); i++) {
            message[i + 2] = data[i]; // Copy data into the message buffer
        }
    }
    HAL_UART_Transmit(&huart1, message, 12, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,
                      GPIO_PIN_RESET); // Set a pin to indicate activity
}

/**
 * @brief  Callback function for UART receive complete interrupt.
 * This function is called when a byte is received via UART.
 * It stores the received byte in a circular buffer and restarts the UART
 * receive interrupt.
 *
 * Remember that this function is called from an interrupt context, so it should
 * be kept short and efficient.
 *
 * @param  huart: Pointer to the UART handle.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,
                          GPIO_PIN_SET); // Set a pin to indicate activity

        uart_rx_received = true;

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,
                          GPIO_PIN_RESET); // Set a pin to indicate activity
        // Restart interrupt for next byte
        HAL_UART_Receive_DMA(&huart1, uartRxBuf, UART_RX_BUF_SIZE);
    }
}