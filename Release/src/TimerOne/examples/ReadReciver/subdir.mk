################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
PDE_SRCS += \
..\src\TimerOne\examples\ReadReciver\ReadReciver.pde 

PDE_DEPS += \
.\src\TimerOne\examples\ReadReciver\ReadReciver.pde.d 


# Each subdirectory must supply rules for building sources it contributes
src\TimerOne\examples\ReadReciver\ReadReciver.o: ..\src\TimerOne\examples\ReadReciver\ReadReciver.pde
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"E:\ide\sloeber\/arduinoPlugin/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2/bin/avr-g++" -c -g -Os -Wall -Wextra -std=gnu++11 -fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -flto -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10802 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR   -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\cores\arduino" -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\variants\standard" -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\libraries\EEPROM" -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\libraries\EEPROM\src" -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\libraries\SPI" -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\libraries\SPI\src" -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\libraries\Wire" -I"E:\ide\sloeber\arduinoPlugin\packages\arduino\hardware\avr\1.6.20\libraries\Wire\src" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"
	@echo 'Finished building: $<'
	@echo ' '


