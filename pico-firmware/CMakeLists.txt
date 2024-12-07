cmake_minimum_required(VERSION 3.13...3.27)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pico_coldfire_bdm)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(pico_coldfire_bdm
    main.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(pico_coldfire_bdm pico_stdlib)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(pico_coldfire_bdm)

# enable usb output, disable uart output
pico_enable_stdio_usb(pico_coldfire_bdm 1)
pico_enable_stdio_uart(pico_coldfire_bdm 0)