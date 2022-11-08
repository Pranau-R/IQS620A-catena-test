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
* @file 	    I2C.cpp									 					  *
* @brief 	    Arduino I2C library for Azoteq Devices              		  *
* @author 		AJ van der Merwe - Azoteq PTY Ltd                             *
* @credits		Robert Samuel for laying the foundation for HW I2C			  *
* @version 		V1.0.0                                                        *
* @date 	    27/06/2016                                                    *
*******************************************************************************/

#if(ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif
#include <inttypes.h>
#include "I2C.h"

// Private defines
#define START           	0x08
#define REPEATED_START  	0x10
#define MT_SLA_ACK			0x18
#define MT_SLA_NACK			0x20
#define MT_DATA_ACK     	0x28
#define MT_DATA_NACK    	0x30
#define MR_SLA_ACK			0x40
#define MR_SLA_NACK			0x48
#define MR_DATA_ACK     	0x50
#define MR_DATA_NACK    	0x58
#define LOST_ARBTRTN    	0x38
#define TWI_STATUS      	(TWSR & 0xF8)
#define I2C_TRANSMITTER		0
#define I2C_RECEIVER		1
#define ACK					1
#define NACK				0

// Set this instance of the Object
I2C *firstInstance = NULL;
I2C *secondInstance = NULL;


/**
 * @brief	Parameterized constructor. This is the minimum that we need
 * 			to setup an I2C instance
 * @param	[uint8_t] 	i2c_address of this I2C device
 * @param	[int] 		int RDY - RDY pin for this I2C device
 */
I2C::I2C(uint8_t i2c_address, int rdy)
{
	// Setup the RDY pin
    pinMode(rdy, OUTPUT);
	digitalWrite(rdy, HIGH);
	pinMode(rdy, INPUT);
	digitalWrite(rdy, HIGH);
	
	// Attach interrupt to IO 2 or IO 3
	if(rdy == 2 || rdy == 3){
		attachInterrupt(digitalPinToInterrupt(rdy), I2C::RDY_Interrupt, CHANGE);

		// Set the instance automatically
		if(firstInstance == NULL)
			firstInstance = this;
		else
			secondInstance = this;
	}

	// Default Comms speed 400kHz
	this->_commsSpeed = FAST;

	// Setup the I2C Peripheral
	this->setup();

	this->_RDY_Window = false;

	this->_i2c_address = i2c_address;

    // Set a default timeout
    _timeout_ms = 100;

    _startMillis = 0;	// Just init to 0

    // We clear timeout
    _Timeout = false;

    // Get the RDY
    this->_rdy = rdy;

    // We set a default Polling RDY
    this->_RDY_Type = Polling;

    // Default is 8 bit comms
    this->_commsSize = Size_8_bit;

    // Unlock the Module
    unlock();
}

/**
 * @brief	Parameterized constructor. This is used to setup the
 * 			RDY type of the I2C device. This can be Active High, Active Low
 * 			or Polling
 * @param	[uint8_t] 		i2c_address of this I2C device
 * @param	[int] 			int RDY - RDY pin for this I2C device
 * @param	[RDY_Type_e] 	RDY type - High, Low, Polling
 */
I2C::I2C(uint8_t i2c_address, int rdy, RDY_Type_e rdy_type)
{
	// Setup the RDY pin
    pinMode(rdy, OUTPUT);
	digitalWrite(rdy, HIGH);
	pinMode(rdy, INPUT);
	digitalWrite(rdy, HIGH);

	// Attach interrupt to IO 2 or IO 3
	if(rdy == 2 || rdy == 3){
		attachInterrupt(digitalPinToInterrupt(rdy), I2C::RDY_Interrupt, CHANGE);

		// Set the instance automatically
		if(firstInstance == NULL)
			firstInstance = this;
		else
			secondInstance = this;
	}

	// Default Comms speed 400kHz
	this->_commsSpeed = FAST;

	// Setup the I2C Peripheral
	this->setup();

	this->_RDY_Window = false;

	this->_i2c_address = i2c_address;

    // Set a default timeout
    _timeout_ms = 100;

    _startMillis = 0;	// Just init to 0

    // We clear timeout
    _Timeout = false;

    // Get the RDY
    this->_rdy = rdy;

    // We set a default Polling RDY
    this->_RDY_Type = rdy_type;

    // Default is 8 bit comms
    this->_commsSize = Size_8_bit;

    // Unlock the Module
    unlock();
}

