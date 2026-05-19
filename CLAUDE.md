# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Requires `arm-none-eabi-gcc` toolchain (via STM32CubeCLT ≥ 1.18.0) and Ninja on PATH.

```bash
# Configure (choose preset: Debug, Release, RelWithDebInfo, MinSizeRel)
cmake --preset Debug

# Build
cmake --build --preset Debug

# Output binary
build/Debug/2025-Software.elf
```

## Code Architecture

This is STM32F412ZG firmware running FreeRTOS, built around a **custom bytecode virtual machine** that interprets programs loaded from `binary_file.bin`.

### The Binary VM (`Middlewares/gwtonn/src/program_controller.c`)

The core concept: `binary_file.bin` is converted to an ELF object at link time via `objcopy` and placed in a custom `.program_data_block` linker section (see `STM32F412XX_FLASH.ld`). The VM reads bytecode from `_program_data_start` at runtime.

Bytecode format per instruction byte:
- Upper 4 bits (`OPCODE`): instruction index into `program_controller_function[]` dispatch table
- Lower 4 bits (`REGISTER`): stored in `register1` for the called function to use

The binary's first 12 bytes are three `uint32_t` pointers: `start_program_pointer`, `pauze_program_pointer`, `end_program_pointer`.

Opcodes: `HALT(0)`, `PAUSE(1)`, `DELAY(2)`, `SET_PIN_STATE(3)`, `PIN_TOGGLE(4)`, `LOG_PROGRAM_STATE(5)`, `JUMP(6)`. Opcode `0xFF` (erased flash) is treated as `HALT` with an error logged.

### FreeRTOS Tasks (defined in `Core/Src/freertos.c`)

| Task | Stack | Purpose |
|------|-------|---------|
| `programTask` (128×4 B) | Runs the VM interpreter loop |
| `logTask` (400×4 B) | Receives `telemetry_t` from `loggerQueueHandle` (queue of 10), writes CSV to SD card |
| `can_thread` (128×4 B) | Receives messages over UART1 (DMA, 12-byte frames), dispatches by message code |

External interrupts (PB1 = emergency kill, PB15 = trigger/pause) set event flags via `ext_interrupt_eventHandle`. The VM polls these in `vm_delay()` and the main loop.

### gwtonn Middleware (`Middlewares/gwtonn/`)

Custom application code lives here — this is the only directory safe to modify freely:

- `program_controller.c/h` — VM interpreter, pin mapping (17 pins across GPIOA/B), GPIO callbacks
- `logger.c/h` — `logger_task`: formats telemetry as CSV `[HH:MM:SS],ip,shutdown_idx,temp,vref,trigger` to `logger.txt` on SD
- `can.c/h` — "CAN" over UART1 (named for intent, not hardware). 12-byte fixed frames. Receives `MESSAGE_CODE_TIME` to sync RTC; sends events/telemetry/board-started
- `sd_card.c/h` — FatFS file write abstraction over SPI1
- `internal_sensors.c/h` — STM32 internal ADC (temperature, Vref via DMA), RTC get/set
- `datetime.c/h` — Dense 64-bit date/time encoding/decoding
- `asserts.h` — `ASSERT_NULL(ptr, err_code)` macro: prints and returns on NULL

### FatFS Configuration (`FATFS/Target/ffconf.h`)

Keep FatFS as small as possible. The critical settings that control memory footprint are:

| Setting | Value | Why |
|---------|-------|-----|
| `_MAX_SS` | `512` | SD cards use 512-byte sectors; setting equal to `_MIN_SS` eliminates runtime sector-size detection and fixes all internal buffers at 512 bytes |
| `_FS_TINY` | `1` | `FIL` structs carry no private sector buffer; they share the `FATFS.win[]` buffer instead, saving ~512 bytes per open file |
| `_USE_LFN` | `3` | LFN working buffer allocated on heap (not stack) via `pvPortMalloc` |

**Do not raise `_MAX_SS` above 512** — each increase adds that many bytes to every `FATFS` and `FIL` struct, which directly inflates task stack usage in `sd_card.c`. **Do not disable `_FS_TINY`** without also increasing `logTask` stack.

If STM32CubeMX regenerates `ffconf.h` and resets these values, restore them manually — they are intentional optimizations, not defaults.

### STM32CubeMX Generated Files

`Core/`, `Drivers/`, `FATFS/`, and `cmake/stm32cubemx/` are STM32CubeMX-generated. **Do not edit** outside of `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` guards — regeneration will overwrite changes. Exception: `FATFS/Target/ffconf.h` is user-owned and must be kept at the settings above after any regeneration.

### Peripherals

| Peripheral | Use |
|-----------|-----|
| SPI1 (PA4–PA7) | SD card (FatFS) |
| USART3 (PD8/PD9) | Debug `printf` output |
| USART1 | "CAN" external communication (DMA RX) |
| ADC (DMA) | Internal temperature sensor + Vref |
| RTC | Log timestamps |
| EXTI PB1 | Emergency kill switch |
| EXTI PB15 | Program trigger/pause |

### Coding Conventions

- 4-space indent, 80-column limit (enforced by `.clang-format`)
- Format with `clang-format` before committing
- Use `ASSERT_NULL(ptr, err_code)` for NULL guard checks in middleware functions
- `printf` goes to USART3; keep ISR callbacks short (no blocking calls)
- `logTask` stack is 400×4 = 1.6 KB — kept small deliberately. Do not add large local variables (buffers, structs) to `logger_task()` or `write_file()`; allocate on heap instead
