#include <Arduino.h>
#include <TinyGPS++.h>
#include "ross.h"
#include "soss.h"
#include "Hardware_config.h"
#include "AstroLogic.h"
#include "Diagnostics.h"
#include "GPS_sg.h"
#include "NexStar_sg.h"

TinyGPSPlus gps;

// --- Bridge-Guard Physical Memory Definitions ---
volatile SystemState currentState = STATE_IDLE;
volatile int bufIndex = 0;
volatile bool packetReady = false;
char goldenPacket[85];

// --- System State Flags ---
bool muzzleActive = true;
bool negotiationActive = false;
unsigned long lastSip = 0;
const int sipInterval = 3000;

soss nexSerial(NEX_RX_PIN, false);
ross nexTalker(NEX_TX_PIN, true);

void setup()
{
  pinMode(NEX_RX_PIN, OUTPUT);
  digitalWrite(NEX_RX_PIN, HIGH);
  pinMode(NEX_TX_PIN, INPUT);

  Serial.begin(115200);
  nexTalker.begin(19200);
  nexSerial.begin(19200);

  // nexTalker.begin(19200); // If soss has a begin
  setupGPS();
  Serial.println(F("--- StarGazer v1.0: Ross/Soss Stage 1 ---"));

  INIT_DIAGNOSTICS();
}

void loop()
{
  // 1. THE SIPHONER: Check for 1 byte of GPS, then yield.
  captureGpsBurst();

  // 2. THE LISTENER: Priority check for Mount commands.
  processNexStar();

  // 3. THE TRANSLATOR: Update the 24-bit cache when GPS is fresh.
  if (gps.location.isUpdated())
  {
    // Wrapped to show the "Thinking State" duration on D7
    syncEventAnchor([]()
                    {
        translateAndPack(gps.location.lat(), false);
        translateAndPack(gps.location.lng(), true); });

    // PROOF OF CARRY: Verified every 10 seconds to keep the bus clear.
    static unsigned long lastProof = 0;
    if (millis() - lastProof > 10000)
    {
      Serial.print(F("PAYLOAD_VERIFIED: "));
      for (int i = 0; i < 3; i++)
      {
        if (nexPayload[i] < 0x10)
          Serial.print('0');
        Serial.print(nexPayload[i], HEX);
      }
      Serial.println();
      lastProof = millis();
    }
  }

  // HEARTBEAT: Proof of loop stability
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000)
  {
    Serial.println(F("BRAIN_CHECK: Looping..."));
    lastHeartbeat = millis();
  }
}

//==================================================

// main.cpp

/**
 * @brief The Atomic Wrap: Ensures D7 is strictly a "Thinking State" indicator.
 * Snap HIGH, execute, snap LOW. No exceptions.
 */
void syncEventAnchor(void (*func)())
{
  if (func != nullptr)
  {
    // Diagnostics Overhaul: Bypassed SYNC_HIGH() and SYNC_LOW() macros
    // to leave the D7 physical pin completely inert on the logic analyzer.
    func(); // Execute NexStar Response directly
  }
}