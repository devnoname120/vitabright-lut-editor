cmake_minimum_required(VERSION 2.8)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else ()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif ()
endif ()


set(SHORT_NAME VitaBrightTool)
set(VITA_TITLEID VBRIGHTTL)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(ENV{PKG_CONFIG_PATH} "$ENV{VITASDK}/arm-vita-eabi/lib/pkgconfig")
include(FindPkgConfig)

# Default build type
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release") # For CMake GUI
endif ()


set(VITA_ELF_NAME ${SHORT_NAME}.elf)
add_executable(${VITA_ELF_NAME} main.c)

target_link_libraries(${VITA_ELF_NAME}
        vitabright_stub
        vita2d
        SceDisplay_stub
        SceGxm_stub
        SceSysmodule_stub
        SceCtrl_stub
        ScePgf_stub
        ScePvf_stub
        SceCommonDialog_stub
        freetype
        png
        jpeg
        z
        m
        c
        SceAppMgr_stub
        )

vita_create_self(${SHORT_NAME}.self ${VITA_ELF_NAME} UNSAFE UNCOMPRESSED)
vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
        NAME ${SHORT_NAME}
        FILE resources/colorbars.png resources/colorbars.png
        FILE resources/matrix.png resources/matrix.png
        FILE resources/tv.jpg resources/tv.jpg
        )

get_filename_component(psvitaipFilePath psvitaip.txt REALPATH)
set(PSVITAIP 192.168.1.10)

add_custom_target(send
        COMMAND curl --ftp-method nocwd -T ${SHORT_NAME}.self ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/eboot.bin
        DEPENDS ${SHORT_NAME}.self
        )

add_custom_target(reload
        COMMAND nc ${PSVITAIP} 1338 reload ${VITA_TITLEID}
        )

add_custom_target(vpksend
        COMMAND curl --ftp-method nocwd -T ${SHORT_NAME}.vpk ftp://${PSVITAIP}:1337/ux0:/
        DEPENDS ${SHORT_NAME}.vpk
        )