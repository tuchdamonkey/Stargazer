#ifndef ASTROLOGIC_H
#define ASTROLOGIC_H

#include <Arduino.h>

// Pillar 2: Translation (Lean Version)
// Scaler: 2^24 / 360 = 46603.3777
#define COORD_TO_24BIT 46603.3777

/**
 * Assembles a NexStar GPS/Time Update Packet (0x3B Command)
 * Packet Structure: [3B] [Len] [Source] [Dest] [Cmd] [Payload...] [Checksum]
 */
void buildNEXPacket(uint8_t *buf, double lat, double lon, uint8_t h, uint8_t m, uint8_t s) {
    
    // --- TOP LEVEL SANITIZATION ---
    // Nashville Fix: Normalize West to 0-360 range
    if (lon < 0) {
        lon += 360.0; 
    }
    // (Optional) If your mount expects 0-360 for Lat as well, you'd do it here.

    // 1. Preamble
    buf[0] = 0x3B;
    
    // 2. Length (13 bytes follow this one)
    buf[1] = 0x0D; 

    // 3. Routing
    buf[2] = 0x0E; // Source: GPS
    buf[3] = 0x01; // Destination: Main Control
    buf[4] = 0x3B; // Command: Time/Location Update

    // 4. Payload: Latitude (24-bit)
    uint32_t lat24 = (uint32_t)(lat * COORD_TO_24BIT);
    buf[5] = (lat24 >> 16) & 0xFF;
    buf[6] = (lat24 >> 8) & 0xFF;
    buf[7] = lat24 & 0xFF;

    // 5. Payload: Longitude (24-bit)
    uint32_t lon24 = (uint32_t)(lon * COORD_TO_24BIT);
    buf[8] = (lon24 >> 16) & 0xFF;
    buf[9] = (lon24 >> 8) & 0xFF;
    buf[10] = lon24 & 0xFF;

    // 6. Payload: Time (Raw Bytes)
    buf[11] = h;
    buf[12] = m;
    buf[13] = s;

    // 7. The Bodyguard (Checksum)
    uint16_t sum = 0;
    for (uint8_t i = 1; i <= 13; i++) {
        sum += buf[i];
    }
    buf[14] = (uint8_t)((-sum) & 0xFF); 
}

// We use macros or inline to save stack space
inline void packNEXCoord(double coord, uint8_t &hi, uint8_t &mid, uint8_t &lo) {
    if (coord < 0) coord += 360.0; 
    uint32_t val = (uint32_t)(coord * COORD_TO_24BIT);
    hi  = (val >> 16) & 0xFF;
    mid = (val >> 8) & 0xFF;
    lo  =  val & 0xFF;
}

// Time is just a direct pass-through for NEX, no helper needed.
// Use: packet[7] = gps.time.hour(); 

/**
 * Pillar 3: Delivery (Checksum)
 * Calculates the Two's Complement checksum for a NEX AUX packet.
 * packet: the array of bytes
 * len: total length of the packet
 */
uint8_t calculateNEXChecksum(uint8_t *packet, uint8_t len) {
    uint16_t sum = 0;
    // Start at index 1 to skip the Preamble (0x3B)
    for (uint8_t i = 1; i < len - 1; i++) {
        sum += packet[i];
    }
    // Two's complement: sum the bytes, then take (0 - sum) & 0xFF
    return (uint8_t)((-sum) & 0xFF);
}

// ============================================================================
// DIAGNOSTICS & LOGGING
// ============================================================================

/**
 * Narrates the outgoing hex for the logic analyzer/Serial Monitor.
 */
void logHexPacket(const char* label, uint8_t* packet, uint8_t len) {
    Serial.print(label);
    Serial.print(": ");
    for (uint8_t i = 0; i < len; i++) {
        if (*(packet + i) < 0x10) Serial.print("0"); 
        Serial.print(*(packet + i), HEX);
        Serial.print(" ");
    }
    Serial.println();
}



#endif