#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// --- GPS (NEO-7M) ---
#define GPS_RX_PIN 3 // Vetted: Connects to Nano D3
#define GPS_TX_PIN 2 // Vetted: Connects to Nano D2

// --- NexStar (Aux Port) ---
#define NEX_TX_PIN 5  // Vetted: Hardware Serial TX
#define NEX_RX_PIN 4  // Vetted: Hardware Serial RX

// --- System Indicators ---
#define STATUS_LED 13 // Built-in Nano LED

#endif