################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
ASM_SRCS += \
../system/startup_ldf/Debug/AudioLoopback.dxe.asm 

DOJ_SRCS += \
../system/startup_ldf/Debug/AudioLoopback.doj \
../system/startup_ldf/Debug/AudioLoopback.dxe.doj 

SRC_OBJS += \
./system/startup_ldf/Debug/AudioLoopback.dxe.doj 

ASM_DEPS += \
./system/startup_ldf/Debug/AudioLoopback.dxe.d 


# Each subdirectory must supply rules for building sources it contributes
system/startup_ldf/Debug/%.doj: ../system/startup_ldf/Debug/%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin Assembler'
	easmblkfn.exe -file-attr ProjectName="haip_controller" -proc ADSP-BF537 -si-revision any -g -D_DEBUG -i"C:\Users\joseb\Desktop\POPBL5T\haip_controller2\haip_controller\src\haip_controller\system" -i"C:/Analog Devices/ADSP-BF537_EZKIT-Rel1.0.0/BF537_EZ-KIT_Lite/Blackfin/include" -gnu-style-dependencies -MM -Mo "system/startup_ldf/Debug/AudioLoopback.dxe.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


