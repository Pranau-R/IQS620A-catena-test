################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../libraries/Display/Display.cpp 

LINK_OBJ += \
./libraries/Display/Display.cpp.o 

CPP_DEPS += \
./libraries/Display/Display.cpp.d 


# Each subdirectory must supply rules for building sources it contributes
libraries/Display/Display.cpp.o: ../libraries/Display/Display.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\eclipse\arduinoPlugin\tools\arduino\avr-gcc\4.8.1-arduino5/bin/avr-g++" -c -g -Os -std=gnu++11 -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10606 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR     -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\cores\arduino" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\variants\standard" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\libraries\Wire" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\libraries\Wire\utility" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