/**
 * @brief	Parameterized constructor. This is used to setup the
 * 			RDY type of the I2C device. This can be Active High, Active Low
 * 			or Polling
 * @param	[uint8_t] 		i2c_address of this I2C device
 * @param	[int] 			int RDY - RDY pin for this I2C device
 * @param	[RDY_Type_e] 	RDY type - High, Low, Polling
 * @param	[unsigned long] Timeout of the I2C transfers
 */
I2C::I2C(uint8_t i2c_address, int rdy, RDY_Type_e rdy_type, unsigned long timeout_ms)
{
	// Setup the RDY pin
    pinMode(rdy, OUTPUT);
	digitalWrite(rdy, HIGH);
	pinMode(rdy, INPUT);
	digitalWrite(rdy, HIGH);

	// Attach interrupt to IO 2 or IO 3
	if(rdy == 2 || rdy == 3){
		attachInterrupt(digitalPinToInterrupt(rdy), I2C::RDY_Interrupt, CHANGE);

		// Set the instance automatically
		if(firstInstance == NULL)
			firstInstance = this;
		else
			secondInstance = this;
	}

	// Default Comms speed 400kHz
	this->_commsSpeed = SLOW;

	// Setup the I2C Peripheral
	this->setup();

	this->_RDY_Window = false;

	this->_i2c_address = i2c_address;

    // Set a default timeout
    _timeout_ms = timeout_ms;

    _startMillis = 0;	// Just init to 0

    // We clear timeout
    _Timeout = false;

    // Get the RDY
    this->_rdy = rdy;

    // We set a default Polling RDY
    this->_RDY_Type = rdy_type;

    // Default is 8 bit comms
    this->_commsSize = Size_8_bit;

    // Unlock the Module
    unlock();
}


/**
 * @brief	Set the instance for interrupt usage of the RDY pins
 * @param	None
 * @retval	None
 */
void I2C::setInstance(void)
{
	if(firstInstance == NULL)
		firstInstance = this;
	else
		secondInstance = this;
}


/**
 * @brief	Lock the I2C Module, so that we can only access it one instance at a time
 * @param	None
 * @retval	None
 */
void I2C::lock(void)
{
	moduleLock = BUSY;	// Set as busy, now we know it is locked
}


/**
 * @brief	Unlock the I2C Module, so that we know we can now read or write
 * @param	None
 * @retval	None
 */
void I2C::unlock(void)
{
	moduleLock = IDLE;	// Set as idle, now we know it is unlocked
}

/**
 * @brief	I2C Write function that gets called in the source. It writes to the required
 * 			register address, the no of bytes specified. This is the 16 bit register
 * 			address version, but the same 1 is used for the 8 bit implementation
 * @note	Remember to set the Comms Size - default is 8 bit
 * @param	[uint16_t]		register address to write to
 * @param	[uint8_t]		number of bytes to write to the address
 * @param	[uint8_t *]		pointer to the array from where to write the data
 * @param	[I2C_Stop_e]	send a stop on the bus or repeat start condition
 */
uint8_t I2C::write(uint16_t address, uint8_t no_of_bytes, uint8_t *pData, I2C_Stop_e i2c_stop)
{
	uint8_t res = 0;
	uint8_t i;

	// restart timeout, so that we can also check if modulelock gets stuck
	this->reset_timeout();

	// Check whether the I2C Module is locked and wait until it is unlocked
	while(moduleLock && !this->_Timeout);

	// Lock the I2C Module so that another instance cannot access it
	lock();

	// Send the start to the IC
	res = this->poll(I2C_TRANSMITTER);

	// We received an error, give up
	if(res)
	{
		// Reset the peripheral before we quit
		this->reset();
		return res;
	}

	// Send the 16-bit address - MSB
	if(this->_commsSize == Size_16_bit)
	{
		// Send the 8 bit register address
		res = this->sendByte((uint8_t)(address>>8));

		// We received an error, give up
		if(res)
		{
			// Reset the peripheral before we quit
			this->reset();
			return res;
		}
	}

	// Send the 8 bit register address
	res = this->sendByte((uint8_t)address);

	// We received an error, give up
	if(res)
	{
		// Reset the peripheral before we quit
		this->reset();
		return res;
	}

	// No send the number of bytes required to send
	for(i = 0; (i < no_of_bytes) && !res; i++)
	{
		// Send each required byte
		res = this->sendByte(pData[i]);
	}

	// We received an error, give up
	if(res)
	{
		// Reset the peripheral before we quit
		this->reset();
		return res;
	}

	// Now send a stop if we have to
	if(i2c_stop)
	{
		this->stop();

		// just quickly wait for RDY
		while(this->isDeviceReady() && (this->_rdy != NO_RDY) && (this->_RDY_Type != Polling));
	}

	// Unlock the module before we finish
	unlock();

	return res;
}


