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
void toggleDiagnostic()
{
  PIND |= (1 << 7);
}

// Inform the compiler that our clock exists inside NexStar_sg.h
extern unsigned long lastNexActivity;

void syncEventAnchor(void (*func)())
{
  if (func != nullptr)
  {
    SYNC_LOW();  // Drop D7: Entering Protected / Active Priority State
    func();      // Execute the task (e.g., processNexStar)
    SYNC_HIGH(); // Raise D7: Returning to Silent / Open State

    // External tracking assignment
    lastNexActivity = millis();
  }
}
#endif
