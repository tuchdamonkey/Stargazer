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

void setup()
{
  Serial.begin(115200);
  setupGPS();
  Serial.println(F("--- StarGazer v1.0: Ross/Soss Stage 1 ---"));
}

void loop()
{
  switch (currentState)
  {

  case STATE_IDLE:
    // The Nano is chilling. PCINT is watching the pin.
    // This is where you'd put your "modest UI" code later.
    break;

  case STATE_ACQUIRE:
    // A start bit was detected! Now we perform one
    // precision bit-bang read to get the character.
    char c = readRossByte();
    processIncomingByte(c);
    break;

  case STATE_VALIDATE:
    if (verifyChecksum(goldenPacket))
    {
      packetReady = true;
      currentState = STATE_RELAY;
    }
    else
    {
      currentState = STATE_IDLE;
    }
    break;

  case STATE_RELAY:
    if (nexStarIsReady())
    {
      relayToNexStar(goldenPacket);
      packetReady = false;
      currentState = STATE_IDLE;
    }
    break;
  }
}