/**
 * @brief	I2C read function that gets called in the source. It reads the required
 * 			register address, the no of bytes specified. This is the 16 bit register
 * 			address version, but the same 1 is used for the 8 bit implementation
 * @note	Remember to set the Comms Size - default is 8 bit
 * @param	[uint16_t]		register address to read from - 16 bit address if we like
 * @param	[uint8_t]		number of bytes to read from the address
 * @param	[uint8_t *]		pointer to the array from where to store the data
 * @param	[I2C_Stop_e]	send a stop on the bus or repeat start condition
 */
uint8_t I2C::read(uint16_t address, uint8_t no_of_bytes, uint8_t *pData, I2C_Stop_e i2c_stop)
{
	uint8_t res = 0;
	uint8_t i;

	// restart timeout, so that we can also check if modulelock gets stuck
	this->reset_timeout();

	// Check whether the I2C Module is locked and wait until it is unlocked
	while(moduleLock && !this->_Timeout);

	// Lock the I2C Module so that another instance cannot access it
	lock();

	// Send the start to the IC
	res = this->poll(I2C_TRANSMITTER);

	// We received an error, give up
	if(res)
	{
		// Reset the peripheral before we quit
		this->reset();
		return res;
	}

	// Send the 16-bit address - MSB
	if(this->_commsSize == Size_16_bit)
	{
		// Send the 8 bit register address
		res = this->sendByte((uint8_t)(address>>8));

		// We received an error, give up
		if(res)
		{
			// Reset the peripheral before we quit
			this->reset();
			return res;
		}
	}

	// Send the 8 bit register address
	res = this->sendByte(address);

	// We received an error, give up
	if(res)
	{
		// Reset the peripheral before we quit
		this->reset();
		return res;
	}

	// Send the start to the IC - now we are in receiving mode
	res = this->poll(I2C_RECEIVER);

	// We received an error, give up
	if(res)
	{
		// Reset the peripheral before we quit
		this->reset();
		return res;
	}

	// No send the number of bytes required to send
	for(i = 0; (i < no_of_bytes) && !res; i++)
	{
		// Read the last byte w
		if(i == (no_of_bytes - 1))
		{
			if(this->receiveByte(NACK))
			{
				return ERR_TIMEOUT;
			}
		}
		else
		{
			if(this->receiveByte(ACK))
			{
				return ERR_TIMEOUT;
			}
		}
		*pData = TWDR;
		pData++;
	}

	// We received an error, give up
	if(res)
	{
		// Reset the peripheral before we quit
		this->reset();
		return res;
	}

	// Now send a stop if we have to
	if(i2c_stop)
	{
		this->stop();

		// just quickly wait for RDY
		while(this->isDeviceReady() && (this->_rdy != NO_RDY) && (this->_RDY_Type != Polling));
	}


	// Unlock the module before we finish
	unlock();

	return res;
}


/**
 * @brief	Check whether this device is RDY. We see if the interrupt set it to ready or if the RDY is active
 * @TODO	Still need to be able to feed in the user's own callback
 * @param	None
 * @retval	[bool] is this device RDY (true) or not (false)
 */
bool I2C::isDeviceReady(void)
{
	bool tempWindow = false;
	
	if(this->_rdy == 2 || this->_rdy == 3)
	{
		tempWindow = (this->_RDY_Window || (!(bool)(digitalRead(this->_rdy) ^ (uint8_t)this->_RDY_Type))); // || this->_RDY_Type == Polling);
		this->_RDY_Window = false;
	}
	else if(this->_rdy != NO_RDY)
		tempWindow = ((!(bool)(digitalRead(this->_rdy) ^ (uint8_t)this->_RDY_Type))); //|| this->_RDY_Type == Polling);

	return tempWindow;
}

