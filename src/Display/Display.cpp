/******************************************************************************
 *                                                                             *
 *                                                                             *
 *                           	Copyright by                                  *
 *                                                                             *
 *                         	  Azoteq (Pty) Ltd                                *
 *                     	  Republic of South Africa                            *
 *                                                                             *
 *                     	   Tel: +27(0)21 863 0033                             *
 *                    	  E-mail: info@azoteq.com                             *
 *                                                                             *
 *=============================================================================*
 * @file 	    Display.cpp									 				  *
 * @brief 	    Arduino Display library for Azoteq Devices with 7 Segment	  *
 * 				displays								            		  *
 * @author 		AJ van der Merwe - Azoteq PTY Ltd                             *
 * @credits		Iwan de Klerk for the basic working of the 7 Segment		  *
 * 				Adapted from Arduino DirectDrive8888 library				  *
 * @version 		V1.0.0                                                        *
 * @date 	    21/07/2016                                                    *
 *******************************************************************************/

#if(ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include <inttypes.h>
#include "Display.h"
#include "../TimerOne/TimerOne.h"
#include "string.h"

// Private defines
/**************************************************************************************************/
/*																								  */
/*						 	IO Definition for 7-segment display 								  */
/*																								  */
/**************************************************************************************************/
#define DIG3_SEL  3
#define DIG2_SEL  4
#define DIG1_SEL  5
#define DIG0_SEL  6
#define DEC_SEL   1
#define LED_A     13
#define LED_B     12
#define LED_C     11
#define LED_D     10
#define LED_E     9
#define LED_F     7
#define LED_G     8

typedef struct Digit {
	byte digit_pin;			// To which pin is this attached
	bool display_digit;		// Display this digit or not?
} Digit_t;

//With this pin setup you can connect pin 1 through 12 on the display to pins 2-13 on Arduino
byte pins[12] = {DEC_SEL, DIG3_SEL, DIG2_SEL, DIG1_SEL, DIG0_SEL, LED_F, LED_G,
		LED_E, LED_D, LED_C, LED_B, LED_A};

byte rows[4] = {DIG0_SEL, DIG1_SEL, DIG2_SEL, DIG3_SEL};
byte cols[8] = {LED_A, LED_B, LED_C, LED_D, LED_E, LED_F, LED_G, DEC_SEL};

byte screen[8] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile byte row = 0;
volatile byte col = 0;

// Create the digits
Digit_t digits[4] = {0};

// Frame buffer total Render Timer
static Timer_t frameTimer = {0};				// Render Timer

uint8_t displayBuffer[4] = {0};	// Display Buffer to render to screen

bool noData = true;

byte numbers[30][7] = {
		// A B C D E F G
		{0,0,0,0,0,0,1},  // zero
		{1,0,0,1,1,1,1},  // one
		{0,0,1,0,0,1,0},  // two
		{0,0,0,0,1,1,0},  // three
		{1,0,0,1,1,0,0},  // four
		{0,1,0,0,1,0,0},  // five
		{0,1,0,0,0,0,0},  // six
		{0,0,0,1,1,1,1},  // seven
		{0,0,0,0,0,0,0},  // eight
		{0,0,0,0,1,0,0},  // nine
		{1,1,1,1,1,1,1},  // off
		{1,1,1,1,1,1,0},  // minus
		{1,1,1,1,1,1,0},  // undefined - 12
		{1,0,0,0,0,1,0},  // dark
		{1,1,1,0,0,0,1},  // light
		{1,1,0,1,0,1,0},  // north
		{0,1,0,0,1,0,0},  // south
		{0,1,1,0,0,0,0},  // E
		{1,1,1,1,0,1,0},  // r
		{1,1,1,0,0,0,0},  // t
		{1,0,0,1,0,0,0},  // H
		{1,1,1,0,0,0,1},  // L
		{0,0,1,1,0,0,0},  // P
		{0,1,1,0,0,0,1},  // C
		{0,0,0,0,0,0,1},  // o
		{1,1,1,1,0,0,1},  // i
		{1,0,0,1,1,1,0},  // '-1'
		{0,0,1,1,1,0,0},  // degree symbol
		{1,1,1,1,0,0,0},  // right arrowhead
		{1,0,0,1,1,1,0}   // left arrow head
};

// Set this instance of the Object
Display *instance = NULL;

/**
 * @brief	Empty Constructor for Display
 * @param	None
 * @retval	None
 */
Display::Display()
{

	// Set the appropriate instance variables
	this->_renderTime = 1000;
	this->_clearDisplay = false;
	this->_clearTime = 1000;		// 1 sec clear time

	// Init the display timer
	Timer1.initialize(this->_renderTime);

	for (int i = 0; i < 12; i++)
		pinMode(pins[i], OUTPUT);

	// Set the instance
	if(instance == NULL)
		instance = this;

//	Serial.begin(9600);

	// Setup the digits
	for(int i = 0; i < 4; i++)
	{
		digits[i].digit_pin = rows[i];
		digits[i].display_digit = false;
	}

	frameTimer.TimerExpired = false;
	frameTimer.Timer_counter = this->_clearTime;	// 1 ms timer

	// Start Render Loop
	this->setDisplayTimer(&frameTimer);

	// Attach the Screen Refresh timer
	Timer1.attachInterrupt(Display::render);
}

