/******************************************************************************
 *                                                                            *
 *                                                                            *
 *                           	Copyright by                                    *
 *                                                                            *
 *                         	  Azoteq (Pty) Ltd                                *
 *                     	  Republic of South Africa                            *
 *                                                                            *
 *                     	   Tel: +27(0)21 863 0033                             *
 *                    	  E-mail: info@azoteq.com                             *
 *                                                                            *
 *============================================================================*
 * @file 	    IQS62x_Demo.ino							 					                          *
 * @brief 	  Arduino example code for IQS62x			              		          *
 * @author 	  AJ van der Merwe; L Theron; PJ Winterbach - Azoteq PTY Ltd      *
 * @version 	V1.3.0                                                          *
 * @date 	    10/25/2018                                                      *
 ******************************************************************************/

#include "Arduino.h"
#include "IQS62x.h"
#include "IQS620_Init.h"
#include "Types.h"
//#include "math.h"
#include "limits.h"
#include <stm32_eeprom.h>
//#include "src/Display/Display.h"
#include "src/I2C/I2C.h"
#include <Wire.h>

/*	Global defines	-----------------------------------------------------------*/

// Mode Leds
#define LED_1         14
#define LED_2         15
#define LED_3         16
#define LED_4         17
#define NR_OF_MODES   4

#define	MS_500		  500
#define	ONE_SEC		  1000
#define	TWO_SEC		  2000
#define THREE_SEC   3000
#define	TWELVE_SEC	12000
#define	MS_5		    5

//-----------------------------------------------------------------------------
//------------------------ IQS624 constants -----------------------------------
//-----------------------------------------------------------------------------
#define MEM_COUNT_RESET              5
#define RPM_COUNT_RESET             10
#define RPM_CONSTANT 	              11	// 60/(report rate (ms) * 360)
#define RELATIVE_DIFF_THRESH        10
#define REFRESH_RPM_DISPLAY         0xFFFF

// Defines for IQS621
#define COIL_THRESHOLD				    7000

#define FERRITE
// fun

// Debug with serial comms
 #define DEBUG

// Define if absolute temperature, or delta temp
#define	ABS_TEMP

// Beta
#define BETA 		1.0

// Create constants to determine the ATI Settings
#define COUNT_VALUE			             1200
#define BASE_THRESHOLD		            500
#define STOP_THRESHOLD		            100

// Define Addresses for EEPROM
#define WRITE_FLAG_ADDRESS		        0x00
#define COS_PHASE_ADDRESS		          0x01
#define SINE_PHASE_ADDRESS		        0x02
#define ATI_FLAG_ADDRESS		          0x03
#define HALL_ATI_CH2_CH3_ADDRESS		  0x04
#define HALL_ATI_CH4_CH5_ADDRESS		  0x05

/*	Typedefs		---------------------- ----------------------------------------*/

// Enum to move between modes for this arduino demo
typedef enum Modes {
	Mode_1 = 0,				// SAR Mode
	Mode_2 = 1,				// Movement Mode
	Mode_3 = 2,				// Temp Mode
	Mode_4 = 3,				// Hall Mode
	Mode_5 = 4,				// Extra Mode
	Mode_6 = 5				// Extra Mode
} Mode_e;

// Enum to show button presses
typedef enum Button_state {
	Touch,
	Pressed,
	Released,
	Double_Tap,
	Btn_Timeout,
	Btn_LongPress
} Button_state_e;

// Enum to show button presses
typedef enum Loop_state {
	Run,
	Switch_Mode,
	Display_Info
} Loop_state_e;

// Enum to determine what to show on screen
typedef enum Display_state {
	Display_Int,
	Display_String
} Display_state_e;

// Enum to determine what to show on screen
typedef enum IC_Type {
	IQS620,
	IQS620n,
	IQS621,
	IQS622,
	IQS624,
  IQS624n,
  IQS625
} IC_Type_e;

typedef enum Quads {
	Quad_1 = 0,
	Quad_2 = 1,
	Quad_4 = 2,
	Quad_3 = 3
}Quads_t;


/*	Global Variables	---------------------------------------------------------*/

// The Mode state of the demo
Mode_e Mode;

// What are we displaying
Display_state_e displayState;

// What type of IC is this?
IC_Type_e ICType;

// Leds for convenience
int Leds[4] = {LED_1, LED_2, LED_3, LED_4};

// Timer 1
Timer_t Mode_Switch_Timer = {0};		// Mode switch timer
Timer_t ErrorTimer	= {0};			// Error Timer
Timer_t MainTimer	= {0};			// Error Timer
Timer_t ButtonTimer	= {0};			// Button double tap Timer
Timer_t CalibTimer	= {0};			// Calibration Timeout Timer
Timer_t CoilTimer 	= {0};			// Coil saturation timer

// Which state of the loop are we in?
Loop_state_e Loop = Run;

// Mode button
Button_state_e mode_button = Released;

// Aux button
Button_state_e aux_button = Released;

// I2C Instance
I2C i2c(I2C_ADDRESS, IQS62x_RDY, Active_Low);

// Test Display
//Display disp = Display();

//ProxFusion IC's
IQS620_t iqs620;				  // Create variable for iqs620
IQS620n_t iqs620n;				// Create variable for iqs620A
IQS621_t iqs621;				  // Create variable for iqs621
IQS622_t iqs622;        	// Create variable for iqs622
IQS624_t iqs624;				  // Create variable for iqs624
IQS624n_t iqs624n;        // Create variable for iqs624
IQS625_t iqs625;          // Create variable for iqs625

// A number to display
int16_t display_number = 0;

// A String to display
char display_string[4] = {0};

// Indicate first entry to multi mode
bool getTempReference = true;

// Indicate chip is ready for polling
bool chipReady = false;
bool pushed = false;
bool pushedflag = false;
// Temperature reference
uint16_t TemperatureReference = 0;
uint16_t Temperature = 0;
int16_t deltaTemp = 0;

// Buffer to read data into
uint8_t buffer[20];

// Global for storing the degrees from IQS624
int16_t deg_mem = 0;
int16_t deg = 0;
int16_t relative_counter = 0;
int16_t relative_counter2 = 0;
int16_t diff_mem = 0;
int16_t speed_rpm = 0;
bool getDeg = true;
bool move_forw = false;
bool move_back = false;
bool rpm_refresh = true;
uint8_t mem_count = MEM_COUNT_RESET;
uint16_t RPM_refresh_count = REFRESH_RPM_DISPLAY;
int16_t old_interval;
// Create a mode button
bool modeButton = false;

// Flag to indicate whether we should do a setup of inductive sensor
bool setupInductiveSensor = true;

bool coilCheck = true;

bool first = true; // First calibration entry
uint8_t algoSM = 0;
uint8_t littleSM = 0;
bool modeEntry = true;

// Global to indicate that calibration was done.

/*bool writeRegister()
	{
    Wire.beginTransmission((uint8_t) I2C_ADDRESS);
    Wire.write(command);
    Wire.write(parameter);

    if (Wire.endTransmission() != 0)
        {
        return false;
        }

    return true;
    }*/

