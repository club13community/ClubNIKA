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
	arm-none-eabi-as -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -I"E:/git/ClubNIKAfirmware/StdPeriph_Driver/inc" -I"E:/git/ClubNIKAfirmware/inc" -I"E:/git/ClubNIKAfirmware/CMSIS/device" -I"E:/git/ClubNIKAfirmware/CMSIS/core" -I"E:/git/ClubNIKAfirmware/fatfs/inc" -I"E:/git/ClubNIKAfirmware/freeRTOS/inc" -I"E:/git/ClubNIKAfirmware/freeRTOS/ARM_CM3" -I"E:/git/ClubNIKAfirmware/ClockControl/inc" -I"E:/git/ClubNIKAfirmware/SupplySystem/inc" -I"E:/git/ClubNIKAfirmware/GSMService/inc" -I"E:/git/ClubNIKAfirmware/UserInterface/inc" -I"E:/git/ClubNIKAfirmware/SoundService/inc" -I"E:/git/ClubNIKAfirmware/ApplicationLogic/inc" -I"E:/git/ClubNIKAfirmware/MessageRouter/inc" -I"E:/git/ClubNIKAfirmware/WirelessInterface/inc" -I"E:/git/ClubNIKAfirmware/SPIExtension/inc" -I"E:/git/ClubNIKAfirmware/WiredSensorMonitor/inc" -I"E:/git/ClubNIKAfirmware/UARTExtension/inc" -I"E:/git/ClubNIKAfirmware/I2CExtension/inc" -I"E:/git/ClubNIKAfirmware/VoltageMeter/inc" -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


