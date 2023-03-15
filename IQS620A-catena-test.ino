#include "Arduino.h"
#include "IQS62x.h"
#include "Types.h"
#include "limits.h"
#include "MCCI_I2C.h"
#include <stm32_eeprom.h>
#include <Wire.h>

/*  Global defines  -----------------------------------------------------------*/

#define	MS_500      500
#define	ONE_SEC     1000
#define	TWO_SEC     2000
#define THREE_SEC   3000
#define	TWELVE_SEC  12000
#define	MS_5        5

/*  Typedefs        ---------------------- ----------------------------------------*/

// Enum to move between modes for this arduino demo
typedef enum Modes
    {
	Mode_1 = 0				// SAR Mode
    } Mode_e;

// Enum to show button presses
typedef enum Button_state
    {
    Touch,
    Pressed,
    Released,
    Btn_Timeout
    } Button_state_e;

// Enum to show button presses
typedef enum Loop_state
    {
    Run,
    Switch_Mode
    } Loop_state_e;

// Enum to determine what to show on screen
typedef enum IC_Type
    {
    IQS620n = 1
    } IC_Type_e;

/** Enum for RDY active low and active high or Polling */
typedef enum RDY_Type
    {
    Active_Low = 0,
    Active_High = 1,
    Polling = 2
    }RDY_Type_e;

/*  Global Variables    ---------------------------------------------------------*/

// The Mode state of the demo
Mode_e Mode;

// What type of IC is this?
IC_Type_e ICType;

cI2C gI2C;

// Timer 1
Timer_t Mode_Switch_Timer   = {0};          // Mode switch timer
Timer_t ErrorTimer          = {0};          // Error Timer
Timer_t MainTimer           = {0};          // Error Timer
Timer_t ButtonTimer         = {0};          // Button double tap Timer

// Which state of the loop are we in?
Loop_state_e Loop = Run;

// Mode button
Button_state_e mode_button = Released;

// Aux button
Button_state_e aux_button = Released;

RDY_Type_e _RDY_Type;
volatile bool _RDY_Window;

//ProxFusion IC's
IQS620n_t iqs620n;              // Create variable for iqs620A

// A number to display
int16_t printNumber = 0;

// Indicate chip is ready for polling
bool chipReady = false;

// Buffer to read data into
uint8_t buffer[20];

// Create a mode button
bool modeButton = false;

