#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "Hardware_Config.h"

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

void setupGPS() {
  // Most NEO-7M modules default to 9600 baud
  gpsSerial.begin(9600);
}

void processGPS() {
    while (gpsSerial.available() > 0)
    {gps.encode(gpsSerial.read());
        /* code */
    }
    
}

void displayGPS() {
  if (gps.location.isUpdated()) {
    Serial.print("LAT: "); Serial.println(gps.location.lat(), 6);
    Serial.print("LNG: "); Serial.println(gps.location.lng(), 6);
    Serial.print("SATS: "); Serial.println(gps.satellites.value());

//----- UTC Time -----
   Serial.print("UTC: ");
if (gps.time.isValid()) {
  if (gps.time.hour() < 10) Serial.print(F("0"));
  Serial.print(gps.time.hour());
  Serial.print(F(":"));
  
  // Use .minute() instead of .min()
  if (gps.time.minute() < 10) Serial.print(F("0"));
  Serial.print(gps.time.minute());
  Serial.print(F(":"));
  
  // Use .second() instead of .sec()
  if (gps.time.second() < 10) Serial.print(F("0"));
  Serial.print(gps.time.second());
  Serial.println();
} else {
  Serial.println("WAITING FOR TIME...");
}
}
}


bool checkGPS() {
  // This is the "Office Clerk" checking if there's mail
  if (gpsSerial.available() > 0) {
    return true;
  }
  return false;
}

#endif