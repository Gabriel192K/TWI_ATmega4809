/* Dependencies */
#include <TWI_ATmega4809.h>

/* Macros */
#define TWI_FREQUENCY (const uint32_t)400000
#define UART          Serial2

void setup(void)
{
    UART.begin(115200);
    UART.print("\tTWI - Bus Scanner\n");
    TWI_ATmega4809.begin(TWI_FREQUENCY);
}

void loop(void)
{
    UART.print("Scanning TWI bus\n");
    for (uint8_t address = 1; address < 127; address++)
    {
        TWI_ATmega4809.beginTransmission(address);
        const uint8_t result = TWI_ATmega4809.endTransmission((const uint8_t)1);
        switch (result)
        {
            case TWI_RESULT_OK:
                UART.print("Result OK\n");
                UART.print("TWI device found at address: 0x");
                UART.println(address, HEX);
                break;
            case TWI_RESULT_FAIL:
                UART.print("Result fail\n");
                break;
            case TWI_RESULT_BUFFER_OVERFLOW:
                UART.print("Result buffer overflow\n");
                break;
            case TWI_RESULT_NACK_RECEIVED:
                UART.print("Result NACK received\n");
                break;
            case TWI_RESULT_ARBITRATION_LOST:
                UART.print("Result arbitration lost\n");
                break;
            case TWI_RESULT_BUS_ERROR:
                UART.print("Result bus error\n");
                break;
            default:
                UART.print("Result unknown error");
                break;
        }
        delay(100);
    }
    delay(1000);
}
