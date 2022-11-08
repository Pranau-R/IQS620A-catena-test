#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2017-11-13 16:48:18

#include "Arduino.h"
#include "Arduino.h"
#include "IQS62x.h"
#include "IQS620_Init.h"
#include "Types.h"
#include "math.h"
#include "limits.h"
#include <EEPROM.h>
#include "src/Display/Display.h"
#include "src/I2C/I2C.h"
void setup() ;
void loop() ;
void process_IQS620_events() ;
void process_IQS620n_events() ;
void process_IQS621_events(bool *refreshDisplay) ;
void process_IQS624_events(bool *refreshDisplay) ;
void check_mode_button() ;
void sar_raw_mode() ;
void movement_raw_mode() ;
void temp_mode() ;
void hall_raw_mode() ;
void get_temp_reference() ;
uint16_t absolute(int16_t value) ;
void nsar_raw_mode() ;
void nmovement_raw_mode() ;
void ntemp_mode() ;
void nhall_raw_mode() ;
void nget_temp_reference() ;
void iqs_setup() ;
uint8_t setup_iqs621(Setup_Type_e setupType) ;
void redo_ati(void) ;
void absoluteMode() ;
void relativeMode() ;
void light_darkMode() ;
void proxMode() ;
void inductiveMode(bool *refreshDisplay) ;
bool isCoilPresent(void) ;
void absoluteWheelMode(bool *refreshDisplay) ;
void relativeWheelMode(bool *refreshDisplay) ;
void speedMode(bool *refreshDisplay);
void rawMode(bool *refreshDisplay);
void run_ati_algo(void) ;
void calibration(void) ;
void setTimer(Timer_t* timer) ;
void setTimer(Timer_t* timer, uint32_t time) ;
bool timerExpired(Timer_t* timer) ;
void clear_mode_leds() ;
void init_mode_leds() ;

#include "IQS62x_Demo.ino"


#endif
