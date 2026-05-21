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

// Nex gap silence timing
unsigned long lastNexActivity = 0;
const unsigned long nexSilenceWindow = 500; // 500ms required for "Silence"

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
    // ARBITRATION CUSHION: Wait exactly 3ms after the master finishes talking.
    // This allows the telescope hardware transceivers to switch directions and settle.
    delay(3);

    if (muzzleActive)
        return;

    // Stream the payload cleanly
    nexSerial.write(PREAMBLE);
    for (uint8_t i = 0; i < len; i++)
    {
        nexSerial.write(p[i]);
    }

    uint8_t chk = calculateChecksum(p, len);
    nexSerial.write(chk);
}

void processNexStar()
{

    // Check if at least a minimal header has landed in the stream buffer
    if (nexTalker.available() >= 2)
    {
        // Verify the preamble alignment frame
        if (nexTalker.peek() == PREAMBLE)
        {
            nexTalker.read();               // Consume Preamble (0x3B)
            uint8_t len = nexTalker.read(); // Extract the designated Length byte (0x03)

            // CRITICAL TIMING GATE: Wait for the trailing payload + checksum byte
            unsigned long timeout = millis();
            while (nexTalker.available() < (len + 1))
            {
                if (millis() - timeout > 15) // Lifted to 15ms to tolerate GPS bit-bang overlap
                {
                    Serial.println(F("--- HANDSHAKE ERROR: Packet Truncated ---"));
                    return;
                }
            }

            // Ingest the remaining packet body into a secure local storage array
            uint8_t rxBuf[12];
            for (uint8_t i = 0; i < (len + 1); i++)
            {
                rxBuf[i] = nexTalker.read();
            }

            // Map variables cleanly based on fixed offset indices:
            // rxBuf[0] = Source Address (0x0D)
            // rxBuf[1] = Destination Address (0xB0)
            // rxBuf[2] = Command Identification (0xFE)
            // rxBuf[3] = Checksum Frame (0x42)
            uint8_t dest = rxBuf[1];
            uint8_t cmd = rxBuf[2];

            if (dest == ADDR_GPS)
            {
                if (cmd == CMD_GET_VER)
                {
                    toggleDiagnostic(); // <<< DIAGNOSTIC: REMOVE OR COMMENT OUT EASILY
                    negotiationActive = true;

                    // Response structure payload: Len, Src, Dest, Cmd, VerMajor, VerMinor
                    uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x02};

                    // Direct main thread invocation to shield against lambda capture issues
                    sendNexPacket(verResp, 6);

                    Serial.println(F(">>> v1.4: Handshake Response Fired! <<<"));
                    negotiationActive = false;
                }
            }
        }
        else
        {
            // If the buffer alignment slips, clear individual junk bytes to scroll forward
            nexTalker.read();
        }
    }
}

#endif