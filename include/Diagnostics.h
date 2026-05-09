#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <Arduino.h>

/* ==========================================================================
   DEVELOPER TOOL: THE MANTRA MONITOR
   --------------------------------------------------------------------------
   D7 is the SYNC pin (Hardwired to Diagnostic Port).
   This pin is Bit 7 of Port D on the ATmega328P.
   ========================================================================== */

#define DIAGNOSTIC_MODE // Comment this out to disable all D7 activity for "Field Ready" builds

#ifdef DIAGNOSTIC_MODE
    // --- 1. HARDWARE INIT ---
    // Sets D7 as Output via the Data Direction Register
    #define INIT_DIAGNOSTICS() (DDRD |= (1 << 7))

    // --- 2. THE MANTRA SWITCHES ---
    // D7 HIGH: Nano is focused on the NexStar Bus (Default)
    #define LISTENER_ON()      (PORTD |= (1 << 7)) 

    // D7 LOW: Nano has stepped out to siphon GPS data
    #define SIPHONER_ON()      (PORTD &= ~(1 << 7))

    // --- 3. COMPREHENSION PROBE ---
    /**
     * Creates a 50us "Notch" (High-Low-High) on the logic analyzer.
     * Use this when the Nano recognizes the 0xB0 Address.
     */
    inline void pulseComprehension() {
        PIND |= (1 << 7); // Flip state (High -> Low)
        delayMicroseconds(50); 
        PIND |= (1 << 7); // Flip state (Low -> High)
    }

#else
    // --- NULL DEFINITIONS ---
    // These ensure main.cpp doesn't crash when DIAGNOSTIC_MODE is off.
    #define INIT_DIAGNOSTICS() 
    #define LISTENER_ON()      
    #define SIPHONER_ON()      
    inline void pulseComprehension() {}
#endif

#endif // DIAGNOSTICS_H