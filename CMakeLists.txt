cmake_minimum_required ( VERSION 3.9 )
project ( EFIGenieW806 C CXX ASM )

set(LIBRARIES	libs)

include_directories(include)
include_directories(wm-sdk-w806/include)
include_directories(wm-sdk-w806/include/arch/xt804)
include_directories(wm-sdk-w806/include/arch/xt804/csi_core)
include_directories(wm-sdk-w806/include/arch/xt804/csi_dsp)
include_directories(wm-sdk-w806/include/driver)
include_directories(${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/W80x/include)

file(GLOB_RECURSE USER_CSOURCES src/*.c)
file(GLOB_RECURSE USER_CPPSOURCES src/*.cpp)
file(GLOB_RECURSE EmbeddedIOService_SOURCES ${LIBRARIES}/EFIGenie/libs/EmbeddedIOServices/HardwareAbstractions/W80x/src/*.cpp)

set(SOURCE_FILES 
        ${USER_CSOURCES} 
        ${USER_CPPSOURCES} 
        ${EmbeddedIOService_SOURCES})
add_executable(firmware.elf ${SOURCE_FILES})

add_subdirectory (${LIBRARIES}/EFIGenie)
target_link_libraries(firmware.elf EFIGenie)

set(HEX_FILE ${PROJECT_SOURCE_DIR}/build/firmware.hex)
set(BIN_FILE ${PROJECT_SOURCE_DIR}/build/firmware.bin)
add_custom_command(TARGET firmware.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex $<TARGET_FILE:firmware.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} ARGS -O binary $<TARGET_FILE:firmware.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