//////////////////////////////// 	Setters		///////////////////////////////
// Only implement the necessary setters ATM

/**
 * @brief	Set the RDY pin after the initialization
 * @param	[int] io to connect RDY to
 * @retval	None
 */
void I2C::setRdyPin(int rdy)
{
	this->_rdy = rdy;
}

/**
 * @brief	Set the RDY Type after the initialization
 * @param	[RDY_Type_e] Active High, Active Low, Polling
 * @retval	None
 */
void I2C::setRdyType(RDY_Type_e rdy_type)
{
	this->_RDY_Type = rdy_type;
}

/**
 * @brief	Set the Timeout after the initialization
 * @param	[unsigned long] timeout in ms
 * @retval	None
 */
void I2C::setTimeout(unsigned long timeout)
{
	this->_timeout_ms = timeout;
}

/**
 * @brief	Set the Comms speed of I2C module
 * @param	[unsigned long] timeout in ms
 * @retval	None
 */
void I2C::setCommsSpeed(uint32_t commsSpeed)
{
	this->_commsSpeed = commsSpeed;
	this->setup();	// Re-init I2C
}

/**
 * @brief	Set the Comms speed of I2C module
 * @param	[unsigned long] timeout in ms
 * @retval	None
 */
void I2C::setCommsSize(Comms_Size_e commsSize)
{
	this->_commsSize = commsSize;
}


//////////////////////////////// 	Private Methods		///////////////////////////////


/**
 * @brief	Here we initialise the Hardware for I2C Comms
 * @param	None
 * @retval	None
 */
void I2C::setup(void)
{
	/*
	 * Set SDA and SCL to inputs
	 */
	#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega328P__)
		SetBit(PORTC, 4);
		SetBit(PORTC, 5);
	#else
		SetBit(PORTD, 0);
		SetBit(PORTD, 1);
	#endif
	/*
	 * Set prescaler and clock frequency
	 */
	ClearBit(TWSR, TWPS0);
	ClearBit(TWSR, TWPS1);
	TWBR = ((F_CPU / this->_commsSpeed) - 16) / 2;
	/*
	 * Enable module, and set to ACK
	 */
	SetBit(TWCR, TWEA);
	SetBit(TWCR, TWEN);
}

/**
 * @brief	Reset the I2C Bus. This is used when something terrible went
 * 			wrong
 * @param	None
 * @retval	None
 */
void I2C::reset(void)
{
	/*
	 * Release SCL and SDA lines and re-enable module
	 */
	TWCR = 0;
	SetBit(TWCR, TWEA);
	SetBit(TWCR, TWEN);

	// Unlock the I2C Module
	unlock();
}

/**
 * @brief	This is the interrupt handler for I2C RDY Pins, 2 and 3. For this to
 * 			work, we need an instance of the I2C object. We are only creating 2
 * 			instances, 1 for each interrupt
 * @param	None
 * @retval	None
 */
void I2C::RDY_Interrupt(void)
{
	// Get the RDY state
	// If the first instance exists, get the ready State
	if(firstInstance)
		firstInstance->_RDY_Window = !(bool)(digitalRead(firstInstance->_rdy) ^ (uint8_t)firstInstance->_RDY_Type);
	// If the second instance exists, get the ready State
	if(secondInstance)
		secondInstance->_RDY_Window = !(bool)(digitalRead(secondInstance->_rdy) ^ (uint8_t)firstInstance->_RDY_Type);

}

/**
 * @brief	Reset the timeout for the I2C functions
 * @param	None
 * @retval	None
 */
void I2C::reset_timeout(void)
{
	// Get the current millis
	_startMillis = millis();
	// Reset timeout flag
	_Timeout = false;
}


/**
 * @brief	Check whether we have a i2c timeout - based on current millis and
 * 			start millis - thus a software timeout
 * @param	None
 * @retval	false if no timeout and true if timeout
 */
bool I2C::check_timeout(void)
{
	// This is a timeout
	if((millis() - _startMillis) >= this->_timeout_ms)
		_Timeout = true;
	// We haven't timed out yet
	else
		_Timeout = false;

	return _Timeout;
}


/**
 * @brief	Start the I2C Transfer by creating an I2C Start condition
 * 			on the bus
 * @param	None
 * @retval	[uint8_t] 0 - Success; 1 - Timeout Error
 */
