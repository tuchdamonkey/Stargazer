#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "Hardware_Config.h"

extern SoftwareSerial nexSerial;

#ifndef NEXSTAR_SG_H
#define NEXSTAR_SG_H

#include <Arduino.h>
#include "AstroLogic.h"

// --- AUX BUS PROTOCOL CONSTANTS ---
const uint8_t PREAMBLE = 0x3B;
const uint8_t ADDR_GPS = 0xB0;
const uint8_t ADDR_HC  = 0x01;

// Handshake Milestones
const uint8_t CMD_GET_VER = 0xFE;
const uint8_t CMD_GET_LOC = 0x03;
const uint8_t CMD_GET_TIME = 0x04;

// --- PACKET SHELLS ---
// [LEN] [SRC] [DEST] [ID] [DATA...] [CHK]

// Response to Presence Check (ACK)
// Length 3: Source, Dest, ID
uint8_t packet_Ack(5) = {0x03, ADDR_GPS, ADDR_HC, 0x01, 0x00};

// Response to Get Version (0xFE)
// Length 5: Source, Dest, ID, Ver1, Ver2
uint8_t packet_Ver(7) = {0x05, ADDR_GPS, ADDR_HC, CMD_GET_VER, 0x01, 0x02, 0x00};

// Response to Get Location (0x03)
// Length 7: Source, Dest, ID, Lat(3), Lon(3)
uint8_t packet_Pos(9) = {0x07, ADDR_GPS, ADDR_HC, CMD_GET_LOC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/**
 * Tomorrow's Logic: 
 * We will build a state machine here to listen for the PREAMBLE,
 * verify if the DEST matches ADDR_GPS, and then dispatch these packets.
 */

#endif