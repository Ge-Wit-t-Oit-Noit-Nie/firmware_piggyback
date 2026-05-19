/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fatfs.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "logger.h"
#include "program_controller.h"

#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for programTask */
osThreadId_t programTaskHandle;
const osThreadAttr_t programTask_attributes = {
    .name = "programTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for logTask */
osThreadId_t logTaskHandle;
const osThreadAttr_t logTask_attributes = {
    .name = "logTask",
    .stack_size = 400 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for loggerQueue */
osMessageQueueId_t loggerQueueHandle;
const osMessageQueueAttr_t loggerQueue_attributes = {
    .name = "loggerQueue"};

/* Definitions for can_thread */
osThreadId_t can_threadHandle;
const osThreadAttr_t can_thread_attributes = {
    .name = "can_thread",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/* Definitions for ext_interrupt_event */
osEventFlagsId_t ext_interrupt_eventHandle;
const osEventFlagsAttr_t ext_interrupt_event_attributes = {
    .name = "ext_interrupt_event"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of loggerQueue */
  loggerQueueHandle = osMessageQueueNew(10, sizeof(telemetry_t), &loggerQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of programTask */
  programTaskHandle = osThreadNew(program_controller_task, NULL, &programTask_attributes);

  /* creation of logTask */
  logTaskHandle = osThreadNew(logger_task, NULL, &logTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* creation of logTask */
  can_threadHandle = osThreadNew(can_thread_handler, NULL, &can_thread_attributes);
/* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of ext_interrupt_event */
  ext_interrupt_eventHandle = osEventFlagsNew(&ext_interrupt_event_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_program_controller_task */
/**
 * @brief  Function implementing the programTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_program_controller_task */
__weak void program_controller_task(void *argument)
{
  /* USER CODE BEGIN program_controller_task */
  UNUSED(argument); // Mark variable as 'UNUSED' to suppress 'unused-variable' warning

  /* USER CODE END program_controller_task */
}

/* USER CODE BEGIN Header_logger_task */
/**
 * @brief Function implementing the logTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_logger_task */
__weak void logger_task(void *argument)
{
  /* USER CODE BEGIN logger_task */
  UNUSED(argument); // Mark variable as 'UNUSED' to suppress 'unused-variable' warning
  /* USER CODE END logger_task */
}

/**
 * @brief Function implementing the can interface thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_logger_task */
__weak void can_thread_handler(void *argument)
{
  UNUSED(argument); // Mark variable as 'UNUSED' to suppress 'unused-variable' warning
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // You can log the error, blink an LED, or halt the system
    printf("Stack overflow in task: %s\n", pcTaskName);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14,
                      GPIO_PIN_SET); // Set an error pin (for example, PB0)

    // Optional: halt the system
    taskDISABLE_INTERRUPTS();
    for (;;)
        ; // Stay here to prevent further damage
}
    /* USER CODE END Application */
