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
* @file 	    I2C.h									 					  *
* @brief 	    Arduino I2C library for Azoteq Devices              		  *
* @author 		AJ van der Merwe - Azoteq PTY Ltd                             *
* @credits		Robert Samuel for laying the foundation for HW I2C			  *
* @version 		V1.0.0                                                        *
* @date 	    27/06/2016                                                    *
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_H
#define __I2C_H

#if(ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

// Global defines
#define	NO_RDY					(uint8_t)99  		// No RDY pin defined
#define SUCCESS					0			 		// I2C Successful transfer
#define ERR_TIMEOUT				1			 		// I2C Timeout error
#define ERR_BUS					2		     		// Bus Error
#define	END_WINDOW				(uint16_t)0xEEEE 	// End window macro for IQS5xx

// Bit test helper macros
#define BitIsSet(VAR,Index)		(VAR & (1<<Index)) != 0
#define BitIsNotSet(VAR,Index)	(VAR & (1<<Index)) == 0

#define SetBit(VAR,Index)		VAR |= (1<<Index)
#define ClearBit(VAR,Index)		VAR &= (uint8_t)(~(1<<Index))

// Define the states for the locks of the I2C
#define IDLE					0
#define BUSY					1

/**
 * I2C Comms Speed
 */
#define	SLOW					(uint32_t)50000		// 50 kHz
#define	MED						(uint32_t)100000	// 100 kHz
#define	FAST					(uint32_t)400000	// 400 kHz

/** Enum to determine Stop or repeat start */
typedef enum I2C_Stop {
	I2C_Repeat_Start = 0,
	I2C_Stop = 1
}I2C_Stop_e;

/** Enum for RDY active low and active high or Polling */
typedef enum RDY_Type {
	Active_Low = 0,
	Active_High = 1,
	Polling = 2
}RDY_Type_e;

/** Enum for Comms size - 8bit or 16bit register */
typedef enum {
	Size_8_bit = 0,
	Size_16_bit = 1
}Comms_Size_e;

class I2C
{
	protected:
		unsigned long _timeout_ms;      // number of milliseconds to wait for the next char before aborting timed read
		unsigned long _startMillis;  	// used for timeout measurement

    public:
		// Public variables
		uint8_t moduleLock;

		// Constructors
		I2C(uint8_t i2c_address, int rdy);
		I2C(uint8_t i2c_address, int rdy, RDY_Type_e rdy_type);
		I2C(uint8_t i2c_address, int rdy, RDY_Type_e rdy_type, unsigned long timeout_ms);

        uint8_t write(uint16_t address, uint8_t no_of_bytes, uint8_t *pData, I2C_Stop_e i2c_stop);
        uint8_t read(uint16_t address, uint8_t no_of_bytes, uint8_t *pData, I2C_Stop_e i2c_stop);
		bool isDeviceReady(void);
		void setInstance(void);
		void lock(void);
		void unlock(void);

		// Setters
		void setRdyPin(int rdy);
		void setRdyType(RDY_Type_e rdy_type);
		void setTimeout(unsigned long timeout);
		void setCommsSpeed(uint32_t commsSpeed);
		void setCommsSize(Comms_Size_e commsSize);

    private:
		// Object Params
        volatile int _rdy;
        uint8_t _i2c_address;
        RDY_Type_e _RDY_Type;
        volatile bool _RDY_Window;
		bool _Timeout;
		uint32_t _commsSpeed;
		Comms_Size_e _commsSize;
		void setup(void);
		void reset(void);
		void reset_timeout(void);
		bool check_timeout(void);
		static void RDY_Interrupt(void);
		// Helper functions
		uint8_t start(void);
		uint8_t stop(void);
		uint8_t sendAddress(uint8_t address);
		uint8_t sendByte(uint8_t data);
		uint8_t receiveByte(uint8_t ack);
		uint8_t poll(uint8_t direction);
};


#endif /* __I2C_H */

