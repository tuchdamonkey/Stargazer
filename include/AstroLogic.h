#ifndef ASTROLOGIC_H
#define ASTROLOGIC_H

#include <Arduino.h>

// ==========================================
// 1. CONSTANTS & MATRICES
// ==========================================
// Scaler: 2^24 / 360 = 46603.377777...
// Appended 'double' literal precision identifier (UL) to protect coordinates
#define COORD_TO_24BIT 46603.3777777778

// ==========================================
// 2. TEXT PROCESSING & PARSING UTILITIES
// ==========================================

/**
 * Helper: Returns a pointer to the start of the Nth field in a comma-separated string.
 * index: 0 for the header, 1 for the first data field, etc.
 */
const char *findField(const char *str, int index)
{
    int count = 0;
    const char *p = str;
    while (count < index)
    {
        p = strchr(p, ',');
        if (!p)
            return nullptr; // Field not found
        p++;                // Move past the comma
        count++;
    }
    return p;
}

/**
 * Pillar 1.5: The Translator
 * Converts NMEA DDM (DDMM.MMMM) char array to Decimal Degrees float.
 */
float convertNMEAToDecimal(const char *raw, char dir)
{
    if (raw == nullptr || strlen(raw) < 5)
        return 0.0;

    // 1. Locate the decimal "anchor"
    const char *dot = strchr(raw, '.');
    if (dot == nullptr)
        return 0.0;

    // 2. The minutes always start 2 places before the dot
    const char *minuteStart = dot - 2;

    // 3. Extract Minutes (atof reads until it hits a non-numeric/null)
    float mins = atof(minuteStart);

    // 4. Extract Degrees
    char degBuf[5]; // Max 3 digits for Longitude (180) + null
    int degLen = minuteStart - raw;
    if (degLen > 4)
        degLen = 4; // Safety clamp

    strncpy(degBuf, raw, degLen);
    degBuf[degLen] = '\0'; // Manual null termination
    float degs = atof(degBuf);

    // 5. Final Calculation
    float decimalDegrees = degs + (mins / 60.0);

    // 6. Hemisphere Correction
    if (dir == 'S' || dir == 'W')
    {
        decimalDegrees *= -1.0;
    }

    return decimalDegrees;
}

// ==========================================
// 3. MATH & PACKING HELPERS
// ==========================================

/**
 * Pillar 2: Coordinate Packing Math
 * Converts coordinate maps to the 24-bit telescope position architecture.
 */
inline void packNEXCoord(double coord, uint8_t &hi, uint8_t &mid, uint8_t &lo)
{
    if (coord < 0)
        coord += 360.0;
    uint32_t val = (uint32_t)(coord * COORD_TO_24BIT);
    hi = (val >> 16) & 0xFF;
    mid = (val >> 8) & 0xFF;
    lo = val & 0xFF;
}

/**
 * Pillar 3: Delivery (Checksum)
 * Calculates the Two's Complement checksum for a NEX AUX packet.
 * packet: the array of bytes
 * len: total length of the packet
 */
uint8_t calculateNEXChecksum(uint8_t *packet, uint8_t len)
{
    uint16_t sum = 0;
    // Start at index 1 to skip the Preamble (0x3B)
    for (uint8_t i = 1; i < len - 1; i++)
    {
        sum += packet[i];
    }
    // Two's complement: sum the bytes, then take (0 - sum) & 0xFF
    return (uint8_t)((-sum) & 0xFF);
}

// ==========================================
// 4. PACKET GENERATION DRIVERS
// ==========================================

/**
 * Assembles a NexStar GPS/Time Update Packet (0x3B Command)
 * Packet Structure: [3B] [Len] [Source] [Dest] [Cmd] [Payload...] [Checksum]
 */
void buildNEXPacket(uint8_t *buf, double lat, double lon, uint8_t h, uint8_t m, uint8_t s)
{
    // 1. Preamble
    buf[0] = 0x3B;

    // 2. Length (13 bytes follow this one)
    buf[1] = 0x0D;

    // 3. Routing
    buf[2] = 0x0E; // Source: GPS
    buf[3] = 0x01; // Destination: Main Control
    buf[4] = 0x3B; // Command: Time/Location Update

    // 4. Payload: Latitude (24-bit) via top-defined helper
    packNEXCoord(lat, buf[5], buf[6], buf[7]);

    // 5. Payload: Longitude (24-bit) via top-defined helper
    packNEXCoord(lon, buf[8], buf[9], buf[10]);

    // 6. Payload: Time (Raw Bytes)
    buf[11] = h;
    buf[12] = m;
    buf[13] = s;

    // 7. The Bodyguard (Checksum)
    // Refactored to leverage your unified checksum algorithm directly!
    buf[14] = calculateNEXChecksum(buf, 15);
}

// ============================================================================
// 5. DIAGNOSTICS & LOGGING
// ============================================================================

/**
 * Narrates the outgoing hex for the logic analyzer/Serial Monitor.
 */
void logHexPacket(const char *label, uint8_t *packet, uint8_t len)
{
    Serial.print(label);
    Serial.print(": ");
    for (uint8_t i = 0; i < len; i++)
    {
        if (*(packet + i) < 0x10)
            Serial.print("0");
        Serial.print(*(packet + i), HEX);
        Serial.print(" ");
    }
    Serial.println();
}

#endif