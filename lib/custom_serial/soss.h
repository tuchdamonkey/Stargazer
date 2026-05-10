/*
 * soss.h - Send-Only Software Serial (Optimized for StarGazer)
 * Isolated for standard logic transmission on D4.
 */

#ifndef SOSS_H
#define SOSS_H

#include <inttypes.h>
#include <Stream.h>

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class soss : public Stream
{
private:
    // --- Physical Hardware Mapping ---
    uint8_t _transmitPin;           // Tracked for begin() calls
    uint8_t _transmitBitMask;
    volatile uint8_t *_transmitPortRegister;

    // --- Timing & Logic ---
    uint16_t _tx_delay;
    uint8_t _inverse_logic : 1;     // Use bitfields for memory efficiency
    uint8_t _errors_ok : 1;

    // --- Private Hardware Methods ---
    void tx_pin_write(uint8_t pin_state);
    void setTX(uint8_t transmitPin);

    // --- High-Precision Timing ---
    static inline void tunedDelay(uint16_t delay);

public:
    // Constructor: Needs pin and logic preference
    soss(uint8_t transmitPin, bool inverse_logic = false, bool errors_ok = false);
    ~soss();

    // Mandatory Lifecycle Methods
    void begin(long speed);
    void end();

    // Stream Implementation (Mouth only, so these are dummy stubs)
    virtual size_t write(uint8_t byte);
    virtual int read() { return -1; }
    virtual int available() { return 0; }
    virtual int peek() { return -1; }
    virtual void flush() { /* No buffer to flush */ }

    using Print::write;
};

#endif  // SOSS_H