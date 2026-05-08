#include <Arduino.h>
#include "Hardware_config.h"
#include "Diagnostics.h"
#include "ross.h"

// Initialize ross on the NexStar RX pin
ross nexSerial(NEX_RX_PIN);

void setup()
{
  Serial.begin(115200);
  nexSerial.begin(19200);

  // Use your macro to set D7 as output
  INIT_DIAGNOSTICS();
  SYNC_LOW(); // Ensure we start at 0V

  Serial.println(F("\n--- BATTING CAGE: Polarity & Preamble Test ---"));

  // PROBE: What does the Nano see when the bus is idle?
  delay(500);
  int idleState = digitalRead(NEX_RX_PIN);

  Serial.print(F("MOSFET IDLE CHECK: Pin is "));
  Serial.println(idleState == HIGH ? F("HIGH (Standard)") : F("LOW (Inverted)"));

  if (idleState == LOW)
  {
    Serial.println(F("WARNING: MOSFET is inverting. Standard UART logic will fail."));
  }

  Serial.println(F("LOCKED AND LOADED. Listening for 0x3B..."));
}

void loop()
{
  // Wait for the pin to move (Start Bit)
  if (digitalRead(NEX_RX_PIN) == HIGH)
  { // Assuming LOW is idle, HIGH is the start
    unsigned long start = micros();

    // Wait for it to flip back
    while (digitalRead(NEX_RX_PIN) == HIGH)
      ;
    unsigned long duration = micros() - start;

    // Report the width of the "swing"
    Serial.print(F("BIT_WIDTH: "));
    Serial.print(duration);
    Serial.println(F(" us"));

    // Visual strike on LA
    SYNC_HIGH();
    delayMicroseconds(100);
    SYNC_LOW();
  }
}