################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CMSIS/core/core_cm3.c 

OBJS += \
./CMSIS/core/core_cm3.o 

C_DEPS += \
./CMSIS/core/core_cm3.d 


# Each subdirectory must supply rules for building sources it contributes
CMSIS/core/%.o: ../CMSIS/core/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F1 -DSTM32F103VETx -DDEBUG -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER -I"E:/git/ClubNIKAfirmware/StdPeriph_Driver/inc" -I"E:/git/ClubNIKAfirmware/inc" -I"E:/git/ClubNIKAfirmware/CMSIS/device" -I"E:/git/ClubNIKAfirmware/CMSIS/core" -I"E:/git/ClubNIKAfirmware/fatfs/inc" -I"E:/git/ClubNIKAfirmware/freeRTOS/inc" -I"E:/git/ClubNIKAfirmware/freeRTOS/ARM_CM3" -I"E:/git/ClubNIKAfirmware/ClockControl/inc" -I"E:/git/ClubNIKAfirmware/SupplySystem/inc" -I"E:/git/ClubNIKAfirmware/GSMService/inc" -I"E:/git/ClubNIKAfirmware/UserInterface/inc" -I"E:/git/ClubNIKAfirmware/SoundService/inc" -I"E:/git/ClubNIKAfirmware/ApplicationLogic/inc" -I"E:/git/ClubNIKAfirmware/MessageRouter/inc" -I"E:/git/ClubNIKAfirmware/WirelessInterface/inc" -I"E:/git/ClubNIKAfirmware/SPIExtension/inc" -I"E:/git/ClubNIKAfirmware/WiredSensorMonitor/inc" -I"E:/git/ClubNIKAfirmware/UARTExtension/inc" -I"E:/git/ClubNIKAfirmware/I2CExtension/inc" -I"E:/git/ClubNIKAfirmware/VoltageMeter/inc" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


