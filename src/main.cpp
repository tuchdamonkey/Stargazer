#include <Arduino.h>
#include "Hardware_Config.h" 
#include "GPS_sg.h"

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
 processGPS(); // This "encodes" the raw lines into useful data
  
  // Only print to the screen once every second so we can read it
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    displayGPS(); 
    lastPrint = millis();
  }
}