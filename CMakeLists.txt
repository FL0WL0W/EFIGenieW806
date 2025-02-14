cmake_minimum_required (VERSION 3.30[3.30])
Set(FETCHCONTENT_QUIET FALSE)
include(FetchContent)

set(LIBRARIES_DIR	    ${CMAKE_SOURCE_DIR}/libs)

#Fetch wm-sdk-w806
FetchContent_Declare(
    wm-sdk-w806
    GIT_REPOSITORY    https://github.com/FL0WL0W/wm-sdk-w806
    GIT_TAG           main
    GIT_PROGRESS      TRUE
    SOURCE_DIR        ${LIBRARIES_DIR}/wm-sdk-w806
)
FetchContent_MakeAvailable(wm-sdk-w806)

# build wm-sdk-w806 libraries
include_directories(${LIBRARIES_DIR}/wm-sdk-w806/include)
include_directories(${LIBRARIES_DIR}/wm-sdk-w806/include/arch)
include_directories(${LIBRARIES_DIR}/wm-sdk-w806/include/arch/xt804)
include_directories(${LIBRARIES_DIR}/wm-sdk-w806/include/arch/xt804/csi_core)
include_directories(${LIBRARIES_DIR}/wm-sdk-w806/include/driver)
project(w806_bsp C ASM)
file(GLOB_RECURSE w806_bsp_csrcs ${LIBRARIES_DIR}/wm-sdk-w806/platform/arch/xt804/bsp/*.c)
file(GLOB_RECURSE w806_bsp_asmsrcs ${LIBRARIES_DIR}/wm-sdk-w806/platform/arch/xt804/bsp/*.S)
add_library(w806_bsp STATIC ${w806_bsp_csrcs} ${w806_bsp_asmsrcs})
project(w806_drivers C)
file(GLOB_RECURSE w806_drivers_csrcs ${LIBRARIES_DIR}/wm-sdk-w806/platform/drivers/*.c)
add_library(w806_drivers STATIC ${w806_drivers_csrcs})
# project(w806_libc C)
# file(GLOB_RECURSE w806_libc_csrcs ${LIBRARIES_DIR}/wm-sdk-w806/platform/arch/xt804/libc/*.c)
# add_library(w806_libc STATIC ${w806_libc_csrcs})

#Fetch EFIGenie
FetchContent_Declare(
    EFIGenie
    GIT_REPOSITORY    https://github.com/FL0WL0W/EFIGenie
    GIT_TAG           main
    GIT_PROGRESS      TRUE
    GIT_SHALLOW       TRUE
    SOURCE_DIR        ${LIBRARIES_DIR}/EFIGenie
)
FetchContent_MakeAvailable(EFIGenie)

set(TARGET		W806EFIGenie)

project(${TARGET} C CXX ASM)

include_directories(include)
include_directories(${LIBRARIES_DIR}/EmbeddedIOServices/HardwareAbstractions/W80x/include)
include_directories(${LIBRARIES_DIR}/EmbeddedIOServices/HardwareAbstractions/ATTiny427Expander/include)

file(GLOB_RECURSE USER_CSOURCES src/*.c)
file(GLOB_RECURSE USER_CPPSOURCES src/*.cpp)
file(GLOB_RECURSE EmbeddedIOServiceW80x_SOURCES ${LIBRARIES_DIR}/EmbeddedIOServices/HardwareAbstractions/W80x/src/*.cpp)

set(SRCS 
        ${USER_CSOURCES} 
        ${USER_CPPSOURCES}
        ${EmbeddedIOServiceW80x_SOURCES})
add_executable(${TARGET}.elf ${SRCS})

target_link_libraries(${TARGET}.elf w806_bsp)
target_link_libraries(${TARGET}.elf w806_drivers)
# target_link_libraries(${TARGET}.elf w806_libc)
target_link_libraries(${TARGET}.elf EFIGenie)

# modify wm_tool so it prints download status on newlines since vscode buffers the output
file(READ ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/tools/W806/wm_tool.c FILE_CONTENTS)
string(REPLACE "wm_tool_printf(\"0%% [\");" "wm_tool_printf(\"0%% -\\r\\n\");" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "wm_tool_printf(\"#\");" "wm_tool_printf(\"####\\r\\n\");" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "wm_tool_printf(\"] 100%%\\r\\n\");" "wm_tool_printf(\"100%%\\r\\n\");" FILE_CONTENTS "${FILE_CONTENTS}")
file(WRITE ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/tools/W806/wm_tool.c "${FILE_CONTENTS}")

set(BIN_FILE ${CMAKE_BINARY_DIR}/${TARGET}.bin)
add_custom_command(TARGET ${TARGET}.elf POST_BUILD
        COMMAND gcc ${CMAKE_BINARY_DIR}/_deps/wm-sdk-w806-src/tools/W806/wm_tool.c -lpthread -o "${CMAKE_BINARY_DIR}/wm_tool"
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}.elf> ${BIN_FILE}
        COMMAND ${CMAKE_BINARY_DIR}/wm_tool -b ${BIN_FILE} -fc 0 -it 0 -ih 8002000 -ra 8002400 -ua 8002000 -nh 0 -un 0 -o "${CMAKE_BINARY_DIR}/${TARGET}"
        COMMAND cat "${CMAKE_BINARY_DIR}/${TARGET}.img" > "${CMAKE_BINARY_DIR}/${TARGET}.fls"
        COMMENT "Building ${BIN_FILE} ${CMAKE_BINARY_DIR}/${TARGET}.fls")

set(UPLOAD_ADDRESS "http://192.168.4.1/upload/w806" CACHE STRING "web address to upload img to")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/upload.success
    COMMAND ${CMAKE_COMMAND} -E echo "Uploading to ${UPLOAD_ADDRESS}"
    COMMAND bash -c "curl --data-binary @${CMAKE_BINARY_DIR}/${TARGET}.fls http://192.168.4.1/upload/w806"
    COMMENT "Uploading ${CMAKE_BINARY_DIR}/${TARGET}.fls")

add_custom_target(
    upload
    DEPENDS ${TARGET}.elf
    DEPENDS ${TARGET}.elf ${CMAKE_BINARY_DIR}/upload.success
)

set(DL_PORT "ttyUSB0" CACHE STRING "port to flash firmware to")
set(DL_BAUD "2000000" CACHE STRING "baud rate to flash device")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/flashed.success
    COMMAND ${CMAKE_COMMAND} -E echo "Flashing to ${DL_PORT} at ${DL_BAUD}"
    COMMAND ${CMAKE_BINARY_DIR}/wm_tool -c ${DL_PORT} -rs rts -ds ${DL_BAUD} -dl "${CMAKE_BINARY_DIR}/${TARGET}.fls"
    COMMENT "Flashing ${CMAKE_BINARY_DIR}/${TARGET}.fls"
    VERBATIM)

add_custom_target(
    flash
    DEPENDS ${TARGET}.elf
    DEPENDS ${TARGET}.elf ${CMAKE_BINARY_DIR}/flashed.success
)
