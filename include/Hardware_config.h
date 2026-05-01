#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// --- GPS Office (NEO-7M) ---
#define GPS_RX_PIN 3 // Vetted: Connects to Nano D2
#define GPS_TX_PIN 2 // Vetted: Connects to Nano D3

// --- NexStar Office (Aux Port) ---
#define AUX_TX_PIN 0  // Vetted: Hardware Serial TX
#define AUX_RX_PIN 1  // Vetted: Hardware Serial RX

// --- System Indicators ---
#define STATUS_LED 13 // Built-in Nano LED

#endif