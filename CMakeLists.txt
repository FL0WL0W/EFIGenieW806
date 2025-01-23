cmake_minimum_required ( VERSION 3.9 )
Set(FETCHCONTENT_QUIET FALSE)
include(FetchContent)

#Fetch wm-sdk-w806
FetchContent_Declare(
    wm-sdk-w806
    GIT_REPOSITORY    https://github.com/IOsetting/wm-sdk-w806
    GIT_TAG           main
    GIT_PROGRESS      TRUE
)
FetchContent_Populate(wm-sdk-w806)
 
# build libraries
include_directories(${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/include)
include_directories(${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/include/arch)
include_directories(${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/include/arch/xt804)
include_directories(${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/include/arch/xt804/csi_core)
include_directories(${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/include/driver)
project(w806_drivers C)
file(GLOB_RECURSE w806_drivers_csrcs ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/platform/drivers/*.c)
add_library(w806_drivers STATIC ${w806_drivers_csrcs})
project(w806_bsp C)
file(GLOB_RECURSE w806_bsp_csrcs ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/platform/arch/xt804/bsp/*.c)
file(GLOB_RECURSE w806_bsp_asmsrcs ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/platform/arch/xt804/bsp/*.S)
add_library(w806_bsp STATIC ${w806_bsp_csrcs} ${w806_bsp_asmsrcs})
project(w806_libc C)
file(GLOB_RECURSE w806_libc_csrcs ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/platform/arch/xt804/libc/*.c)
add_library(w806_libc STATIC ${w806_libc_csrcs})

#Fetch EFIGenie
FetchContent_Declare(
    EFIGenie
    GIT_REPOSITORY    https://github.com/FL0WL0W/EFIGenie
    GIT_TAG           main
    GIT_PROGRESS      TRUE
)
FetchContent_MakeAvailable(EFIGenie)

project ( EFIGenieW806 C CXX ASM )

set(LIBRARIES	libs)

include_directories(include)
include_directories(libs/EmbeddedIOServices/HardwareAbstractions/W80x/include)

file(GLOB_RECURSE USER_CSOURCES src/*.c)
file(GLOB_RECURSE USER_CPPSOURCES src/*.cpp)
file(GLOB_RECURSE EmbeddedIOService_SOURCES libs/EmbeddedIOServices/HardwareAbstractions/W80x/src/*.cpp)

set(SOURCE_FILES 
        ${USER_CSOURCES} 
        ${USER_CPPSOURCES} 
        ${EmbeddedIOService_SOURCES})
add_executable(firmware.elf ${SOURCE_FILES})

target_link_libraries(firmware.elf w806_bsp)
target_link_libraries(firmware.elf w806_libc)
target_link_libraries(firmware.elf w806_drivers)
target_link_libraries(firmware.elf EFIGenie)

set(BIN_FILE ${CMAKE_BINARY_DIR}/firmware.bin)
add_custom_command(TARGET firmware.elf POST_BUILD
        COMMAND bash -c "gcc ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/tools/W806/wm_tool.c -lpthread -o ${CMAKE_BINARY_DIR}/wm_tool"
        COMMAND ${CMAKE_OBJCOPY} ARGS -O binary $<TARGET_FILE:firmware.elf> ${BIN_FILE}
        COMMAND bash -c "${CMAKE_BINARY_DIR}/wm_tool -b ${BIN_FILE} -fc 0 -it 0 -ih 8002000 -ra 8002400 -ua 8002000 -nh 0 -un 0 -o ${CMAKE_BINARY_DIR}/firmware"
        COMMENT "Building ${BIN_FILE} ${CMAKE_BINARY_DIR}/firmware.img")