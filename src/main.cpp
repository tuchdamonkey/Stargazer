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
const unsigned long sipInterval = 5000; // 10-second "Siphon" rhythm

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
  LISTENER_ON(); // == DIAGNOSTIC TOOL ===D7 High: We boot into Listener-First posture
}

void loop()
{

  // --- 1. THE SIPHONER: 5s HEARTBEAT ---
  // We only step out to the GPS if the 10s timer has expired AND the bus is quiet.
  if (millis() - lastSip >= sipInterval && !negotiationActive)
  {
    // PRE-SIPHON DESK SWEEP:
    // If the bus is quiet but there's "ghost data" in the buffer, kill it now.
    while (nexSerial.available() > 0)
      (void)nexSerial.read();

    SIPHONER_ON();
    captureGpsBurst();

    // NEW: The Feeder Logic
    if (siloReady)
    {
      for (int i = 0; i < siloIndex; i++)
      {
        gps.encode(gpsSilo[i]); // Feed the TinyGPS engine
      }
      siloReady = false; // Reset for next time
      siloIndex = 0;     // Clear the index
    }

    LISTENER_ON();
    lastSip = millis();
  }

  // --- 2. THE LISTENER: Priority check for Mount commands ---
  processNexStar();

  // --- 3. THE TRANSLATOR: Update the 24-bit cache when GPS is fresh ---
  if (gps.location.isUpdated())
  {
    syncEventAnchor([]()
                    {
        // Translate Lat and store in the Lat bucket
        packNEXCoord(gps.location.lat(), nexPayload_Lat[0], nexPayload_Lat[1], nexPayload_Lat[2]);
        
        // Translate Lon and store in the Lon bucket
        packNEXCoord(gps.location.lng(), nexPayload_Lon[0], nexPayload_Lon[1], nexPayload_Lon[2]); });

    // PROOF OF CARRY: Log update status (Internal Diagnostics)
    static unsigned long lastProof = 0;
    if (millis() - lastProof > 10000)
    {
      Serial.print(F("PAYLOAD_VERIFIED | "));

      // Print Lat
      Serial.print(F("Lat: "));
      for (int i = 0; i < 3; i++)
        Serial.print(nexPayload_Lat[i], HEX);

      // Print UTC Time from TinyGPS++
      Serial.print(F(" | UTC: "));
      if (gps.time.isValid())
      {
        if (gps.time.hour() < 10)
          Serial.print(F("0"));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        if (gps.time.minute() < 10)
          Serial.print(F("0"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        if (gps.time.second() < 10)
          Serial.print(F("0"));
        Serial.print(gps.time.second());
      }
      else
      {
        Serial.print(F("WAITING_FOR_FIX"));
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
  LISTENER_ON(); // Ensure we are in the Mantra state during sensitive ops
  func();
  // We stay LISTENER_ON because that is our new default!
}