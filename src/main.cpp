#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Hardware_Config.h"
#include "GPS_sg.h"
#include "AstroLogic.h"

// ===== DEFINITIONS =====
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial nexSerial(NEX_RX_PIN, NEX_TX_PIN);

uint8_t packet[15]; // 15byte storage container for astrologic

// ===== GLOBAL VARIABLES =====

// ===== PROTOTYPES =====
void debugPrintPacket(); // This tells the compiler the function exists at the bottom

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("--- STARGAZER OFFICE IS OPEN ---");

  gpsSerial.begin(9600);
  nexSerial.begin(9600); // Ready for the future handshake

  setupGPS();
  pinMode(STATUS_LED, OUTPUT);

  // Quick flash to show the "Office" is powered on
  digitalWrite(STATUS_LED, HIGH);
  delay(500);
  digitalWrite(STATUS_LED, LOW);
}
void loop()
{
  // Standard GPS feeding logic
  while (gpsSerial.available() > 0)
  {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated())
  {
    buildNEXPacket(
        packet,
        gps.location.lat(),
        gps.location.lng(),
        gps.time.hour(),
        gps.time.minute(),
        gps.time.second());

    // SQUASHED ERROR: Serial1 is now nexSerial
    nexSerial.write(packet, 15);

    // This is our current priority:
    debugPrintPacket();
  }
}

// ===== Diagnostics =====

void debugPrintPacket()
{
  Serial.print("NEX Packet: ");
  for (int i = 0; i < 15; i++)
  {
    if (packet[i] < 0x10)
      Serial.print("0"); // Add leading zero for clean alignment
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println(); // New line for the next update
}