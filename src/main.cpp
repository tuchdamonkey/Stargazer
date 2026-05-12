#include <Arduino.h>
#include <TinyGPS++.h>
#include "ross.h"
#include "soss.h"
#include "Hardware_config.h"
#include "AstroLogic.h"
#include "Diagnostics.h"
#include "GPS_sg.h"
#include "NexStar_sg.h"

//================================================================================================================
// NOTE: See HEAD: "STABLE TX RX - voidprocessNexstar stable, rectified TX polarity" for code prior to GPS MUZZLE!!
//================================================================================================================


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

// --- Temporary Diagnostic Muzzle ---
// Set to true to suspend all GPS background activity for Task 2.1 testing.
bool gpsMuzzle = true;

// --- System State Flags ---
bool muzzleActive = false;
bool negotiationActive = false;
unsigned long lastSip = 0;
const unsigned long sipInterval = 5000; // 10-second "Siphon" rhythm

ross nexSerial(NEX_TX_PIN, false);
soss nexTalker(NEX_RX_PIN, false);


void setup()
{
  Serial.begin(115200);
  
  // Ross/Soss Initialization
  nexSerial.begin(19200);
  nexTalker.begin(19200);

  // Muzzle Check: Only setup GPS if muzzle is false
  if (!gpsMuzzle)
  {
    setupGPS();
  }

  Serial.println(F("--- StarGazer v1.0: Ross/Soss Stage 1 ---"));

  INIT_DIAGNOSTICS();
  
  // FORCE PRIORITY: Ensure ross is the active SoftwareSerial listener
  nexSerial.listen();
  LISTENER_ON(); 
}

void loop()
{
  // 1. THE LISTENER: Priority check for Mount commands
  processNexStar();

  // 2. THE TRANSLATOR: Only runs if muzzle is OFF
  if (!gpsMuzzle && gps.location.isUpdated())
  {
    syncEventAnchor([]()
                    {
        packNEXCoord(gps.location.lat(), nexPayload_Lat[0], nexPayload_Lat[1], nexPayload_Lat[2]);
        packNEXCoord(gps.location.lng(), nexPayload_Lon[0], nexPayload_Lon[1], nexPayload_Lon[2]); 
    });
  }

  // HEARTBEAT
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