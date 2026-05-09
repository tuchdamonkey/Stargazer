#include <Arduino.h>
#include <TinyGPS++.h>
#include "ross.h"
#include "soss.h"
#include "Hardware_config.h"
#include "AstroLogic.h"
#include "Diagnostics.h"
#include "GPS_sg.h"
#include "NexStar_sg.h"

extern uint8_t nexPayload_Lat[3];
extern uint8_t nexPayload_Lon[3];
extern uint8_t nexPayload_Date[4];
extern uint8_t nexPayload_Time[3];

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

ross nexSerial(NEX_TX_PIN, true);
soss nexTalker(NEX_RX_PIN, true);

void setup()
{
  Serial.begin(115200);
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
    syncEventAnchor([]()
                    {
        // Translate Lat and store in the Lat bucket
        packNEXCoord(gps.location.lat(), nexPayload_Lat[0], nexPayload_Lat[1], nexPayload_Lat[2]);
        
        // Translate Lon and store in the Lon bucket
        packNEXCoord(gps.location.lng(), nexPayload_Lon[0], nexPayload_Lon[1], nexPayload_Lon[2]); });

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
    SYNC_HIGH(); // Enter Locked State
    func();      // Execute NexStar Response
    SYNC_LOW();  // Exit Locked State
  }
}