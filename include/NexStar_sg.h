#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "ross.h"
#include "soss.h"
#include "AstroLogic.h"
#include "Diagnostics.h"

// ==========================================
// 1. EXTERNAL LINKAGE & SHARED PROPERTY DEFINITIONS
// ==========================================
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
extern unsigned long lastNexActivity;
const unsigned long nexSilenceWindow = 500; // 500ms required for "Silence"

// --- STAGE 2: TRANSLATION & BUFFERING ---
// REMOVED 'static'. This turns it into a standard global array so that
// both main.cpp and this header modify the exact same space in RAM.
extern uint8_t nexPayload[3];

// ==========================================
// 2. INDEPENDENT MATH UTILITIES
// ==========================================
uint8_t calculateChecksum(uint8_t *p, uint8_t len)
{
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len; i++)
        sum += p[i];
    return (uint8_t)((~sum + 1) & 0xFF);
}

// ==========================================
// 3. HARDWARE CONFIGURATION INITIALIZERS
// ==========================================
void setupNexStar()
{
    nexTalker.begin(19200);
    nexSerial.begin(19200);

    // --- NANO RECEIVE PATH (D5) ---
    pinMode(NEX_TX_PIN, INPUT);

    // --- NANO TRANSMIT PATH (D4) ---
    pinMode(NEX_RX_PIN, OUTPUT);
    digitalWrite(NEX_RX_PIN, HIGH);
}

// ==========================================
// 4. LOW-LEVEL TRANSLATION HELPERS
// ==========================================
inline void translateAndPack(float coord, bool isLongitude)
{
    float normalizedCoord = coord;
    if (isLongitude && coord < 0)
        normalizedCoord += 360.0;

    uint32_t precise24bit = (uint32_t)(normalizedCoord * COORD_TO_24BIT);

    // Slicing into the global unified nexPayload cache
    nexPayload[0] = (uint8_t)((precise24bit >> 16) & 0xFF);
    nexPayload[1] = (uint8_t)((precise24bit >> 8) & 0xFF);
    nexPayload[2] = (uint8_t)(precise24bit & 0xFF);
}

// ==========================================
// 5. CORE EGRESS TRANSMISSION DRIVERS
// ==========================================
void sendNexPacket(uint8_t *p, uint8_t len)
{
    while (nexTalker.available() > 0)
    {
        nexTalker.read();
    }

    // Checksum calculated early while array pointer is completely isolated
    uint8_t chk = calculateChecksum(p, len);

    delay(3); // Arbitration cushion

    if (muzzleActive)
        return;

    nexSerial.write(PREAMBLE);
    for (uint8_t i = 0; i < len; i++)
    {
        nexSerial.write(p[i]);
    }

    nexSerial.write(chk);
}

// ==========================================
// 6. HIGH-LEVEL STATE MACHINES & CONSUMERS
// ==========================================
void processNexStar()
{
    if (nexTalker.available() >= 2)
    {
        if (nexTalker.peek() == PREAMBLE)
        {
            nexTalker.read();
            uint8_t len = nexTalker.read();

            unsigned long timeout = millis();
            while (nexTalker.available() < (len + 1))
            {
                if (millis() - timeout > 15)
                {
                    Serial.println(F("--- HANDSHAKE ERROR: Packet Truncated ---"));
                    return;
                }
            }

            uint8_t rxBuf[12];
            for (uint8_t i = 0; i < (len + 1); i++)
            {
                rxBuf[i] = nexTalker.read();
            }

            uint8_t dest = rxBuf[1];
            uint8_t cmd = rxBuf[2];

            if (dest == ADDR_GPS)
            {
                if (cmd == CMD_GET_VER)
                {
                    toggleDiagnostic();
                    negotiationActive = true;

                    uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x00};

                    sendNexPacket(verResp, 6);
                    nexTalker.listen();

                    negotiationActive = false;
                }
            }
        }
        else
        {
            nexTalker.read();
        }
    }
}

#endif // NEXSTAR_SG_H