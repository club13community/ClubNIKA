################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freeRTOS/croutine.c \
../freeRTOS/event_groups.c \
../freeRTOS/list.c \
../freeRTOS/queue.c \
../freeRTOS/stream_buffer.c \
../freeRTOS/tasks.c \
../freeRTOS/timers.c 

OBJS += \
./freeRTOS/croutine.o \
./freeRTOS/event_groups.o \
./freeRTOS/list.o \
./freeRTOS/queue.o \
./freeRTOS/stream_buffer.o \
./freeRTOS/tasks.o \
./freeRTOS/timers.o 

C_DEPS += \
./freeRTOS/croutine.d \
./freeRTOS/event_groups.d \
./freeRTOS/list.d \
./freeRTOS/queue.d \
./freeRTOS/stream_buffer.d \
./freeRTOS/tasks.d \
./freeRTOS/timers.d 


# Each subdirectory must supply rules for building sources it contributes
freeRTOS/%.o: ../freeRTOS/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F1 -DSTM32F103VETx -DDEBUG -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER -I"E:/EmbedProjects/ClubNIKA/StdPeriph_Driver/inc" -I"E:/EmbedProjects/ClubNIKA/inc" -I"E:/EmbedProjects/ClubNIKA/CMSIS/device" -I"E:/EmbedProjects/ClubNIKA/CMSIS/core" -I"E:/EmbedProjects/ClubNIKA/fatfs/inc" -I"E:/EmbedProjects/ClubNIKA/freeRTOS/inc" -I"E:/EmbedProjects/ClubNIKA/freeRTOS/ARM_CM3" -I"E:/EmbedProjects/ClubNIKA/SDCard/inc" -I"E:/EmbedProjects/ClubNIKA/ClockControl/inc" -I"E:/EmbedProjects/ClubNIKA/SupplySystem/inc" -I"E:/EmbedProjects/ClubNIKA/GSMService/inc" -I"E:/EmbedProjects/ClubNIKA/UserInterface/inc" -I"E:/EmbedProjects/ClubNIKA/SoundService/inc" -I"E:/EmbedProjects/ClubNIKA/ApplicationLogic/inc" -I"E:/EmbedProjects/ClubNIKA/MessageRouter/inc" -I"E:/EmbedProjects/ClubNIKA/WirelessInterface/inc" -I"E:/EmbedProjects/ClubNIKA/SPIExtension/inc" -I"E:/EmbedProjects/ClubNIKA/WiredSensorMonitor/inc" -I"E:/EmbedProjects/ClubNIKA/UARTExtension/inc" -I"E:/EmbedProjects/ClubNIKA/I2CExtension/inc" -I"E:/EmbedProjects/ClubNIKA/VoltageMeter/inc" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


