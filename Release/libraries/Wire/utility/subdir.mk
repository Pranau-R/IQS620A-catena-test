################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/eclipse/arduinoPlugin/packages/arduino/hardware/avr/1.6.9/libraries/Wire/utility/twi.c 

C_DEPS += \
./libraries/Wire/utility/twi.c.d 

LINK_OBJ += \
./libraries/Wire/utility/twi.c.o 


# Each subdirectory must supply rules for building sources it contributes
libraries/Wire/utility/twi.c.o: C:/eclipse/arduinoPlugin/packages/arduino/hardware/avr/1.6.9/libraries/Wire/utility/twi.c
	@echo 'Building file: $<'
	@echo 'Starting C compile'
	"C:\eclipse\arduinoPlugin\tools\arduino\avr-gcc\4.8.1-arduino5/bin/avr-gcc" -c -g -Os -std=gnu11 -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10606 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR     -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\cores\arduino" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\variants\standard" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\libraries\Wire" -I"C:\eclipse\arduinoPlugin\packages\arduino\hardware\avr\1.6.9\libraries\Wire\utility" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


