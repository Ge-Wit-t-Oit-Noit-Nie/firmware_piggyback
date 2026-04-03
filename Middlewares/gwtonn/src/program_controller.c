/**
 ******************************************************************************
 * @file   program_controller.c
 * @brief  Implementation of the program controller
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
#include <main.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "asserts.h"
#include "cmsis_os2.h"
#include "fatfs.h"
#include "internal_sensors.h"
#include "asserts.h"
#include "program_controller.h"
#include "logger.h"
#include "can.h"

/***************************************
 * Private functions
 **************************************/
typedef uint16_t (*program_controller_function_t)(
    program_controller_registers_t *program_controller_registers);

#define READ_MEMORY_BYTE(index) \
  (((uint8_t *)&_program_data_start)[index]) // Read a byte from the program memory

#define NUMBER_OF_PINS_MAPPED 17
typedef struct
{
  uint16_t pin_number;
  GPIO_TypeDef *port;
} pin_mapping_t;

static const pin_mapping_t pin_mapping[NUMBER_OF_PINS_MAPPED] = {
    {GPIO_PIN_0, GPIOA}, // Pin 0 on GPIOA
    {GPIO_PIN_1, GPIOA},  {GPIO_PIN_2, GPIOA},  {GPIO_PIN_3, GPIOA},
    {GPIO_PIN_8, GPIOA},  {GPIO_PIN_11, GPIOA}, {GPIO_PIN_12, GPIOA},
    {GPIO_PIN_15, GPIOA},

    {GPIO_PIN_2, GPIOB},  {GPIO_PIN_4, GPIOB},  {GPIO_PIN_5, GPIOB},
    {GPIO_PIN_6, GPIOB},  {GPIO_PIN_8, GPIOB},  {GPIO_PIN_9, GPIOB},
    {GPIO_PIN_12, GPIOB}, {GPIO_PIN_13, GPIOB}, {GPIO_PIN_15, GPIOB}};

/**
 * @brief  Function to exit the program.
 *
 * This function will terminate the program and turn off the green LED.
 * It will also terminate the program controller task.
 *
 * @note   This function is called when the program is finished or when the
 * kill switch is pressed.
 * * @param  program_controller_registers: Pointer to the program controller
 * registers.
 * @retval 0 = VM_EXIT
 */
uint16_t
vm_exit(program_controller_registers_t *program_controller_registers)
{
  UNUSED(program_controller_registers);
  printf("Exit program\n\r");
  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
  osThreadTerminate(programTaskHandle);
  return VM_EXIT;
}

uint16_t
vm_pause(program_controller_registers_t *program_controller_registers)
{
  UNUSED(program_controller_registers);
  printf("Pause program\n\r");
  // TODO: Implement pause functionality
  return VM_EXIT;
}
/**
 * @brief  Function to delay the program.
 *
 * This function will delay the program for the amount of time specified in
 * register1. It will also check for the kill switch and pause events.
 * @param  program_controller_registers: Pointer to the program controller
 * registers.
 * @retval 0 = VM_OK if the delay was successful; VM_EXIT if the program was
 * terminated; VM_ERROR if the program controller registers are NULL.
 */
uint16_t
vm_delay(program_controller_registers_t *program_controller_registers)
{
  ASSERT_NULL(program_controller_registers, VM_ERROR);

  uint16_t delay_time = (program_controller_registers->register1 << 8) | 
                        READ_MEMORY_BYTE(++program_controller_registers->instruction_pointer);

  printf("Delay for %d s\n\r", delay_time);
  uint32_t state = osEventFlagsWait(
      ext_interrupt_eventHandle,
      EXTERN_INTERRUPT_EVENT_KILL | EXTERN_INTERRUPT_EVENT_PAUZE,
      osFlagsWaitAny,
      pdMS_TO_TICKS(delay_time * 100));
  if (osFlagsErrorTimeout == (osFlagsErrorTimeout & state))
  {
    program_controller_registers->instruction_pointer++;
  }
  else if (EXTERN_INTERRUPT_EVENT_KILL == (state & EXTERN_INTERRUPT_EVENT_KILL))
  {
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    osThreadTerminate(programTaskHandle);
    return VM_EXIT;
  }
  else if (EXTERN_INTERRUPT_EVENT_PAUZE == (state & EXTERN_INTERRUPT_EVENT_PAUZE))
  {
    program_controller_registers->instruction_pointer = program_controller_registers->end_program_pointer;
  }
  return VM_OK;
}

