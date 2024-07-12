/* Dependencies */
#include "TWI_ATmega4809.h"

__TWI_ATMEGA4809__::__TWI_ATMEGA4809__(TWI_t* twi)
{
    this->twi = twi;
}

__TWI_ATMEGA4809__::~__TWI_ATMEGA4809__()
{
    this->twi = NULL;
}

const uint8_t __TWI_ATMEGA4809__::begin(const uint32_t frequency)
{
    if (this->began)                                               // If already began do not bother and return
        return (TWI_RESULT_FAIL);
    
    this->began = 1;
    this->setFrequency(frequency);                                 // Set the TWI frequency
    this->result = TWI_RESULT_UNKNOWN;                             // Set the result as unknown
    PORTMUX.TWISPIROUTEA |= 0x03;                                  // Idk wtf this do
                                                                   // Maybe sets the pins for TWI instead of SPI??
    
	this->twi->MCTRLA = TWI_RIEN_bm | TWI_WIEN_bm | TWI_ENABLE_bm; // Enable TWI bus, read & write interrupts
	this->twi->MSTATUS = TWI_BUSSTATE_IDLE_gc;                     // Set the bus state as idle
    return (TWI_RESULT_OK);
}

void __TWI_ATMEGA4809__::setFrequency(const uint32_t frequency)
{
    uint16_t t_rise;
    switch (frequency)
    {
        default:
        case 100000:
            t_rise = 1000;
            break;
        case 400000:
            t_rise = 300;
            break;
        case 1000000:
            t_rise = 120;
            break;
    }
    const uint32_t baud = ((F_CPU / frequency) - ((F_CPU / 1000000) * t_rise / 1000) - 10) / 2;
	this->twi->MBAUD = (const uint8_t)baud;
}

void __TWI_ATMEGA4809__::beginTransmission(const uint8_t address)
{
    this->result = TWI_RESULT_UNKNOWN;          // Result is unknown, we will fill this up with a result after communication finish
    this->address = (address << 1) & ~(1 << 0); // Shift the address and clear its R/W bit
    this->bytesToProcess = 0;                   // Clear the size of buffer
    this->bytesProcessed = 0;                   // Clear the index of buffer
}

const uint8_t __TWI_ATMEGA4809__::write(const uint8_t byte)
{
    if (bytesToProcess >= TWI_BUFFER_SIZE)        // If the buffer size can overflow dont bother and return
        return (TWI_RESULT_BUFFER_OVERFLOW);
    this->buffer[this->bytesToProcess++] = byte;
    return (TWI_RESULT_OK);
}

const uint8_t __TWI_ATMEGA4809__::endTransmission(const uint8_t sendStop)
{
    this->sendStop = sendStop;                 // Fill the sendStop flag
	this->twi->MADDR = this->address;                // Set the target address, this initiates a TWI communication
    while(this->result == TWI_RESULT_UNKNOWN); // Wait until we get a result from TWI communication
    return (this->result);
}

const uint8_t __TWI_ATMEGA4809__::endTransmission(void)
{
    return (this->endTransmission((const uint8_t)1));
}

const uint8_t __TWI_ATMEGA4809__::requestFrom(const uint8_t address, const uint8_t amount, const uint8_t sendStop)
{
    if (amount > TWI_BUFFER_SIZE)               // If amount is way beyond our buffer size dont bother and return
        return (TWI_RESULT_BUFFER_OVERFLOW);
    
    this->result = TWI_RESULT_UNKNOWN;          // Result is unknown, we will fill this up with a result after communication finish
    this->bytesToProcess = amount;              // Set how many bytes we want to receive
    this->bytesProcessed = 0;                   // Clear the index of buffer
    this->sendStop = sendStop;                  // Fill the sendStop flag
    this->address = (address << 1) | (1 << 0);  // Shift the address and set its R/W bit
	this->twi->MADDR = this->address;                 // Set the target address, this initiates a TWI communication
    while(this->result == TWI_RESULT_UNKNOWN); // Wait until we get a result from TWI communication
    return (TWI_RESULT_OK);
}

