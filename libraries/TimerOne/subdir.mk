################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../libraries/TimerOne/TimerOne.cpp 

LINK_OBJ += \
./libraries/TimerOne/TimerOne.cpp.o 

CPP_DEPS += \
./libraries/TimerOne/TimerOne.cpp.d 


# Each subdirectory must supply rules for building sources it contributes
libraries/TimerOne/TimerOne.cpp.o: ../libraries/TimerOne/TimerOne.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\eclipse\arduinoPlugin\tools\arduino\avr-gcc\4.8.1-arduino5/bin/avr-g++" -c -g -Os -std=gnu++11 -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10606 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR     -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\cores\arduino" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\variants\standard" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\libraries\Wire" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\libraries\Wire\utility" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


