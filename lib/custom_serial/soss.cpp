/*
 soss - adapted from SoftwareSerial by Nick Gammon 28th June 2012
 Optimized for StarGazer Mouth (D4)
*/

#include <avr/pgmspace.h>
#include "Arduino.h"
#include "soss.h"

// --- Lookup table ---
typedef struct _DELAY_TABLE
{
    long baud;
    unsigned short rx_delay_centering;
    unsigned short rx_delay_intrabit;
    unsigned short rx_delay_stopbit;
    unsigned short tx_delay;
} DELAY_TABLE;

#if F_CPU == 16000000
static const DELAY_TABLE PROGMEM table[] = {
    { 19200, 54, 117, 117, 114 }
};
const int XMIT_START_ADJUSTMENT = 5;

#elif F_CPU == 8000000
static const DELAY_TABLE table[] PROGMEM = {
    { 19200, 20, 55, 55, 52 }
};
const int XMIT_START_ADJUSTMENT = 4;

#elif F_CPU == 20000000
static const DELAY_TABLE PROGMEM table[] = {
    { 19200, 71, 148, 148, 145 }
};
const int XMIT_START_ADJUSTMENT = 6;

#else
#error This version of soss supports only 20, 16 and 8MHz processors
#endif

// --- Private Timing Method ---
inline void soss::tunedDelay(uint16_t delay)
{
    uint8_t tmp = 0;
    asm volatile("sbiw    %0, 0x01 \n\t"
                 "ldi %1, 0xFF \n\t"
                 "cpi %A0, 0xFF \n\t"
                 "cpc %B0, %1 \n\t"
                 "brne .-10 \n\t"
                 : "+r"(delay), "+a"(tmp)
                 : "0"(delay));
}

void soss::tx_pin_write(uint8_t pin_state)
{
    if (pin_state == LOW)
        *_transmitPortRegister &= ~_transmitBitMask;
    else
        *_transmitPortRegister |= _transmitBitMask;
}

// --- Constructor ---
soss::soss(uint8_t transmitPin, bool inverse_logic, bool errors_ok) : 
    _transmitPin(transmitPin), 
    _inverse_logic(inverse_logic), 
    _errors_ok(errors_ok)
{
    _tx_delay = 0; // Initialize to zero until begin() is called
}

// --- Destructor ---
soss::~soss()
{
    end();
}

void soss::setTX(uint8_t tx)
{
    digitalWrite(tx, HIGH);
    pinMode(tx, OUTPUT);
    _transmitBitMask = digitalPinToBitMask(tx);
    uint8_t port = digitalPinToPort(tx);
    _transmitPortRegister = portOutputRegister(port);
}

// --- Public methods ---

void soss::begin(long speed)
{
    setTX(_transmitPin); 

    _tx_delay = 0;
    for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
    {
        long baud = pgm_read_dword(&table[i].baud);
        if (baud == speed)
        {
            _tx_delay = pgm_read_word(&table[i].tx_delay);
            break;
        }
    }
}

void soss::end()
{
    // Implementation can be empty or used to release pin
}

// NOTE: read(), available(), peek(), and flush() are defined in soss.h 
// as inline functions. Do not define them here to avoid redefinition errors.

size_t soss::write(uint8_t b)
{
    if (_tx_delay == 0)
    {
        setWriteError();
        return 0;
    }

    uint8_t oldSREG = SREG;
    if (!_errors_ok)
    {
        cli(); // turn off interrupts for a clean txmit
    }

    // Write the start bit
    tx_pin_write(_inverse_logic ? HIGH : LOW);
    tunedDelay(_tx_delay + XMIT_START_ADJUSTMENT);

    // Write each of the 8 bits
    if (_inverse_logic)
    {
        for (byte mask = 0x01; mask; mask <<= 1)
        {
            if (b & mask)          
                tx_pin_write(LOW); // send 1
            else
                tx_pin_write(HIGH); // send 0

            tunedDelay(_tx_delay);
        }
        tx_pin_write(LOW); // restore pin to natural state
    }
    else
    {
        for (byte mask = 0x01; mask; mask <<= 1)
        {
            if (b & mask)           
                tx_pin_write(HIGH); // send 1
            else
                tx_pin_write(LOW); // send 0

            tunedDelay(_tx_delay);
        }
        tx_pin_write(HIGH); // restore pin to natural state
    }

    if (!_errors_ok)
    {
        SREG = oldSREG; // restore interrupts
    }
    tunedDelay(_tx_delay);

    return 1;
}