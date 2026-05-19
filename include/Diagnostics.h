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
#define SYNC_HIGH() (PORTD |= (1 << 7))
#define SYNC_LOW() (PORTD &= ~(1 << 7))

// Fast Toggle: Writing to the PIN register toggles the state
void toggleDiagnostic()
{
  PIND |= (1 << 7);
}

// Inform the diagnostic compiler that our clock and window exist
extern unsigned long lastNexActivity;
extern const unsigned long nexSilenceWindow;

// Use your custom class types matching main.cpp definitions
class ross;
extern ross nexTalker;

/**
 * @brief Evaluates whether the system has leased execution time to the GPS.
 * @return true if the state machine is explicitly in the GPS Siphon state.
 */
bool isGpsPermissionGranted()
{
  // The gate opens ONLY when the background engine confirms we are in siphon mode
  return (currentState == STATE_GPS_SIPHON);
}

// Keep the syncEventAnchor intact for translation tracking
void syncEventAnchor(void (*func)())
{
  if (func != nullptr)
  {
    SYNC_LOW();
    func();
    SYNC_HIGH();
    lastNexActivity = millis();
  }
}

#endif