cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME firmware)

set (MX_Defines_Syms
	#STM32F412Zx
    STM32F412Rx
)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Middlewares/gwtonn/src/program_controller.c
    Middlewares/gwtonn/src/can.c
    Middlewares/gwtonn/src/datetime.c
    Middlewares/gwtonn/src/internal_sensors.c
    Middlewares/gwtonn/src/logger.c
    Middlewares/gwtonn/src/sd_card.c
    Middlewares/gwtonn/src/user_diskio_spi.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Middlewares/gwtonn/include/
    .
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    # Add user defined libraries
)

# Add a script before build that will perform the objcopy operation
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_LINK
    COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-littlearm -B arm ${CMAKE_CURRENT_SOURCE_DIR}/binary_file.bin ${CMAKE_CURRENT_BINARY_DIR}/binary_file.o --rename-section .data=.program_data_block
    COMMENT "Generating binary file for ELF"
)