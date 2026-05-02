#include <Arduino.h>
#include "Hardware_Config.h" 
#include "GPS_sg.h"
#include "AstroLogic.h"

// ===== GLOBAL VARIABLES =====
uint8_t packet[15]; // 15byte storage container for astrologic


void setup() {

  Serial.begin(115200);
  delay(2000); // Give the Serial Monitor time to "catch" the port
  Serial.println("--- STARGAZER OFFICE IS OPEN ---");
  setupGPS();

  // Initialize the Status LED
  pinMode(STATUS_LED, OUTPUT);
  
  // Quick flash to show the "Office" is powered on
  digitalWrite(STATUS_LED, HIGH);
  delay(500);
  digitalWrite(STATUS_LED, LOW);
}
void loop() {
    // 1. Check if GPS has new data
    if (gps.location.isUpdated()) {
        
        // 2. Call the builder (The Execution)
        buildNEXPacket(packet, gps.location.lat(), gps.location.lng(), hour, minute, second);
        
        // 3. Send the packet to the telescope
        Serial1.write(packet, 15); 
        
        // 4. Debug: See it in the Serial Monitor
        debugPrintPacket(); 
    }
}