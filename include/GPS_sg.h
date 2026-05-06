#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_Config.h"

// Managed in NexStar_sgh.h or main.cpp
extern bool negotiationActive;

// Instances defined in main.cpp
extern TinyGPSPlus gps;

void muzzleGPS()
{
  // v8.3.1 Strategy: Disable unnecessary NMEA sentences at the hardware level
  // This reduces CPU load and prevents buffer overflows
  gpsSerial.println(F("$PUBX,40,GLL,0,0,0,0*5C"));
  gpsSerial.println(F("$PUBX,40,VTG,0,0,0,0*5E"));
  gpsSerial.println(F("$PUBX,40,GSV,0,0,0,0*59"));
  gpsSerial.println(F("$PUBX,40,GSA,0,0,0,0*4E"));
}

void setupGPS()
{
  gpsSerial.begin(9600); // 9600 from Hardware_Config.h
  muzzleGPS();           // Apply the "Mission Muzzle"
}

void processGPS()
{
  // THE SURGICAL SIP: Only process GPS if we aren't in a NexStar handshake[cite: 1]
  if (negotiationActive)
    return;

  while (gpsSerial.available() > 0)
  {
    gps.encode(gpsSerial.read());
  }
}

void displayGPS()
{
  // Only show/log coordinates if the data is fresh (under 2 seconds)[cite: 1]
  if (gps.location.isValid() && gps.location.age() < 2000)
  {
    Serial.print(F("LAT: "));
    Serial.println(gps.location.lat(), 6);
    Serial.print(F("LON: "));
    Serial.println(gps.location.lng(), 6);
  }
  else if (gps.location.age() > 5000)
  {
    Serial.println(F("STALE GPS DATA - WAITING FOR FIX"));
  }
}

bool checkGPS()
{
  return (gpsSerial.available() > 0);
}

#endif