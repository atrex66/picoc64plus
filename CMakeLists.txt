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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(c64_plus C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(c64_plus 
        src/6502_opcodes.c
        src/c64basic_emu.c
        src/syscalls.c
        src/modules/pwm.c
        #libs/pico-lfs/src/pico_lfs.c
        )

pico_set_program_name(c64_plus "c64_plus")
pico_set_program_version(c64_plus "0.1")

#pico_enable_stdio_usb(c64_plus 1)

# Copy entire binary to SRAM and run from there (no XIP cache pressure)
pico_set_binary_type(c64_plus copy_to_ram)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(c64_plus 0)
pico_enable_stdio_usb(c64_plus 1)

#target_compile_definitions(c64_plus PRIVATE
  #LFS_THREADSAFE=1
  #LFS_NO_DEBUG=1
#)

#add_subdirectory(libs/pico-lfs)

# Add the standard include files to the build
target_include_directories(c64_plus PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/src/include
  #      ${CMAKE_CURRENT_LIST_DIR}/libs/pico-lfs/src
 #       ${CMAKE_CURRENT_LIST_DIR}/libs/pico-lfs/libs/littlefs/
)


# Add the standard library to the build
target_link_libraries(c64_plus
        pico_stdlib
        pico_multicore
        hardware_vreg
        hardware_clocks
        hardware_pwm
        hardware_i2c
        hardware_dma
        hardware_adc
#        pico-lfs
        )

pico_add_extra_outputs(c64_plus)

target_compile_definitions(c64_plus PRIVATE
    PICO_STDIO_USB_RESET_INTERFACE_SUPPORT=1
)

target_compile_options(c64_plus PRIVATE 
    -O3
    #-mcpu=cortex-m33
)

add_custom_command(TARGET c64_plus POST_BUILD
    COMMAND arm-none-eabi-size $<TARGET_FILE:c64_plus>
)

target_link_options(c64_plus PRIVATE
    "-Wl,-Map=${CMAKE_BINARY_DIR}/c64_plus.map"
)