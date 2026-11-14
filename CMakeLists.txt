cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
    if(DEFINED ENV{PICO_SDK_PATH})
        set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/.toolchain/pico-sdk/pico_sdk_init.cmake)
        set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/.toolchain/pico-sdk)
    endif()
endif()

if(NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH РЅРµ Р·Р°РґР°РЅ. Р—Р°РїСѓСЃС‚Рё tools/setup-toolchain.ps1 РёР»Рё tools/setup-toolchain.sh")
endif()

if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2 CACHE STRING "Pico board target")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(edgetx_elrs_usb_sim C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(edgetx_elrs_usb_sim
    src/main.c
    src/crsf.c
    src/usb_descriptors.c
)

target_include_directories(edgetx_elrs_usb_sim PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(edgetx_elrs_usb_sim
    pico_stdlib
    hardware_uart
    hardware_irq
    tinyusb_device
)

pico_enable_stdio_usb(edgetx_elrs_usb_sim 0)
pico_enable_stdio_uart(edgetx_elrs_usb_sim 0)
pico_add_extra_outputs(edgetx_elrs_usb_sim)

if(PICO_NO_PICOTOOL)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_command(TARGET edgetx_elrs_usb_sim POST_BUILD
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/tools/bin2uf2.py
            $<TARGET_FILE_DIR:edgetx_elrs_usb_sim>/edgetx_elrs_usb_sim.bin
            $<TARGET_FILE_DIR:edgetx_elrs_usb_sim>/edgetx_elrs_usb_sim.uf2
            --family 0xE48BFF59
            --base 0x10000000
        VERBATIM
    )
endif()
