#include "Arduino.h"
#include "IQS62x.h"
#include "IQS620_Init.h"
#include "Types.h"
#include "limits.h"
#include <stm32_eeprom.h>
#include <Wire.h>

/*  Global defines  -----------------------------------------------------------*/

#define	MS_500      500
#define	ONE_SEC     1000
#define	TWO_SEC     2000
#define THREE_SEC   3000
#define	TWELVE_SEC  12000
#define	MS_5        5

#define FERRITE
// fun

// Debug with serial comms
 #define DEBUG

// Define if absolute temperature, or delta temp
#define	ABS_TEMP

// Beta
#define BETA        1.0

// Create constants to determine the ATI Settings
#define COUNT_VALUE	                    1200
#define BASE_THRESHOLD                  500
#define STOP_THRESHOLD                  100

// Define Addresses for EEPROM
#define WRITE_FLAG_ADDRESS              0x00
#define COS_PHASE_ADDRESS               0x01
#define SINE_PHASE_ADDRESS              0x02
#define ATI_FLAG_ADDRESS                0x03
#define HALL_ATI_CH2_CH3_ADDRESS        0x04
#define HALL_ATI_CH4_CH5_ADDRESS        0x05

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
    Double_Tap,
    Btn_Timeout,
    Btn_LongPress
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
    IQS620n
    } IC_Type_e;

typedef enum Quads
    {
    Quad_1 = 0,
    Quad_2 = 1,
    Quad_4 = 2,
    Quad_3 = 3
    }Quads_t;

/** Enum for RDY active low and active high or Polling */
typedef enum RDY_Type
    {
    Active_Low = 0,
    Active_High = 1,
    Polling = 2
    }RDY_Type_e;

/** Enum for Comms size - 8bit or 16bit register */
typedef enum
    {
    Size_8_bit = 0,
    Size_16_bit = 1
    }Comms_Size_e;

/*  Global Variables    ---------------------------------------------------------*/

// The Mode state of the demo
Mode_e Mode;

// What type of IC is this?
IC_Type_e ICType;

// Timer 1
Timer_t Mode_Switch_Timer   = {0};          // Mode switch timer
Timer_t ErrorTimer          = {0};          // Error Timer
Timer_t MainTimer           = {0};          // Error Timer
Timer_t ButtonTimer         = {0};          // Button double tap Timer
Timer_t CalibTimer          = {0};          // Calibration Timeout Timer
Timer_t CoilTimer           = {0};          // Coil saturation timer

// Which state of the loop are we in?
Loop_state_e Loop = Run;

// Mode button
Button_state_e mode_button = Released;

// Aux button
Button_state_e aux_button = Released;

RDY_Type_e _RDY_Type;
volatile bool _RDY_Window;
bool _Timeout;
uint32_t _commsSpeed;
Comms_Size_e _commsSize;

//ProxFusion IC's
IQS620n_t iqs620n;              // Create variable for iqs620A

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

// Create a mode button
bool modeButton = false;

// Flag to indicate whether we should do a setup of inductive sensor
bool setupInductiveSensor = true;

bool coilCheck = true;

bool first = true; // First calibration entry
uint8_t algoSM = 0;
uint8_t littleSM = 0;
bool modeEntry = true;
/**
 * @brief	Check whether this device is RDY. We see if the interrupt set it to ready or if the RDY is active
 * @TODO	Still need to be able to feed in the user's own callback
 * @param	None
 * @retval	[bool] is this device RDY (true) or not (false)
 */
bool isDeviceReady(void)
    {
    bool tempWindow = false;

    if(IQS62x_RDY == D12 || IQS62x_RDY == D6)
        {
        tempWindow = (_RDY_Window || (!(bool)(digitalRead(IQS62x_RDY) ^ (uint8_t)_RDY_Type))); // || this->_RDY_Type == Polling);
        _RDY_Window = false;
        }
    else if(IQS62x_RDY != NO_RDY)
        tempWindow = ((!(bool)(digitalRead(IQS62x_RDY) ^ (uint8_t)_RDY_Type))); //|| this->_RDY_Type == Polling);

    return tempWindow;
    }

// Global to indicate that calibration was done.

bool writeRegister(uint16_t command, uint8_t* pData)
    {
    Wire.beginTransmission((uint8_t) I2C_ADDRESS);
    Wire.write(command);

    // No send the number of bytes required to write
    for(uint8_t i = 0; (i < sizeof(pData)); i++)
        {
        // Send each required byte
        Wire.write(pData[i]);
        }

    if (Wire.endTransmission() != 0)
        {
        return false;
        }

    return true;
    }

