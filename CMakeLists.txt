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
set(CMAKE_PROJECT_NAME 11.5)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        user/main.c
        user/hardware/motor/Motor.c
        user/hardware/motor/Motor.h
        user/config/pidConfig.h
        user/common/bit.h
        user/common/args_define.h
        user/common/pid/pid.h
        user/common/pid/pid.c
        user/common/math/math_base.h
        user/interrupt/interrupt_TIM.c
        user/hardware/motor/motor_ir2104.h
        user/hardware/motor/motor_ir2104.c
        user/hardware/led/led.h
        user/hardware/led/led.c
        user/hardware/key/key.c
        user/hardware/key/key.h
        user/config/keyConfig.h
        user/hardware/motor/motor_encoder.c
        user/hardware/motor/motor_encoder.h
        user/config/motorConfig.h
        user/common/args_listener.c
        user/common/args_listener.h
)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# ---------------------------------------------------------------------------
# 嵌入式辅助：链接后生成 .hex / .bin，并提供 flash / erase / reset_target 目标
# ---------------------------------------------------------------------------
include(cmake/embedded.cmake)
