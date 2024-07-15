/* Dependencies */
#include "TWI_ATmega4809.h"

/*!
 * @brief  __TWI_ATMEGA4809__ constructor
 * @param  twi
 *         The TWI structure pointer to memory
 */
__TWI_ATMEGA4809__::__TWI_ATMEGA4809__(TWI_t* twi)
{
    this->twi = twi;
}

/*!
 * @brief  __TWI_ATMEGA4809__ destructor
 */
__TWI_ATMEGA4809__::~__TWI_ATMEGA4809__()
{
    this->twi = NULL;
}

/*!
 * @brief  Starting the TWI bus implementation
 * @param  frequency
 *         The frequency of TWI bus
 * @return Fail if already started this TWI bus, otherwise OK
 */
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

/*!
 * @brief  Setting the TWI bus frequency
 * @param  frequency
 *         The frequency of TWI bus
 */
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

/*!
 * @brief  Starting a master write transmission
 * @param  address
 *         The address of TWI slave device
 */
void __TWI_ATMEGA4809__::beginTransmission(const uint8_t address)
{
    this->result = TWI_RESULT_UNKNOWN;          // Result is unknown, we will fill this up with a result after communication finishes
    this->address = (address << 1) & ~(1 << 0); // Shift the address and clear its R/W bit
    this->bytesToProcess = 0;                   // Clear the size of buffer
    this->bytesProcessed = 0;                   // Clear the index of buffer
}

/*!
 * @brief  Writing a byte into the buffer
 * @param  byte
 *         The byte to write into buffer
 * @return Buffer overflow if no more space is available into the buffer, otherwise OK
 */
const uint8_t __TWI_ATMEGA4809__::write(const uint8_t byte)
{
    if (bytesToProcess >= TWI_BUFFER_SIZE)        // If the buffer size can overflow dont bother and return
        return (TWI_RESULT_BUFFER_OVERFLOW);
    this->buffer[this->bytesToProcess++] = byte;
    return (TWI_RESULT_OK);
}

/*!
 * @brief  Writing a word into the buffer
 * @param  word
 *         The word to write into buffer
 * @return Buffer overflow if no more space is available into the buffer, otherwise OK
 */
const uint8_t __TWI_ATMEGA4809__::write(const uint16_t word)
{
    if (this->write((const uint8_t)word) == TWI_RESULT_BUFFER_OVERFLOW)        return (TWI_RESULT_BUFFER_OVERFLOW);
    if (this->write((const uint8_t)(word >> 8)) == TWI_RESULT_BUFFER_OVERFLOW) return (TWI_RESULT_BUFFER_OVERFLOW);
    return (TWI_RESULT_OK);
}

/*!
 * @brief  Writing a dword into the buffer
 * @param  dword
 *         The dword to write into buffer
 * @return Buffer overflow if no more space is available into the buffer, otherwise OK
 */
const uint8_t __TWI_ATMEGA4809__::write(const uint32_t dword)
{
    if (this->write((const uint8_t)dword) == TWI_RESULT_BUFFER_OVERFLOW)         return (TWI_RESULT_BUFFER_OVERFLOW);
    if (this->write((const uint8_t)(dword >> 8)) == TWI_RESULT_BUFFER_OVERFLOW)  return (TWI_RESULT_BUFFER_OVERFLOW);
    if (this->write((const uint8_t)(dword >> 16)) == TWI_RESULT_BUFFER_OVERFLOW) return (TWI_RESULT_BUFFER_OVERFLOW);
    if (this->write((const uint8_t)(dword >> 24)) == TWI_RESULT_BUFFER_OVERFLOW) return (TWI_RESULT_BUFFER_OVERFLOW);
    return (TWI_RESULT_OK);
}

/*!
 * @brief  Writing a known-sized array into the buffer
 * @param  array
 *         The array to write into buffer
 * @param  size
 *         The size of the array to write into buffer
 * @return Buffer overflow if no more space is available into the buffer, otherwise OK
 */
