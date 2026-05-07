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

ross nexSerial(NEX_RX_PIN);
soss nexTalker(NEX_TX_PIN);

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
  // 1. HIGHEST PRIORITY: Constant Background Siphon
  captureGpsBurst();

  // HEARTBEAT: Confirms the Nano is looping even without GPS lock
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000)
  {
    Serial.println(F("BRAIN_CHECK: Looping..."));
    lastHeartbeat = millis();
  }

  // 2. THE EVENT TRIGGER: Only work when TinyGPS++ has a full lock
  if (gps.location.isUpdated())
  {
    SYNC_HIGH(); // D7 HIGH: Nano is now "Thinking"

    // Perform the silent 24-bit math
    translateAndPack(gps.location.lat(), false);
    translateAndPack(gps.location.lng(), true);

    // 3. THE STAND-IN REPORT
    Serial.print(F("NEX_READY: "));
    for (int i = 0; i < 3; i++)
    {
      if (nexPayload[i] < 0x10)
        Serial.print('0');
      Serial.print(nexPayload[i], HEX);
    }
    Serial.println();

    SYNC_LOW(); // D7 LOW: Translation complete
  }

  // 4. AUX BUS LISTENER: Ready to serve the cache
  processNexStar();
} // <--- This was the "wall" causing the errors!

// --- The Linker's Bridge ---
void syncEventAnchor(void (*func)())
{
  if (func != nullptr)
  {
    SYNC_HIGH(); // Force High at the start of the bridge
    func();      // Execute the NexStar packet send (the lambda)
    SYNC_LOW();  // Force Low the MOMENT the work is done
  }
}

//==================================================

void syncEventAnchor(void (*func)())
{
  if (func != nullptr)
  {
    SYNC_HIGH(); // Force High at the start of the bridge
    func();      // Execute the NexStar packet send
    SYNC_LOW();  // Force Low the MOMENT the work is done
  }
}