/**
 * @brief	Parameterized Constructor for Display - this allows the setup of a timer for
 * 			only displaying the screen briefly
 * @param	[bool]		should we clear the display after writing?
 * @param	[uint32_t]	the time before we clear the screen
 * @param	[uint32_t]	render time of the screen - default is 500us
 */
Display::Display(bool clearDisplay, uint32_t clearTime, uint32_t renderTime)
{

	// Set the appropriate instance variables
	this->_renderTime = renderTime;
	this->_clearDisplay = clearDisplay;
	this->_clearTime = clearTime;

	// Init the display timer
	Timer1.initialize(this->_renderTime);

	for (int i = 0; i < 12; i++)
		pinMode(pins[i], OUTPUT);

	// Set the instance
	if(instance == NULL)
		instance = this;

//	Serial.begin(9600);

	// Setup the digits
	for(int i = 0; i < 4; i++)
	{
		digits[i].digit_pin = rows[i];
		digits[i].display_digit = false;
	}

	frameTimer.TimerExpired = false;
	frameTimer.Timer_counter = this->_clearTime;	//  ms timer

	// Start Render Loop
	this->setDisplayTimer(&frameTimer);

	// Attach the Screen Refresh timer
	Timer1.attachInterrupt(Display::render);
}


/**
 * @brief	Write the desired number to the LCD. This can be signed or unsigned
 * @TODO	Little less
 * @param	[int16_t]	The signed number to write
 * @retval	None
 */
void Display::write(int16_t data)
{
	uint16_t div = 1000;

	row = 0;	// Reset display
//	Serial.println(data);

	// We handle overflow with this simple way
	if(data < -999 || data > 9999)
		data = 0;

	// format display data
	for(int i = 0; i < 4; i++)
	{
		if(data < 0 && !i)
		{
			displayBuffer[i] = MINUS;
			data = -data;		// Make data positive again for correct display

//			Serial.println(displayBuffer[i]);
		}
		else
			displayBuffer[i] = (data/(div))%10;

		div /= 10;

//		Serial.println(displayBuffer[i]);
//		displayBuffer[i];
//		delayMicroseconds(1000);
//		Serial.flush();
	}

	for(int i = 0; i < 4; i++)
	{
		// Let's create an if hell
		if(i == 0 && !displayBuffer[i]) {
			digits[i].display_digit = false; // do not display top value
		}

		else if((i == 1) && !displayBuffer[i])
		{
			if(!displayBuffer[i-1] || (displayBuffer[i-1] == MINUS))
				digits[i].display_digit = false; // do not display top value
			else
				digits[i].display_digit = true; 	// display the values
		}

		else if((i == 2) && !displayBuffer[i])
		{
			if(!displayBuffer[i-1] && (!displayBuffer[i-2] || displayBuffer[i-2] == MINUS))
				digits[i].display_digit = false; // do not display this value
			else
				digits[i].display_digit = true; 	// display the values

		}
		else
			digits[i].display_digit = true; 	// display the values
	}


	this->setDisplayTimer(&frameTimer);

	// There is data to display
	noData = false;

}


/**
 * @brief	Write a string to the LCD. Only if the characters exist
 * @TODO	A lot
 * @param	[String]	the string to write
 * @retval	None
 */
void Display::write(String data)
{
	uint8_t length = 4;		// Default length
//	Serial.println(sizeof(data));
	uint8_t intval = 0;


	// format display data
	for(int i = 0; i < length; i++)
	{

		if(data[i] == ' ')
			data[i] = LED_OFF;
		else if(data[i] == 'D' || data[i] == 'd')
			data[i] = DARK;
		else if(data[i] == 'L' || data[i] == 'l')
			data[i] = LIGHT;
		else if(data[i] == 'N' || data[i] == 'n')
			data[i] = NORTH;
		else if(data[i] == 'S' || data[i] == 's')
			data[i] = SOUTH;
		else if(data[i] == 'T' || data[i] == 't')
			data[i] = T;
		else if(data[i] == 'H' || data[i] == 'h')
			data[i] = H;
		else if(data[i] == 'E' || data[i] == 'e')
			data[i] = E;
		else if(data[i] == 'R' || data[i] == 'r')
			data[i] = R;
		else if(data[i] == 'L' || data[i] == 'l')
			data[i] = L;
		else if(data[i] == 'P' || data[i] == 'p')
			data[i] = P;
		else if(data[i] == 'C' || data[i] == 'c')
			data[i] = C;
		else if(data[i] == 'O' || data[i] == 'o')
			data[i] = O;
		else if(data[i] == 'I' || data[i] == 'i')
			data[i] = I;
		else if(data[i] == '~')	// weird syntax for -1
			data[i] = MINUS_1;
		else if(data[i] == '*')
			data[i] = DEG;
		else if(data[i] == '>')
			data[i] = R_ARROW;
		else if(data[i] == '<')
			data[i] = L_ARROW;
		else if(data[i] == '-')
			data[i] = UNDEFINED;

		// Display the character
		displayBuffer[i] = data[i];

		// Is this maybe an int value
		intval = ((uint8_t)data[i] - 48);

		// Subtract the ASCII value from the char value to get the index
		if(intval >= 0 && intval < 10)
			displayBuffer[i] = intval;



//		Serial.println(displayBuffer[i]);

		digits[i].display_digit = true; 	// display the values
	}


	this->setDisplayTimer(&frameTimer);

	// There is data to display
	noData = false;
}


