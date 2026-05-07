#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <Arduino.h>

/**
 * Diagnostics.h
 *
 * D7 is the SYNC pin (Hardwired to Diagnostic Port).
 * This pin is Bit 7 of Port D.
 */

// Initialize D7 as output by setting Bit 7 of the Data Direction Register
#define INIT_DIAGNOSTICS() (DDRD |= (1 << 7))

// Direct Port Manipulation for zero-latency toggling
#define SYNC_HIGH() (PORTD |= (1 << 7)) // low-level equivalent of pinMode(7, OUTPUT)
#define SYNC_LOW() (PORTD &= ~(1 << 7)) // low-level equivalent of digitalWrite(7, HIGH)


// Fast Toggle: Writing to the PIN register toggles the state
void toggleDiagnostic() {
    PIND |= (1 << 7); 
}
#endif