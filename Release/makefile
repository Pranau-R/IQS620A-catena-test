################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ..\makefile.init

RM := del 

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src\TimerOne\examples\ReadReciver\subdir.mk
-include src\TimerOne\examples\ISRBlink\subdir.mk
-include src\TimerOne\subdir.mk
-include src\I2C\subdir.mk
-include src\FFT\examples\fft_codec\subdir.mk
-include src\FFT\examples\fft_adc_serial\subdir.mk
-include src\FFT\examples\fft_adc\subdir.mk
-include src\Display\subdir.mk
-include src\ArduinoFFT2\reorder_table_creator\subdir.mk
-include src\ArduinoFFT2\FFT\examples\fft_codec\subdir.mk
-include src\ArduinoFFT2\FFT\examples\fft_adc_serial\subdir.mk
-include src\ArduinoFFT2\FFT\examples\fft_adc\subdir.mk
-include libraries\Wire\src\utility\subdir.mk
-include libraries\Wire\src\subdir.mk
-include libraries\SPI\src\subdir.mk
-include core\core\subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ..\makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
AR += \
D:\Workspace_arduino\CappoProxFusion_EvKit\ArduinoCode\IQS62x_Demo\Release\arduino.ar \

ELF += \
IQS62x_Demo.elf \

HEX += \
IQS62x_Demo.hex \


# All Target
all: IQS62x_Demo

# Tool invocations
IQS62x_Demo: $(HEX)
	@echo 'Building target: $@'
	@echo 'Printing size:'
	"E:\ide\sloeber\/arduinoPlugin/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2/bin/avr-size" -A "D:/Workspace_arduino/CappoProxFusion_EvKit/ArduinoCode/IQS62x_Demo/Release/IQS62x_Demo.elf"
	@echo 'Finished building target: $@'
	@echo ' '

D:\Workspace_arduino\CappoProxFusion_EvKit\ArduinoCode\IQS62x_Demo\Release\arduino.ar: $(AR_OBJ)
	@echo 'Starting archiver'
	"E:\ide\sloeber\/arduinoPlugin/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2/bin/avr-gcc-ar" rcs    D:/Workspace_arduino/CappoProxFusion_EvKit/ArduinoCode/IQS62x_Demo/Release/arduino.ar $(AR_OBJ)
	@echo 'Finished building: $@'
	@echo ' '

IQS62x_Demo.elf: $(LINK_OBJ) $(AR)
	@echo 'Starting combiner'
	"E:\ide\sloeber\/arduinoPlugin/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2/bin/avr-gcc" -Wall -Wextra -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections -mmcu=atmega328p -o "D:\Workspace_arduino\CappoProxFusion_EvKit\ArduinoCode\IQS62x_Demo/Release/IQS62x_Demo.elf"   $(LINK_OBJ) $(AR)   D:/Workspace_arduino/CappoProxFusion_EvKit/ArduinoCode/IQS62x_Demo/Release/arduino.ar  "-LD:\Workspace_arduino\CappoProxFusion_EvKit\ArduinoCode\IQS62x_Demo/Release" -lm
	@echo 'Finished building: $@'
	@echo ' '

IQS62x_Demo.hex: $(ELF)
	@echo 'Create eeprom image'
	${A.RECIPE.OBJCOPY.EEP.PATTERN}
	${A.RECIPE.OBJCOPY.HEX.PATTERN}
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C_UPPER_DEPS)$(PDE_DEPS)$(C_DEPS)$(AR)$(CC_DEPS)$(AR_OBJ)$(C++_DEPS)$(LINK_OBJ)$(CXX_DEPS)$(ASM_DEPS)$(HEX)$(INO_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(ELF)$(CPP_DEPS)
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ..\makefile.targets