const uint8_t __TWI_ATMEGA4809__::requestFrom(const uint8_t address, const uint8_t amount)
{
    return (this->requestFrom(address, amount, (const uint8_t)1));
}

const uint8_t __TWI_ATMEGA4809__::available(void)
{
    return (this->bytesProcessed);
}

const uint8_t __TWI_ATMEGA4809__::read(void)
{
    if (!this->available()) // If no bytes are available protect from buffer underflow
        return (0);
    return (this->buffer[this->bytesToProcess - this->bytesProcessed--]);
}

const uint8_t __TWI_ATMEGA4809__::end(void)
{
    if (!this->began)
        return (TWI_RESULT_FAIL);
    
    this->began = 0;
    this->result = TWI_RESULT_UNKNOWN;         // Set the result as unknown
    PORTMUX.TWISPIROUTEA &= ~0x03;             // Idk wtf this do
                                               // Maybe sets the pins for TWI instead of SPI??
	this->twi->MCTRLA = 0x00;                  // Enable TWI bus, read & write interrupts
	this->twi->MSTATUS = TWI_BUSSTATE_IDLE_gc; // Set the bus state as idle
    return (TWI_RESULT_OK);
}

void __TWI_ATMEGA4809__::masterISRHandler(void)
{
    const uint8_t currentStatus = this->twi->MSTATUS;
    switch (currentStatus & (TWI_WIF_bm | TWI_RIF_bm | TWI_ARBLOST_bm | TWI_BUSERR_bm))
    {
        /* Master write interrupt */
        case TWI_WIF_bm: 
            if (currentStatus & TWI_RXACK_bm)                            // If NOT acknowledged (NACK) by slave cancel the transaction
            {
                if (this->sendStop)                                      // If should send stop
                    this->twi->MCTRLB = TWI_MCMD_STOP_gc;                // Issue a ACK with a stop signal
                else                                                     // If should send a repeated start
                    this->twi->MCTRLB = TWI_MCMD_REPSTART_gc;            // Issue a ACK with a repeated start signal
                this->result = TWI_RESULT_NACK_RECEIVED;                 // Result is a NACK received
            }
            else if (this->bytesProcessed < this->bytesToProcess)        // If there are bytes to be processed
                this->twi->MDATA = this->buffer[this->bytesProcessed++]; // Push a byte from the buffer
            else                                                         // If transaction finished, issue NACK and STOP or REPSTART
            {
                if (this->sendStop)                                      // If should send stop
                    this->twi->MCTRLB = TWI_MCMD_STOP_gc;                // Issue a ACK with a stop signal
                else                                                     // If should send a repeated start
                    this->twi->MCTRLB = TWI_MCMD_REPSTART_gc;            // Issue a ACK with a repeated start signal
                this->result = TWI_RESULT_OK;                            // Result is OK
            }
            break;
        
        /* Master read interrupt */
        case TWI_RIF_bm:
            if (this->bytesProcessed < this->bytesToProcess)                  // If there are bytes to be processed
                this->buffer[this->bytesProcessed++] = this->twi->MDATA;      // Push a byte into the buffer
            else                                                              // If buffer overflow occured, issue NACK and STOP or REPSTART
            {
                if (this->sendStop)                                           // If should send stop
                    this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;     // Issue a NACK with a stop signal
                else                                                          // If should send a repeated start
                    this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_REPSTART_gc; // Issue a NACK with a repeated start signal
                this->result = TWI_RESULT_BUFFER_OVERFLOW;                    // Result is a buffer overflow
                this->bytesProcessed = 0;                                     // Clear the amount of bytes processed
                return;
            }
            if (this->bytesProcessed < this->bytesToProcess)                  // If more bytes to read, issue ACK and start a byte read
                this->twi->MCTRLB = TWI_MCMD_RECVTRANS_gc;                    // Issue a receive signal
            else                                                              // If transaction finished, issue NACK and STOP or REPSTART
            {
                if (this->sendStop)                                           // If should send stop
                    this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;     // Issue a NACK with a stop signal
                else                                                          // If should send a repeated start
                    this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_REPSTART_gc; // Issue a NACK with a repeated start signal
                this->result = TWI_RESULT_OK;                                 // Result is OK
            }
            break;

        /* Arbitration lost interrupt */
        case TWI_ARBLOST_bm:
            this->result = TWI_RESULT_ARBITRATION_LOST; // Result is an albitration lost
            this->twi->MSTATUS = currentStatus;
            break;
        
        /* Bus error interrupt */
        case TWI_BUSERR_bm:
            this->result = TWI_RESULT_BUS_ERROR; // Result is a bus error
            this->twi->MSTATUS = currentStatus;
            break;

        default: // Unexpected state
            this->result = TWI_RESULT_FAIL; // Finish with fail
            break;
    }
}

