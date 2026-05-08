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

void setup() {
  Serial.begin(115200);
  nexSerial.begin(9600); // The Hack: 104us per bit
  INIT_DIAGNOSTICS();
  SYNC_LOW();
  
  Serial.println(F("\n--- THE 9600 BAUD HAIL MARY ---"));
  Serial.println(F("Listening for Inverted 0x3B at half-speed..."));
}

void loop() {
  if (nexSerial.available() > 0) {
    // Mirror the bits because of the MOSFET inversion
    uint8_t incoming = ~nexSerial.read(); 

    if (incoming == 0x3B) {
      SYNC_HIGH();
      delay(50);
      SYNC_LOW();
      Serial.println(F("HACK STRIKE! Captured 0x3B at 9600 logic."));
    }
  }
}