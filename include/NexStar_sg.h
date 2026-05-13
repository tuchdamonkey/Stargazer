#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include "Hardware_config.h"
#include "ross.h"
#include "soss.h"
#include "AstroLogic.h"
#include "Diagnostics.h"

extern ross nexSerial;
extern soss nexTalker;
extern bool negotiationActive;
extern void syncEventAnchor(void (*func)());

// --- ADJUSTABLE TUNING KNOBS ---
#define NEX_TIMEOUT_MS 50
#define NEX_SILENCE_GAP 500

#define CAPTURE_SIZE 12
volatile uint8_t captureBin[CAPTURE_SIZE]; // Storage for raw NexStar bytes
volatile uint8_t captureCount = 0;         // Index for the bin
bool reportPending = false;                // Trigger for the D7 dump

unsigned long lastNexByteTime = 0;

const uint8_t PREAMBLE = 0x3B;
const uint8_t ADDR_GPS = 0xB0;
const uint8_t ADDR_HC = 0x0D;

const uint8_t CMD_GET_VER = 0xFE;
const uint8_t CMD_GET_LOC = 0x01;
const uint8_t CMD_GET_TIME = 0x03;

// --- THE ATOMIC CACHE ---
// These buffers hold the pre-baked 24-bit NexStar coordinates.
// Stage 2 (AstroLogic) fills them; Stage 3 (NexStar_sg) serves them.
uint8_t nexPayload_Lat[3] = {0, 0, 0};
uint8_t nexPayload_Lon[3] = {0, 0, 0};
uint8_t nexPayload_Date[4] = {0, 0, 0, 0};
uint8_t nexPayload_Time[3] = {0, 0, 0};

void sendNexPacket(uint8_t *p, uint8_t len)
{
    nexTalker.write(PREAMBLE);
    // Write the payload
    for (uint8_t i = 0; i < len; i++)
    {
        nexTalker.write(p[i]);
    }
    // Calculate checksum using AstroLogic's brain
    // We add 2 to len because p doesn't include Preamble or the Checksum itself
    uint8_t fullPacket[len + 2];
    fullPacket[0] = PREAMBLE;
    memcpy(&fullPacket[1], p, len);

    uint8_t chk = calculateNEXChecksum(fullPacket, len + 2);
    nexTalker.write(chk);
}

// 1. The Helper (9600 Baud Bit-Banger)
void d7SerialWrite(uint8_t b) {
  uint16_t bitDelay = 104; 
  digitalWrite(7, LOW); // Start Bit
  delayMicroseconds(bitDelay);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(7, (b & (1 << i)) ? HIGH : LOW);
    delayMicroseconds(bitDelay);
  }
  digitalWrite(7, HIGH); // Stop Bit
  delayMicroseconds(bitDelay);
}

// 2. The Reporter
void performD7Dump() {
  delay(10); 
  for (uint8_t i = 0; i < captureCount; i++) {
    d7SerialWrite(captureBin[i]);
  }
  captureCount = 0;
  reportPending = false;
  LISTENER_ON(); // Return D7 to HIGH
}

// 3. The Siphoner
volatile uint32_t transitionCount = 0;

void processNexStar() {
  bool currentState = digitalRead(NEX_RX_PIN);
  static bool lastState = HIGH;

  // Count every single time the pin moves
  if (currentState != lastState) {
    transitionCount++;
    lastState = currentState;
    
    if (!reportPending) {
        SIPHONER_ON(); // Drop D7 on the very first twitch
        reportPending = true;
    }
  }

  // After 500ms of no activity, we'll use our D7 bit-bang 
  // to "tell" you the transition count.
  if (reportPending && (millis() - lastNexByteTime > 500)) {
     // For this test, we won't dump hex, 
     // we just want to see if D7 returns to HIGH.
     LISTENER_ON();
     reportPending = false;
     transitionCount = 0;
  }
}



#endif