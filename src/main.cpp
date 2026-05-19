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
volatile SystemState currentState = STATE_NEX_LISTENING;
volatile int bufIndex = 0;
volatile bool packetReady = false;
char goldenPacket[85];

// --- System State Flags ---
bool muzzleActive = false;
bool negotiationActive = false;
unsigned long lastSip = 0;
const int sipInterval = 3000;

ross nexTalker(NEX_TX_PIN, false);
soss nexSerial(NEX_RX_PIN, false);

// --- Function Prototypes ---
void manageSystemState();

void setup()
{
  pinMode(NEX_RX_PIN, OUTPUT);
  digitalWrite(NEX_RX_PIN, HIGH);
  pinMode(NEX_TX_PIN, INPUT);

  Serial.begin(115200);
  nexSerial.begin(19200);
  nexTalker.begin(19200);

  setupGPS();
  setupNexStar();
  Serial.println(F("--- StarGazer v1.0: Ross/Soss Stage 1 ---"));

  INIT_DIAGNOSTICS();
}

void loop()
{
  // 1. THE PRIORITY LISTENER: Always check the telescope mount first with absolute zero latency
  processNexStar();

  // 2. THE PASSPORT CONTROL
  // --- System state management ---
  manageSystemState();

  // --- GPS management ---

  if (isGpsPermissionGranted())
  {
    captureGpsBurst();
  }

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

/**
 * @brief Background execution engine that manages microcontroller ownership.
 * Handles the "Do Not Disturb" state logic outside of the main loop.
 */
void manageSystemState()
{
  // RAW HARDWARE WIRE-TRAP: If the NexStar line physically drops LOW (Start Bit),
  // instantly force the system into ENGAGED state before software parsing even begins.
  if (digitalRead(NEX_RX_PIN) == LOW)
  {
    currentState = STATE_NEX_ENGAGED;
    lastNexActivity = millis();
  }

  // Evaluate the current state rules
  switch (currentState)
  {
  case STATE_NEX_LISTENING:
    // If bytes arrive via the software buffer, instantly lock down
    if (nexTalker.available() > 0)
    {
      currentState = STATE_NEX_ENGAGED;
      lastNexActivity = millis();
    }
    break;

  case STATE_NEX_ENGAGED:
    // Check the clock: Have we reached a verified 500ms of absolute silence?
    if (millis() - lastNexActivity >= nexSilenceWindow)
    {
      currentState = STATE_GPS_SIPHON; // Safety window cleared. Lease permission to GPS.
    }
    break;

  case STATE_GPS_SIPHON:
    // Run the live distraction
    captureGpsBurst();

    // Immediate eviction check: If a byte sneaks into the buffer during the siphon,
    // instantly strip ownership away from the GPS and lock the door.
    if (nexTalker.available() > 0)
    {
      currentState = STATE_NEX_ENGAGED;
      lastNexActivity = millis();
    }
    break;
  }
}