uint8_t I2C::start(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)))
	{
		// Check the I2C timeout
		if(this->check_timeout())
		{
			this->reset();
			return ERR_TIMEOUT;
		}
	}
	if((TWI_STATUS == START) || (TWI_STATUS == REPEATED_START))
	{
		return SUCCESS;
	}
	if(TWI_STATUS == LOST_ARBTRTN)
	{
		this->reset();
	}
	return ERR_BUS;
}

/**
 * @brief	Stop the I2C transfer by creating a Stop condition on the bus
 * @param	None
 * @retval	[uint8_t] 0 - Success; 1 - Timeout Error
 */
uint8_t I2C::stop(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)| (1<<TWSTO);
	while((TWCR & (1<<TWSTO)))
	{
		// Check the I2C timeout
		if(this->check_timeout())
		{
			this->reset();
			return ERR_TIMEOUT;
		}
	}
	return SUCCESS;
}

/**
 * @brief	Stop the I2C transfer by creating a Stop condition on the bus
 * @param	None
 * @retval	[uint8_t] 0 - Success; 1 - Timeout Error
 */
uint8_t I2C::sendAddress(uint8_t address)
{
	TWDR = address;

	TWCR = (1<<TWINT) | (1<<TWEN);

	while(!(TWCR & (1<<TWINT)))
	{
		// Check the I2C timeout
		if(this->check_timeout())
		{
			this->reset();
			return ERR_TIMEOUT;
		}
	}
	if((TWI_STATUS == MT_SLA_ACK) || (TWI_STATUS == MR_SLA_ACK))
	{
		return SUCCESS;
	}

	if((TWI_STATUS == MT_SLA_NACK) || (TWI_STATUS == MR_SLA_NACK))
	{
		this->stop();
	}
	else
	{
		this->reset();
	}
	return ERR_TIMEOUT;
}


/**
 * @brief	Stop the I2C transfer by creating a Stop condition on the bus
 * @param	None
 * @retval	[uint8_t] 0 - Success; 1 - Timeout Error
 */
uint8_t I2C::sendByte(uint8_t data)
{
	TWDR = data;

	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)))
	{
		if(this->check_timeout())
		{
			this->reset();
			return ERR_TIMEOUT;
		}
	}
	if(TWI_STATUS == MT_DATA_ACK)
	{
		return SUCCESS;
	}
	if(TWI_STATUS == MT_DATA_NACK)
	{
		this->stop();
	}
	else
	{
		this->reset();
	}
	return ERR_TIMEOUT;
}


/**
 * @brief	Stop the I2C transfer by creating a Stop condition on the bus
 * @param	[uint8_t] read with an ack 1, or with nack 0
 * @retval	[uint8_t] 0 - Success; 1 - Timeout Error
 */
uint8_t I2C::receiveByte(uint8_t ack)
{
	if(ack)
	{
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	}
	else
	{
		TWCR = (1<<TWINT) | (1<<TWEN);
	}
	while(!(TWCR & (1<<TWINT)))
	{
		if(this->check_timeout())
		{
			this->reset();
			return ERR_TIMEOUT;
		}
	}
	if(TWI_STATUS == LOST_ARBTRTN)
	{
		this->reset();
		return ERR_BUS;
	}

	if(((TWI_STATUS == MR_DATA_NACK) && (!ack)) ||
	 ((TWI_STATUS == MR_DATA_ACK) && (ack)))
	{
		return SUCCESS;
	}
	else
	{
		return ERR_TIMEOUT;
	}
}



/**
 * @brief	Start the I2C transfer to the IC, by polling the device.
 * 			Repeatedly sent start and address as well as the direction of the transfer
 * 			here we also start the I2C Timeout
 * @param	None
 * @retval	[uint8_t] 0 - Success; 1 - Timeout Error
 */
uint8_t I2C::poll(uint8_t direction)
{
	uint8_t res = 0;

	// Reset the I2C Timeout
	this->reset_timeout();

	do {
		res = 0;

		// Create start
		res = this->start();

		res	|= this->sendAddress((this->_i2c_address<<1)|direction);

	}while(res && !this->check_timeout()); //&& !this->isDeviceReady());

	// We timed out
	if(_Timeout)
		return ERR_TIMEOUT;

	return SUCCESS;
}





