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
    // 1. DATA TRACKING: If data is present, update our "Last Seen" timestamp
    if (nexSerial.available() > 0)
    {
        SIPHONER_ON(); //GPS DEBUG MUZZLE: REPLACES lastNexByteTime = millis();

        // Check for the Preamble
        if (nexSerial.peek() == PREAMBLE)
        {
            negotiationActive = true;
            (void)nexSerial.read(); // Consume the 0x3B

            // 3. SAFETY GATE: Wait for the 4-byte header
            unsigned long headerStart = millis();
            while (nexSerial.available() < 4)
            {
                if (millis() - headerStart > NEX_TIMEOUT_MS)
                {
                    // No need to release yet; the Silence Gap will handle it if the bus stays dead
                    return;
                }
            }

            (void)nexSerial.read(); // Len
            (void)nexSerial.read(); // Src
            uint8_t dest = nexSerial.read();
            uint8_t cmd = nexSerial.read();

            if (dest == ADDR_GPS)
            {
                pulseComprehension();

                if (cmd == CMD_GET_VER)
                {
                    syncEventAnchor([]()
                                    {
                        uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x04};
                        sendNexPacket(verResp, 6); });
                }
                else if (cmd == CMD_GET_LOC)
                {
                    syncEventAnchor([]()
                                    {
                        uint8_t locResp[10];
                        locResp[0] = 0x09;
                        locResp[1] = ADDR_GPS;
                        locResp[2] = ADDR_HC;
                        locResp[3] = CMD_GET_LOC;
                        memcpy(&locResp[4], nexPayload_Lat, 3);
                        memcpy(&locResp[7], nexPayload_Lon, 3);
                        sendNexPacket(locResp, 10); });
                }
                // DELIVERY COMPLETE: Transaction ended successfully
                negotiationActive = false;
                while (nexSerial.available() > 0)
                    (void)nexSerial.read(); // Clear "Echoes"
            }
        }
        else
        {
            // If it's not a Preamble, it's noise or a mid-packet byte we don't want
            (void)nexSerial.read();
        }
    }

    // --- THE SILENCE-BASED BUFFER WIPE ---
    // If the muzzle is on but the bus has been quiet for 500ms...
    if (negotiationActive && (millis() - lastNexByteTime > NEX_SILENCE_GAP))
    {
        negotiationActive = false;
        // Aggressive Flush: Leave the room clean
        while (nexSerial.available() > 0)
            (void)nexSerial.read();
    }
}

#endif