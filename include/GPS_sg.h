#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_Config.h"

// We'll keep these variables for the rest of the system to see
extern bool negotiationActive;

// The Ross/Soss "Gatekeeper"
bool muzzleActive = true;

void setupGPS()
{
  // We define the pin as an input, but we don't attach a library to it.
  pinMode(GPS_RX_PIN, INPUT);
  muzzleActive = true;
}

void muzzleGPS(bool closed)
{
  muzzleActive = closed;
}

// This is the "Ross/Soss" engine for verification
void sipGPS(uint32_t durationMs)
{
  // If the muzzle is on, we don't even look at the pin.
  // This is the 'Silence' that protects the NexStar handshake.
  if (muzzleActive)
    return;

  uint32_t startTime = millis();

  // While our "sip" window is open...
  while (millis() - startTime < durationMs)
  {
    // We read the RAW logic state of the pin.
    // 0 (LOW) = Start bit or Data. 1 (HIGH) = Idle.
    if (digitalRead(GPS_RX_PIN) == LOW)
    {
      Serial.print("0");
    }
    else
    {
      Serial.print("-"); // Using a dash for '1' makes it easier to see data pulses
    }
  }
  Serial.println(); // Sip complete
}

#endif