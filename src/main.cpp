#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "GPS_sg.h"

// --- THE FIX: DEFINITIONS (No 'extern' here) ---
TinyGPSPlus gps;          // The physical parser object
bool muzzleActive = true; // The physical state flag
bool negotiationActive = false; 

unsigned long lastSip = 0;
const int sipInterval = 3000; 

void setup() {
  Serial.begin(115200); 
  setupGPS();
  Serial.println(F("--- StarGazer v1.0: Ross/Soss Stage 1 ---"));
}

void loop() {
  if (millis() - lastSip >= sipInterval) {
    muzzleGPS(false); 
    Serial.println(F("[GATE OPEN - SEARCHING...]"));

    sipGPS(80); 

    muzzleGPS(true); 
    Serial.println(F("[GATE CLOSED]"));

    lastSip = millis();
  }
}