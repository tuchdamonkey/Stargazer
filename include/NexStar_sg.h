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
extern bool muzzleActive;

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
    // Rem muzzle checking or logging if active
    if (muzzleActive)
        return;

    // DO NOT manually call pinMode() or digitalWrite() here.
    // Let nexSerial (soss) stream the bits natively through direct port manipulation.

    // 1. Fire the Preamble
    nexSerial.write(PREAMBLE);

    // 2. Stream the Payload array elements exactly as packed
    for (uint8_t i = 0; i < len; i++)
    {
        nexSerial.write(p[i]);
    }

    // 3. Calculate the Checksum using the explicit Celestron-aligned routine
    // Pass the payload pointer and its designated length element
    uint8_t chk = calculateNEXChecksum(p, len);
    nexSerial.write(chk);
}

void processNexStar()
{
    // Check if at least a minimal packet header has arrived (Preamble + Length)
    if (nexTalker.available() >= 2)
    {
        // Peek at the first byte without consuming it to verify the preamble
        if (nexTalker.peek() == PREAMBLE)
        {
            // Advance past the preamble byte safely since it's already verified
            nexTalker.read();

            // Read the length byte
            uint8_t len = nexTalker.read();

            // CRITICAL GATE: Wait for the entire trailing body specified by 'len'
            // plus its trailing checksum byte to land in the hardware buffer
            unsigned long timeout = millis();
            while (nexTalker.available() < (len + 1))
            {
                if (millis() - timeout > 10) // 10ms safety breakout
                {
                    Serial.println(F("--- HANDSHAKE ERROR: Packet Truncated ---"));
                    return;
                }
            }

            // Advance past the source byte (not needed for filtering)
            nexTalker.read();

            // Read the destination and command bytes we actually care about
            uint8_t dest = nexTalker.read();
            uint8_t cmd = nexTalker.read();

            if (dest == ADDR_GPS)
            {
                if (cmd == CMD_GET_VER)
                {
                    negotiationActive = true;

                    syncEventAnchor([]()
                                    {
                        // Response data: Length, Src, Dest, Cmd, VerMajor, VerMinor
                        uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x02};
                        sendNexPacket(verResp, 6); });

                    Serial.println(F(">>> v1.4: Handshake Response Fired! <<<"));

                    // Consume the remaining checksum byte left in the buffer to clear the track
                    if (nexTalker.available() > 0)
                        nexTalker.read();

                    negotiationActive = false;
                }
            }
        }
        else
        {
            // If the buffer doesn't start with 0x3B, flush the single junk byte to keep scrolling
            nexTalker.read();
        }
    }
}

#endif