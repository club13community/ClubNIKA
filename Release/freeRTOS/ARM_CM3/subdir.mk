################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freeRTOS/ARM_CM3/port.c 

OBJS += \
./freeRTOS/ARM_CM3/port.o 

C_DEPS += \
./freeRTOS/ARM_CM3/port.d 


# Each subdirectory must supply rules for building sources it contributes
freeRTOS/ARM_CM3/%.o: ../freeRTOS/ARM_CM3/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F1 -DSTM32F103VETx -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER -I"E:/EmbedProjects/ClubNIKA/StdPeriph_Driver/inc" -I"E:/EmbedProjects/ClubNIKA/inc" -I"E:/EmbedProjects/ClubNIKA/CMSIS/device" -I"E:/EmbedProjects/ClubNIKA/CMSIS/core" -I"E:/EmbedProjects/ClubNIKA/fatfs/inc" -I"E:/EmbedProjects/ClubNIKA/freeRTOS/inc" -I"E:/EmbedProjects/ClubNIKA/freeRTOS/ARM_CM3" -I"E:/EmbedProjects/ClubNIKA/SDCard/inc" -I"E:/EmbedProjects/ClubNIKA/ClockControl/inc" -I"E:/EmbedProjects/ClubNIKA/SupplySystem/inc" -I"E:/EmbedProjects/ClubNIKA/GSMService/inc" -I"E:/EmbedProjects/ClubNIKA/UserInterface/inc" -I"E:/EmbedProjects/ClubNIKA/SoundService/inc" -I"E:/EmbedProjects/ClubNIKA/ApplicationLogic/inc" -I"E:/EmbedProjects/ClubNIKA/MessageRouter/inc" -I"E:/EmbedProjects/ClubNIKA/WirelessInterface/inc" -I"E:/EmbedProjects/ClubNIKA/UARTExtension/inc" -I"E:/EmbedProjects/ClubNIKA/I2CExtension/inc" -I"E:/EmbedProjects/ClubNIKA/SPIExtension/inc" -I"E:/EmbedProjects/ClubNIKA/WiredSensorMonitor/inc" -O3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