/**
 * @brief  Function to turn on a pin.
 *
 * This function will turn on the pin specified in the lowest 5 bits (0x1F) in
 * resister1. It will also increment the instruction pointer.
 *
 * @param  program_controller_registers: Pointer to the program controller
 * registers.
 * @retval 0 = VM_OK; VM_ERROR if the program controller registers are NULL or
 * if the pin index is invalid.
 */
uint16_t
vm_set_pin_state(program_controller_registers_t *program_controller_registers)
{
  ASSERT_NULL(program_controller_registers, VM_ERROR);

  uint8_t pin_state = (program_controller_registers->register1 & 0x01);
  uint16_t pin_index = (READ_MEMORY_BYTE(++program_controller_registers->instruction_pointer) & 0x1F);
  printf("Turn on pin %d\n\r", pin_index);
  if (NUMBER_OF_PINS_MAPPED <= pin_index)
  {
    return VM_ERROR; // Error code
  };

  HAL_GPIO_WritePin(pin_mapping[pin_index].port,
                    pin_mapping[pin_index].pin_number, pin_state);

  program_controller_registers->instruction_pointer++;
  return VM_OK;
}

/**
 * @brief  Function to toggle a pin.
 *
 * This function will toggle the pin specified in the lowest 5 bits (0x1F) in
 * resister1. It will also increment the instruction pointer.
 *
 * @param  program_controller_registers: Pointer to the program controller
 * registers.
 * @retval 0 = VM_OK; VM_ERROR if the program controller registers are NULL or
 * if the pin index is invalid.
 */
uint16_t
vm_pin_toggle(program_controller_registers_t *program_controller_registers)
{
  ASSERT_NULL(program_controller_registers, VM_ERROR);

  uint16_t pin_index = (READ_MEMORY_BYTE(++program_controller_registers->instruction_pointer) & 0x1F);

  printf("Toggle pin %d\n\r", pin_index);
  if (NUMBER_OF_PINS_MAPPED <= pin_index)
  {
    return VM_ERROR; // Error code
  };

  HAL_GPIO_TogglePin(pin_mapping[pin_index].port,
                     pin_mapping[pin_index].pin_number);
  program_controller_registers->instruction_pointer++;
  return VM_OK;
}
/**
 * @brief  Function to send telemetry data.
 *
 * This function will send telemetry data to the logger queue.
 * It will also increment the instruction pointer.
 *
 * @param  program_controller_registers: Pointer to the program controller
 * registers.
 * @retval 0 = VM_OK; VM_ERROR if the program controller registers are NULL.
 */
uint16_t
vm_send_telemetry(
    program_controller_registers_t *program_controller_registers)
{
  ASSERT_NULL(program_controller_registers, VM_ERROR);

  printf("Send telemetry\n\r");
  telemetry_t telemetry = {
      program_controller_registers->instruction_pointer,
      program_controller_registers->end_program_pointer,
      0,
  };

  uint16_t temperature = is_get_temperature() * 100;
  uint16_t vref = is_get_vref() * 100;
  uint8_t message[] =
      {
          (temperature >> 8) & 0xFF,
          (temperature & 0xFF),
          (vref >> 8) & 0xFF,
          (vref & 0xFF),
      };

  can_write(MESSAGE_LOG_TELEMETRY, message, sizeof(message));

  if (osOK != osMessageQueuePut(loggerQueueHandle, &telemetry, 0, 0U))
  {
    printf("Error: Could not send message to loggerQueueHandle\n\r");
  }
  program_controller_registers->instruction_pointer++;
  return VM_OK;
}

/**
 * @brief  Function to jump to a specific instruction.
 *
 * This function will jump to the instruction specified in register1.
 * It will also set the instruction pointer to the value in register1.
 *
 * @param  program_controller_registers: Pointer to the program controller
 * registers.
 * @retval 0 = VM_OK; VM_ERROR if the program controller registers are NULL.
 */
uint16_t
vm_jump(program_controller_registers_t *program_controller_registers)
{
  ASSERT_NULL(program_controller_registers, VM_ERROR);

  uint32_t new_ip = (program_controller_registers->register1 & 0x01) << 16;
  new_ip |= (READ_MEMORY_BYTE(++program_controller_registers->instruction_pointer) << 8);
  new_ip |= READ_MEMORY_BYTE(++program_controller_registers->instruction_pointer);

  printf("Jump to instruction %lu\n\r", new_ip);
  program_controller_registers->instruction_pointer =
      program_controller_registers->start_program_pointer + new_ip;
  return VM_OK;
}

/**
 * Map all the functions to the opcodes.
 * The order of the functions in this array must match the order of the opcodes
 * in the enum. The functions are called in the program_controller_task()
 * function.
 */
