set(PROJECT_NAME Bad-Apple-IIgs)

cmake_minimum_required(VERSION 3.13...3.27)
include(pico_sdk_import.cmake)
project(${PROJECT_NAME} C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_add_extra_outputs(${PROJECT_NAME})

include(FetchContent)

FetchContent_Declare(a2pico
        GIT_REPOSITORY https://github.com/oliverschmidt/a2pico.git
        GIT_TAG main
        )
FetchContent_MakeAvailable(a2pico)

FetchContent_Declare(fatfs
        GIT_REPOSITORY https://github.com/FarLeftLane/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git
        GIT_TAG main
        SOURCE_SUBDIR src
        )
FetchContent_MakeAvailable(fatfs)

set_source_files_properties(incbin.S OBJECT_DEPENDS firmware.rom)
add_custom_command(
        COMMAND cl65 -t apple2 -C apple2-asm.cfg
                        ${CMAKE_CURRENT_SOURCE_DIR}/firmware.S
                     -o ${CMAKE_CURRENT_BINARY_DIR}/firmware.rom                    
        MAIN_DEPENDENCY firmware.S
        OUTPUT firmware.rom
        VERBATIM
        )

target_sources(${PROJECT_NAME} PRIVATE
        board.c
        hw_config.c
        incbin.S
        main.c
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        a2pico
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
        pico_multicore
        pico_stdlib
        )
