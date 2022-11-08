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
* @file 	    Display.h									 				  *
* @brief 	    Arduino Display library for Azoteq Devices with 7 Segment	  *
* 				displays								            		  *
* @author 		AJ van der Merwe - Azoteq PTY Ltd                             *
* @credits		Iwan de Klerk for the basic working of the 7 Segment		  *
* @version 		V1.0.0                                                        *
* @date 	    21/07/2016                                                    *
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DISPLAY_H
#define __DISPLAY_H

#if(ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

#include "Types.h"

// Global defines

// Define Extra characters
#define LED_OFF					10
#define MINUS					11
#define UNDEFINED				12
#define DARK					13
#define LIGHT					14
#define NORTH					15
#define SOUTH					16
#define E						17
#define R						18
#define T						19
#define H						20
#define L						21
#define P						22
#define C						23
#define O						24
#define I						25
#define MINUS_1					26
#define DEG						27
#define R_ARROW					28
#define L_ARROW					29

// Define the states for the locks of the Display
#define IDLE					0
#define BUSY					1


class Display
{
	protected:

    public:
		// Public variables

		// Constructors
		Display();		// Empty Constructor
		Display(bool clearDisplay, uint32_t clearTime, uint32_t renderTime=500);		// Empty Constructor


        void write(int16_t data);
        void write(String data);
        void writeError(uint16_t errorCode);

		// Setters
        void setClearDisplay(bool clearDisplay);
        void setClearTime(uint32_t clearTime);
        void setRenderTime(uint32_t renderTime);

    private:
        // Object Params
        bool _clearDisplay;
        uint32_t _clearTime;
        uint32_t _renderTime;
        static void render(void);
        void clearScreen(void);
        byte pin(byte original);
        void on(byte row, byte column);
        void setDisplayTimer(Timer_t* timer);
        void setDisplayTimer(Timer_t* timer, uint32_t time);
        bool displayTimerExpired(Timer_t* timer);
};


#endif /* __DISPLAY_H */

