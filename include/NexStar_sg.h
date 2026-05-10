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

void processNexStar()
{
    // --- MANTRA CHECK: DEFAULT TO LISTENER ---
    // D7 stays HIGH here. The ross::recv() interrupt is always armed.
    LISTENER_ON();

    // 1. DATA TRACKING
    if (nexSerial.available() > 0)
    {
        lastNexByteTime = millis();
    }

    // 2. THE HANDSHAKE CUE (Simplified)
    // We only attempt to parse when we haven't heard a byte for a few milliseconds.
    // This ensures we aren't "thinking" while the telescope is still "speaking."
    if (nexSerial.available() > 0 && (millis() - lastNexByteTime > NEX_SILENCE_GAP))
    {
        while (nexSerial.available() > 0)
        {
            // CUE: Look for the Preamble (0x3B)
            if (nexSerial.peek() == PREAMBLE)
            {
                // ATOMIC PROOF: Trigger D7 Notch the microsecond 3B is seen
                pulseComprehension();

                // Header check: [3B] [Len] [Src] [Dest]
                if (nexSerial.available() >= 4)
                {
                    (void)nexSerial.read(); // Burn 3B
                    (void)nexSerial.read(); // Burn Len
                    (void)nexSerial.read(); // Burn Src
                    uint8_t dest = nexSerial.read();

                    if (dest == ADDR_GPS)
                    {
                        negotiationActive = true;
                        // ... Run Command Logic (Version/Location) ...
                    }
                }
                break;
            }
            else
            {
                (void)nexSerial.read(); // Scrub noise
            }
        }
    }

    // --- MANTRA TRANSITION: THE SIPHON GATE ---
    // Only drop the Listener (D7 LOW) if the bus is silent AND no negotiation is active.
    if (!negotiationActive && (millis() - lastNexByteTime > NEX_SILENCE_GAP))
    {
        SIPHONER_ON();
        // Inside main.cpp, the loop will see D7 is LOW and allow siphonGPS()
    }
}

#endif