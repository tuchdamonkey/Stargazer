#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "GPS_sg.h"
#include "NexStar_sg.h"
#include "AstroLogic.h"
#include <avr/wdt.h>

// ===== GLOBAL OBJECTS =====
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial nexSerial(NEX_RX_PIN, NEX_TX_PIN);
TinyGPSPlus gps;

// ===== GLOBAL STATE =====
bool negotiationActive = false; // The v8.3.1 "Master Guard"
uint8_t packet[15];             // Coordinate storage container

// ===== PROTOTYPES =====
void debugPrintPacket();

void setup()
{
  wdt_disable(); // Stay calm during boot

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

  initSyncPin(); // (de)activate via diagnostics.h
}

void loop()
{
  // --- STEP 1: THE PRIMARY EAR ---
  // Ensure the Nano is listening to the NexStar bus[cite: 14, 18].
  // Without this, nexSerial.available() will always be 0.
  nexSerial.listen();

  // --- STEP 2: PRIORITY ONE - THE HANDSHAKE ---
  // Immediately check for the 0x3B Preamble while nexSerial is active.
  processNexStar();

  // --- STEP 3: THE SURGICAL SIP ---
  // Only divert attention to GPS if the mount bus is silent and no conversation
  // is currently active.
  if (!negotiationActive && nexSerial.available() == 0)
  {
    gpsSerial.listen(); // Switch interrupt to GPS pins[cite: 14]

    // Process a small chunk of GPS data[cite: 14]
    processGPS();

    // IMPORTANT: Return to NexStar immediately to minimize the blind spot[cite: 18]
    nexSerial.listen();
  }

  // --- STEP 4: BACKGROUND PROCESSING ---
  displayGPS(); // Minimal serial output[cite: 14, 17]

  if (gps.location.isUpdated() && !negotiationActive)
  {
    buildNEXPacket(
        packet,
        gps.location.lat(),
        gps.location.lng(),
        gps.time.hour(),
        gps.time.minute(),
        gps.time.second());

    debugPrintPacket();
  }
}
// ===== DIAGNOSTICS =====
void debugPrintPacket()
{
  Serial.print(F("NEX Output: "));
  for (int i = 0; i < 15; i++)
  {
    if (packet[i] < 0x10)
      Serial.print("0");
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}