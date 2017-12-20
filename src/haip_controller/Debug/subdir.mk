################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../haip_16QAM_mapping.c \
../haip_commons.c \
../haip_demodulator.c \
../haip_hamming_7_4_ext_coding.c \
../haip_modem.c \
../haip_modulator.c \
../haip_srcos_filter_fr32.c \
../haip_tx_rx.c 

LDF_SRCS += \
../app.ldf 

SRC_OBJS += \
./haip_16QAM_mapping.doj \
./haip_commons.doj \
./haip_demodulator.doj \
./haip_hamming_7_4_ext_coding.doj \
./haip_modem.doj \
./haip_modulator.doj \
./haip_srcos_filter_fr32.doj \
./haip_tx_rx.doj 

C_DEPS += \
./haip_16QAM_mapping.d \
./haip_commons.d \
./haip_demodulator.d \
./haip_hamming_7_4_ext_coding.d \
./haip_modem.d \
./haip_modulator.d \
./haip_srcos_filter_fr32.d \
./haip_tx_rx.d 


# Each subdirectory must supply rules for building sources it contributes
%.doj: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn.exe -c -file-attr ProjectName="haip_controller" -proc ADSP-BF537 -flags-compiler --no_wrap_diagnostics -si-revision any -g -D_DEBUG -I"C:\Users\joseb\Desktop\POPBL5T\haip_controller\src\haip_controller\system" -I"C:/Analog Devices/ADSP-BF537_EZKIT-Rel1.0.0/BF537_EZ-KIT_Lite/Blackfin/include" -I"C:\Analog Devices\CrossCore Embedded Studio 1.0.2\Blackfin\lib\src\libdsp" -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -double-size-32 -decls-strong -cplbs -sdram -gnu-style-dependencies -MD -Mo "haip_16QAM_mapping.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