bool readRegisters(uint16_t command, std::uint8_t *pBuffer, size_t nBuffer)
    {
    if (pBuffer == nullptr || nBuffer > 32)
        {
        // Serial.println("Line : 297");
        return false;
        }

    Wire.beginTransmission((uint8_t) I2C_ADDRESS);
    if (Wire.write((uint8_t)command) != 1)
        {
        // Serial.println("Line : 304");
        return false;
        }
    if (Wire.endTransmission() != 0)
        {
        // Serial.println("Line : 309");
        return false;
        }

    auto nReadFrom = Wire.requestFrom((uint8_t) I2C_ADDRESS, std::uint8_t(nBuffer));

    if (nReadFrom != nBuffer)
        {
        // Serial.println("Line : 316");
        return false;
        }

    auto const nResult = unsigned(Wire.available());

    if (nResult > nBuffer)
        {
        // Serial.println("Line : 324");
        return false;
        }

    for (unsigned i = 0; i < nResult; ++i)
        {
        // Serial.println("Line : 330");
        pBuffer[i] = Wire.read();
        // Serial.print("pBuffer = ");
        // Serial.println(pBuffer[i]);
        }

    if (nResult != nBuffer)
        {
        // Serial.println("Line : 336");
        return false;
        }

    // Serial.println("Line : 340");
    return true;
    }

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

    _RDY_Type = Polling;
    _RDY_Window = false;

    Serial.println ("**** This is an Example for IQS620AEV1 ****");

    // Get the Version info
    readRegisters(VERSION_INFO, buffer, sizeof(buffer));

    // Set the appropriate IC
    if(buffer[0] == IQS620_PRODUCT_NR && buffer[1] == IQS620N_SOFTWARE_NR && buffer[2] == IQS620N_HARDWARE_NR)
        {
        ICType = IQS620n;
        }
    // No valid IC type found
    else
        {
        //disp.writeError(2);
        Serial.println("Err invalid IC...");
        while(1);
        }
    // Serial.println(ICType);
    // Do initial setup
    iqs_setup();

    //disp.write(display_number = (((uint8_t)Mode%4)+1)*1111);    // Write mode to display
    //Serial.print("Mode : ");
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
    //Add your repeated code here
    bool refreshDisplay = false;
    uint8_t res = 0;

    if(ICType == IQS620n)
        {
        // Read version number to insure we still have the correct device attached - otherwise, do setup
        res = readRegisters(VERSION_INFO, buffer, sizeof(buffer));

        // System flags, Global Events and PXS UI Flags - 9 bytes
        res |= readRegisters(SYSTEM_FLAGS, &iqs620n.SystemFlags.SystemFlags, sizeof(&iqs620n.SystemFlags.SystemFlags));

        // Read PXS Channel Data - 12 bytes
        res |= readRegisters(CHANNEL_DATA, &iqs620n.Ch[0].Ch_Low, sizeof(&iqs620n.Ch[0].Ch_Low));

        // Read LTA value of Channel 1 for Movement mode
        res |= readRegisters(LTA+2, &iqs620n.LTA1.Ch_Low, sizeof(&iqs620n.LTA1.Ch_Low));
        }

    // A read error occurred
    if(res)
        {
        // Serial.print("res : ");
        // Serial.println(res);
        }

    // Now we write to display
    refreshDisplay = true;

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
            // Always reset the getTemperatureReference flag
            getTempReference = true;

            // Get the reference degrees again
            getDeg = true;

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

        //disp.write((((uint8_t)Mode%4)+1)*1111);    // Write mode to display);

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

    // Wait for IC to become ready - a timeout should exit this
    // while(!isDeviceReady());

    if (ICType == IQS620n)
        {
        Serial.println ("620n Found!");
        delay(1000); //Wait here for device splash on disp
        // setup device
        res = setup_iqs620n();
        }

    // An error occured
    if(res)
        {
        Serial.print("res: ");
        Serial.println(res);
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

    // while(!isDeviceReady());

    res |= writeRegister(DEV_SETTINGS, (uint8_t *)nDevSetup);

    res |= writeRegister(PXS_SETTINGS_0, (uint8_t *)nPXS_Setup_0);

    res |= writeRegister(PXS_SETTINGS_1, (uint8_t *)nPXS_Setup_1);

    res |= writeRegister(PXS_UI_SETTINGS, (uint8_t *)nPXSUi);

    res |= writeRegister(SAR_UI_SETTINGS, (uint8_t *)nSARUi);

    res |= writeRegister(METAL_UI_SETTINGS, (uint8_t *)nMetalDetect);

    res |= writeRegister(HALL_SENS_SETTINGS, (uint8_t *)nHall_Sens);

    res |= writeRegister(HALL_UI_SETTINGS, (uint8_t *)nHall_UI);

    res |= writeRegister(TEMP_UI_SETTINGS, (uint8_t *)nTemp_UI);

    // Wait for Redo Ati to complete
    do
        {
        // Wait for device to become ready
        // while(!isDeviceReady());

        res |= readRegisters(SYSTEM_FLAGS, &iqs620n.SystemFlags.SystemFlags, sizeof(&iqs620n.SystemFlags.SystemFlags));
        
        Serial.print("res : ");
        Serial.println(res);
        Serial.print("InAti : ");
        Serial.println(iqs620n.SystemFlags.InAti);
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
	Serial.print("SAR counts:");
    Serial.print("\t");
    Serial.println(display_number);
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