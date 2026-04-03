/**
 ******************************************************************************
 * @file   internal_sensors.h
 * @brief  Headerfile for the internal_sensors.c file
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

#ifndef INC_PROGAM_CONTROLLER_H_
#define INC_PROGAM_CONTROLLER_H_

#include "cmsis_os2.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif // End of extern "C" block

#define EXTERN_INTERRUPT_EVENT_PAUZE 0x01
#define EXTERN_INTERRUPT_EVENT_KILL 0x02

#define MASK_OPCODE 0xF0
#define OPCODE(x) ((uint8_t)((x & MASK_OPCODE) >> 4))

#define MASK_REGISTER 0x0F
#define REGISTER(x) ((uint8_t)(x & MASK_REGISTER))

#define VM_OK 0x0002
#define VM_EXIT 0x0002
#define VM_ERROR 0x0001

#define ERR_HARD_FAILURE 0x01
#define ERR_INVALID_OPCODE 0x02

  extern uint32_t _program_data_start;
  extern uint32_t _program_data_end;

  extern osThreadId_t programTaskHandle;
  extern const osThreadAttr_t programTask_attributes;
  /* Definitions for ext_interrupt_event */
  extern osEventFlagsId_t ext_interrupt_eventHandle;
  extern const osEventFlagsAttr_t ext_interrupt_event_attributes;

  typedef struct
  {
    uint32_t instruction_pointer;
    uint8_t register1; // At this point, we only need one register

    uint32_t start_program_pointer;
    uint32_t pauze_program_pointer;
    uint32_t end_program_pointer;

  } program_controller_registers_t;

#ifdef __cplusplus
}
#endif // End of extern "C" block

#endif // INC_PROGAM_CONTROLLER_H_