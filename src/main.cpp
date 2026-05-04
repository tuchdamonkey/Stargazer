#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "GPS_sg.h"
#include "NexStar_sg.h"
#include "AstroLogic.h"

// ===== GLOBAL OBJECTS =====
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial nexSerial(NEX_RX_PIN, NEX_TX_PIN);
TinyGPSPlus gps;

// ===== GLOBAL STATE =====
bool negotiationActive = false; // The v8.3.1 "Master Guard"
uint8_t packet[15];             // Coordinate storage container

// ===== PROTOTYPES =====
void debugPrintPacket();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("--- STARGAZER OFFICE IS OPEN ---"));

  // Start the NexStar line (19200) and GPS line (9600 with Muzzle)
  setupNexStar();
  setupGPS(); 

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
  delay(500);
  digitalWrite(STATUS_LED, LOW);
}

void loop() {
  displayGPS;
  // 1. PRIORITY ONE: The Handshake
  // Immediately sets negotiationActive = true if a mount query is detected.
  processNexStar(); 

  // 2. PRIORITY TWO: The Surgical Sip
  // Aborts instantly if negotiationActive is true, protecting mount timing.
  processGPS(); 

  // 3. PRIORITY THREE: Processing & UI
  // This gate protects the mission from any future "Heavy" tasks like OLED updates.
  if (gps.location.isUpdated() && !negotiationActive) {
    
    // Build the packet using AstroLogic
    buildNEXPacket(
        packet,
        gps.location.lat(),
        gps.location.lng(),
        gps.time.hour(),
        gps.time.minute(),
        gps.time.second());

    // Diagnostic Output
    debugPrintPacket();
    
    // Future expansion: updateOLED() would go here.
  }
}

// ===== DIAGNOSTICS =====
void debugPrintPacket() {
  Serial.print(F("NEX Output: "));
  for (int i = 0; i < 15; i++) {
    if (packet[i] < 0x10) Serial.print("0"); 
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}