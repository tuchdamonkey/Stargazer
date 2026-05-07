#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include "Hardware_config.h"
#include "custom_serial/ross.h"
#include "custom_serial/soss.h"
#include "AstroLogic.h"
#include "Diagnostics.h"

extern ross nexSerial;
extern soss nexTalker;
extern bool negotiationActive;

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
    // 1. Normalization (Western Hemisphere Check)
    float normalizedCoord = coord;
    if (isLongitude && coord < 0)
    {
        normalizedCoord += 360.0;
    }

    // 2. Scaling (The NEX Constant from AstroLogic.h)
    uint32_t precise24bit = (uint32_t)(normalizedCoord * COORD_TO_24BIT);

    // 3. Bit-Slicing (Splicing into High, Mid, Low bytes)
    nexPayload[0] = (uint8_t)((precise24bit >> 16) & 0xFF);
    nexPayload[1] = (uint8_t)((precise24bit >> 8) & 0xFF);
    nexPayload[2] = (uint8_t)(precise24bit & 0xFF);

    // 4. Data Scope (Serial verification for Ross/Soss validation)
    Serial.print(F("[NEX_MATH] "));
    Serial.print(isLongitude ? F("Lon: ") : F("Lat: "));
    Serial.print(coord, 6);
    Serial.print(F(" -> HEX: "));
    for (int i = 0; i < 3; i++)
    {
        if (nexPayload[i] < 0x10)
            Serial.print('0');
        Serial.print(nexPayload[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

void setupNexStar()
{
    nexSerial.begin(19200);
    pinMode(NEX_RX_PIN, INPUT_PULLUP);
    pinMode(NEX_TX_PIN, INPUT); // v1.0 Ghost Mode default
    digitalWrite(NEX_TX_PIN, LOW);
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
    pinMode(NEX_TX_PIN, OUTPUT);

    // USE nexTalker (soss) for Transmitting
    nexTalker.write(PREAMBLE);
    for (uint8_t i = 0; i < len; i++)
    {
        nexTalker.write(p[i]);
    }

    uint8_t chk = calculateChecksum(p, len);
    nexTalker.write(chk);

    // nexTalker (soss) handles the bit-timing for the output
    pinMode(NEX_TX_PIN, INPUT);
    digitalWrite(NEX_TX_PIN, LOW);
}

void processNexStar()
{
    if (nexSerial.available() > 0)
    {
        if (nexSerial.read() == PREAMBLE)
        {
            negotiationActive = true;

            uint8_t len = nexSerial.read();
            uint8_t src = nexSerial.read();
            (void)len; // Silence "unused" warning[cite: 4]
            (void)src; // Silence "unused" warning[cite: 4]

            uint8_t dest = nexSerial.read();

            if (dest == ADDR_GPS)
            {
                uint8_t cmd = nexSerial.read();

                if (cmd == CMD_GET_VER)
                {
                    auto nexStrike = []()
                    {
                        uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x02};
                        sendNexPacket(verResp, 6);
                    };

                    syncEventAnchor(nexStrike);
                    Serial.println(F(">>> v1.1.1: Event Strike (GET_VER) Captured on D7"));
                }
            }
            negotiationActive = false;
        }
    }
}

#endif