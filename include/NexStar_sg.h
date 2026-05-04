#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Hardware_config.h"
#include "AstroLogic.h"
#include "Diagnostics.h"

extern SoftwareSerial nexSerial;
extern bool negotiationActive;

// --- AUX BUS PROTOCOL CONSTANTS ---
const uint8_t PREAMBLE = 0x3B;
const uint8_t ADDR_GPS = 0xB0; // verified via LA capture
const uint8_t ADDR_HC = 0x0D;  // verified LA capture

// Handshake Milestones
const uint8_t CMD_GET_VER = 0xFE;
const uint8_t CMD_GET_LOC = 0x01;  // Stage 2: Coordinates
const uint8_t CMD_GET_TIME = 0x03; // Stage 3: Time/Date

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
    nexSerial.write(PREAMBLE);
    nexSerial.write(p, len);
    uint8_t chk = calculateChecksum(p, len);
    nexSerial.write(chk);
    nexSerial.flush();
    pinMode(NEX_TX_PIN, INPUT);    // Return to high-impedance
    digitalWrite(NEX_TX_PIN, LOW); // explicit silence
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