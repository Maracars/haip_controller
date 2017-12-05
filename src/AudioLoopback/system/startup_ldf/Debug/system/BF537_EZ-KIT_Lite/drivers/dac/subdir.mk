################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Analog\ Devices/ADSP-BF537_EZKIT-Rel1.0.0/BF537_EZ-KIT_Lite/Blackfin/src/drivers/dac/ad1854/adi_ad1854.c 

OBJS += \
./system/BF537_EZ-KIT_Lite/drivers/dac/adi_ad1854.doj 

C_DEPS += \
./system/BF537_EZ-KIT_Lite/drivers/dac/adi_ad1854.d 


# Each subdirectory must supply rules for building sources it contributes
system/BF537_EZ-KIT_Lite/drivers/dac/adi_ad1854.doj: C:/Analog\ Devices/ADSP-BF537_EZKIT-Rel1.0.0/BF537_EZ-KIT_Lite/Blackfin/src/drivers/dac/ad1854/adi_ad1854.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn.exe -c -file-attr ProjectName="AudioLoopback" -proc ADSP-BF537 -flags-compiler --no_wrap_diagnostics -si-revision any -g -D_DEBUG -I"C:\Users\nerea\proyecto modem\PRACS_BLACKFIN_ENUNCIADO\WS\AudioLoopback\system" -I"C:/Analog Devices/ADSP-BF537_EZKIT-Rel1.0.0/BF537_EZ-KIT_Lite/Blackfin/include" -I"C:\Analog Devices\CrossCore Embedded Studio 1.0.2\Blackfin\lib\src\libdsp" -structs-do-not-overlap -no-multiline -warn-protos -double-size-32 -decls-strong -cplbs -sdram -gnu-style-dependencies -MD -Mo "$(basename $@).d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


