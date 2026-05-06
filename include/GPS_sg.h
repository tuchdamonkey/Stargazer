#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h"

extern bool negotiationActive;
bool muzzleActive = true;

// The 9600 baud "Bit-Time" in microseconds
const uint16_t bitPeriod = 104; 

void setupGPS() {
    pinMode(GPS_RX_PIN, INPUT);
    muzzleActive = true; 
}

void muzzleGPS(bool closed) {
    muzzleActive = closed;
}

// Manual UART decoder (The "Soss" Engine)
char readGpsChar() {
    uint32_t startWait = micros();
    // 1. Wait for Start Bit (LOW)
    while (digitalRead(GPS_RX_PIN) == HIGH) {
        if (micros() - startWait > 20000) return 0; // Timeout
    }

    // 2. Jump to the middle of the first data bit
    delayMicroseconds(bitPeriod + (bitPeriod / 2));

    char c = 0;
    for (int i = 0; i < 8; i++) {
        if (digitalRead(GPS_RX_PIN) == HIGH) {
            c |= (1 << i);
        }
        delayMicroseconds(bitPeriod);
    }
    return c;
}

void sipGPS(uint16_t charCount) {
    if (muzzleActive) return;

    Serial.print(F("GPS SIP: "));
    
    // We grab exactly charCount characters
    for (int i = 0; i < charCount; i++) {
        char c = readGpsChar();
        if (c > 31 && c < 127) { // Only print printable ASCII
            Serial.print(c);
        } else if (c == '\n' || c == '\r') {
            Serial.print(" "); // Keep it on one line for the monitor
        }
    }
    Serial.println(F(" [END]"));
}

#endif