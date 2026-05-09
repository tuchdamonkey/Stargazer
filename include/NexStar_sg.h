#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include "Hardware_config.h"
#include "ross.h"
#include "soss.h"
#include "AstroLogic.h"

extern ross nexSerial;
extern soss nexTalker;
extern bool negotiationActive;
extern void syncEventAnchor(void (*func)());

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
    // 1. Check if the 'ross' buffer has data
    if (nexSerial.available() > 0)
    {
        // 2. Look for the Preamble (0x3B)
        if (nexSerial.read() == PREAMBLE)
        {
            negotiationActive = true;

            // 3. Wait for the 4-byte header: [Len] [Src] [Dest] [Cmd]
            while (nexSerial.available() < 4)
                ;

            uint8_t len = nexSerial.read();
            uint8_t src = nexSerial.read();
            uint8_t dest = nexSerial.read();
            uint8_t cmd = nexSerial.read(); // NOW 'cmd' is defined!

            // 4. Is the message for the GPS?
            if (dest == ADDR_GPS)
            {
                // STAGE 1: Handshake
                if (cmd == CMD_GET_VER)
                {
                    syncEventAnchor([]()
                                    {
                        uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x04};
                        sendNexPacket(verResp, 6); });
                }

                // STAGE 2: The Location "Carry"
                else if (cmd == CMD_GET_LOC)
                {
                    syncEventAnchor([]()
                                    {
                        uint8_t locResp[10];
                        locResp[0] = 0x09; // Length (Src+Dest+Cmd+6 payload bytes)
                        locResp[1] = ADDR_GPS;
                        locResp[2] = ADDR_HC;
                        locResp[3] = CMD_GET_LOC;
                        
                        // Grab the pre-baked 24-bit hex from our buckets
                        memcpy(&locResp[4], nexPayload_Lat, 3);
                        memcpy(&locResp[7], nexPayload_Lon, 3);
                        
                        sendNexPacket(locResp, 10); });
                    Serial.println(F(">>> v1.4: Coordinates Delivered to Bus"));
                }
            }
            negotiationActive = false;
        }
    }
}

#endif