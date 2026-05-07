#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "custom_serial/ross.h"
#include "AstroLogic.h"
#include "Diagnostics.h"
#include "GPS_sg.h"
#include "NexStar_sg.h"

// --- Bridge-Guard Physical Memory Definitions ---
volatile SystemState currentState = STATE_IDLE;
volatile int bufIndex = 0;
volatile bool packetReady = false;
char goldenPacket[85];

// --- System State Flags ---
bool muzzleActive = true;
bool negotiationActive = false;
unsigned long lastSip = 0;
const int sipInterval = 3000;

custom_serial nexSerial(NEX_RX_PIN, NEX_TX_PIN);

void setup()
{
  Serial.begin(115200);
  setupGPS();
  Serial.println(F("--- StarGazer v1.0: Ross/Soss Stage 1 ---"));

  INIT_DIAGNOSTICS();
}

void loop()
{
  // 1. ATOMIC CAPTURE
  captureGpsBurst();

  // 2. THE 3s SIP (The Safe Zone)
  if (millis() - lastCarryTime >= sipInterval)
  {
    SYNC_HIGH(); // D7 Pulse: Start Processing

    Serial.println(F("\n--- [STAGE 2: NEX TRANSLATION] ---"));

    if (siloReady)
    {
      char *rmc = strstr(gpsSilo, "$GPRMC");

      if (rmc)
      {
        const char *latStr = findField(rmc, 3);
        const char *latDir = findField(rmc, 4);
        const char *lonStr = findField(rmc, 5);
        const char *lonDir = findField(rmc, 6);

        if (latStr && latDir && lonStr && lonDir)
        {
          float currentLat = convertNMEAToDecimal(latStr, latDir[0]);
          float currentLon = convertNMEAToDecimal(lonStr, lonDir[0]);

          // --- [START STAGE 2 INTEGRATION] ---
          // Convert our validated floats into the 24-bit NexStar payload
          translateAndPack(currentLat, false); // Process Latitude
          translateAndPack(currentLon, true);  // Process Longitude
          // --- [END STAGE 2 INTEGRATION] ---

          Serial.print(F("GPS STATUS: VALIDATED\n"));
          Serial.print(F("LAT: "));
          Serial.println(currentLat, 6);
          Serial.print(F("LON: "));
          Serial.println(currentLon, 6);
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