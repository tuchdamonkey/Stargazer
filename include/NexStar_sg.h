#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "ross.h"
#include "soss.h"
#include "AstroLogic.h"
#include "Diagnostics.h"

extern soss nexSerial;
extern ross nexTalker;
extern bool negotiationActive;
extern void syncEventAnchor(void (*func)());

// --- AUX BUS PROTOCOL CONSTANTS ---
const uint8_t PREAMBLE = 0x3B;
const uint8_t ADDR_GPS = 0xB0; // verified via LA capture
const uint8_t ADDR_HC = 0x0D;  // verified LA capture

// Handshake Milestones
const uint8_t CMD_GET_VER = 0xFE;
const uint8_t CMD_GET_LOC = 0x01;  // Stage 2: Coordinates
const uint8_t CMD_GET_TIME = 0x03; // Stage 3: Time/Date

// --- STAGE 2: TRANSLATION & BUFFERING ---

// Global buffer holding the last validated 3-byte coordinate translation.
// This is the "Atomic Cache" the Nano serves when the HC requests data.
static uint8_t nexPayload[3];

/**
 * Translate & Pack: The unified "Brain" of Stage 2.
 * Converts GPS floats to 24-bit NexStar format and caches for instant delivery.
 */
inline void translateAndPack(float coord, bool isLongitude)
{
    float normalizedCoord = coord;
    if (isLongitude && coord < 0)
        normalizedCoord += 360.0;

    uint32_t precise24bit = (uint32_t)(normalizedCoord * COORD_TO_24BIT);

    // Slicing into the global nexPayload cache
    nexPayload[0] = (uint8_t)((precise24bit >> 16) & 0xFF);
    nexPayload[1] = (uint8_t)((precise24bit >> 8) & 0xFF);
    nexPayload[2] = (uint8_t)(precise24bit & 0xFF);

    // NO SERIAL PRINTS HERE. Silence is speed.
}

void setupNexStar()
{
    nexTalker.begin(19200);
    nexSerial.begin(19200);
    // --- NANO RECEIVE PATH (D5) ---
    // From Nex perspective: NEX_TX_PIN.
    // This pin connects to the 6N137 output. It must be an INPUT.
    // We leave it alone so it can sit at its hardware-driven, inverted idle-LOW state.

    pinMode(NEX_TX_PIN, INPUT);

    // --- NANO TRANSMIT PATH (D4) ---
    // From Nex perspective: NEX_RX_PIN.
    // This pin connects directly to the shared telescope bus through R9.
    // We must drive it HIGH as an active OUTPUT immediately to match
    // the telescope's native 5V idle, stopping it from sagging the bus at boot.

    pinMode(NEX_RX_PIN, OUTPUT);
    digitalWrite(NEX_RX_PIN, HIGH);
}

uint8_t calculateChecksum(uint8_t *p, uint8_t len)
{
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len; i++)
        sum += p[i];
    return (uint8_t)((~sum + 1) & 0xFF);
}

void sendNexPacket(uint8_t *p, uint8_t len)
{
    // Ensure the pin is explicitly configured to drive the line
    pinMode(NEX_RX_PIN, OUTPUT);

    // USE nexTalker (soss) for Transmitting
    nexSerial.write(PREAMBLE);
    for (uint8_t i = 0; i < len; i++)
    {
        nexSerial.write(p[i]);
    }

    uint8_t chk = calculateChecksum(p, len);
    nexSerial.write(chk);

    // Active Idle Realignment: Force the pin HIGH and maintain OUTPUT status.
    // This shuts off the 6N137 LED, completely releasing the physical Aux Bus.
    digitalWrite(NEX_RX_PIN, HIGH);
    pinMode(NEX_RX_PIN, OUTPUT);
}
void processNexStar()
{
    if (nexTalker.available() > 0)
    {
        if (nexTalker.read() == PREAMBLE)
        {
            // We've found the start; Nano is now "attending" to the bus
            negotiationActive = true;

            uint8_t len = nexSerial.read();
            uint8_t src = nexSerial.read();
            uint8_t dest = nexSerial.read();

            if (dest == ADDR_GPS)
            {
                uint8_t cmd = nexTalker.read();

                if (cmd == CMD_GET_VER)
                {
                    // The "Atomic Wrap" starts here
                    syncEventAnchor([]()
                                    {
                        // Response data: Length, Src, Dest, Cmd, VerMajor, VerMinor
                        uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x02};
                        
                        // Execute the strike
                        sendNexPacket(verResp, 6); });

                    // Serial.print is MOVED outside the syncEventAnchor
                    // so it doesn't inflate the "Locked" duration on the LA.
                    Serial.println(F(">>> v1.4: Locked State (GET_VER) Captured on D7"));
                }
            }
            negotiationActive = false;
        }
    }
}

#endif