static const program_controller_function_t program_controller_function[] = {
    vm_exit,           // OPCODE_HALT
    vm_pause,          // OPCODE_PAUSE
    vm_delay,          // OPCODE_DELAY
    vm_set_pin_state,  // OPCODE_SET_PIN_STATE
    vm_pin_toggle,     // OPCODE_PIN_TOGGLE
    vm_send_telemetry, // OPCODE_LOG_PROGRAM_STATE
    vm_jump,           // OPCODE_JUMP
};

/***************************************
 * Public functions
 **************************************/

/**
 * @brief  Function implementing the program_controller_task thread.
 * @param  argument: Not used
 * @retval None
 */
void program_controller_task(void *argument)
{
  UNUSED(argument); // Mark variable as 'UNUSED' to suppress 'unused-variable'
                    // warning

  program_controller_registers_t pcr = {
      .instruction_pointer = 0,
      .register1 = 0,
      .start_program_pointer = 0,
      .pauze_program_pointer = 0,
      .end_program_pointer = 0,
  };
  is_set_time(0, 0, 0); // Set the RTC time to 00:00:00

  // Initialize the program controller registers
  pcr.start_program_pointer = ((uint32_t *)&_program_data_start)[0];
  pcr.pauze_program_pointer = ((uint32_t *)&_program_data_start)[1];
  pcr.end_program_pointer = ((uint32_t *)&_program_data_start)[2];
  pcr.instruction_pointer = pcr.start_program_pointer;

  uint8_t board_started[7] = {0x53, 0x54, 0x41, 0x52, 0x54, 0x45, 0x44}; // "STARTED"
  can_write(MESSAGE_BOARD_STARTED, board_started, 7);

  /* Infinite loop */
  for (;;)
  {
    // Make sure the kill switch is handeld
    uint32_t state = osEventFlagsGet(ext_interrupt_eventHandle);
    if (EXTERN_INTERRUPT_EVENT_KILL == (state & EXTERN_INTERRUPT_EVENT_KILL))
    {
      HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin,
                        GPIO_PIN_RESET);
      osThreadTerminate(programTaskHandle);
      return;
    }

    uint8_t memory_entry = READ_MEMORY_BYTE(pcr.instruction_pointer);
    pcr.register1 = REGISTER(memory_entry);
    uint8_t opcode = OPCODE(memory_entry);
    // If the opcode is 0xFF, this should be interpreted as a 0x00
    // By default, the flash memory is filled with 0xFF, so this is a way to
    // detect the end of the program or an empty instruction.
    // Hitting this will imply that we have an bug in the program
    if (0xFF == opcode)
    {
        uint8_t message[5] = {ERR_INVALID_OPCODE,
                              (pcr.instruction_pointer >> 24) & 0xFF,
                              (pcr.instruction_pointer >> 16) & 0xFF,
                              (pcr.instruction_pointer >> 8) & 0xFF,
                              pcr.instruction_pointer & 0xFF};
        can_write(MESSAGE_LOG_EVENT, message, 5);
        telemetry_t telemetry = {
            pcr.instruction_pointer,
            pcr.end_program_pointer,
            ERR_INVALID_OPCODE, // Error code for invalid opcode
        };
        if (osOK != osMessageQueuePut(loggerQueueHandle, &telemetry, 0, 0U)) {
            printf("Error: Could not send message to loggerQueueHandle\n\r");
        }

        opcode = 0x00;
    }
    program_controller_function[opcode](&pcr);
  }
}

/***************************************
 * Callback functions
 **************************************/

/**
 * @brief  This function is called when an external interrupt occurs.
 * @param  GPIO_Pin: The pin that triggered the interrupt.
 *
 * This function will set the event flag for the interrupt and send telemetry
 * data to the logger queue. It will also check which pin triggered the
 * interrupt and set the corresponding flag.
 *
 * @note   This function is called from the HAL library.
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t flag = 0;
  switch (GPIO_Pin)
  {
  case PROGRAM_STATE_EMERGENCY_Pin:
    flag = EXTERN_INTERRUPT_EVENT_KILL;
    break;
  case PROGRAM_STATE_TRIGGER_Pin:
    flag = EXTERN_INTERRUPT_EVENT_PAUZE;
    break;
  default:
    break;
  }

  telemetry_t telemetry = {
      0,
      0,
      flag,
  };

  if (osOK != osMessageQueuePut(loggerQueueHandle, &telemetry, 0, 0U))
  {
    printf("Error: Could not send message to loggerQueueHandle\n\r");
  }
  // osEventFlagsSet(ext_interrupt_eventHandle, flag);
}