//The setup function is called once at startup of the sketch
void setup()
{
	// Setup Leds
	init_mode_leds();
 
//#ifdef DEBUG
	//Setup serial comms
	Serial.begin(9600);
//#endif

	Wire.begin();

    Serial.println ("**** This is an Example for IQS620AEV1 ****");
	// Check which IC we are using
	while(!Wire.available());	// Wait for device to begome ready

	// Get the Version info
	Wire.read();
	// Set the appropriate IC
	if(buffer[0] == IQS620_PRODUCT_NR)
	{
		if(buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
			ICType = IQS620n;
		else
			ICType = IQS620;
	}
	else if(buffer[0] == IQS621_PRODUCT_NR)
		  ICType = IQS621;
	else if(buffer[0] == IQS622_PRODUCT_NR)
    	ICType = IQS622;
	else if(buffer[0] == IQS624_PRODUCT_NR)
	{
    if(buffer[1] == IQS624N_SOFTWARE_NR)
      ICType = IQS624n;
    else
      ICType = IQS624;
	}
  else if(buffer[0] == IQS625_PRODUCT_NR)
      ICType = IQS625;
	// No valid IC type found
	else
	{
		//disp.writeError(2);
		Serial.println("Err invalid IC...");
		while(1);
	}
  //Serial.println(ICType);
	// Do initial setup
	iqs_setup();

	// Set the appropriate mode led
	digitalWrite(Leds[(uint8_t)Mode], HIGH);

	//disp.write(display_number = (((uint8_t)Mode%4)+1)*1111);    // Write mode to display
	//Serial.print("Mode : ");
  delay(1000);
  
	// Initialise Mode timer
	Mode_Switch_Timer.Timer_counter = ONE_SEC;	// 1s timer

	ErrorTimer.Timer_counter = THREE_SEC;			// 3s timer

	MainTimer.Timer_counter = ONE_SEC;      // 1s timer

	ButtonTimer.Timer_counter = 300;			// 300ms timer

	displayState = Display_Int;					// By default display an int
}

// The loop function is called in an endless loop
void loop()
{
	//Add your repeated code here
	bool refreshDisplay = false;
	uint8_t res = 0;

	// Read IQS device for information
	// Acquire all the necessary data
	if(Wire.available())
	{
		if(ICType == IQS620)
		{
			// Read version number to insure we still have the correct device attached - otherwise, do setup
			res = Wire.read();

			// System flags, Global Events and PXS UI Flags - 9 bytes
			res |= Wire.read();

			// Read PXS Channel Data - 12 bytes
			res |= Wire.read();

			// Read LTA value of Channel 1 for Movement mode
			res |= Wire.read();

			// Set the appropriate IC
			if(buffer[0] == IQS620_PRODUCT_NR)
			{
				if(buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
				{
					ICType = IQS620n;
					// Do setup
					iqs_setup();
				}
			}
			else if(buffer[0] == IQS621_PRODUCT_NR)
			{
				ICType = IQS621;
				// Do setup
				iqs_setup();
			}
			else if(buffer[0] == IQS622_PRODUCT_NR)
     	{
        ICType = IQS622;
        // Do setup
        iqs_setup();
      }
			else if(buffer[0] == IQS624_PRODUCT_NR)
			{
        if(buffer[1] == IQS624N_SOFTWARE_NR)
          ICType = IQS624n;
        else
          ICType = IQS624;
				// Do setup
				iqs_setup();
			}
      else if(buffer[0] == IQS625_PRODUCT_NR)
      {
        ICType = IQS625;
        // Do setup
        iqs_setup();
      }
		}
		else if(ICType == IQS620n)
		{
			// Read version number to insure we still have the correct device attached - otherwise, do setup
			res = Wire.read();

			// System flags, Global Events and PXS UI Flags - 9 bytes
			res |= Wire.read();

			// Read PXS Channel Data - 12 bytes
			res |= Wire.read();

			// Read LTA value of Channel 1 for Movement mode
			res |= Wire.read();

			// Set the appropriate IC
			if(buffer[0] == IQS620_PRODUCT_NR)
			{
				if(buffer[1] != IQS620N_SOFTWARE_NR && buffer[2] != IQS620N_HARDWARE_NR)
				{
					ICType = IQS620;
					// Do setup
					iqs_setup();
				}
			}
			else if(buffer[0] == IQS621_PRODUCT_NR)
			{
				ICType = IQS621;
				// Do setup
				iqs_setup();
			}
			else if(buffer[0] == IQS622_PRODUCT_NR)
      {
        ICType = IQS622;
       	// Do setup
        iqs_setup();
      }
			else if(buffer[0] == IQS624_PRODUCT_NR)
			{
			  if(buffer[1] == IQS624N_SOFTWARE_NR)
          ICType = IQS624n;
        else
          ICType = IQS624;
				// Do setup
				iqs_setup();
			}
      else if(buffer[0] == IQS625_PRODUCT_NR)
      {
        ICType = IQS625;
        // Do setup
        iqs_setup();
      }
		}
		else if(ICType == IQS621)
		{
			// Read version number to insure we still have the correct device attached - otherwise, do setup
			res = i2c.read(VERSION_INFO, 3, buffer, I2C_Repeat_Start);

			// System flags, Global Events and PXS UI Flags - 3 bytes
			res |= i2c.read(SYSTEM_FLAGS, 12, &iqs621.SystemFlags.SystemFlags, I2C_Repeat_Start);

			// Read PXS Channel Data - 4 bytes
			res |= i2c.read(CHANNEL_DATA, 4, &iqs621.Ch[0].Ch_Low, I2C_Stop);

			// Set the appropriate IC
			if(buffer[0] == IQS620_PRODUCT_NR)
			{
				if(buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
					ICType = IQS620n;
				else
					ICType = IQS620;
				// Do setup
				iqs_setup();
			}
      else if(buffer[0] == IQS622_PRODUCT_NR)
      {
        ICType = IQS622;
        // Do setup
        iqs_setup();
      }
			else if(buffer[0] == IQS624_PRODUCT_NR)
			{
				if(buffer[1] == IQS624N_SOFTWARE_NR)
          ICType = IQS624n;
        else
          ICType = IQS624;
				// Do setup
				iqs_setup();
			}
      else if(buffer[0] == IQS625_PRODUCT_NR)
      {
        ICType = IQS625;
        // Do setup
        iqs_setup();
      }
		}
   
		else if(ICType == IQS622)
		{
      // Read version number to insure we still have the correct device attached - otherwise, do setup
      res = i2c.read(VERSION_INFO, 1, buffer, I2C_Repeat_Start);

      // System flags, Global Events, PXS UI Flags, SAR UI Flags, ALS UI Flags, IR Flags, IR UI Flags, IR Values, Hall Flags, Hall Values - 12 bytes
      res |= i2c.read(SYSTEM_FLAGS, 12, &iqs622.SystemFlags.SystemFlags, I2C_Repeat_Start);
     
      // Read PXS Channel Data - 4 bytes
      res |= i2c.read(CHANNEL_DATA, 4, &iqs622.Ch[0].Ch_Low, I2C_Stop);

      // Set the appropriate IC
      if(buffer[0] == IQS620_PRODUCT_NR)
      {
        if(buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
          ICType = IQS620n;
        else
          ICType = IQS620;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS621_PRODUCT_NR)
      {
        ICType = IQS621;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS624_PRODUCT_NR)
      {
        if(buffer[1] == IQS624N_SOFTWARE_NR)
          ICType = IQS624n;
        else
          ICType = IQS624;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS625_PRODUCT_NR)
      {
        ICType = IQS625;
        // Do setup
        iqs_setup();
      }
   }
   
	 else if(ICType == IQS624)
	 {
			// Read version number to insure we still have the correct device attached - otherwise, do setup
			res = i2c.read(VERSION_INFO, 3, buffer, I2C_Repeat_Start);

			// System flags, Global Events, PXS UI Flags and Hall UI Flags - 5 bytes
			res |= i2c.read(SYSTEM_FLAGS, 5, &iqs624.SystemFlags.SystemFlags, I2C_Repeat_Start);

			// Read Hall Degrees output
			res |= i2c.read(HALL_DEG_LSB, 2, &iqs624.HallDegrees.Hall_Degrees_Low, I2C_Repeat_Start);

			if((Mode == Mode_5) || (Mode == Mode_6))
			{
				// Read Hall Degrees output
				res |= i2c.read(CHANNEL_DATA, 12, &iqs624.Ch[0].Ch_Low, I2C_Repeat_Start);

				// Implement a filter - atm not implement
				iqs624.AvgCh[2].Ch = (uint16_t)(iqs624.Ch[2].Ch*BETA + (1.0-BETA)*iqs624.AvgCh[2].Ch);
				iqs624.AvgCh[3].Ch = (uint16_t)(iqs624.Ch[3].Ch*BETA + (1.0-BETA)*iqs624.AvgCh[3].Ch);
				iqs624.AvgCh[4].Ch = (uint16_t)(iqs624.Ch[4].Ch*BETA + (1.0-BETA)*iqs624.AvgCh[4].Ch);
				iqs624.AvgCh[5].Ch = (uint16_t)(iqs624.Ch[5].Ch*BETA + (1.0-BETA)*iqs624.AvgCh[5].Ch);

				// Read Hall Ratio Settings for  output
				res |= i2c.read(0x78, 1, &iqs624.HallRatioSettings.HallRatioSettings, I2C_Repeat_Start);
			}

			// Read Hall Angle difference
			res |= i2c.read(HALL_DEG_DIFF, 1, &iqs624.hallAngleDiff, I2C_Stop);

			// Set the appropriate IC
			if(buffer[0] == IQS620_PRODUCT_NR)
			{
				if(buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
					ICType = IQS620n;
				else
					ICType = IQS620;
				// Do setup
				iqs_setup();
			}
			else if(buffer[0] == IQS621_PRODUCT_NR)
			{
				ICType = IQS621;
				// Do setup
				iqs_setup();
			}
			else if(buffer[0] == IQS622_PRODUCT_NR)
      {
        ICType = IQS622;
       	// Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS624_PRODUCT_NR)
      {
       	if(buffer[1] == IQS624N_SOFTWARE_NR)
        ICType = IQS624n;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS625_PRODUCT_NR)
      {
        ICType = IQS625;
        // Do setup
        iqs_setup();
      }
		}
   
    else if(ICType == IQS624n)
    {
      // Read version number to insure we still have the correct device attached - otherwise, do setup
      res = i2c.read(VERSION_INFO, 3, buffer, I2C_Repeat_Start);

      // System flags, Global Events, PXS UI Flags and Hall UI Flags and filtered degrees - 9 bytes
      res |= i2c.read(SYSTEM_FLAGS, 9, &iqs624n.SystemFlags.SystemFlags, I2C_Repeat_Start);

      //read channels
      //res |= i2c.read(CHANNEL_DATA, 12, &iqs624n.Ch[0].Ch_Low, I2C_Repeat_Start);
        
      // Read Hall Angle difference
      res |= i2c.read(HALL_DEG_DIFF, 1, &iqs624n.hallAngleDiff, I2C_Stop);

      // Set the appropriate IC
      if(buffer[0] == IQS620_PRODUCT_NR)
      {
        if(buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
          ICType = IQS620n;
        else
          ICType = IQS620;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS621_PRODUCT_NR)
      {
        ICType = IQS621;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS622_PRODUCT_NR)
      {
        ICType = IQS622;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS624_PRODUCT_NR)
      {
        if(buffer[1] == !IQS624N_SOFTWARE_NR)
        {
          ICType = IQS624;
          // Do setup
          iqs_setup();
        }
      }
      else if(buffer[0] == IQS625_PRODUCT_NR)
      {
        ICType = IQS625;
        // Do setup
        iqs_setup();
      }
    }

    else if(ICType == IQS625)
    {
      // Read version number to insure we still have the correct device attached - otherwise, do setup
      res = i2c.read(VERSION_INFO, 3, buffer, I2C_Repeat_Start);

      // System flags, Global Events, PXS UI Flags and Hall UI Flags and filtered degrees - 3 bytes
      res |= i2c.read(SYSTEM_FLAGS, 3, &iqs625.SystemFlags.SystemFlags, I2C_Repeat_Start);

      //read channels
      res |= i2c.read(CHANNEL_DATA, 4, &iqs625.Ch[0].Ch_Low, I2C_Repeat_Start);
        
      // Read Hall Degrees Output
      res |= i2c.read(WHEEL_OFFSET_L, 2, &iqs625.HallDegrees.Hall_Degrees_Low, I2C_Stop);

      // Set the appropriate IC
      if(buffer[0] == IQS620_PRODUCT_NR)
      {
        if(buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
          ICType = IQS620n;
        else
          ICType = IQS620;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS621_PRODUCT_NR)
      {
        ICType = IQS621;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS622_PRODUCT_NR)
      {
        ICType = IQS622;
        // Do setup
        iqs_setup();
      }
      else if(buffer[0] == IQS624_PRODUCT_NR)
      {
        if(buffer[1] == IQS624N_SOFTWARE_NR)
          ICType = IQS624n;
        else
          ICType = IQS624;
        // Do setup
        iqs_setup();
      }
    }
    
		// A read error occurred
		if(res)
   {
			//disp.writeError(res);
			//Serial.print("res : ");
			//Serial.println(res);

      clear_mode_leds();
   }

		// Now we write to display
		refreshDisplay = true;

		// reset timer
		setTimer(&ErrorTimer);

		chipReady = true;
	}

	if(timerExpired(&ErrorTimer))
 {
		//disp.writeError(ERR_TIMEOUT);
		//Serial.print("Timer Expired : ");
		//Serial.println(ERR_TIMEOUT);
    clear_mode_leds();
 }

	// Check for mode switching
	check_mode_button();

	// handle the loop state
	switch(Loop)
	{
	case Run:

		// Did we receive data from the IC? If we did, process that data
		if(chipReady){
			// Now process the events for each mode
			if(ICType == IQS620)
				process_IQS620_events();
			else if(ICType == IQS620n)
				process_IQS620n_events();
			else if(ICType == IQS621)
				process_IQS621_events(&refreshDisplay);
     	else if(ICType == IQS622)
       	process_IQS622_events(&refreshDisplay);
			else if(ICType == IQS624)
				process_IQS624_events(&refreshDisplay);
      else if(ICType == IQS624n)
        process_IQS624n_events(&refreshDisplay);
      else if(ICType == IQS625)
        process_IQS625_events(&refreshDisplay);

			// We are done processing this comms window
			chipReady = false;
		}
		break;

	case Switch_Mode:
		// Display the current mode on the led for a certain time

		// We change states if timeout occured
		if(timerExpired(&Mode_Switch_Timer))
		{
			// Switch off the appropriate Mode led
			digitalWrite(Leds[(uint8_t)Mode], LOW); // Clear the mode led
			Loop = Run;	// go to run loop
		}
		// Always reset the getTemperatureReference flag
		getTempReference = true;

		// Get the reference degrees again
		getDeg = true;

		// Display mode
		displayState = Display_Int;		// By default display int

		break;

	case Display_Info:

		// Main Timer
		if(timerExpired(&MainTimer)) {
			Loop = Switch_Mode;	// go to Switch_Mode loop
      // Go to mode 1
      Mode = Mode_1;
      digitalWrite(Leds[(uint8_t)Mode], HIGH);
      //disp.write(display_number = (((uint8_t)Mode%4)+1)*1111);    // Write mode to display
	  //display_number = (((uint8_t)Mode%4)+1)*1111;
	  //Serial.print(Mode : );
	  //Serial.println(display_number);
      setTimer(&Mode_Switch_Timer);	
		}

		break;
	default:;
	}

	if(refreshDisplay && (Loop == Run || Loop == Display_Info)  && !res)
	{
		// Minimize write to display
		if(displayState == Display_Int)
    {
			//disp.write(display_number);
    }
			//Serial.println("Display Number : ");
			//Serial.println(display_number);
		else
      {
			//disp.write(display_string);
      }
			//Serial.println("Display String : ");
			//Serial.println(display_string);
	}
}

/**
 * @brief	Process the Events that the IQS620 reported to us. Here we will run a state machine
 *        to handle the different modes
 * @description
 * @param	None
 * @retval	None
 */
void process_IQS620_events()
{
	// Check that no reset has occured
	if(iqs620.SystemFlags.ShowReset)
	{
		// Re-intialise the IQS620
		iqs_setup();
	
		// Check whether setup was a success
		Serial.println("Reset");
		return;
	}
	// Run a state machine
	switch(Mode)
	{

	// Mode 1 - Multi Mode
	case Mode_1:
		sar_raw_mode();
		break;

		// Mode 2 - SAR Relatief Mode
	case Mode_2:
		movement_raw_mode();
		break;

		// Mode 3 - Metal Raw Mode
	case Mode_3:
		temp_mode();
		break;

		// Mode 4 - ProxSense Mode
	case Mode_4:
		hall_raw_mode();
		break;

	default:
		// error
		break;
	}

}

/**
 * @brief	Process the Events that the IQS620n production version reported to us. Here we will run a state machine
 * 			  to handle the different modes
 * @description
 * @param	None
 * @retval	None
 */
void process_IQS620n_events()
{
//	 Check that no reset has occured
	if(iqs620n.SystemFlags.ShowReset)
	{
		// Re-intialise the IQS620n
		iqs_setup();

    // Check whether setup was a success
    	Serial.println("Reset");
		return;
	}

	// Run a state machine
	switch(Mode)
	{

	// Mode 1 - Multi Mode
	case Mode_1:
		nsar_raw_mode();
		break;

		// Mode 2 - SAR Relatief Mode
	case Mode_2:
		nmovement_raw_mode();
		break;

		// Mode 3 - Metal Raw Mode
	case Mode_3:
		ntemp_mode();
		break;

		// Mode 4 - ProxSense Mode
	case Mode_4:
		nhall_raw_mode();
		break;

	default:
		// error
		break;
	}
}

/**
 * @brief	Process the Events that the IQS621 reported to us. Here we will run a state machine
 * 			to handle the different modes
 * @param	[bool]	should we refresh the display
 * @retval	None
 */
void process_IQS621_events(bool *refreshDisplay)
{

	// Check that no reset has occurred
	if(iqs621.SystemFlags.ShowReset)
	{
		// Re-intialise the IQS621
		iqs_setup();

		// Check whether setup was a success
		Serial.println("Reset");
		return;
	}

	// Check sensor mode
	if(isCoilPresent())
	{
		// We are not in inductive mode!! Error!
		if(Mode != Mode_4)
		{
			Loop = Display_Info;
			displayState = Display_String;
			sprintf(display_string, "coil");
			//disp.write(display_string);			// Insure that coil is displayd on screen

			coilCheck = true;
			*refreshDisplay = true;

			// Auto mode to mode 4
			Mode = Mode_4;
		}
		else {
			// If we are not in run loop, switch back
			if(coilCheck)
			{
				coilCheck = false;
				redo_ati();
				Serial.println("Redo ATI");
				displayState = Display_Int;
				*refreshDisplay = true;

				if(iqs621.sensorMode != Inductive)
				{
					setup_iqs621(Inductive);
				}
			}

		}
	}
	// No data present
	else {
		// We are not in inductive mode!! Error!
		if(Mode == Mode_4)
		{
			Loop = Display_Info;
			displayState = Display_String;
			sprintf(display_string, "coil");
			coilCheck = true;
			*refreshDisplay = true;

			if(iqs621.sensorMode == Inductive){
				setup_iqs621(Capacitive);
			}
		}
		else {
			// If we are not in run loop, switch back
			if(coilCheck)
			{
				coilCheck = false;
				redo_ati();
				Serial.println("Redo ATI");
				displayState = Display_Int;
				*refreshDisplay = true;

				if(iqs621.sensorMode != Capacitive)
				{
					setup_iqs621(Capacitive);
				}
			}

		}
	}

	// Run a state machine
	switch(Mode)
	{
	// Mode 1 - Absolute Mode
	case Mode_1:
		absoluteMode();			// Run absolute mode
		break;

		// Mode 2 - Light/Dark Mode
	case Mode_2:
		light_darkMode();
		break;

		// Mode 3 - ProxSense Mode
	case Mode_3:
		proxMode();
		break;

		// Mode 4 - Inductive Sensing Mode
	case Mode_4:
		inductiveMode(refreshDisplay);
		break;

	default:
		// error
		break;
	}
}

/**
 * @brief  Process the Events that the IQS622 reported to us. Here we will run a state machine
 *      to handle the different modes
 * @param None
 * @retval  None
 */
void process_IQS622_events(bool *refreshDisplay)
{
  // Check that no reset has occured
  if(iqs622.SystemFlags.ShowReset)
  {
    // Re-intialise the IQS622
    iqs_setup();

    // Check whether setup was a success
    Serial.println("Reset");
   return;
  }

  // Run a state machine
  switch(Mode)
  {

  // Mode 1 - IR Mode
  case Mode_1:
    IRMode();
    break;

    // Mode 2 - Basic IR Mode
  case Mode_2:
    basicIRMode();
    break;

    // Mode 3 - Basic ALS Mode
  case Mode_3:
    basicALSMode();
    break;

    // Mode 4 - Prox Mode
  case Mode_4:
    proxMode();
    break;

  default:
    // error
    break;
  }
}

/**
 * @brief	Process the Events that the IQS624 reported to us. Here we will run a state machine
 * 			to handle the different modes
 * @param	None
 * @retval	None
 */
void process_IQS624_events(bool *refreshDisplay)
{
	// Check that no reset has occurred
	if(iqs624.SystemFlags.ShowReset)
	{
		// Re-intialise the IQS624
		iqs_setup();

		// Check whether setup was a success
		Serial.println("Reset");
		return;
	}

	// Run a state machine
	switch(Mode)
	{

	// Mode 1 - Absolute Mode
	case Mode_1:
		absoluteWheelMode(refreshDisplay);
		break;

		// Mode 2 - Relative Mode
	case Mode_2:
		relativeWheelMode(refreshDisplay);
		break;

		// Mode 3 - RPM Mode (Speed mode)
	case Mode_3:
		speedMode(refreshDisplay);
		break;

		// Mode 4 - Raw Mode
	case Mode_4:
		rawMode(refreshDisplay);
		break;

		// ATI Algorithm
	case Mode_5:
		run_ati_algo();
		break;

		// Calibration Mode
	case Mode_6:
		calibration();
		break;

	default:
		// error
		break;
	}
}

/**
 * @brief  Process the Events that the IQS624n reported to us. Here we will run a state machine
 *      to handle the different modes
 * @param None
 * @retval  None
 */
void process_IQS624n_events(bool *refreshDisplay)
{
  // Check that no reset has occurred
  if(iqs624n.SystemFlags.ShowReset)
  {
    // Re-intialise the IQS624n
    iqs_setup();

    // Check whether setup was a success
    Serial.println("Reset");
    return;
  }
 
  // Run a state machine
  switch(Mode)
  {

  // Mode 1 - Absolute Mode
  case Mode_1:
    nabsoluteWheelMode(refreshDisplay);
    break;

    // Mode 2 - Relative Mode
  case Mode_2:
    nrelativeWheelMode(refreshDisplay);
    break;

    // Mode 3 - RPM Mode (Speed mode)
  case Mode_3:
    nspeedMode(refreshDisplay);
    break;

    // Mode 4 - Raw Mode
  case Mode_4:
    nrawMode(refreshDisplay);
    break;

    // ATI Algorithm
  case Mode_5:
    run_ati_algo();
    break;

    // Calibration Mode
  case Mode_6:
    calibration();
    break;

  default:
    // error
    break;
  }
}

/**
 * @brief  Process the Events that the IQS625 reported to us. Here we will run a state machine
 *      to handle the different modes
 * @param None
 * @retval  None
 */
void process_IQS625_events(bool *refreshDisplay)
{
  // Check that no reset has occurred
  if(iqs625.SystemFlags.ShowReset)
  {
    // Re-intialise the IQS625
    iqs_setup();

    // Check whether setup was a success
    Serial.println("Reset");
    return;
  }
 
  // Run a state machine
  switch(Mode)
  {

  // Mode 1 - Interval number Mode
  case Mode_1:
    intervalNumMode10(refreshDisplay);
    break;
      // Mode 2 - Interval number Mode
  case Mode_2:
    intervalNumMode30(refreshDisplay);
    break;
      // Mode 3 - Interval number Mode
  case Mode_3:
    intervalNumMode60(refreshDisplay);
    break;
    // Mode 4 - Relative interval Mode
  case Mode_4:
    intervalRelMode(refreshDisplay);
    break;
/* 
//TODO
    // Mode 3 - RPM Mode (Speed mode)
  case Mode_3:
    arrowMode(refreshDisplay);
    break;
//TODO
    // Mode 4 - Raw Mode
  case Mode_4:
    nrawMode(refreshDisplay);
    break;
//TODO
    // ATI Algorithm
  case Mode_5:
    run_ati_algo();
    break;
//TODO
    // Calibration Mode
  case Mode_6:
    calibration();
    break;
*/
  default:
    // error
    break;
  }
}
/****************************************************************/
/*																								              */
/*									Mode Helper Functions										    */
/*																								              */
/****************************************************************/

/**
 * @brief	Check the mode switch button
 * @param	None
 * @retval	None
 */
void check_mode_button()
{
	// Which button is the mode switch button?
	if(ICType == IQS620)
		modeButton = (bool)iqs620.PXSUIFlags.CH2_T_Out;
	else if(ICType == IQS620n)
		modeButton = (bool)iqs620n.PXSUIFlags.CH2_T_Out;
	else if(ICType == IQS621)
	{
		// Only allow mode switches in capacitive mode
		if(iqs621.sensorMode == Capacitive)
			modeButton = (bool)iqs621.PXSUIFlags.CH1_T_Out;
	}
  else if(ICType == IQS622)
    modeButton = (bool)iqs622.PXSUIFlags.CH1_T_Out;
	else if(ICType == IQS624)
		modeButton = (bool)iqs624.PXSUIFlags.CH1_T_Out;
  else if(ICType == IQS624n)
    modeButton = (bool)iqs624n.PXSUIFlags.CH1_T_Out;
  else if(ICType == IQS625)
    modeButton = (bool)iqs625.PXSUIFlags.CH1_T_Out;
    
	// Step Modes if we need to
	if((mode_button == Released) && (modeButton))
	{
		mode_button = Touch;	// a touch occurred on the mode button
		setTimer(&ButtonTimer,500);
	}
	else if((mode_button == Touch) && (!modeButton))
		mode_button = Pressed;	// the mode button was now pressed
	// Clear Button
	else if((mode_button == Btn_Timeout) && (!modeButton))
	{
		mode_button = Released;
		setTimer(&ButtonTimer,500);
	}
	else if((mode_button == Btn_Timeout) && (modeButton) && (ICType == IQS624))
	{
		mode_button = Btn_LongPress;
		setTimer(&ButtonTimer,1500);
	}
	// Button was pressed, move to next state
	else if((mode_button == Pressed) && (!modeButton)) {
		mode_button = Released;	// No more touch on the button, move to next state

		// Clear Leds
		clear_mode_leds();

		// Next Mode
		Mode = (Mode_e)((uint8_t)Mode + 1);
		if((uint8_t)Mode > (uint8_t)Mode_4) Mode = Mode_1;

		//disp.write((((uint8_t)Mode%4)+1)*1111);    // Write mode to display);

		// Go to Switch mode state
		Loop = Switch_Mode;

		// Switch on the appropriate led
		digitalWrite(Leds[(uint8_t)Mode], HIGH);

		// Start the timer
		setTimer(&Mode_Switch_Timer);
	}
	else if(timerExpired(&ButtonTimer) && mode_button != Released) {

		// If this is the IQS624 we want to be able to long press. Thus move to calibration state
		// and set time
		if(mode_button == Btn_LongPress && ICType == IQS624 && modeButton)
		{
			Mode = Mode_5;

			first = true;

			algoSM = 0;
			littleSM = 0;

			// We enter the calibration mode
			modeEntry = true;

			// Start the timer
			setTimer(&Mode_Switch_Timer);
		}

		mode_button = Btn_Timeout;
	}
}

/**
 * @brief  Function that sets up and initializes the IQS62x device with the desired
 *      settings
 * @param None
 * @retval  None
 */
void iqs_setup()
{
  uint8_t res = 0;
  
  Loop = Display_Info;
  setTimer(&MainTimer);

  // Wait for IC to become ready - a timeout should exit this
  while(!Wire.available());

  if(ICType == IQS620)
  {
    //sprintf(display_string, "620-");
	Serial.print (display_string);
	Serial.print (" 620");
	Serial.println();
    //disp.write(display_string);
    delay(1000); //Wait here for device splash on disp
    // setup device
    res = setup_iqs620();
  }

  else if(ICType == IQS620n)
  {
    sprintf(display_string, "620n");
	//Serial.print (display_string);
	//Serial.print (" 620n");
	//Serial.println();
    //disp.write(display_string);
    Serial.println ("620n Found!");
    delay(1000); //Wait here for device splash on disp
    // setup device
    res = setup_iqs620n();
    //Serial.println(res);
  }

  else if(ICType == IQS621)
  {
    //sprintf(display_string, "621-");
	Serial.print (display_string);
	Serial.print (" 621-");
	Serial.println();
    //disp.write(display_string);
    delay(1000); //Wait here for device splash on disp
    // setup as capacitive by default
    res = setup_iqs621(Capacitive);
  }

  else if(ICType == IQS622)
  {
    //sprintf(display_string, "622-");
	Serial.print (display_string);
	Serial.print (" 622");
	Serial.println();
    //disp.write(display_string);
    delay(1000); //Wait here for device splash on disp
    // setup as capacitive & IR
    res = setup_iqs622(Capacitive);
  }

  else if(ICType == IQS624)
  {
    //sprintf(display_string, "624-");
	Serial.print (display_string);
	Serial.print (" 624");
	Serial.println();
    //disp.write(display_string);
    delay(1000); //Wait here for device splash on disp
    // setup as capacitive & hall rotation by default
    res = setup_iqs624n();
  }
 
  else if(ICType == IQS624n)
  {
    //sprintf(display_string, "624n");
	Serial.print (display_string);
	Serial.print (" 624n");
	Serial.println();
    //disp.write(display_string);
    delay(1000); //Wait here for device splash on disp
    // setup as capacitive & hall rotation by default
    res = setup_iqs624n();
  }

  else if(ICType == IQS625)
  {
    //sprintf(display_string, "625-");
	Serial.print (display_string);
	Serial.print (" 625");
	Serial.println();
    //disp.write(display_string);
    delay(1000); //Wait here for device splash on disp
    // setup as capacitive & hall rotation by default
    res = setup_iqs625();
  }

  // An error occured
  if(res)
  {
    //disp.writeError(res);
  }

}

/**************************************************************************************************/
/*                                                                                                */
/*                    Setup IQS620                                                                */
/*                                                                                                */
/**************************************************************************************************/
/**
 * @brief Check which setup we need to run for the IQS620 - capacitive
 * @param
 * @retval  None
 */
uint8_t setup_iqs620()
{
  uint8_t res = 0;

  while(!Wire.available());
  
    res |= Wire.write(DEV_SETTINGS);

    res |= Wire.write(PXS_SETTINGS_0);

    res |= Wire.write(PXS_SETTINGS_1);

    res |= Wire.write(PXS_UI_SETTINGS);

    res |= Wire.write(SAR_UI_SETTINGS);

    res |= Wire.write(METAL_UI_SETTINGS);

    res |= Wire.write(HALL_SENS_SETTINGS);

    res |= Wire.write(HALL_UI_SETTINGS);

    // Wait for Redo Ati to complete
    do {
      // Wait for device to become ready
      while(!Wire.available());

      res |= Wire.read();
    } while (!res && iqs620.SystemFlags.InAti);
  
  return res;
}

/**************************************************************************************************/
/*									IQS620 MODE 1: SAR Raw Mode									 	                                */
/**************************************************************************************************/
/**
 * @brief	SAR Mode helper
 * @param	None
 * @retval	None
 */
void sar_raw_mode()
{
	/*
	 * Display CH0 Counts on 7-segment
	 * Prox output on LED1
	 */
	display_number = iqs620.Ch[0].Ch;	// Display Channel Data

  Serial.print("SAR counts:");
  Serial.print("\t");
  Serial.println(display_number);

	digitalWrite(LED_1, iqs620.PXSUIFlags.CH0_P_Out);			// Switch Prox led
}

/**************************************************************************************************/
/*									IQS620 MODE 2: Movement Raw Mode									                            */
/**************************************************************************************************/
/**
 * @brief	Movement Mode helper
 * @param	None
 * @retval	None
 */
void movement_raw_mode()
{
	/*
	 * Display CH1 Counts on 7-segment
	 * Movement output on LED2
	 */
	if(iqs620.SARMetalFlags.Quick_Release)
	{
		displayState = Display_Int;
		display_number = iqs620.Ch[1].Ch;	// Display Channel 1 Data

	}
	else {
		displayState = Display_String;
		sprintf(display_string, "----");
	}

	digitalWrite(LED_2, iqs620.SARMetalFlags.Movement);			// Switch Movement led
}


/**************************************************************************************************/
/*									IQS620 MODE 3: Temperature Mode									                              */
/**************************************************************************************************/
/**
 * @brief	Temperature Mode helper
 * @param	None
 * @retval	None
 */
void temp_mode()
{
#ifdef ABS_TEMP
	const float a_b = 281438.5;
	const uint16_t c = -284;
#else
	int8_t tempDelta = 0;
#endif
	/*
	 * Display CH2 Counts on 7-segment
	 * Metal / No-Metal on LED3
	 */
	// Check if it is the first time coming into this mode - get temp reference
	if(getTempReference)
	{
		// do not enter again
		getTempReference = !getTempReference;
		get_temp_reference();
	}

#ifdef ABS_TEMP
	// Calculate the Temp with T = (a/b)*(1/CH3)+c
	display_number = (int16_t)(a_b*((1/(float)iqs620n.Ch[3].Ch)) + c);
#else
	displayState = Display_String;
	tempDelta = (TemperatureReference - iqs620.Ch[3].Ch);

	//	tempDelta < 0 ? tempDelta = -tempDelta : tempDelta;
	tempDelta <= -99 ? tempDelta = -99 : tempDelta;
	tempDelta >= 99 ? tempDelta = 99 : tempDelta;

	sprintf(display_string, "%3d%c", tempDelta, '*');

#endif
}


/**************************************************************************************************/
/*									IQS620 MODE 4: Hall Raw Mode									 	                              */
/**************************************************************************************************/
/**
 * @brief	Hall Raw Mode helper
 * @param	None
 * @retval	None
 */
void hall_raw_mode()
{
	/*
	 * Display Hall output on 7-segment
	 * Hall Threshold triggers on LED4
	 * Direction of Hall - fill display with nnnn or ssss for 0.5-1s
	 */
	display_number = iqs620.HallValue.HallValue;	// Display Hall Value

	// upper leds for North
	if(iqs620.HallFlags.Hall_N_S)
	{
		digitalWrite(LED_3, iqs620.HallFlags.Hall_Prox);
		digitalWrite(LED_4, iqs620.HallFlags.Hall_Touch);
		// Switch off South leds
		digitalWrite(LED_1, LOW);
		digitalWrite(LED_2, LOW);
	}
	// Lower leds for South
	else
	{
		digitalWrite(LED_1, iqs620.HallFlags.Hall_Touch);
		digitalWrite(LED_2, iqs620.HallFlags.Hall_Prox);
		// Switch off North leds
		digitalWrite(LED_3, LOW);
		digitalWrite(LED_4, LOW);
	}
}

/**************************************************************************************************/
/*										Temperature												                                          */
/**************************************************************************************************/
/**
 * @brief	Get the temperature reference
 * @param	None
 * @retval	None
 */
void get_temp_reference()
{
	TemperatureReference = iqs620.Ch[3].Ch;	// Get temperature Channel Data
}

/**
 * @brief	Calculates and returns the absolute value for the given number
 * @param	[int16_t] signed 16 bit value
 * @retval	[uint16_t] unsigned 16 bit value
 */
uint16_t absolute(int16_t value)
{
	return ((value < 0) ? (-value) : value);
}

/**************************************************************************************************/
/*                                                                                                */
/*                    Setup IQS620n                                                               */
/*                                                                                                */
/**************************************************************************************************/
/**
 * @brief Check which setup we need to run for the IQS620n - capacitive
 * @param
 * @retval  None
 */
uint8_t setup_iqs620n()
{
  uint8_t res = 0;

  while(!Wire.available());

    res |= Wire.write(DEV_SETTINGS);

    res |= Wire.write(PXS_SETTINGS_0);

    res |= Wire.write(PXS_SETTINGS_1);

    res |= Wire.write(PXS_UI_SETTINGS);

    res |= Wire.write(SAR_UI_SETTINGS);

    res |= Wire.write(METAL_UI_SETTINGS);

    res |= Wire.write(HALL_SENS_SETTINGS);

    res |= Wire.write(HALL_UI_SETTINGS);

    res |= Wire.write(TEMP_UI_SETTINGS);

    // Wait for Redo Ati to complete
    do {
      // Wait for device to become ready
      while(!Wire.available());

      res |= Wire.read();
    } while (!res && iqs620n.SystemFlags.InAti);
  
  return res;
}

/**************************************************************************************************/
/*									IQS620n MODE 1: SAR Raw Mode									 	                              */
/**************************************************************************************************/
/**
 * @brief	SAR Mode helper
 * @param	None
 * @retval	None
 */
void nsar_raw_mode()
{
	/*
	 * Display CH0 Counts on 7-segment
	 * Prox output on LED1
	 */
	display_number = iqs620n.Ch[0].Ch;	// Display Channel Data

    Serial.print("SAR counts:");
    Serial.print("\t");
    Serial.println(display_number);

	digitalWrite(LED_1, iqs620n.PXSUIFlags.CH0_P_Out);			// Switch Prox led

}

/**************************************************************************************************/
/*									IQS620n MODE 2: Movement Raw Mode									                            */
/**************************************************************************************************/
/**
 * @brief	Movement Mode helper
 * @param	None
 * @retval	None
 */
void nmovement_raw_mode()
{
	/*
	 * Display CH1 Counts on 7-segment
	 * Movement output on LED2
	 */
	if(iqs620n.SARMetalFlags.Quick_Release)
	{
		displayState = Display_Int;
		display_number = iqs620n.Ch[1].Ch;	// Display Channel 1 Data

    Serial.print("Movement:  ");
    Serial.println(display_number);
	}
	else
	{
		displayState = Display_String;
		sprintf(display_string, "----");

    Serial.println("Movement:  -");
	}

	digitalWrite(LED_2, iqs620n.SARMetalFlags.Movement);			// Switch Movement led

}

/**************************************************************************************************/
/*									IQS620n MODE 3: Temperature Mode									                            */
/**************************************************************************************************/
/**
 * @brief	Temperature Mode helper
 * @param	None
 * @retval	None
 */
void ntemp_mode()
{
#ifdef ABS_TEMP
	const float a_b = 0.3333;//281438.5;
  const uint16_t c = 300;
  float tempDelta = 0;
  int16_t tempdec = 5;
#else
	int8_t tempDelta = 0;
#endif
	/*
	 * Display CH2 Counts on 7-segment
	 * Metal / No-Metal on LED3
	 */
	// Check if it is the first time coming into this mode - get temp reference
	if(getTempReference)
	{
		// do not enter again
		getTempReference = !getTempReference;
		nget_temp_reference();
	}

#ifdef ABS_TEMP
	// Calculate the Temp with T = (a/b)*(1/CH3)+c
//	display_number = (c-a_b*(float)iqs620n.Ch[3].Ch)/2;

 displayState = Display_String;
 tempDelta = (c-a_b*(float)iqs620n.Ch[3].Ch)/2;
 tempdec = abs((long long)(tempDelta*10 - floor(tempDelta)*10));
 
 Serial.print("Temperature: ");
 Serial.print(tempDelta);
 Serial.print("\tDelta: ");
 Serial.print(tempdec);
 Serial.println();
 
 sprintf(display_string, "%2d%c%1d",int(tempDelta),'*',tempdec);
#else
	//displayState = Display_String;
//	tempDelta = (TemperatureReference - iqs620n.TempValue.TempValue);

//	tempDelta <= -99 ? tempDelta = -99 : tempDelta;
//	tempDelta >= 99 ? tempDelta = 99 : tempDelta;

//	sprintf(display_string, "t%3d", tempDelta);
#endif

}

/**************************************************************************************************/
/*									IQS620n MODE 4: Hall Raw Mode									 	                              */
/**************************************************************************************************/
/**
 * @brief	Hall Raw Mode helper
 * @param	None
 * @retval	None
 */
void nhall_raw_mode()
{
	/*
	 * Display Hall output on 7-segment
	 * Hall Threshold triggers on LED4
	 * Direction of Hall - fill display with nnnn or ssss for 0.5-1s
	 */
	display_number = iqs620n.HallValue.HallValue;	// Display Hall Value

  Serial.print("Hall-effect amplitude:  ");
  Serial.print(display_number);
  Serial.print("\t");

	// upper leds for North
	if(iqs620n.HallFlags.Hall_N_S)
	{
    Serial.println("Direction: N");
  
		digitalWrite(LED_3, iqs620n.HallFlags.Hall_Prox);
		digitalWrite(LED_4, iqs620n.HallFlags.Hall_Touch);
		// Switch off South leds
		digitalWrite(LED_1, LOW);
		digitalWrite(LED_2, LOW);
	}
	// Lower leds for South
	else
	{
    Serial.println("Direction: S");
    
		digitalWrite(LED_1, iqs620n.HallFlags.Hall_Touch);
		digitalWrite(LED_2, iqs620n.HallFlags.Hall_Prox);
		// Switch off North leds
		digitalWrite(LED_3, LOW);
		digitalWrite(LED_4, LOW);
	}
}

/**************************************************************************************************/
/*										Temperature												                                          */
/**************************************************************************************************/
/**
 * @brief	Get the temperature reference
 * @param	None
 * @retval	None
 */
void nget_temp_reference()
{
	TemperatureReference = iqs620n.TempValue.TempValue;	// Get temperature Channel Data
}


/**************************************************************************************************/
/*																								                                                */
/*										Setup IQS621											                                          */
/*																								                                                */
/**************************************************************************************************/
/**
 * @brief	Check which setup we need to run for the IQS621 - capacitive or inductive
 * @param
 * @retval	None
 */
uint8_t setup_iqs621(Setup_Type_e setupType)
{
	uint8_t res = 0;

	while(!i2c.isDeviceReady());

	// Normal Capacitive sensing mode
	if(setupType == Capacitive)
	{
		// Fill buffer with first settings
		buffer[0] = PXS_SETTINGS_0_0_VAL;
		buffer[1] = PXS_SETTINGS_0_1_VAL;
		buffer[2] = 0x77;
		buffer[3] = 0x7F;
		buffer[4] = PXS_SETTINGS_2_0_VAL;
		buffer[5] = PXS_SETTINGS_2_1_VAL;
		buffer[6] = 0x05;
		buffer[7] = 0x26;
		buffer[8] = 0x00;
		buffer[9] = 0x01;
		buffer[10] = 0xB1;
		buffer[11] = 0x34;
		buffer[12] = 0x58;
		buffer[13] = 0x04;
		res |= i2c.write(PXS_SETTINGS_0_0, 14, buffer, I2C_Repeat_Start);

		// Fill buffer with first settings
		buffer[0] = 0x16;	// Prox
		buffer[1] = 0x10;	// Touch
		buffer[2] = 0x16;	// Prox
		buffer[3] = 0x18;	// Touch
		res |= i2c.write(P_THR_CH0, 4, buffer, I2C_Repeat_Start);

		// Fill buffer with first settings
		buffer[0] = PMU_SETTINGS_STARTUP;
		res |= i2c.write(DEV_SETTINGS+2, 1, buffer, I2C_Repeat_Start);

		// Fill buffer with first settings
		buffer[0] = 0x42;
		res |= i2c.write(SYSTEM_SETTINGS_0, 1, buffer, I2C_Stop);

		if(!res) {
			// Capactive mode
			iqs621.sensorMode = Capacitive;
		}
	}
	// Inductive Sensing mode
	else if (setupType == Inductive)
	{
		// Setup for inductive sensing mode
		// Write PXS Settings
		buffer[0] = 0x03;	// CH 0
		buffer[1] = 0x93;	// CH 1   Metal UI
		buffer[2] = 0x57;	// CH 0
		buffer[3] = 0x0F;	// CH 1 Set Proj_bias to 20uA and small CS
//		buffer[3] = 0x17;	// Set Proj_bias to 20uA
		buffer[4] = 0x60;	// CH 0
		buffer[5] = 0x50; // CH 1
		buffer[6] = 0x26;
		buffer[7] = 0x26;
#ifndef FERRITE
		buffer[8] = 0x40;
#else
		buffer[8] = 0x60;
#endif
		buffer[9] = 0x01;
		buffer[10] = 0xB1;
		buffer[11] = 0x34;
		buffer[12] = 0x58;
		buffer[13] = 0x04;
		res |= i2c.write(PXS_SETTINGS_0, 14, buffer, I2C_Repeat_Start);

		// Write Metal UI settings
		buffer[0] = 0x00;
		buffer[1] = 0x06;
		buffer[2] = 0x16;
		buffer[3] = 0x20;
		res |= i2c.write(0x60, 4, buffer, I2C_Repeat_Start);


		// Fill buffer with first settings
		buffer[0] = 0x42;
		res |= i2c.write(SYSTEM_SETTINGS_0, 1, buffer, I2C_Stop);

		if(!res)
		{
			// Capactive mode
			iqs621.sensorMode = Inductive;
		}
	}

	// Wait for Redo Ati to complete
	do {
		while(!i2c.isDeviceReady());		// Device is not yet ready

		res |= i2c.read(SYSTEM_FLAGS, 1, &iqs621.SystemFlags.SystemFlags, I2C_Stop);

	} while (!res && iqs621.SystemFlags.InAti);


	return res;
}

/**
 * @brief	ATI Helper function for iqs621
 */
void redo_ati(void)
{
	uint8_t res = 0;

	// Fill buffer with first settings
	buffer[0] = 0x42;
	res |= i2c.write(SYSTEM_SETTINGS_0, 1, buffer, I2C_Stop);

	// Wait for Redo Ati to complete
	do {
		while(!i2c.isDeviceReady()); // Device is not ready, wait for it
		res |= i2c.read(SYSTEM_FLAGS, 1, &iqs621.SystemFlags.SystemFlags, I2C_Stop);
	} while (!res && iqs621.SystemFlags.InAti);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//----------------------- IQS621 MODE 1: Absolute mode ---------------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays 0-9999 as absolute lux on the 7-segment display
void absoluteMode()
{
	display_number = iqs621.ALSValue.ALSValue;

  // zero led for: 200 > Lux
  if(display_number <= 200)
  {
    // Switch off all leds
    digitalWrite(LED_4, LOW);
    digitalWrite(LED_3, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_1, LOW);
  }
  // lowest led for: 500 > Lux < 200
  else if(display_number <= 500)
  {
    digitalWrite(LED_4, HIGH);
    // Switch off other leds
    digitalWrite(LED_3, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_1, LOW);
  }
  // Lower 2 leds for: 1000 > Lux > 500
  else if(display_number <= 1000)
  {
    digitalWrite(LED_4, HIGH);
    digitalWrite(LED_3, HIGH);
    // Switch off other leds
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_1, LOW);
  }
  // Lower 2 & Upper 1 leds for: 2000 > Lux > 1000
  else if(display_number <= 2000)
  {
    digitalWrite(LED_4, HIGH);
    digitalWrite(LED_3, HIGH);
    digitalWrite(LED_2, HIGH);
    // Switch off other leds
    digitalWrite(LED_1, LOW);
  }
  // All leds for: 5000 > Lux > 2000
  else if(display_number > 5000)
  {
    digitalWrite(LED_4, HIGH);
    digitalWrite(LED_3, HIGH);
    digitalWrite(LED_2, HIGH);
    digitalWrite(LED_1, HIGH);
  }
  
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//----------------------- IQS621 MODE 2: Relative mode ---------------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (-999) - (999) as relative position on the 7-segment display
void relativeMode()
{
	static bool firstLoopThrough = true;	// This is the first loop
	static int16_t als_mem = 0;				// Memory to compare with
	int16_t als_rel = 0;


	if (firstLoopThrough || iqs621.PXSUIFlags.CH0_T_Out)
	{
		als_mem = iqs621.ALSValue.ALSValue;                   //First, als_mem to current measurement (zero reading)
		firstLoopThrough = false;
	}

	als_rel = iqs621.ALSValue.ALSValue - als_mem;

	// This is the number to display
	display_number = als_rel;
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//-------------------------- IQS621 MODE 3: Light/Dark mode ----------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays the Light/Dark output of the ALS UI on the 7-segment display. Threshold adjustable
void light_darkMode()
{
	static uint16_t als_thr_mem = 100;
	static uint16_t als_thr = 100;
	static uint8_t als_D_thr = 0;
	static uint8_t als_L_thr = 0;
	static bool shouldWriteThreshold = false;
	uint8_t res = 0;					// Result of the I2C transfer
	static bool displayData = true;		// Display the data L or D by default

	if((aux_button == Released) && (iqs621.PXSUIFlags.CH0_T_Out))
		aux_button = Touch;						// a touch occured on the aux button
	else if((aux_button == Touch) && (!iqs621.PXSUIFlags.CH0_T_Out))
		aux_button = Pressed;				// the mode button was now pressed

	// Button was pressed, move to next threshold
	if((aux_button == Pressed) && (!iqs621.PXSUIFlags.CH0_T_Out)) {
		aux_button = Released;				// No more touch on the button, move to next state

		// Now we do our thingy

		// 100 Lux is max for demo
		if(als_thr_mem > 900)
		{
			als_thr = 100;
		}
		// Now we step with 100 at a time up until 1000
		else if(als_thr_mem >= 400)
		{
			als_thr = als_thr_mem + 100;
		}
		//	 For the smaller values, let's step 50
		else
		{
			als_thr = als_thr_mem + 50;
		}

		als_thr_mem = als_thr;

		// Do some scaling
		als_D_thr = (uint8_t)(als_thr/4) & 0xFF;
		als_L_thr = (uint8_t)(als_thr/16) & 0xFF;

		// Now write the values
		shouldWriteThreshold = true;
	}

	if(shouldWriteThreshold)
	{
		// Now we need to write the threshold to the IC
		buffer[0] = als_D_thr;
		buffer[1] = als_L_thr;

		do {
			res = i2c.write(ALS_DARK_THR, 2, buffer, I2C_Stop);				// Write the Light and dark Threshold values
		} while(res);

		if(!res)
		{
			//disp.write(als_thr);

			shouldWriteThreshold = false;

			displayData = false;
			// Start the timer
			setTimer(&Mode_Switch_Timer);
		}
		else
            {
			//disp.writeError(res);
            }
	}

	// Reset display timeout to default

	// should we display the Threshold value or the data?
	if(timerExpired(&Mode_Switch_Timer))
		displayData = true;

	// Update display
	if(displayData)
	{
		// Now we should a String
		displayState = Display_String;

		// Also format the North South bit, if a magnet is present
		if(iqs621.HallFlags.Hall_Prox)
			sprintf(display_string, "%c  %c", iqs621.HallFlags.Hall_N_S ? 'n' : 's', iqs621.ALSFlags.Light_Dark ? 'l': 'd');
		else
			sprintf(display_string, "   %c", iqs621.ALSFlags.Light_Dark ? 'l': 'd');

    // upper leds for North
    if(iqs621.HallFlags.Hall_N_S)
    {
      digitalWrite(LED_3, iqs621.HallFlags.Hall_Prox);
      digitalWrite(LED_4, iqs621.HallFlags.Hall_Touch);
      // Switch off South leds
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
    }
    // Lower leds for South
    else
    {
     digitalWrite(LED_1, iqs621.HallFlags.Hall_Touch);
     digitalWrite(LED_2, iqs621.HallFlags.Hall_Prox);
      // Switch off North leds
      digitalWrite(LED_3, LOW);
      digitalWrite(LED_4, LOW);
    }   
	}
	else
	{
		displayState = Display_Int;
		display_number = als_thr;
	}
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//-------------------------- IQS62x MODE 3: Prox mode ----------------------------------------------
//--------------------------------------------------------------------------------------------------
// Display Raw data received from IQS62x on the 7-segment display
void proxMode()
{
  if(ICType == IQS620)
  {
  	display_number = iqs620.Ch[0].Ch;
    digitalWrite(LED_1, iqs620.PXSUIFlags.CH0_P_Out);
    digitalWrite(LED_2, iqs620.PXSUIFlags.CH0_T_Out);
  }
  else if(ICType == IQS621)
  {
	  display_number = iqs621.Ch[0].Ch;
    digitalWrite(LED_1, iqs621.PXSUIFlags.CH0_P_Out);
    digitalWrite(LED_2, iqs621.PXSUIFlags.CH0_T_Out);
  }
  else if(ICType == IQS622)
  {
    display_number = iqs622.Ch[0].Ch;
    digitalWrite(LED_1, iqs622.PXSUIFlags.CH0_P_Out);
    digitalWrite(LED_2, iqs622.PXSUIFlags.CH0_T_Out);
  }
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//-------------------------- IQS621 MODE 4: Inductive mode -----------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * @brief	Do inductive sensing
 * @param	[bool *] should we refresh display?
 */
void inductiveMode(bool *refreshDisplay)
{
	// If the previous display number matches the current value, do not update display
	if((uint16_t)display_number == iqs621.MetalValue.MetalValue)
		*refreshDisplay = false;

#ifndef FERRITE
	// Display the metal value
	display_number = iqs621.MetalValue.MetalValue;
#else
	iqs621.MetalFlags.Metal_Type ? display_number = iqs621.MetalValue.MetalValue :
			 display_number = -iqs621.MetalValue.MetalValue;
#endif
}


/**
 * @brief	Check is coil is present in whichever mode
 */
bool isCoilPresent(void) {

	if(iqs621.sensorMode == Capacitive) {
		iqs621.coilFirstExit = false;
		return (iqs621.coilPresent = (iqs621.Ch[0].Ch > COIL_THRESHOLD));
	}
	else if(iqs621.sensorMode == Inductive) {
		bool tmp;
		tmp = (iqs621.Ch[1].Ch < COIL_THRESHOLD);

    	// Is the Coil removed, or have we simply got the metal too close to the coil?!
    	if (!tmp) { // Coil is removed or saturated...
    		// Is this the first occurrence?
    		if (iqs621.coilFirstExit) { // No, we've been in here before
    			// Check Coil saturation time-out
    			if (timerExpired(&CoilTimer)) { // Saturated longer than time-out, assume coil removed.
    				iqs621.coilFirstExit = false;
    				return (iqs621.coilPresent = false);
    			}
    			return (iqs621.coilPresent = true); // Not expired, still "coil"
    		}
    		else {
    			// Setup Coil Timer
    		    iqs621.coilFirstExit = true;
				setTimer(&CoilTimer, 2000); // 2s time-out
    			return (iqs621.coilPresent = true);
    		}
    	}
    	else { // No, the coil value is fine...
    		iqs621.coilFirstExit = false;
    		return (iqs621.coilPresent = true);
    	}
	}
	else {
		iqs621.coilFirstExit = false;
		return iqs621.coilPresent = false;
	}

	return false;
}
//--------------------------------------------------------------------------------------------------


/**************************************************************************************************/
/*                                                                                                */
/*                    Setup IQS622                                                                */
/*                                                                                                */
/**************************************************************************************************/

/**
 * @brief Check which setup we need to run for the IQS622 - capacitive or inductive
 * @param
 * @retval  None
 */
uint8_t setup_iqs622(Setup_Type_e setupType)
{
  uint8_t res = 0;

  while(!i2c.isDeviceReady());

  // Normal Capacitive sensing mode
  if(setupType == Capacitive)
  {
    // Fill buffer with first settings
    buffer[0] = PXS_SETTINGS_0_0_VAL;
    buffer[1] = PXS_SETTINGS_0_1_VAL;
    buffer[2] = 0x77;
    buffer[3] = 0x7F;
    buffer[4] = PXS_SETTINGS_2_0_VAL;
    buffer[5] = PXS_SETTINGS_2_1_VAL;
    buffer[6] = 0x05;
    buffer[7] = 0x26;
    buffer[8] = 0x00;
    buffer[9] = 0x01;
    buffer[10] = 0xB1;
    buffer[11] = 0x34;
    buffer[12] = 0x58;
    buffer[13] = 0x04;
    res |= i2c.write(PXS_SETTINGS_0_0, 14, buffer, I2C_Repeat_Start);

    // Fill buffer with first settings
    buffer[0] = 0x16; // Prox
    buffer[1] = 0x10; // Touch
    buffer[2] = 0x16; // Prox
    buffer[3] = 0x18; // Touch
    res |= i2c.write(P_THR_CH0, 4, buffer, I2C_Repeat_Start);

    // Fill buffer with first settings
    buffer[0] = 0x04; // 0x70 - Default: 
    buffer[1] = 0x80; // 0x71 - ALS settings_1
    buffer[2] = 0x04; // 0x72 - IR Settings_0
    buffer[3] = 0x80; // 0x73 - IR Setting_1: Target = 32*32=1024
    res |= i2c.write(0x70, 4, buffer, I2C_Repeat_Start);

    // Fill buffer with first settings
    buffer[0] = PMU_SETTINGS_STARTUP;
    res |= i2c.write(DEV_SETTINGS+2, 1, buffer, I2C_Repeat_Start);

    // Fill buffer with first settings
    buffer[0] = 0x42;
    res |= i2c.write(SYSTEM_SETTINGS_0, 1, buffer, I2C_Stop);

    if(!res)
    {
      // Capactive mode
      iqs621.sensorMode = Capacitive;
    }
  }

  // Wait for Redo Ati to complete
  do {
    while(!i2c.isDeviceReady());    // Device is not yet ready

    res |= i2c.read(SYSTEM_FLAGS, 1, &iqs622.SystemFlags.SystemFlags, I2C_Stop);

          Serial.print("System Flags");
          Serial.print("\t");
          Serial.println(iqs622.SystemFlags.SystemFlags, HEX);

  } while (!res && iqs622.SystemFlags.InAti);

  return res;
}


//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//----------------------------- IQS622 MODE 1: IR mode ---------------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays 0-9999 as the IR output on the 7-segment display
void IRMode()
{
  uint8_t temp;
  
  if(iqs622.IRValue.IRValue < 9999)
  {
    display_number = (iqs622.IRValue.IRValue);
	Serial.print("IQS622: IR Value = ");
    Serial.println(iqs622.IRValue.IRValue);
  }
  else
  {
    display_number = 9999;
  }

    // zero led for: 20 > IRValue
  if(display_number <= 20)
  {
    // Switch off all leds
    digitalWrite(LED_4, LOW);
    digitalWrite(LED_3, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_1, LOW);
  }
  // lowest led for: 100 > IRValue < 20
  else if(display_number <= 100)
  {
    digitalWrite(LED_4, HIGH);
    // Switch off other leds
    digitalWrite(LED_3, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_1, LOW);
  }
  // Lower 2 leds for: 800 > IRValue > 100
  else if(display_number <= 800)
  {
    digitalWrite(LED_4, HIGH);
    digitalWrite(LED_3, HIGH);
    // Switch off other leds
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_1, LOW);
  }
  // Lower 2 & Upper 1 leds for: 2000 > IRValue > 1000
  else if(display_number <= 2000)
  {
    digitalWrite(LED_4, HIGH);
    digitalWrite(LED_3, HIGH);
    digitalWrite(LED_2, HIGH);
    // Switch off other leds
    digitalWrite(LED_1, LOW);
  }
  // All leds for: IRValue > 6000
  else if(display_number > 6000)
  {
    digitalWrite(LED_4, HIGH);
    digitalWrite(LED_3, HIGH);
    digitalWrite(LED_2, HIGH);
    digitalWrite(LED_1, HIGH);
  }
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//----------------------- IQS622 MODE 2: basic IR mode ---------------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays 0-10 (only lower nibble) as the IR output on the 7-segment display
void basicIRMode()
{

  Serial.print("IR value : ");
  Serial.print(0x0F & iqs622.IRFlags.IRFlags);
  Serial.print("\tHallFlags value : 0x");
  Serial.println(0x07 & iqs622.HallFlags.HallFlags);

  display_number = (0x0F & iqs622.IRFlags.IRFlags);

  // Also format the North South bit, if a magnet is present
  if(iqs622.HallFlags.Hall_Prox)
  {
    displayState = Display_String;
    sprintf(display_string, "%c  %c", iqs622.HallFlags.Hall_N_S ? 'n' : 's', display_number);

    // upper leds for North
    if(iqs622.HallFlags.Hall_N_S)
    {
      digitalWrite(LED_3, iqs622.HallFlags.Hall_Prox);
      digitalWrite(LED_4, iqs622.HallFlags.Hall_Touch);
      // Switch off South leds
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
    }
    // Lower leds for South
    else
    {
     digitalWrite(LED_1, iqs622.HallFlags.Hall_Touch);
     digitalWrite(LED_2, iqs622.HallFlags.Hall_Prox);
      // Switch off North leds
      digitalWrite(LED_3, LOW);
      digitalWrite(LED_4, LOW);
    }
  
  }
  else
  {
    displayState = Display_String;
    sprintf(display_string, "%c%c %i", '1', 'r', display_number);
    
    // Switch off all leds
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);
    digitalWrite(LED_4, LOW);
  }
  
}

//--------------------------------------------------------------------------------------------------
//----------------------------- IQS622 MODE 3: basic ALS mode --------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays 0-10 (only lower nibble) as the ALS output on the 7-segment display
void basicALSMode()
{

  Serial.print("ALS value : ");
  Serial.print(0x0F & iqs622.ALSFlags.ALSFlags);
  Serial.print("\tHallFlags value : 0x");
  Serial.println(0x07 & iqs622.HallFlags.HallFlags);

  display_number = (0x0F & iqs622.ALSFlags.ALSFlags);

  // Also format the North South bit, if a magnet is present
  if(iqs622.HallFlags.Hall_Prox)
  {
    displayState = Display_String;
    sprintf(display_string, "%c  %c", iqs622.HallFlags.Hall_N_S ? 'n' : 's', display_number);

    // upper leds for North
    if(iqs622.HallFlags.Hall_N_S)
    {
      digitalWrite(LED_3, iqs622.HallFlags.Hall_Prox);
      digitalWrite(LED_4, iqs622.HallFlags.Hall_Touch);
      // Switch off South leds
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
    }
    // Lower leds for South
    else
    {
     digitalWrite(LED_1, iqs622.HallFlags.Hall_Touch);
     digitalWrite(LED_2, iqs622.HallFlags.Hall_Prox);
      // Switch off North leds
      digitalWrite(LED_3, LOW);
      digitalWrite(LED_4, LOW);
    }
  }
  else
  {
    displayState = Display_String;
    sprintf(display_string, "%c  %i", 'L', display_number);
    

    // Switch off all leds
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);
    digitalWrite(LED_4, LOW);
  }
}
//--------------------------------------------------------------------------------------------------

/**************************************************************************************************/
/*                                                                                                */
/*                    Setup IQS624                                                                */
/*                                                                                                */
/**************************************************************************************************/
/**
 * @brief Check which setup we need to run for the IQS624 - capacitive & hall rotation
 * @param
 * @retval  None
 */
uint8_t setup_iqs624()
{
  uint8_t res = 0;

  while(!i2c.isDeviceReady());
  
    // Fill buffer with first settings
    buffer[0] = 0x48|REDO_ATI_ALL;
    res |= i2c.write(DEV_SETTINGS, 1, buffer, I2C_Repeat_Start);

    buffer[0] = PMU_SETTINGS_STARTUP;
    buffer[1] = 0x0E;         // 15ms Comms report rate
    res |= i2c.write(DEV_SETTINGS+2, 2, buffer, I2C_Repeat_Start);

    // Check to see if phase angles were written
    if(eeprom_read_byte(WRITE_FLAG_ADDRESS) == 0x01)
    {
      buffer[0] = eeprom_read_byte(SINE_PHASE_ADDRESS);
      buffer[1] = eeprom_read_byte(COS_PHASE_ADDRESS);
      res |= i2c.write(HALL_SIN, 2, buffer, I2C_Repeat_Start);
    }
    else {

      buffer[0] = HALL_SIN_STARTUP;
      res |= i2c.write(HALL_SIN, 1, buffer, I2C_Repeat_Start);

      buffer[0] = HALL_COS_STARTUP;
      res |= i2c.write(HALL_COS, 1, buffer, I2C_Repeat_Start);
    }

    // Write Settings if valid
    if(eeprom_read_byte(ATI_FLAG_ADDRESS) == 0x01)
    {
      iqs624.HallAtiSettings_CH2_CH3.ATISettings = eeprom_read_byte(HALL_ATI_CH2_CH3_ADDRESS);
      iqs624.HallAtiSettings_CH4_CH5.ATISettings = eeprom_read_byte(HALL_ATI_CH4_CH5_ADDRESS);
      res |= i2c.write(0x72, 2, &iqs624.HallAtiSettings_CH2_CH3.ATISettings, I2C_Repeat_Start);
    }

    buffer[0] = PXS_CH1_TCH_THRES;
    res |= i2c.write(PXS_CH1_THRES, 1, buffer, I2C_Repeat_Start);

    // Wait for Redo Ati to complete
    do {
      res |= i2c.read(SYSTEM_FLAGS, 1, &iqs624.SystemFlags.SystemFlags, I2C_Stop);
    } while (!res && iqs624.SystemFlags.InAti);

    // Force a write of the read-only auto compensation values
    // Ratio Scale factor - 2 bytes
    while(!i2c.isDeviceReady());
    buffer[0] = 0xD4;
    res |= i2c.write(0xF0, 1, buffer, I2C_Stop);

    while(!i2c.isDeviceReady());
    buffer[0] = 0xFF;
    res |= i2c.write(0xF1, 1, buffer, I2C_Stop);

    while(!i2c.isDeviceReady());
    buffer[0] = 0xD5;
    res |= i2c.write(0xF0, 1, buffer, I2C_Stop);
    
    while(!i2c.isDeviceReady());
    buffer[0] = 0x00;
    res |= i2c.write(0xF1, 1, buffer, I2C_Stop);
  
  return res;
}

/**************************************************************************************************/
/*                                                                                                */
/*                    Setup IQS624n                                                               */
/*                                                                                                */
/**************************************************************************************************/
/**
 * @brief Check which setup we need to run for the IQS624n - capacitive & hall rotation
 * @param
 * @retval  None
 */
uint8_t setup_iqs624n()
{
  uint8_t res = 0;

  while(!i2c.isDeviceReady());
  
    //read FG for calibration data
    buffer[0] = 0x13;
    res |= i2c.write(0xF0, 1, buffer, I2C_Repeat_Start);

    res |= i2c.read(0xF1, 1, &iqs624n.CalibrationBin, I2C_Stop);
    iqs624n.CalibrationBin = iqs624n.CalibrationBin & 0x0F;
    

    // Fill buffer with first settings
    buffer[0] = 0x48|REDO_ATI_ALL;
    res |= i2c.write(DEV_SETTINGS, 1, buffer, I2C_Repeat_Start);

    buffer[0] = PMU_SETTINGS_STARTUP;
    buffer[1] = 0x0E;         // 15ms Comms report rate
    res |= i2c.write(DEV_SETTINGS+2, 2, buffer, I2C_Repeat_Start);

    // Check to see if phase angles were written
    if(eeprom_read_byte(WRITE_FLAG_ADDRESS) == 0x01)
    {
      buffer[0] = eeprom_read_byte(SINE_PHASE_ADDRESS);
      buffer[1] = eeprom_read_byte(COS_PHASE_ADDRESS);
      res |= i2c.write(HALL_SIN, 2, buffer, I2C_Repeat_Start);
    }
    else
    {
      buffer[0] = HALL_SIN_STARTUP;
      res |= i2c.write(HALL_SIN, 1, buffer, I2C_Repeat_Start);

      buffer[0] = HALL_COS_STARTUP;
      res |= i2c.write(HALL_COS, 1, buffer, I2C_Repeat_Start);
    }

    // Write Settings if valid
    if(eeprom_read_byte(ATI_FLAG_ADDRESS) == 0x01)
    {
      iqs624.HallAtiSettings_CH2_CH3.ATISettings = eeprom_read_byte(HALL_ATI_CH2_CH3_ADDRESS);
      iqs624.HallAtiSettings_CH4_CH5.ATISettings = eeprom_read_byte(HALL_ATI_CH4_CH5_ADDRESS);
      res |= i2c.write(0x72, 2, &iqs624n.HallAtiSettings_CH2_CH3.ATISettings, I2C_Repeat_Start);
    }

    //write calibration targets
    buffer[0] = LeftTarget[iqs624n.CalibrationBin-1]/32 | 0xC0;; 
    buffer[1] = RightTarget[iqs624n.CalibrationBin-1]/32 | 0xC0; 
    res |= i2c.write(0x72, 2, buffer, I2C_Repeat_Start);

    buffer[0] = 0x1B;
    buffer[1] = 0x28;
    res |= i2c.write(0x42, 2, buffer, I2C_Repeat_Start);

    buffer[0] = 0x09;
    res |= i2c.write(PXS_CH0_THRES, 1, buffer, I2C_Repeat_Start);
    buffer[0] = PXS_CH1_TCH_THRES;
    res |= i2c.write(PXS_CH1_THRES, 1, buffer, I2C_Repeat_Start);

    //WHEEL FILTER BETA
    buffer[0] = 0x0A;
    res |= i2c.write(0x7B, 1, buffer, I2C_Repeat_Start);
       //zero wheel
    buffer[0] = 0x2C;
    res |= i2c.write(0x70, 1, buffer, I2C_Repeat_Start);

    buffer[0] = 0x0A; // interval divider is 10
    res |= i2c.write(0x7D, 1, buffer, I2C_Stop);

    buffer[0] = 0x48|REDO_ATI_ALL;
    res |= i2c.write(DEV_SETTINGS, 1, buffer, I2C_Stop);
    
    // Wait for Redo Ati to complete
    do {
      res |= i2c.read(SYSTEM_FLAGS, 1, &iqs624n.SystemFlags.SystemFlags, I2C_Stop);
    } while (!res && iqs624n.SystemFlags.InAti);
  
  return res;
}

//--------------------------------------------------------------------------------------------------
//------------------------- IQS624n Version 2 MODE 1: Interval mode --------------------------------
//--------------------------------------------------------------------------------------------------
// Displays 0-44 as absolute position on the 7-segment display
void nabsoluteWheelMode(bool *refreshDisplay)
{
  int16_t deg = 0;

  deg = iqs624n.IntervalNumber;

  if (getDeg){
    relative_counter=0;
    getDeg = false;
    *refreshDisplay = true;
    //disp.write(deg);
  }


  if(iqs624n.PXSUIFlags.CH0_T_Out)
    digitalWrite(LED_1, HIGH);
  else
    digitalWrite(LED_1, LOW);

  // Should we refresh the display?
  display_number == deg ? *refreshDisplay = false : *refreshDisplay = true;

  // Set the number to display
  display_number = deg;

  // Save the degrees for later use
  deg_mem = deg;
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------- IQS624 MODE 2: Relative mode -----------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (-999) - (999) as relative position on the 7-segment display
void nrelativeWheelMode(bool *refreshDisplay)
{
  int16_t diff = 0;

  // First loop through
  if (getDeg){
    deg_mem = iqs624n.FillHallDegrees.HallDegrees;
    relative_counter=0;
    getDeg = false;
    *refreshDisplay = true;
    //disp.write(display_number);
    old_interval = iqs624n.IntervalNumber;
  }

  diff =   iqs624n.IntervalNumber - old_interval;//iqs624n.hallAngleDiff;
  old_interval = iqs624n.IntervalNumber;
  if(diff > 22)
  {
    diff = diff-36;
    }
    if(diff <-22)
    {
     diff = 36+diff;
    }

  
  if( (diff >0) )
  {

      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      digitalWrite(LED_4, HIGH);
      digitalWrite(LED_3, HIGH);
  }
  else if( (diff <0))
  {

      digitalWrite(LED_1, HIGH);
      digitalWrite(LED_2, HIGH);
      digitalWrite(LED_4, LOW);
      digitalWrite(LED_3, LOW);   
  }
  else
  {
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      digitalWrite(LED_4, LOW);
      digitalWrite(LED_3, LOW); 
    
    }
relative_counter = relative_counter - diff; 
  if (relative_counter > 999 || relative_counter < -999){
    relative_counter=0;
  }

  // Should we refresh the display? If the value hasn't changed, we do not need to update display
  display_number == relative_counter ? *refreshDisplay = false : *refreshDisplay = true;

  display_number = relative_counter;
}
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
//-------------------------- IQS624n MODE 3: Speed mode --------------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays RPM of the magnet on the 7-segment display
void nspeedMode(bool *refreshDisplay){

  // constant -> 11
  if (iqs624n.HallFlags.Wheel_Movement || getDeg) {

    if(getDeg){
      getDeg = false;

      // Reset variables
      mem_count = RPM_COUNT_RESET;
      rpm_refresh = true;
      speed_rpm = 0;
      display_number = 0; // clear display
      //disp.write(display_number);
      *refreshDisplay = true;
    }

    if (rpm_refresh){
      diff_mem = iqs624n.hallAngleDiff;
      rpm_refresh = false;
    }

    if (mem_count > 0){
      diff_mem = (diff_mem + iqs624n.hallAngleDiff);
      mem_count--;
    }
    else{
      speed_rpm = (diff_mem/(RPM_COUNT_RESET + 1)) * RPM_CONSTANT;
      mem_count = RPM_COUNT_RESET;
      rpm_refresh = true;
    }

    if (abs((long long)display_number) < abs((long long)speed_rpm)){

      // Negative movement switches on left 2 leds
      if(iqs624n.HallFlags.Movement_Dir)
      {
        clear_mode_leds();  // Switch all leds off
        digitalWrite(LED_1, HIGH);
        digitalWrite(LED_2, HIGH);
      }
      // Positive movement switches on right side leds
      else {
        clear_mode_leds();  // Switch all leds off
        digitalWrite(LED_3, HIGH);
        digitalWrite(LED_4, HIGH);
      }

      // Should we refresh the display? If the value hasn't changed, we do not need to update display
      display_number == speed_rpm ? *refreshDisplay = false : *refreshDisplay = true;

      display_number = speed_rpm;

      RPM_refresh_count = REFRESH_RPM_DISPLAY;
    }
    else{
      if(RPM_refresh_count == 0){
        display_number = 0;
        clear_mode_leds();  // Switch all leds off
      }
      else{
        RPM_refresh_count--;
      }
    }

  }
  else{
    speed_rpm = 0;

    clear_mode_leds();  // Switch all leds off

    // Should we refresh the display? If the value hasn't changed, we do not need to update display
    //    display_number == speed_rpm ? *refreshDisplay = false : *refreshDisplay = true;

    display_number = speed_rpm;
  }
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//-------------------------- IQS624n MODE 4: Raw mode ----------------------------------------------
//--------------------------------------------------------------------------------------------------
// Display Raw data received from IQS624 on the 7-segment display
void nrawMode(bool *refreshDisplay){

  static int16_t tempDegree = 0;

    // First loop through, so lets refreshDisplay
    if(getDeg){
      display_number = 0; // clear display
      *refreshDisplay = true;
      //disp.write(display_number);
      getDeg = false;
    }

  if(iqs624n.PXSUIFlags.CH0_T_Out)
    digitalWrite(LED_1, HIGH);
  else
    digitalWrite(LED_1, LOW);
  // Should we refresh the display? If the value hasn't changed, we do not need to update display
  display_number == iqs624n.FillHallDegrees.HallDegrees ? *refreshDisplay = false : *refreshDisplay = true;

    Serial.println(tempDegree);

  display_number = iqs624n.FillHallDegrees.HallDegrees;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------- IQS624 MODE 1: Absolute mode -----------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays 0-99 as absolute position on the 7-segment display
void absoluteWheelMode(bool *refreshDisplay)
{
	int16_t deg = 0;

	deg = (iqs624.HallDegrees.HallDegrees * 10)/36;


	if (( deg > 90 && deg_mem < 10 )||( deg < 10 && deg_mem > 90 ) || getDeg){

		// First loop - clear display
		if(getDeg) {
			*refreshDisplay = true;
			//disp.write(deg);
		}

		getDeg = false;
	}
	else{
		deg = (deg_mem+deg)/2;
	}

	// If we get to more than 99, loop over to 0
	if (deg == 100){
		deg = 0;
	}

	// Should we refresh the display?
	display_number == deg ? *refreshDisplay = false : *refreshDisplay = true;

	// Set the number to display
	display_number = deg;

	// Save the degrees for later use
	deg_mem = deg;
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------- IQS624 MODE 2: Relative mode -----------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (-999) - (999) as relative position on the 7-segment display
void relativeWheelMode(bool *refreshDisplay)
{
	int16_t diff = 0;

	// First loop through
	if (getDeg){
		deg_mem = iqs624.HallDegrees.HallDegrees;
		relative_counter=0;
		getDeg = false;
		*refreshDisplay = true;
		//disp.write(display_number);
	}

	diff = iqs624.HallDegrees.HallDegrees - deg_mem;

	if (diff > 180){
		diff = -(360 - diff);
	}
	else if(diff < -180){
		diff = (360 + diff);
	}

	if (diff >= RELATIVE_DIFF_THRESH){
		while (diff >= RELATIVE_DIFF_THRESH){
			diff = diff - RELATIVE_DIFF_THRESH;
			relative_counter++;
			deg_mem = iqs624.HallDegrees.HallDegrees;
		}
		deg_mem = deg_mem - diff;
	}
	else if (diff <= (-RELATIVE_DIFF_THRESH)){
		while (diff <= (-RELATIVE_DIFF_THRESH)){
			diff=diff+RELATIVE_DIFF_THRESH;
			relative_counter--;
			deg_mem = iqs624.HallDegrees.HallDegrees;
		}

		deg_mem = deg_mem - diff;
	}

	if (relative_counter > 999 || relative_counter < -999){
		relative_counter=0;
	}

	// Should we refresh the display? If the value hasn't changed, we do not need to update display
	display_number == relative_counter ? *refreshDisplay = false : *refreshDisplay = true;

	display_number = relative_counter;
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//-------------------------- IQS624 MODE 3: Speed mode ---------------------------------------------
//--------------------------------------------------------------------------------------------------
// Displays RPM of the magnet on the 7-segment display
void speedMode(bool *refreshDisplay){

	// constant -> 11
	if (iqs624.HallFlags.Wheel_Movement || getDeg) {

		if(getDeg){
			getDeg = false;

			// Reset variables
			mem_count = RPM_COUNT_RESET;
			rpm_refresh = true;
			speed_rpm = 0;
			display_number = 0;	// clear display
			//disp.write(display_number);
			*refreshDisplay = true;
		}

		if (rpm_refresh){
			diff_mem = iqs624.hallAngleDiff;
			rpm_refresh = false;
		}

		if (mem_count > 0){
			diff_mem = (diff_mem + iqs624.hallAngleDiff);
			mem_count--;
		}
		else{
			speed_rpm = (diff_mem/(RPM_COUNT_RESET + 1)) * RPM_CONSTANT;
			mem_count = RPM_COUNT_RESET;
			rpm_refresh = true;
		}

		if (abs((long long)display_number) < abs((long long)speed_rpm)){

			// Negative movement switches on left 2 leds
			if(iqs624.HallFlags.Movement_Dir)
			{
				clear_mode_leds();	// Switch all leds off
				digitalWrite(LED_1, HIGH);
				digitalWrite(LED_2, HIGH);
			}
			// Positive movement switches on right side leds
			else {
				clear_mode_leds();	// Switch all leds off
				digitalWrite(LED_3, HIGH);
				digitalWrite(LED_4, HIGH);
			}

			// Should we refresh the display? If the value hasn't changed, we do not need to update display
			display_number == speed_rpm ? *refreshDisplay = false : *refreshDisplay = true;

			display_number = speed_rpm;

			RPM_refresh_count = REFRESH_RPM_DISPLAY;
		}
		else{
			if(RPM_refresh_count == 0){
				display_number = 0;
				clear_mode_leds();	// Switch all leds off
			}
			else{
				RPM_refresh_count--;
			}
		}

	}
	else{
		speed_rpm = 0;

		clear_mode_leds();	// Switch all leds off

		// Should we refresh the display? If the value hasn't changed, we do not need to update display
		// display_number == speed_rpm ? *refreshDisplay = false : *refreshDisplay = true;

		display_number = speed_rpm;
	}
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//-------------------------- IQS624 MODE 4: Raw mode -----------------------------------------------
//--------------------------------------------------------------------------------------------------
// Display Raw data received from IQS624 on the 7-segment display
void rawMode(bool *refreshDisplay){

	static int16_t tempDegree = 0;

	if (iqs624.HallFlags.Wheel_Movement || getDeg) {

		// First loop through, so lets refreshDisplay
		if(getDeg){
			*refreshDisplay = true;
			//disp.write(display_number);
		}

		tempDegree = iqs624.HallDegrees.HallDegrees;
		deg_mem = iqs624.HallDegrees.HallDegrees;
		getDeg = false;
		mem_count = MEM_COUNT_RESET;
	}
	else{

		if(((iqs624.HallDegrees.HallDegrees < 10) && (deg_mem > 350)) || ((iqs624.HallDegrees.HallDegrees > 350) && (deg_mem < 10))){
			deg_mem = iqs624.HallDegrees.HallDegrees;
			mem_count = MEM_COUNT_RESET;
		}

		if (mem_count > 0){
			deg_mem = (iqs624.HallDegrees.HallDegrees + deg_mem)/2;
			mem_count--;
		}
		else{
			tempDegree = deg_mem;
			mem_count = MEM_COUNT_RESET;
		}
	}

	// Should we refresh the display? If the value hasn't changed, we do not need to update display
	display_number == tempDegree ? *refreshDisplay = false : *refreshDisplay = true;

		Serial.println(tempDegree);

	display_number = tempDegree;
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//-------------------------- MODE 5: ATI Algorithm mode --------------------------------------------
//--------------------------------------------------------------------------------------------------
// Calibrate the wheel for better accuracy
void run_ati_algo(void)
{
	// Moerse klomp statics
	static float maxCH2 = 0, maxCH3 = 0;
	static uint16_t noise = 80;
	static uint8_t countATIRoutines_CH2_CH3 = 0;
	int16_t deltaCH2 = 0, deltaCH3 = 0;
	static bool doneWithCH2 = false;
	static uint8_t countMaxes = 0;
	static int16_t counter = 0;
	static uint8_t binCounter_1 = 0, binCounter_2 = 0, binCounter_3 = 0, binCounter_4 = 0;
	static Quads_t quads;


	// Run Calibration state machine
	switch(algoSM)
	{
	// Loop 1 - get min/max
	case 0:

		// First entry to this ATI calibration mode
		if(modeEntry)
		{
			modeEntry = false;

			// Wipe that side of the EEPROM
			eeprom_write_byte(ATI_FLAG_ADDRESS, 0x00);
			eeprom_write_byte(HALL_ATI_CH2_CH3_ADDRESS, 0);
			eeprom_write_byte(HALL_ATI_CH4_CH5_ADDRESS, 0);


			counter = 0;
			binCounter_1 = 0;
			binCounter_2 = 0;
			binCounter_3 = 0;
			binCounter_4 = 0;

			/*
			 * Set ATI target to 25 x 32 = 800 counts
			 * Set the Base value to 200 and check the swing. The max value should be around 1200 counts +- 100
			 * We only need to check channel 2 and 3; we assume that channel 4 and 5 is close enough
			 */
			iqs624.HallAtiSettings_CH2_CH3.ATI_Base = Base_200;
			iqs624.HallAtiSettings_CH2_CH3.ATI_Target = 25;
			iqs624.HallAtiSettings_CH4_CH5.ATI_Base = Base_200;
			iqs624.HallAtiSettings_CH4_CH5.ATI_Target = 25;
			// Write ATI Settings
			i2c.write(0x72, 2, &iqs624.HallAtiSettings_CH2_CH3.ATISettings, I2C_Stop);
		}

		if(first)
		{
			first = false;
			// Display info
			displayState = Display_String;
			Loop = Display_Info;
			sprintf(display_string, "rot ");

			// reset the max values
			maxCH2 = 0;
			maxCH3 = 0;

			countMaxes = 0;

			// Read base values and target
			i2c.read(0x72, 2, &iqs624.HallAtiSettings_CH2_CH3.ATISettings, I2C_Stop);

			Serial.println("ATI - Rotate");
		}

		// Get Max and values
		if(iqs624.AvgCh[2].Ch >= maxCH2)
			maxCH2 = (float)iqs624.AvgCh[2].Ch;

		if(iqs624.AvgCh[3].Ch >= maxCH3)
			maxCH3 = (float)iqs624.AvgCh[3].Ch;

		// scale noise 10% of ATI target
		noise = (iqs624.HallAtiSettings_CH2_CH3.ATI_Target*32)/5;

		// Stop rotation
//		if((maxCH2 != 0 && maxCH3 != 0) &&
//			((maxCH2 > (iqs624.AvgCh[2].Ch*1.0 + noise*1.0)) &&
//			(maxCH3 > (iqs624.AvgCh[3].Ch*1.0 + noise*1.0)))) {

		if((maxCH2 != 0 && maxCH3 != 0) && (binCounter_1 > 1 && binCounter_2 > 1 &&
				binCounter_3 > 1 && binCounter_4 > 1)) {

				first = true;
				countMaxes++;
				algoSM++;	// Move to next state

				Serial.println("");
				Serial.println("Noise: ");
				Serial.println(noise);
				Serial.println("Max: ");
				Serial.println(maxCH2);
				Serial.println(maxCH3);

				break;
		}

		// Obtain Quads
		quads = (Quads_t)iqs624.HallRatioSettings.Quadrature;

		// Check quads
		switch(quads)
		{
			case Quad_1:

				if(counter++ >= 2) // counter
				{
					counter = 0;
					binCounter_1++;
					Serial.println("Bin Quad 1");
				}
				break;

			case Quad_2:

				if(counter++ >= 2) // counter
				{
					counter = 0;
					binCounter_2++;
					Serial.println("Bin Quad 2");
				}
				break;

			case Quad_3:

				if(counter++ >= 2) // counter
				{
					counter = 0;
					binCounter_3++;
					Serial.println("Bin Quad 3");
				}
				break;

			case Quad_4:

				if(counter++ >= 2) // counter
				{
					counter = 0;
					binCounter_4++;
					Serial.println("Bin Quad 4");
				}
				break;
		}

		break;

		// Determine angles
	case 1:

		if(first)
		{
			first = false;

			displayState = Display_String;
			Loop = Display_Info;
			sprintf(display_string, "stop");

			Serial.println("ATI - Wait");
		}


		// Check the difference between the max of the channels and COUNT_VALUE
		// If the max value is less than 500 away from COUNT_VALUE, change the target.
		// If the target is more than 500 away, change the base value
		// Get the deltas
		deltaCH2 = (int16_t)(maxCH2-COUNT_VALUE);
		deltaCH3 = (int16_t)(maxCH3-COUNT_VALUE);

		if(abs((long long)(deltaCH2)) < BASE_THRESHOLD)
		{
			if (abs((long long)(deltaCH2)) > STOP_THRESHOLD)
			{
				// Let us move the target
				if(deltaCH2 < 0) {
					iqs624.HallAtiSettings_CH2_CH3.ATI_Target += 1;

					if(countATIRoutines_CH2_CH3 > 8)
					{
						countATIRoutines_CH2_CH3 = 0;

						if(iqs624.HallAtiSettings_CH2_CH3.ATI_Base > Base_100)
							iqs624.HallAtiSettings_CH2_CH3.ATI_Base -= 1;
					}
				}
				else {
					iqs624.HallAtiSettings_CH2_CH3.ATI_Target -= 1;

					if(countATIRoutines_CH2_CH3 > 8)
					{
						countATIRoutines_CH2_CH3 = 0;

						if(iqs624.HallAtiSettings_CH2_CH3.ATI_Base < Base_200)
							iqs624.HallAtiSettings_CH2_CH3.ATI_Base += 1;
					}
				}

				countATIRoutines_CH2_CH3++;

				// Copy the settings for the 1 channel set to the next channel set
				iqs624.HallAtiSettings_CH4_CH5.ATISettings = iqs624.HallAtiSettings_CH2_CH3.ATISettings;
				// Write ATI Settings
				i2c.write(0x72, 2, &iqs624.HallAtiSettings_CH2_CH3.ATISettings, I2C_Stop);
			}
			// Stop running
//			else if ((abs(long long)((deltaCH2) < STOP_THRESHOLD)) && (abs(long long)((deltaCH3) < STOP_THRESHOLD + 50)))
			else if ((abs((long long)(deltaCH2)) < STOP_THRESHOLD) && (abs((long long)(deltaCH3)) < (STOP_THRESHOLD + 150)))
			{
				doneWithCH2 = true;
				Serial.println("Stop stop now!");
			}
		}
		// otherwise we step the base values
		else if (abs((long long)(deltaCH2)) > BASE_THRESHOLD){

			if (abs((long long)(deltaCH2)) > STOP_THRESHOLD)
			{
				// Let us move the target
				if(deltaCH2 < 0){
					iqs624.HallAtiSettings_CH2_CH3.ATI_Target += 3;

					if(countATIRoutines_CH2_CH3 > 8)
					{
						countATIRoutines_CH2_CH3 = 0;

						if(iqs624.HallAtiSettings_CH2_CH3.ATI_Base > Base_100)
							iqs624.HallAtiSettings_CH2_CH3.ATI_Base -= 1;
					}

				}
				else{
					iqs624.HallAtiSettings_CH2_CH3.ATI_Target -= 3;

					if(countATIRoutines_CH2_CH3 > 8)
					{
						countATIRoutines_CH2_CH3 = 0;

						if(iqs624.HallAtiSettings_CH2_CH3.ATI_Base < Base_200)
							iqs624.HallAtiSettings_CH2_CH3.ATI_Base += 1;
					}
				}

				countATIRoutines_CH2_CH3++;

				// Copy the settings for the 1 channel set to the next channel set
				iqs624.HallAtiSettings_CH4_CH5.ATISettings = iqs624.HallAtiSettings_CH2_CH3.ATISettings;
				// Write ATI Settings
				i2c.write(0x72, 2, &iqs624.HallAtiSettings_CH2_CH3.ATISettings, I2C_Stop);
			}
			// Stop running - let the stop condition also look at channel 3
			else if ((abs((long long)(deltaCH2)) < STOP_THRESHOLD) && (abs((long long)(deltaCH3)) < (STOP_THRESHOLD + 150)))
			{
				doneWithCH2 = true;
				Serial.println("Stop stop now!");
			}
		}

		if(doneWithCH2)
		{
			first = true;
			algoSM = 0;	// Move to next state

			// Go to Switch mode state
			Loop = Switch_Mode;

			// Now run calibration
			Mode = Mode_6;

			// Start the timer
			setTimer(&Mode_Switch_Timer);

			doneWithCH2 = false;

			Serial.println("Stepping out!");

			Serial.println("Settings:");
			Serial.println(iqs624.HallAtiSettings_CH2_CH3.ATI_Base);
			Serial.println(iqs624.HallAtiSettings_CH2_CH3.ATI_Target);
			Serial.println(iqs624.HallAtiSettings_CH4_CH5.ATI_Base);
			Serial.println(iqs624.HallAtiSettings_CH4_CH5.ATI_Target);

			// Save values in EEPROM
			eeprom_write_byte(ATI_FLAG_ADDRESS, 0x01);
			eeprom_write_byte(HALL_ATI_CH2_CH3_ADDRESS, iqs624.HallAtiSettings_CH2_CH3.ATISettings);
			eeprom_write_byte(HALL_ATI_CH4_CH5_ADDRESS, iqs624.HallAtiSettings_CH4_CH5.ATISettings);

			break;
		}
		else
		{
			// Run ATI
			// Fill buffer with first settings
			buffer[0] = 0x48|REDO_ATI_ALL;
			i2c.write(DEV_SETTINGS, 1, buffer, I2C_Stop);

			// Wait for Redo Ati to complete
			do {
				i2c.read(SYSTEM_FLAGS, 1, &iqs624.SystemFlags.SystemFlags, I2C_Stop);
			} while (iqs624.SystemFlags.InAti);

			first = true;
			algoSM = 0;	// Move to next state

			// reset the max values
			maxCH2 = 0;
			maxCH3 = 0;
			counter = 0;
			binCounter_1 = 0;
			binCounter_2 = 0;
			binCounter_3 = 0;
			binCounter_4 = 0;
		}

		break;
	}
}
//--------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------------
//-------------------------- MODE 6: Calibrate mode ------------------------------------------------
//--------------------------------------------------------------------------------------------------
// Calibrate the wheel for better accuracy
void calibration(void)
{
	static float maxCH2 = 0, maxCH3 = 0, maxCH4 = 0, maxCH5 = 0;
	static float minCH2 = USHRT_MAX, minCH3 = USHRT_MAX, minCH4 = USHRT_MAX, minCH5 = USHRT_MAX;
	static uint16_t ch2Old = 0;
	static int16_t counter = 0;
	static uint8_t binCounter_1 = 0, binCounter_2 = 0, binCounter_3 = 0, binCounter_4 = 0;
	static uint16_t initCH2 = 0, initCH3 = 0, initCH4 = 0, initCH5 = 0;
	float nCH2, nCH3, nCH4, nCH5;
	static float fnCH2 = 1000.0f, fnCH3 = 1000.0f, fnCH4 = 1000.0f, fnCH5 = 1000.0f;
	float thetaA, thetaB, theta;
	static float dP1 = 1.0f, dP2 = 1.0f;
	uint8_t cosPhase = 0, sinPhase = 0;
	static Quads_t quads;

	// Run Calibration state machine
	switch(littleSM)
	{
	// Loop 1 - get min/max
	case 0:

		if(first)
		{
			first = false;
			counter = 0;
			displayState = Display_String;
			Loop = Display_Info;
			sprintf(display_string, "cal1");

			// Start Timeout timer
			setTimer(&CalibTimer,TWELVE_SEC);

			// Get initial channel values
			initCH2 = iqs624.AvgCh[2].Ch;
			initCH3 = iqs624.AvgCh[3].Ch;
			initCH4 = iqs624.AvgCh[4].Ch;
			initCH5 = iqs624.AvgCh[5].Ch;

			ch2Old = initCH2;


			Serial.println(initCH2);
			Serial.println(initCH3);
			Serial.println(initCH4);
			Serial.println(initCH5);

		}

		// Get Max and min values
		if(iqs624.AvgCh[2].Ch <= minCH2)
			minCH2 = (float)iqs624.AvgCh[2].Ch;
		if(iqs624.AvgCh[2].Ch >= maxCH2)
			maxCH2 = (float)iqs624.AvgCh[2].Ch;

		if(iqs624.AvgCh[3].Ch <= minCH3)
			minCH3 = (float)iqs624.AvgCh[3].Ch;
		if(iqs624.AvgCh[3].Ch >= maxCH3)
			maxCH3 = (float)iqs624.AvgCh[3].Ch;

		if(iqs624.AvgCh[4].Ch <= minCH4)
			minCH4 = (float)iqs624.AvgCh[4].Ch;
		if(iqs624.AvgCh[4].Ch >= maxCH4)
			maxCH4 = (float)iqs624.AvgCh[4].Ch;

		if(iqs624.AvgCh[5].Ch <= minCH5)
			minCH5 = (float)iqs624.AvgCh[5].Ch;
		if(iqs624.AvgCh[5].Ch >= maxCH5)
			maxCH5 = (float)iqs624.AvgCh[5].Ch;

		if((minCH2 != USHRT_MAX && minCH3 != USHRT_MAX && minCH4 != USHRT_MAX && minCH5 != USHRT_MAX) &&
				(maxCH2 != 0 && maxCH3 != 0 && maxCH4 != 0 && maxCH5 != 0) && (binCounter_1 > 1 && binCounter_2 > 1 &&
						binCounter_3 > 1 && binCounter_4 > 1)) {
					first = true;
					counter = 0;
					binCounter_1 = 0;
					binCounter_2 = 0;
					binCounter_3 = 0;
					binCounter_4 = 0;

					littleSM++;	// Move to next state
					setTimer(&CalibTimer,TWELVE_SEC);
					break;
		}

		// Obtain Quads
		quads = (Quads_t)iqs624.HallRatioSettings.Quadrature;

//		if(counter < 55 && (abs(long long)((ch2Old*1.0 - iqs624.AvgCh[2].Ch*1.0) > 9))){
		if(abs((long long)(ch2Old*1.0 - iqs624.AvgCh[2].Ch*1.0)) > 8){

			ch2Old = iqs624.AvgCh[2].Ch;
			Serial.println(counter);


			// Check quads
			switch(quads)
			{
				case Quad_1:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_1++;
						Serial.println("Bin Quad 1");
					}

					break;

				case Quad_2:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_2++;
						Serial.println("Bin Quad 2");
					}

					break;

				case Quad_3:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_3++;
						Serial.println("Bin Quad 3");
					}

					break;

				case Quad_4:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_4++;
						Serial.println("Bin Quad 4");
					}

					break;
			}



			setTimer(&CalibTimer,TWELVE_SEC);
		}
		break;

		// Determine angles
	case 1:

		if(first)
		{
			Serial.println("Next 1");
			first = false;
			counter = 0;

			displayState = Display_String;
			Loop = Display_Info;
			sprintf(display_string, "cal2");

			// Reset counter
			setTimer(&CalibTimer,TWELVE_SEC);

			// Get initial channel values
			initCH2 = iqs624.AvgCh[2].Ch;
			initCH3 = iqs624.AvgCh[3].Ch;
			initCH4 = iqs624.AvgCh[4].Ch;
			initCH5 = iqs624.AvgCh[5].Ch;

			ch2Old = initCH2;

			// Calculate denominator saving in channel min
			minCH2 = (maxCH2*1.0/minCH2)-1.0;
			minCH3 = (maxCH3*1.0/minCH3)-1.0;
			minCH4 = (maxCH4*1.0/minCH4)-1.0;
			minCH5 = (maxCH5*1.0/minCH5)-1.0;
		}

		//
		nCH2 = ((maxCH2*1.0/iqs624.AvgCh[2].Ch*1.0)-1.0)/minCH2;
		nCH3 = ((maxCH3*1.0/iqs624.AvgCh[3].Ch*1.0)-1.0)/minCH3;
		nCH4 = ((maxCH4*1.0/iqs624.AvgCh[4].Ch*1.0)-1.0)/minCH4;
		nCH5 = ((maxCH5*1.0/iqs624.AvgCh[5].Ch*1.0)-1.0)/minCH5;

		if(abs((long long)(nCH4-0.5)) < dP1)
		{
			dP1 = abs((long long)(nCH4-0.5));
			fnCH2 = nCH2;
			fnCH4 = nCH4;
			Serial.println(dP1);
		}

		if(abs((long long)(nCH5-0.5)) < dP2)
		{
			dP2 = abs((long long)(nCH5-0.5));
			fnCH3 = nCH3;
			fnCH5 = nCH5;
		}

		if((maxCH2 != 0 && maxCH3 != 0 && maxCH4 != 0 && maxCH5 != 0) && (binCounter_1 > 1 && binCounter_2 > 1 &&
					binCounter_3 > 1 && binCounter_4 > 1)) {

				first = true;
				counter = 0;
				littleSM = 0;	// Move to next state
				setTimer(&CalibTimer,TWELVE_SEC);

				thetaA = asin(2.0*(fnCH4-fnCH2));
				thetaB = asin(2.0*(fnCH5-fnCH3));

				theta = (abs((long long)(thetaA)) + abs((long long)(thetaB)))/2.0;

				cosPhase = (uint8_t)(cos(theta)*256.0);
				sinPhase = (uint8_t)(sin(theta)*256.0);

				sprintf(display_string, "done");

				// Write to IC
				buffer[0] = sinPhase;
				buffer[1] = cosPhase;
				i2c.write(HALL_SIN, 2, buffer, I2C_Stop);

				// Save to EEPROM
				eeprom_write_byte(WRITE_FLAG_ADDRESS, 0x01);
				eeprom_write_byte(COS_PHASE_ADDRESS, cosPhase);
				eeprom_write_byte(SINE_PHASE_ADDRESS, sinPhase);

				// Reset variables
				binCounter_1 = 0;
				binCounter_2 = 0;
				binCounter_3 = 0;
				binCounter_4 = 0;

				dP1 = 1.0f;
				dP2 = 1.0f;
				fnCH2 = 1000.0f;
				fnCH3 = 1000.0f;
				fnCH4 = 1000.0f;
				fnCH5 = 1000.0f;

				maxCH2 = 0;
				maxCH3 = 0;
				maxCH4 = 0;
                maxCH5 = 0;

                minCH2 = USHRT_MAX;
                minCH3 = USHRT_MAX;
                minCH4 = USHRT_MAX;
                minCH5 = USHRT_MAX;

				displayState = Display_String;
				sprintf(display_string, "done");

				// Go to Switch mode state
				Loop = Switch_Mode;

				Mode = Mode_1;

				//disp.write(display_string);
//				// Start the timer
				setTimer(&Mode_Switch_Timer);

				break;
		}

		// Obtain Quads
		quads = (Quads_t)iqs624.HallRatioSettings.Quadrature;

		// Check the delta for movement
//		if(counter < 55 && (abs(long long)((ch2Old*1.0 - iqs624.AvgCh[2].Ch*1.0) > 9))){
		if(abs((long long)((ch2Old*1.0 - iqs624.AvgCh[2].Ch*1.0))) > 9) {
//			counter++;
			ch2Old = iqs624.AvgCh[2].Ch;
			Serial.println(counter);

			// Check quads
			switch(quads)
			{
				case Quad_1:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_1++;
					}

					break;

				case Quad_2:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_2++;
					}

					break;

				case Quad_3:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_3++;
					}

					break;

				case Quad_4:

					if(counter++ >= 2) // counter
					{
						counter = 0;
						binCounter_4++;
					}

					break;
			}

			setTimer(&CalibTimer,TWELVE_SEC);
		}

		break;
	}

	//Check timeout
	if(timerExpired(&CalibTimer))
	{
		Mode = Mode_1;

		sprintf(display_string, "fail");

		eeprom_write_byte(WRITE_FLAG_ADDRESS, 0x00);
		eeprom_write_byte(COS_PHASE_ADDRESS, 0);
		eeprom_write_byte(SINE_PHASE_ADDRESS, 0);

		// Go to Switch mode state
		Loop = Switch_Mode;

		displayState = Display_String;

		//disp.write(display_string);

		// Switch on the appropriate led
		digitalWrite(Leds[(uint8_t)Mode], HIGH);

		// Start the timer
		setTimer(&Mode_Switch_Timer);
	}

}
//--------------------------------------------------------------------------------------------------

/**************************************************************************************************/
/*                                                                                                */
/*                    Setup IQS625                                                                */
/*                                                                                                */
/**************************************************************************************************/
/**
 * @brief Check which setup we need to run for the IQS625 - capacitive & hall rotation
 * @param
 * @retval  None
 */
uint8_t setup_iqs625()
{
  uint8_t res = 0;

  while(!i2c.isDeviceReady());

    //read FG for calibration data
    buffer[0] = 0x13;
    res |= i2c.write(0xF0, 1, buffer, I2C_Stop);
    res |= i2c.read(0xF1, 1, &iqs625.CalibrationBin, I2C_Stop);
    iqs625.CalibrationBin = iqs625.CalibrationBin & 0x0F;
    
    // Fill buffer with first settings

    buffer[0] = PMU_SETTINGS_STARTUP;
    buffer[1] = 0x0E;         // 15ms Comms report rate
    res |= i2c.write(DEV_SETTINGS+2, 2, buffer, I2C_Repeat_Start);

    // Check to see if phase angles were written
    if(eeprom_read_byte(WRITE_FLAG_ADDRESS) == 0x01)
    {
      buffer[0] = eeprom_read_byte(SINE_PHASE_ADDRESS);
      buffer[1] = eeprom_read_byte(COS_PHASE_ADDRESS);
      res |= i2c.write(HALL_SIN, 2, buffer, I2C_Repeat_Start);
    }
    else
    {
      buffer[0] = HALL_SIN_STARTUP;
      res |= i2c.write(HALL_SIN, 1, buffer, I2C_Repeat_Start);

      buffer[0] = HALL_COS_STARTUP;
      res |= i2c.write(HALL_COS, 1, buffer, I2C_Repeat_Start);
    }

    // Write Settings if valid
    if(eeprom_read_byte(ATI_FLAG_ADDRESS) == 0x01)
    {
      iqs625.HallAtiSettings_CH2_CH3.ATISettings = eeprom_read_byte(HALL_ATI_CH2_CH3_ADDRESS);
      iqs625.HallAtiSettings_CH4_CH5.ATISettings = eeprom_read_byte(HALL_ATI_CH4_CH5_ADDRESS);
      res |= i2c.write(0x72, 2, &iqs625.HallAtiSettings_CH2_CH3.ATISettings, I2C_Repeat_Start);
    }

    //write calibration targets
    buffer[0] = LeftTarget[iqs625.CalibrationBin-1]/32 | 0xC0;; 
    buffer[1] = RightTarget[iqs625.CalibrationBin-1]/32 | 0xC0; 
    res |= i2c.write(0x72, 2, buffer, I2C_Repeat_Start);

    buffer[0] = 0x26;
    res |= i2c.write(0x43, 1, buffer, I2C_Repeat_Start);

    buffer[0] = 0x09;
    res |= i2c.write(PXS_CH0_THRES, 1, buffer, I2C_Repeat_Start);
    buffer[0] = PXS_CH1_TCH_THRES;
    res |= i2c.write(PXS_CH1_THRES, 1, buffer, I2C_Repeat_Start);

    //WHEEL FILTER BETA
    buffer[0] = 0x0A;
    res |= i2c.write(0x7B, 1, buffer, I2C_Repeat_Start);
       //zero wheel
    buffer[0] = 0x2C;
    res |= i2c.write(0x70, 1, buffer, I2C_Repeat_Start);

    buffer[0] = 0x0A; // interval divider is 10
    res |= i2c.write(INTERVAL_UI_DIV, 1, buffer, I2C_Repeat_Start);

    buffer[0] = 0x48|REDO_ATI_ALL;
    res |= i2c.write(DEV_SETTINGS, 1, buffer, I2C_Stop);
    
    // Wait for Redo Ati to complete
    do {
      res |= i2c.read(SYSTEM_FLAGS, 1, &iqs625.SystemFlags.SystemFlags, I2C_Stop);
    } while (!res && iqs625.SystemFlags.InAti);
    
  return res;
}
//--------------------------------------------------------------------------------------------------
//--------------------------- IQS625 MODE 1: Interval number mode ----------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (0) - (35) as the interval number on the 7-segment display
void intervalNumMode10(bool *refreshDisplay)
{

  static uint16_t interval_size = 10;

  uint8_t res = 0;                  // Result of the I2C transfer

  // First loop through
  if (getDeg)
  {
    // write the starting interval size to the IQS625
    buffer[0] = interval_size;
    while(!i2c.isDeviceReady());
    do {
      res = i2c.write(INTERVAL_UI_DIV, 1, buffer, I2C_Stop);       // Write the interval size to the interval UI divider
    } while(res);
    
    getDeg = false;
    *refreshDisplay = true;
    display_number = interval_size;
    displayState = Display_String;
    sprintf(display_string, "%3d%c", display_number, '*');
    //disp.write(display_string);
    delay(1000);
    clear_mode_leds();
    displayState = Display_Int;
  }

   deg = iqs625.Interval_number;

  if(iqs625.PXSUIFlags.CH0_T_Out)
    digitalWrite(LED_1, HIGH);
  else
    digitalWrite(LED_1, LOW);

  // Should we refresh the display?
  display_number == deg ? *refreshDisplay = false : *refreshDisplay = true;

  // Set the number to display
  display_number = deg;

  // Save the degrees for later use
  deg_mem = deg;
  
  

}

//--------------------------------------------------------------------------------------------------
//--------------------------- IQS625 MODE 2: Interval number mode ----------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (0) - (11) as the interval number on the 7-segment display
void intervalNumMode30(bool *refreshDisplay)
{

  static uint16_t interval_size = 30;

  uint8_t res = 0;                  // Result of the I2C transfer

  // First loop through
  if (getDeg)
  {
    // write the starting interval size to the IQS625
    buffer[0] = interval_size;
    while(!i2c.isDeviceReady());
    do {
      res = i2c.write(INTERVAL_UI_DIV, 1, buffer, I2C_Stop);       // Write the interval size to the interval UI divider
    } while(res);
    
    getDeg = false;
    *refreshDisplay = true;
    display_number = interval_size;
    displayState = Display_String;
    sprintf(display_string, "%3d%c", display_number, '*');
    //disp.write(display_string);
    delay(1000);
    clear_mode_leds();
    displayState = Display_Int;
  }

   deg = iqs625.Interval_number;

  if(iqs625.PXSUIFlags.CH0_T_Out)
    digitalWrite(LED_1, HIGH);
  else
    digitalWrite(LED_1, LOW);

  // Should we refresh the display?
  display_number == deg ? *refreshDisplay = false : *refreshDisplay = true;

  // Set the number to display
  display_number = deg;

  // Save the degrees for later use
  deg_mem = deg;
  
  

}

//--------------------------------------------------------------------------------------------------
//--------------------------- IQS625 MODE 3: Interval number mode ----------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (0) - (5) as the interval number on the 7-segment display
void intervalNumMode60(bool *refreshDisplay)
{

  static uint16_t interval_size = 60;

  uint8_t res = 0;                  // Result of the I2C transfer

  // First loop through
  if (getDeg)
  {
    // write the starting interval size to the IQS625
    buffer[0] = interval_size;
    while(!i2c.isDeviceReady());
    do {
      res = i2c.write(INTERVAL_UI_DIV, 1, buffer, I2C_Stop);       // Write the interval size to the interval UI divider
    } while(res);
    
    getDeg = false;
    
    display_number = interval_size;
    displayState = Display_String;
    sprintf(display_string, "%3d%c", display_number, '*');
    //disp.write(display_string);
    delay(1000);
    clear_mode_leds();
    *refreshDisplay = true;
    displayState = Display_Int;
  }

   deg = iqs625.Interval_number;

  if(iqs625.PXSUIFlags.CH0_T_Out)
    digitalWrite(LED_1, HIGH);
  else
    digitalWrite(LED_1, LOW);

  // Should we refresh the display?
  display_number == deg ? *refreshDisplay = false : *refreshDisplay = true;

  // Set the number to display
  display_number = deg;

  // Save the degrees for later use
  deg_mem = deg;
  
  

}
//--------------------------------------------------------------------------------------------------
//--------------------------- IQS625 MODE 4: Interval relative mode ----------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (-999) - (999) as the relative number on the 7-segment display
void intervalRelMode(bool *refreshDisplay)
{

  static uint16_t interval_size = 10;
 
  uint8_t res = 0;                  // Result of the I2C transfer


  int16_t diff = 0;

  // First loop through
  if (getDeg)
  {
    // write the starting interval size to the IQS625
    buffer[0] = interval_size;
    while(!i2c.isDeviceReady());
    do {
      res = i2c.write(INTERVAL_UI_DIV, 1, buffer, I2C_Stop);       // Write the interval size to the interval UI divider
    } while(res);
    
    getDeg = false;
    relative_counter = 0;
    *refreshDisplay = true;
    display_number = interval_size;
    displayState = Display_String;
    sprintf(display_string, "%c%c%c%c",'R','E','L',' ');
    //disp.write(display_string);
    delay(1000);
    clear_mode_leds();
    *refreshDisplay = true;
    displayState = Display_Int;
    old_interval = iqs625.Interval_number;
  }
  
    diff =   iqs625.Interval_number - old_interval;//iqs624n.hallAngleDiff;
  old_interval = iqs625.Interval_number;
  if(diff > 22)
  {
    diff = diff-36;
    }
    if(diff <-22)
    {
     diff = 36+diff;
    }

  
  if((diff >0) )
  {

      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      digitalWrite(LED_4, HIGH);
      digitalWrite(LED_3, HIGH);
  }
  else if((diff <0))
  {

      digitalWrite(LED_1, HIGH);
      digitalWrite(LED_2, HIGH);
      digitalWrite(LED_4, LOW);
      digitalWrite(LED_3, LOW);   
  }
    else
  {
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      digitalWrite(LED_4, LOW);
      digitalWrite(LED_3, LOW); 
    
    }
relative_counter = relative_counter - diff; 
  if (relative_counter > 999 || relative_counter < -999){
    relative_counter=0;
  }

  // Should we refresh the display? If the value hasn't changed, we do not need to update display
  display_number == relative_counter ? *refreshDisplay = false : *refreshDisplay = true;

  display_number = relative_counter;
}

//--------------------------------------------------------------------------------------------------
//--------------------------- IQS625 MODE 3: arrow relative mode ----------------------------------
//--------------------------------------------------------------------------------------------------
// Displays (-999) - (999) as the relative number on the 7-segment display
void arrowMode(bool *refreshDisplay)
{
  static uint16_t interval_size_mem = 10;
  static uint16_t interval_size = 10;
  static bool shouldWriteThreshold = false;
  uint8_t res = 0;                  // Result of the I2C transfer
  static bool displayData = true;   // Display the interval output by default

  int16_t diff = 0;

  // First loop through
  if (getDeg)
  {
    // write the starting interval size to the IQS625
    buffer[0] = interval_size;
    while(!i2c.isDeviceReady());
    do {
      res = i2c.write(INTERVAL_UI_DIV, 1, buffer, I2C_Stop);       // Write the interval size to the interval UI divider
    } while(res);
    
    getDeg = false;
    relative_counter = 0;
    *refreshDisplay = true;
    //disp.write(display_number);
    old_interval = iqs625.Interval_number;
  }
  
  if((aux_button == Released) && (iqs625.PXSUIFlags.CH0_T_Out))
    aux_button = Touch;           // a touch occured on the aux button
  else if((aux_button == Touch) && (!iqs625.PXSUIFlags.CH0_T_Out))
    aux_button = Pressed;       // the mode button was now pressed

  // Button was pressed, move to next threshold
  if((aux_button == Pressed) && (!iqs625.PXSUIFlags.CH0_T_Out))
  {
    clear_mode_leds();
    aux_button = Released;        // No more touch on the button, move to next state

    // Now we do our thingy
    // 180 Deg is max for IQS625
    if(interval_size_mem >= 180)
    {
      interval_size = 10;
    }
    // Now we step with 30 at a time up until 120
    else if(interval_size_mem >= 90 && interval_size_mem < 120)
    {
      interval_size = interval_size_mem + 30;
    }
    // Now we step with 60 at a time up until 180
    else if(interval_size_mem >= 120)
    {
      interval_size = interval_size_mem + 60;
    }
    //   For the smaller values, let's step 10
    else
    {
      interval_size = interval_size_mem + 10;
    }

    interval_size_mem = interval_size;

    relative_counter = 0;
    old_interval = iqs625.Interval_number;
  
    // Now write the values
    shouldWriteThreshold = true;
  }

  if(shouldWriteThreshold)
  {
    // Now we need to write the interval size to the IQS625
    buffer[0] = interval_size;
    while(!i2c.isDeviceReady());
    do {
      res = i2c.write(INTERVAL_UI_DIV, 1, buffer, I2C_Stop);       // Write the interval size to the interval UI divider
    } while(res);

    if(!res)
    {
      //disp.write(interval_size);

      shouldWriteThreshold = false;

      displayData = false;
      // Start the timer
      setTimer(&Mode_Switch_Timer);
    }
    else
      {
      //disp.writeError(res);
      }
  }

  // Reset display timeout to default

  // should we display the Threshold value or the data?
  if(timerExpired(&Mode_Switch_Timer))
    displayData = true;

  // Update display
  if(displayData)
  {
    displayState = Display_Int;
    
    diff = iqs625.Interval_number - old_interval;

    if(diff == 0)
    {
      //do nothing
    }
    else if((diff > 0 && diff < ((360/interval_size)-1) && (old_interval != 0))
          || diff == (-1*((360/interval_size)-1))) //increment || roll over from max to 0
    {
        relative_counter++;
        digitalWrite(LED_1, LOW);
        digitalWrite(LED_2, LOW);
        digitalWrite(LED_4, HIGH);
        digitalWrite(LED_3, HIGH);

        displayState = Display_String;
        sprintf(display_string, "%c%c%c%c",'<','-','-','-');
    }
    else if((diff < 0 && diff > (-1*(360/interval_size)-1) && (old_interval != ((360/interval_size)-1)))
          || diff == (1*((360/interval_size)-1))) //decrement || roll over from 0 to max
    {
        relative_counter--;
        digitalWrite(LED_1, HIGH);
        digitalWrite(LED_2, HIGH);
        digitalWrite(LED_4, LOW);
        digitalWrite(LED_3, LOW);

        displayState = Display_String;
        sprintf(display_string, "%c%c%c%c",'-','-','-','>');
    }
    old_interval = old_interval + diff;

    if (relative_counter > 999 || relative_counter < -999)
    {
      relative_counter = 0;
    }
  
    // Should we refresh the display? If the value hasn't changed, we do not need to update display
    display_number == relative_counter ? *refreshDisplay = false : *refreshDisplay = true;

    display_number = relative_counter;  
  }
  else //Display the interval size currently set
  {
    display_number = interval_size;
    displayState = Display_String;
    sprintf(display_string, "%3d%c", display_number, '*');
    clear_mode_leds();
  }
}

/**************************************************************************************************/
/*																								                                                */
/*										Timer Functions											                                        */
/*																								                                                */
/**************************************************************************************************/

/**
 * @brief
 */
void setTimer(Timer_t* timer)
{
	timer->TimerExpired = false;
	timer->Timer_start = millis();	// get this instant millis
}

/**
 * @brief
 */
void setTimer(Timer_t* timer, uint32_t time)
{
	timer->TimerExpired = false;
	timer->Timer_start = millis();	// get this instant millis
	timer->Timer_counter = time;	// the timeout time for the timer
}

bool timerExpired(Timer_t* timer)
{
	// This is a timeout
	if(((millis() - timer->Timer_start) >= timer->Timer_counter))
		timer->TimerExpired = true;
	// We haven't timed out yet
	else
		timer->TimerExpired  = false;

	// Return the state of this timer
	return timer->TimerExpired;
}

/**
 * @brief	Clear the Mode leds before the next mode can be set
 * @param	None
 * @retval	None
 */
void clear_mode_leds()
{
	// Clear all mode leds
	for(int i = 0; i < NR_OF_MODES; i++)
		digitalWrite(Leds[i], LOW);
}

/**
 * @brief	Initialize the Mode leds as outputs and switch off
 * @param	None
 * @retval	None
 */
void init_mode_leds()
{
	// Clear all mode leds
	for(int i = 0; i < NR_OF_MODES; i++) {
		pinMode(Leds[i], OUTPUT);
		digitalWrite(Leds[i], LOW);
	}
}
