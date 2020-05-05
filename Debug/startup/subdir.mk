################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../startup/startup_stm32.s 

OBJS += \
./startup/startup_stm32.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -I"E:/EmbedProjects/ClubNIKA/StdPeriph_Driver/inc" -I"E:/EmbedProjects/ClubNIKA/inc" -I"E:/EmbedProjects/ClubNIKA/CMSIS/device" -I"E:/EmbedProjects/ClubNIKA/CMSIS/core" -I"E:/EmbedProjects/ClubNIKA/fatfs/inc" -I"E:/EmbedProjects/ClubNIKA/freeRTOS/inc" -I"E:/EmbedProjects/ClubNIKA/freeRTOS/ARM_CM3" -I"E:/EmbedProjects/ClubNIKA/SDCard/inc" -I"E:/EmbedProjects/ClubNIKA/ClockControl/inc" -I"E:/EmbedProjects/ClubNIKA/SupplySystem/inc" -I"E:/EmbedProjects/ClubNIKA/GSMService/inc" -I"E:/EmbedProjects/ClubNIKA/UserInterface/inc" -I"E:/EmbedProjects/ClubNIKA/SoundService/inc" -I"E:/EmbedProjects/ClubNIKA/ApplicationLogic/inc" -I"E:/EmbedProjects/ClubNIKA/MessageRouter/inc" -I"E:/EmbedProjects/ClubNIKA/WirelessInterface/inc" -I"E:/EmbedProjects/ClubNIKA/SPIExtension/inc" -I"E:/EmbedProjects/ClubNIKA/WiredSensorMonitor/inc" -I"E:/EmbedProjects/ClubNIKA/UARTExtension/inc" -I"E:/EmbedProjects/ClubNIKA/I2CExtension/inc" -I"E:/EmbedProjects/ClubNIKA/VoltageMeter/inc" -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