//The setup function is called once at startup of the sketch
void setup()
    {
    Serial.begin(115200);

    Wire.begin();
    while(!Serial);

    // Setup the RDY pin
    pinMode(IQS62x_RDY, OUTPUT);
    digitalWrite(IQS62x_RDY, HIGH);
    pinMode(IQS62x_RDY, INPUT);
    digitalWrite(IQS62x_RDY, HIGH);

    delay(100);

    _RDY_Type = Polling;
    _RDY_Window = false;

    Serial.println ("#### This is an Example for IQS620AEV1 ####");

    // Get the Version info

    uint8_t res = 0;
    res = setup_iqs620n();
    gI2C.readRegisters(VERSION_INFO, buffer, sizeof(buffer));

    // Set the appropriate IC
    if(buffer[0] == IQS620_PRODUCT_NR && buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
        {
        ICType = IQS620n;
        }
    // No valid IC type found
    else
        {
        Serial.println("Err invalid IC! Check wiring...");
        while(1);
        }

    // Do initial setup
    iqs_setup();

    delay(1000);

    // Initialise Mode timer
    Mode_Switch_Timer.Timer_counter = ONE_SEC;  // 1s timer

    ErrorTimer.Timer_counter = THREE_SEC;       // 3s timer

    MainTimer.Timer_counter = ONE_SEC;          // 1s timer

    ButtonTimer.Timer_counter = 300;            // 300ms timer
    }

// The loop function is called in an endless loop
void loop()
    {
    uint8_t res = 0;

    if(ICType == IQS620n)
        {
        // Read version number to insure we still have the correct device attached - otherwise, do setup
        res = gI2C.readRegisters(VERSION_INFO, buffer, sizeof(buffer));

        // System flags, Global Events and PXS UI Flags - 9 bytes
        res |= gI2C.readRegisters(SYSTEM_FLAGS, &iqs620n.SystemFlags.SystemFlags, sizeof(&iqs620n.SystemFlags.SystemFlags));

        // Read PXS Channel Data - 12 bytes
        res |= gI2C.readRegisters(CHANNEL_DATA, &iqs620n.Ch[0].Ch_Low, sizeof(&iqs620n.Ch[0].Ch_Low));

        // Read LTA value of Channel 1 for Movement mode
        res |= gI2C.readRegisters(LTA+2, &iqs620n.LTA1.Ch_Low, sizeof(&iqs620n.LTA1.Ch_Low));
        }

    // A read error occurred
    if(res)
        {
        // Serial.print("res : ");
        // Serial.println(res);
        }

    // reset timer
    setTimer(&ErrorTimer);

    chipReady = true;

    if(timerExpired(&ErrorTimer))
        {
        //Serial.print("Timer Expired : ");
        //Serial.println(ERR_TIMEOUT);
        }

    // Check for mode switching
    check_mode_button();

    // handle the loop state
    switch(Loop)
        {
        case Run:
            // Did we receive data from the IC? If we did, process that data
            if(chipReady)
                {
                if(ICType == IQS620n)
                    process_IQS620n_events();

                // We are done processing this comms window
                chipReady = false;
                }
            break;

        case Switch_Mode:
            // Display the current mode on the led for a certain time

            // We change states if timeout occured
            if(timerExpired(&Mode_Switch_Timer))
                {
                Loop = Run;	// go to run loop
                }

            break;
        default:;
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

        default:
            // error
            break;
        }
    }

/****************************************************************/
/*                                                              */
/*                                  Mode Helper Functions       */
/*                                                              */
/****************************************************************/

/**
 * @brief	Check the mode switch button
 * @param	None
 * @retval	None
 */
void check_mode_button()
    {
    // Which button is the mode switch button?
    if(ICType == IQS620n)
        modeButton = (bool)iqs620n.PXSUIFlags.CH2_T_Out;
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
    // Button was pressed, move to next state
    else if((mode_button == Pressed) && (!modeButton))
        {
        mode_button = Released;	// No more touch on the button, move to next state

        // Next Mode
        Mode = Mode_1;

        // Go to Switch mode state
        Loop = Switch_Mode;

        // Start the timer
        setTimer(&Mode_Switch_Timer);
        }
    else if(timerExpired(&ButtonTimer) && mode_button != Released)
        {
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

    setTimer(&MainTimer);

    if (ICType == IQS620n)
        {
        Serial.println ("620n Found!");
        delay(1000); //Wait here for device splash on serial
        // setup device
        res = setup_iqs620n();
        }

    // An error occured
    if(res)
        {
        // Serial.print("res: ");
        // Serial.println(res);
        }
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

    res |= gI2C.writeRegister(DEV_SETTINGS, (uint8_t *)nDevSetup);

    res |= gI2C.writeRegister(PXS_SETTINGS_0, (uint8_t *)nPXS_Setup_0);

    res |= gI2C.writeRegister(PXS_SETTINGS_1, (uint8_t *)nPXS_Setup_1);

    res |= gI2C.writeRegister(PXS_UI_SETTINGS, (uint8_t *)nPXSUi);

    res |= gI2C.writeRegister(SAR_UI_SETTINGS, (uint8_t *)nSARUi);

    res |= gI2C.writeRegister(METAL_UI_SETTINGS, (uint8_t *)nMetalDetect);

    res |= gI2C.writeRegister(HALL_SENS_SETTINGS, (uint8_t *)nHall_Sens);

    res |= gI2C.writeRegister(HALL_UI_SETTINGS, (uint8_t *)nHall_UI);

    res |= gI2C.writeRegister(TEMP_UI_SETTINGS, (uint8_t *)nTemp_UI);

    // Wait for Redo Ati to complete
    do
        {
        res |= gI2C.readRegisters(SYSTEM_FLAGS, &iqs620n.SystemFlags.SystemFlags, sizeof(&iqs620n.SystemFlags.SystemFlags));
        }
    while (!res && iqs620n.SystemFlags.InAti);

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
    printNumber = iqs620n.Ch[0].Ch;  // Display Channel Data
    Serial.print("SAR counts:");
    Serial.print("\t");
    Serial.println(printNumber);
    }

/**************************************************************************************************/
/*                                                                                                */
/*                                     Timer Functions                                            */
/*                                                                                                */
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