void __TWI_ATMEGA4809__::masterArbitrationLostBusErrorHandler(void)
{
    const uint8_t currentStatus = this->twi->MSTATUS;
	if (currentStatus & TWI_BUSERR_bm)               // If bus error.
		this->result = TWI_RESULT_BUS_ERROR;
	else                                             // If arbitration lost.
		this->result = TWI_RESULT_ARBITRATION_LOST;
	this->twi->MSTATUS = currentStatus;
}

void __TWI_ATMEGA4809__::masterWriteHandler(void)
{
	if (this->twi->MSTATUS & TWI_RXACK_bm)                                // If NOT acknowledged (NACK) by slave cancel the transaction.
    {
		if(this->sendStop)                                          // If should send stop
			this->twi->MCTRLB = TWI_MCMD_STOP_gc;
		else                                                        // If should send a repeated start
			this->twi->MCTRLB = TWI_MCMD_REPSTART_gc;
		this->masterTransactionFinished(TWI_RESULT_NACK_RECEIVED); // Finish with NACK received
	}
	else if (this->bytesProcessed < this->bytesToProcess)                  // If more bytes to write, send data.
		this->twi->MDATA = this->buffer[this->bytesProcessed++];
	else                                                            // If transaction finished, send ACK/STOP condition if instructed and set RESULT OK.
    {
		if(this->sendStop)                                          // If should send stop
			this->twi->MCTRLB = TWI_MCMD_STOP_gc;
		else                                                        // If should send a repeated start
			this->twi->MCTRLB = TWI_MCMD_REPSTART_gc;
		this->masterTransactionFinished(TWI_RESULT_OK);            // Finish with OK
	}
}

void __TWI_ATMEGA4809__::masterReadHandler(void)
{
	if (this->bytesProcessed < this->bytesToProcess)                         // Fetch data if bytes to be read.
		this->buffer[this->bytesProcessed++] = this->twi->MDATA;
	else                                                              // If buffer overflow, issue NACK/STOP and BUFFER_OVERFLOW condition.
    {
		if(this->sendStop)                                            // If should send stop
			this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
		else                                                          // If should send a repeated start
			this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_REPSTART_gc;
		
		this->masterTransactionFinished(TWI_RESULT_BUFFER_OVERFLOW);
		this->bytesProcessed = 0;
		return;
	}
	if (this->bytesProcessed < this->bytesToProcess)                         // If more bytes to read, issue ACK and start a byte read.
		this->twi->MCTRLB = TWI_MCMD_RECVTRANS_gc;
	else                                                              // If transaction finished, issue NACK and STOP condition if instructed.
    {
		if(this->sendStop)                                            // If should send stop
			this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
        else                                                          // If should send a repeated start
			this->twi->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_REPSTART_gc;
		this->masterTransactionFinished(TWI_RESULT_OK);
	}
}

void __TWI_ATMEGA4809__::masterTransactionFinished(const uint8_t result)
{
	this->result = result;
}

#if defined(__AVR_ATmega4809__)
__TWI_ATMEGA4809__ TWI_ATmega4809 = __TWI_ATMEGA4809__(&TWI0);
ISR(TWI0_TWIM_vect)
{
    TWI_ATmega4809.masterISRHandler();
}
#endif
