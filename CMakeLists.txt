cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(rp2040_trackball_mouse)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project

# Make sure TinyUSB can find tusb_config.h
#target_include_directories(rp2040_trackball_mouse PUBLIC
#        ${CMAKE_CURRENT_LIST_DIR})
#target_include_directories(rp2040_trackball_mouse PUBLIC
#    ${PICO_SDK_PATH}/lib/tynyusb/lib/networking)

add_executable(rp2040_trackball_mouse
    main.c usb_descriptors.c adns5050.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(rp2040_trackball_mouse PUBLIC pico_stdlib tinyusb_device tinyusb_board)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(rp2040_trackball_mouse)

include_directories(rp2040_trackball_mouse PRIVATE ${CMAKE_CURRENT_LIST_DIR})
