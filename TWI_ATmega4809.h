#ifndef __TWI_ATMEGA4809_H__
#define __TWI_ATMEGA4809_H__

/* Dependencies */
#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/* Macros */
#define TWI_BUFFER_SIZE (const uint8_t)32

/* Variables */
typedef enum TWI_RESULT_enum
{
	TWI_RESULT_UNKNOWN          = 0,
	TWI_RESULT_OK               = 1,
    TWI_RESULT_FAIL             = 2,
	TWI_RESULT_BUFFER_OVERFLOW  = 3,
    TWI_RESULT_NACK_RECEIVED    = 4,
    TWI_RESULT_ARBITRATION_LOST = 5,
	TWI_RESULT_BUS_ERROR        = 6,
    TWI_RESULT_CLKHOLD          = 7,
	TWI_RESULT_IDLE             = 8,
    TWI_RESULT_OWNER            = 9,
    TWI_RESULT_BUSY             = 10,
}TWI_RESULT_t;



class __TWI_ATMEGA4809__
{
    public:
        __TWI_ATMEGA4809__(TWI_t* twi);
        ~__TWI_ATMEGA4809__();
        const uint8_t  begin            (const uint32_t frequency);
        void           setFrequency     (const uint32_t frequency);
        void           beginTransmission(const uint8_t address);
        const uint8_t  write            (const uint8_t byte);
        const uint8_t  endTransmission  (const uint8_t sendStop);
        const uint8_t  endTransmission  (void);
        const uint8_t  requestFrom      (const uint8_t address, const uint8_t amount, const uint8_t sendStop);
        const uint8_t  requestFrom      (const uint8_t address, const uint8_t amount);
        const uint8_t  available        (void);
        const uint8_t  read             (void);
        const uint8_t  end             (void);
        void           masterISRHandler(void);
    private:
        TWI_t* twi;
        uint8_t began;
        volatile uint8_t result;
        volatile uint8_t address;
        volatile uint8_t buffer[TWI_BUFFER_SIZE];
        volatile uint8_t bytesToProcess;
        volatile uint8_t bytesProcessed;
        volatile uint8_t sendStop;
        void masterArbitrationLostBusErrorHandler(void);
        void masterWriteHandler                  (void);
        void masterReadHandler                   (void);
        void masterTransactionFinished           (const uint8_t result);

};

extern __TWI_ATMEGA4809__ TWI_ATmega4809;

#endif