/**
 * @brief	Write an error to the screen
 * @TODO	A lot
 * @param	[uint16_t]	error to write to screen
 * @retval	None
 */
void Display::writeError(uint16_t errorCode)
{

//	Serial.println(errorCode);


	// format display data
	for(int i = 0; i < 4; i++)
	{
		if(!i)
			displayBuffer[i] = E;
		else if(i && i != 3)
			displayBuffer[i] = R;
		else
			displayBuffer[i] = errorCode;

//		Serial.println(displayBuffer[i]);

		digits[i].display_digit = true; 	// display the values
	}


	this->setDisplayTimer(&frameTimer);

	// There is data to display
	noData = false;
}


//////////////////////////////// 	Private Methods		///////////////////////////////


// interrupt routine
/**
 * @brief	This is the render loop that is continuously called from the timer
 * @param	None
 * @retval	None
 */
void Display::render() {


	// If we should clear the display, do it now
	if(noData && instance->_clearDisplay)
	{
		instance->clearScreen();
		return;
	}

	// switch other off
	for(int i = 0; i < 4; i++)
	{
		if((i != row) || !digits[i].display_digit) digitalWrite(digits[i].digit_pin, HIGH);
	}

	// Write the appropriate digits
	for(int i = 0; i < 7; i++)
	{
		digitalWrite(cols[i], numbers[displayBuffer[row]][i]);
	}

	if(digits[row].display_digit)
		digitalWrite(rows[row], LOW); // switch on appropriate Digit

	if(++row >= 4) row = 0;

	// Start Render Loo
	if(instance->displayTimerExpired(&frameTimer) && instance->_clearDisplay)
	{
		// There is no more data to display
		noData = true;

		// Clear the screen
		instance->clearScreen();
	}


}

/**
 * @brief	Clear the screen. Thus switch off all leds
 * @param	None
 * @retval	None
 */
void Display::clearScreen()
{
	for(int j = 0; j < 4; j++)
	{
		for(int i = 0; i < 7; i++)
		{
			digitalWrite(cols[i], HIGH);
		}

		digitalWrite(rows[j], HIGH);

		displayBuffer[j] = 0;
	}
}

//////////////////////////////// 	Setters		///////////////////////////////
// Only implement the necessary setters ATM

/**
 * @brief	Should we clear the display after writing to it
 * @param	[bool] set whether we should clear the display or not
 * @retval	None
 */
void Display::setClearDisplay(bool clearDisplay)
{
	this->_clearDisplay = clearDisplay;
}


/**
 * @brief	Set the time before we clear the screen
 * @param	[uint32_t] the time before we clear the screen
 * @retval	None
 */
void Display::setClearTime(uint32_t clearTime)
{
	this->_clearTime = clearTime;
}


/**
 * @brief	Set the render time for the screen
 * @param	[uint32_t] render time for the screen
 * @retval	None
 */
void Display::setRenderTime(uint32_t renderTime)
{
	this->_renderTime = renderTime;
}



/**************************************************************************************************/
/*																								  */
/*										Timer Functions											  */
/*																								  */
/**************************************************************************************************/

/**
 * @brief	Set a timer the required milliseconds
 * @param	[Timer_t*]	pointer to timer instance
 * @retval	None
 */
void Display::setDisplayTimer(Timer_t* timer)
{
	timer->TimerExpired = false;
	timer->Timer_start = millis();	// get this instant millis
}

/**
 * @brief	Set a timer the required milliseconds
 * @param	[Timer_t*]	pointer to timer instance
 * @param	[uint32_t]	time for the timer
 * @retval	None
 */
void Display::setDisplayTimer(Timer_t* timer, uint32_t time)
{
	timer->TimerExpired = false;
	timer->Timer_start = millis();	// get this instant millis
	timer->Timer_counter = time;	// the timeout time for the timer
}

/**
 * @brief	Check whether the specified timer has expired
 * @param	[Timer_t*]	pointer to timer instance
 * @retval	[bool]		did timer expire?
 */
bool Display::displayTimerExpired(Timer_t* timer)
{
	// This is a timeout
	if((millis() - timer->Timer_start) >= timer->Timer_counter)
		timer->TimerExpired = true;
	// We haven't timed out yet
	else
		timer->TimerExpired  = false;

	// Return the state of this timer
	return timer->TimerExpired;
}

/**
 * @brief	Return the Pin associated with this LED
 * @param	[byte]	pin
 * @retval	[byte]	pin
 */
byte Display::pin(byte original) {
	return pins[original - 1];
}





