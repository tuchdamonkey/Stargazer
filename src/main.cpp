#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "Diagnostics.h"
#include "GPS_sg.h"

// --- Bridge-Guard Physical Memory Definitions ---
// This is where the 'extern' promises from the headers are fulfilled.
volatile SystemState currentState = STATE_IDLE;
volatile int bufIndex = 0;
volatile bool packetReady = false;
char goldenPacket[85];

// --- System State Flags ---
bool muzzleActive = true;
bool negotiationActive = false;
unsigned long lastSip = 0;
const int sipInterval = 3000;

// Note: TinyGPS++ is actually not needed for this "Bridge-Guard"
// because we are relaying the raw "Golden Packet" strings directly.

void setup()
{
  // Use 115200 for the Serial Monitor so it doesn't slow down the Nano
  Serial.begin(115200);
  setupGPS();
  Serial.println(F("--- StarGazer v1.0: Ross/Soss Stage 1 ---"));

  INIT_DIAGNOSTICS();
}

void loop()
{
  // 1. ATOMIC CAPTURE
  captureGpsBurst();

  // 2. THE 10s PORTHOLE
  if (siloReady && (millis() - lastCarryTime >= carryInterval))
  {
    SYNC_HIGH(); // This is your "Mark Carry Start"

    Serial.println(F("--- [VALIDATED GPS DATA] ---"));
    Serial.print(gpsSilo);
    Serial.println(F("----------------------------"));

    lastCarryTime = millis();

    SYNC_LOW(); // This is your "Mark Carry End"
  }
}