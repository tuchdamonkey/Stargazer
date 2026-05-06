#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h"

// Managed in main.cpp
extern bool negotiationActive;

// Internal Ross/Soss state
bool muzzleActive = true;

void setupGPS() {
    pinMode(GPS_RX_PIN, INPUT);
    muzzleActive = true; 
}

void muzzleGPS(bool closed) {
    muzzleActive = closed;
}

// THE SIP: Manually polling the pin for a set window
void sipGPS(uint32_t durationMs) {
    if (muzzleActive) return;

    uint32_t startTime = millis();
    while (millis() - startTime < durationMs) {
        // Raw logic read: 0 = Data, 1 = Idle (shown as '-')
        if (digitalRead(GPS_RX_PIN) == LOW) Serial.print("0");
        else Serial.print("-");
    }
    Serial.println(); // Sip end
}

#endif