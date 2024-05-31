cmake_minimum_required ( VERSION 3.9 )

# build libraries
include_directories(wm-sdk-w806/include)
include_directories(wm-sdk-w806/include/arch)
include_directories(wm-sdk-w806/include/arch/xt804)
include_directories(wm-sdk-w806/include/arch/xt804/csi_core)
include_directories(wm-sdk-w806/include/driver)
project(w806_drivers C)
file(GLOB_RECURSE w806_drivers_csrcs wm-sdk-w806/platform/drivers/*.c)
add_library(w806_drivers STATIC ${w806_drivers_csrcs})
project(w806_bsp C)
file(GLOB_RECURSE w806_bsp_csrcs wm-sdk-w806/platform/arch/xt804/bsp/*.c)
file(GLOB_RECURSE w806_bsp_asmsrcs wm-sdk-w806/platform/arch/xt804/bsp/*.S)
add_library(w806_bsp STATIC ${w806_bsp_csrcs} ${w806_bsp_asmsrcs})
project(w806_libc C)
file(GLOB_RECURSE w806_libc_csrcs wm-sdk-w806/platform/arch/xt804/libc/*.c)
add_library(w806_libc STATIC ${w806_libc_csrcs})

project ( EFIGenieW806 C CXX ASM )

set(LIBRARIES	libs)

include_directories(include)
include_directories(${LIBRARIES}/EmbeddedIOServices/HardwareAbstractions/W80x/include)

file(GLOB_RECURSE USER_CSOURCES src/*.c)
file(GLOB_RECURSE USER_CPPSOURCES src/*.cpp)
file(GLOB_RECURSE EmbeddedIOService_SOURCES ${LIBRARIES}/EmbeddedIOServices/HardwareAbstractions/W80x/src/*.cpp)

set(SOURCE_FILES 
        ${USER_CSOURCES} 
        ${USER_CPPSOURCES} 
        ${EmbeddedIOService_SOURCES})
add_executable(firmware.elf ${SOURCE_FILES})

add_subdirectory (${LIBRARIES}/EFIGenie)
target_link_libraries(firmware.elf w806_bsp)
target_link_libraries(firmware.elf w806_libc)
target_link_libraries(firmware.elf w806_drivers)
target_link_libraries(firmware.elf EFIGenie)

set(HEX_FILE ${PROJECT_SOURCE_DIR}/build/firmware.hex)
set(BIN_FILE ${PROJECT_SOURCE_DIR}/build/firmware.bin)
add_custom_command(TARGET firmware.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex $<TARGET_FILE:firmware.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} ARGS -O binary $<TARGET_FILE:firmware.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
