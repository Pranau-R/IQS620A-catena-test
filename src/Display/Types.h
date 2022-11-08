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
* @file 	    Types.h									 					  *
* @brief 	    Header for some custom types								  *
* @author 		AJ van der Merwe - Azoteq PTY Ltd                             *
* @version 		V1.0.0                                                        *
* @date 	    19/07/2016                                                    *
*******************************************************************************/


#ifndef __TYPES_H__
#define __TYPES_H__

#include "Arduino.h"


// 'Timer' object
typedef struct Timer {
	uint32_t Timer_counter;		// This timer's counter
	uint32_t Timer_start;		// This timer's start ms
	bool TimerExpired;			// Flag indicating whether timer expired
} Timer_t;



#endif  /* __TYPES_H__	*/

