# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion RISCV_PICO_2_3_1_0)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(firmware C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(firmware
        src/main.cpp
        src/encoder.cpp
        src/lcd_pcf8574.cpp
        src/motor.cpp
        src/servo.cpp
)

set_source_files_properties(
        src/main.cpp
        src/encoder.cpp
        src/lcd_pcf8574.cpp
        src/motor.cpp
        src/servo.cpp
        PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra"
)

pico_set_program_name(firmware "firmware")
pico_set_program_version(firmware "0.1")

# USB CDCでテレメトリを出力する。UARTは未使用。
pico_enable_stdio_uart(firmware 0)
pico_enable_stdio_usb(firmware 1)

# USBホストが読んでいない場合に制御ループが長時間ブロックしないよう、
# stdoutのタイムアウトを短くする。
target_compile_definitions(firmware PRIVATE
        PICO_STDIO_USB_STDOUT_TIMEOUT_US=2000
)

# Add the standard library to the build
target_link_libraries(firmware
        pico_stdlib
        hardware_pwm
        hardware_irq
        hardware_gpio
        hardware_i2c
        pico_time
)

# Add the standard include files to the build
target_include_directories(firmware PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
)

pico_add_extra_outputs(firmware)

