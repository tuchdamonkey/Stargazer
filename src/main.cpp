#include <Arduino.h>
#include "Hardware_config.h"
#include "GPS_sg.h"

bool negotiationActive = false; // Start with GPS focus
unsigned long lastSip = 0;
const int sipInterval = 3000; // Sip every 3 seconds

void setup()
{
  Serial.begin(115200); // Speed this up!
  setupGPS();
  Serial.println("--- StarGazer v1.0: Ross/Soss Stage 1 ---");
}

void loop()
{
  // Stage 1 Agenda: Verify the GPS 'Ticking'
  if (millis() - lastSip >= sipInterval)
  {

    Serial.println("[GATE OPEN]");
    muzzleGPS(false); // Open the ear

    sipGPS(200); // 200ms Sip (enough for a chunk of NMEA)

    muzzleGPS(true); // Close the ear
    Serial.println("[GATE CLOSED]");

    lastSip = millis();
  }
}