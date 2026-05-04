#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Hardware_config.h"
#include "AstroLogic.h"

extern SoftwareSerial nexSerial;
extern bool negotiationActive; // Managed here to shield GPS timing

// --- AUX BUS PROTOCOL CONSTANTS ---
const uint8_t PREAMBLE = 0x3B;
const uint8_t ADDR_GPS = 0xB0;
const uint8_t ADDR_HC = 0x01;

// Handshake Milestones
const uint8_t CMD_GET_VER = 0xFE;
const uint8_t CMD_GET_LOC = 0x01;  // NexStar GPS Query is usually 0x01
const uint8_t CMD_GET_TIME = 0x03; // Time/Date is usually 0x03[cite: 1]

void setupNexStar()
{
    nexSerial.begin(19200); // Standard Aux Bus Speed[cite: 1]
    pinMode(NEX_RX_PIN, INPUT_PULLUP);
    pinMode(NEX_TX_PIN, INPUT); // Stay in "Ghost Mode" until response[cite: 1]
}

// Helper to calculate checksum: Two's complement of the sum of bytes (excluding preamble)[cite: 1]
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
    nexSerial.write(PREAMBLE);
    nexSerial.write(p, len);

    // Calculate and send the final byte as the checksum[cite: 1]
    uint8_t chk = calculateChecksum(p, len);
    nexSerial.write(chk);

    nexSerial.flush();
    pinMode(NEX_TX_PIN, INPUT); // Return to high-impedance[cite: 1]
    digitalWrite(NEX_TX_PIN, LOW);
}

void processNexStar()
{
    if (nexSerial.available() > 0)
    {
        // 1. IMPACT: Immediately halt GPS processing[cite: 1]
        negotiationActive = true;

        if (nexSerial.read() == PREAMBLE)
        {
            uint8_t len = nexSerial.read();
            uint8_t src = nexSerial.read();
            uint8_t dest = nexSerial.read();

            // Only respond if the mount is looking for the GPS (0xB0)[cite: 1]
            if (dest == ADDR_GPS)
            {
                uint8_t cmd = nexSerial.read();

                if (cmd == CMD_GET_VER)
                {
                    uint8_t verResp[] = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x02};
                    sendNexPacket(verResp, 6);
                }
                // Additional Handshake Logic for Location/Time will go here
            }
        }

        // 2. RELEASE: Re-open the gate for GPS sips[cite: 1]
        negotiationActive = false;
    }
}

#endif