const uint8_t __TWI_ATMEGA4809__::write(const uint8_t* array, const uint8_t size)
{
    for (const uint8_t* p = array; p < (array + size); p++)
    {
        if (this->write((const uint8_t)*p) == TWI_RESULT_BUFFER_OVERFLOW)
            return (TWI_RESULT_BUFFER_OVERFLOW);
    }
    return (TWI_RESULT_OK);
}

/*!
 * @brief  Writing a known-sized memory zone into the buffer
 * @param  data
 *         The known-sized memory zone to write into buffer
 * @param  size
 *         The size of the array to write into buffer
 * @return Buffer overflow if no more space is available into the buffer, otherwise OK
 */
const uint8_t __TWI_ATMEGA4809__::write(const void* data, const uint8_t size)
{
    return (this->write((const uint8_t*)data, size));
}

/*!
 * @brief  Ending a master write transmission
 * @param  sendStop
 *         The flag used for sending a stop after transmission or a repeated start
 * @return The result of transmission
 */
const uint8_t __TWI_ATMEGA4809__::endTransmission(const uint8_t sendStop)
{
    this->sendStop = sendStop;                 // Fill the sendStop flag
	this->twi->MADDR = this->address;          // Set the target address, this initiates a TWI communication
    while(this->result == TWI_RESULT_UNKNOWN); // Wait until we get a result from TWI communication
    return (this->result);
}

/*!
 * @brief  Ending a master write transmission with a stop
 * @return The result of transmission
 */
const uint8_t __TWI_ATMEGA4809__::endTransmission(void)
{
    return (this->endTransmission((const uint8_t)1));
}

/*!
 * @brief  Starting a master read request 
 * @param  address
 *         The address of TWI slave device
 * @param  amount
 *         The amount to expect to receive from the TWI slave device
 * @param  sendStop
 *         The flag used for sending a stop after request or a repeated start
 */
const uint8_t __TWI_ATMEGA4809__::requestFrom(const uint8_t address, const uint8_t amount, const uint8_t sendStop)
{
    if (amount > TWI_BUFFER_SIZE)              // If amount is way beyond our buffer size dont bother and return
        return (TWI_RESULT_BUFFER_OVERFLOW);
    
    this->result = TWI_RESULT_UNKNOWN;         // Result is unknown, we will fill this up with a result after communication finish
    this->bytesToProcess = amount;             // Set how many bytes we want to receive
    this->bytesProcessed = 0;                  // Clear the index of buffer
    this->sendStop = sendStop;                 // Fill the sendStop flag
    this->address = (address << 1) | (1 << 0); // Shift the address and set its R/W bit
	this->twi->MADDR = this->address;          // Set the target address, this initiates a TWI communication
    while(this->result == TWI_RESULT_UNKNOWN); // Wait until we get a result from TWI communication
    return (TWI_RESULT_OK);
}

/*!
 * @brief  Starting a master read request with a stop
 * @param  address
 *         The address of TWI slave device
 * @param  amount
 *         The amount to expect to receive from the TWI slave device
 */
const uint8_t __TWI_ATMEGA4809__::requestFrom(const uint8_t address, const uint8_t amount)
{
    return (this->requestFrom(address, amount, (const uint8_t)1));
}

/*!
 * @brief  Getting the amount of bytes available into the buffer after requesting them
 * @return The amount of bytes available into the buffer after requesting them
 */
const uint8_t __TWI_ATMEGA4809__::available(void)
{
    return (this->bytesProcessed);
}

/*!
 * @brief  Getting the next byte from the buffer
 * @return The next byte from the buffer
 */
const uint8_t __TWI_ATMEGA4809__::read(void)
{
    if (!this->available()) // If no bytes are available protect from buffer underflow
        return (0);
    return (this->buffer[this->bytesToProcess - this->bytesProcessed--]);
}

/*!
 * @brief  Ending the TWI bus implementation
 * @return Fail if already ended this TWI bus, otherwise OK
 */
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

/*!
 * @brief  The TWI master ISR handler
 */
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

#if defined(__AVR_ATmega4809__)
__TWI_ATMEGA4809__ TWI_ATmega4809 = __TWI_ATMEGA4809__(&TWI0);
ISR(TWI0_TWIM_vect)
{
    TWI_ATmega4809.masterISRHandler();
}
#endif
