#THIS FILE IS AUTO GENERATED FROM THE TEMPLATE! DO NOT CHANGE!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.27)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(ClubNIKA C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

#Uncomment for hardware floating point
#add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
#add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
#add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

#Uncomment for software floating point
add_compile_options(-mfloat-abi=soft)

# removed below -mthumb-interwork
add_compile_options(-mcpu=cortex-m3 -mthumb)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-O0 -g)
endif ()

include_directories(ApplicationLogic/inc ClockControl/inc fatfs/inc freeRTOS/inc freeRTOS/ARM_CM3 I2CExtension/inc inc
        GSMService/inc GSMService/sim900/inc Alarm/inc
        MessageRouter/inc SoundService/inc SPIExtension/inc SupplySystem/inc UARTExtension/inc UserInterface/inc
        VoltageMeter/inc WiredZones/inc WirelessInterface/inc Timing/inc Flash/inc
        SDCard/inc SDCard/driver/inc Settings/inc
        Logging/inc
        CMSIS/core CMSIS/device StdPeriph_Driver/inc periph_utils/inc)

add_definitions(-DSTM32 -DSTM32F1 -DSTM32F103VETx -DDEBUG -DSTM32F10X_HD -DUSE_HAL_DRIVER -DUSE_STDPERIPH_DRIVER)

file(GLOB_RECURSE SOURCES "ApplicationLogic/*.*" "CMSIS/*.*" "ClockControl/*.*" "I2CExtension/*.*"
        "GSMService/*.*" "GSMService/sim900/*.*" "Alarm/*.*"
        "MessageRouter/*.*" "SPIExtension/*.*" "SoundService/*.*" "StdPeriph_Driver/*.*" "SupplySystem/*.*"
        "UARTExtension/*.*" "UserInterface/*.*" "UserInterface/controllers/*.*" "VoltageMeter/*.*" "WiredZones/*.*" "WirelessInterface/*.*"
        "Flash/*.*" "fatfs/*.*" "freeRTOS/*.*" "inc/*.*" "src/*.*" "startup/*.*" "Timing/*.*" "periph_utils/*.*"
        "SDCard/*.*" "SDCard/driver/*.*" "Settings/*.*" "Logging/*.*"
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/LinkerScript.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
# removed below -mthumb-interwork
add_link_options(-mcpu=cortex-m3 -mthumb)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
