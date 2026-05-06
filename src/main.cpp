#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "GPS_sg.h"

// --- Bridge-Guard Physical Memory Definitions ---
// This is where the 'extern' promises from the headers are fulfilled.
volatile SystemState currentState = STATE_IDLE;
volatile int bufIndex = 0;
volatile bool packetReady = false;
char goldenPacket[85];

// --- System State Flags ---
bool muzzleActive = true;
bool negotiationActive = false;
unsigned long lastSip = 0;
const int sipInterval = 3000;

// Note: TinyGPS++ is actually not needed for this "Bridge-Guard"
// because we are relaying the raw "Golden Packet" strings directly.

void setup()
{
  // Use 115200 for the Serial Monitor so it doesn't slow down the Nano
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
    break;

  case STATE_ACQUIRE:
  {
    // Precision bit-bang read
    char c = readRossByte();
    processGPSByte(c); // Match the name in GPS_sg.h
    break;
  }

  case STATE_VALIDATE:
    // Checksum logic can be added to GPS_sg.h later
    // For now, we assume if it hit '\n', it's a good packet.
    packetReady = true;
    currentState = STATE_RELAY;
    break;

  case STATE_RELAY:
    // This is where NexStar_sg.h will plug in.
    // For the first test, we just print to the PC.
    Serial.print(F("Relaying: "));
    Serial.println(goldenPacket);

    packetReady = false;
    currentState = STATE_IDLE;
    break;

  default:
    currentState = STATE_IDLE;
    break;
  }
}