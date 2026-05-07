#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "AstroLogic.h"
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

  // 2. THE 10s PORTHOLE (The Safe Zone)
  if (millis() - lastCarryTime >= sipInterval)
  {
    SYNC_HIGH(); // D7 Pulse: Start Processing

    Serial.println(F("\n--- [STAGE 1.5: TRANSLATION] ---"));

    if (siloReady)
    {
      // 1. Locate the RMC sentence within the silo
      char *rmc = strstr(gpsSilo, "$GPRMC");

      if (rmc)
      {
        // 2. Slice the fields
        const char *latStr = findField(rmc, 3);
        const char *latDir = findField(rmc, 4);
        const char *lonStr = findField(rmc, 5);
        const char *lonDir = findField(rmc, 6);

        // 3. Translate to Decimal Degrees
        if (latStr && latDir && lonStr && lonDir)
        {
          float currentLat = convertNMEAToDecimal(latStr, latDir[0]);
          float currentLon = convertNMEAToDecimal(lonStr, lonDir[0]);

          // 4. Output results
          Serial.print(F("GPS STATUS: VALIDATED\n"));
          Serial.print(F("LAT: "));
          Serial.println(currentLat, 6);
          Serial.print(F("LON: "));
          Serial.println(currentLon, 6);

          // (Optional) Diagnostic: Print the raw coordinates we grabbed
          // Serial.print(F("RAW: ")); Serial.println(latStr);
        }
      }
    }
    else
    {
      Serial.println(F("STATUS: SILO INCOMPLETE/CHECKSUM FAIL"));
    }

    lastCarryTime = millis();
    SYNC_LOW(); // D7 Pulse: End Processing
  }
}