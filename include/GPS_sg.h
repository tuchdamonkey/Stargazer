#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "Hardware_Config.h"

TinyGPSPlus gps;
extern SoftwareSerial gpsSerial;

void setupGPS()
{
  // Most NEO-7M modules default to 9600 baud
  gpsSerial.begin(9600);
}

void processGPS()
{
  while (gpsSerial.available() > 0)
  {
    gps.encode(gpsSerial.read());
    /* code */
  }
}

void displayGPS()
{
  // Check if location is valid AND if the data is less than 2 seconds old
  if (gps.location.isValid() && gps.location.age() < 2000)
  {
    Serial.print("LAT: ");
    Serial.println(gps.location.lat(), 6);
    // ... rest of your display code ...
  }
  else if (gps.location.age() > 5000)
  {
    // If it's been more than 5 seconds, let's signal a warning
    Serial.println("STALE GPS DATA - CHECK SIGNAL");
  }
}

bool checkGPS()
{
  // This is the "Office Clerk" checking if there's mail
  if (gpsSerial.available() > 0)
  {
    return true;
  }
  return false;
}

#endif