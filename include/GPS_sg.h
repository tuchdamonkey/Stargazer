#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h"
#include "Diagnostics.h"

// ==========================================
// 1. EXTERNAL LINKAGES & FORWARD REGISTRIES
// ==========================================
extern char goldenPacket[85];
extern volatile int bufIndex;
extern volatile SystemState currentState;

#define SILO_SIZE 160

// --- THE FIX: GLOBAL STORAGE BOUNDARIES ---
// Changed to 'extern' declarations to prevent multiple definition errors.
// These variables must be physically instantiated inside your main.cpp file.
extern char gpsSilo[SILO_SIZE];
extern int siloIndex;
extern bool siloReady;

extern unsigned long lastCarryTime;
const unsigned long carryInterval = 10000;

// ==========================================
// 2. LOW-LEVEL BIT-SHIFTERS & EGRESS DRIVERS
// ==========================================

/**
 * NATIVE RAW TRANSMIT: Streams a byte bit-by-bit to the GPS module.
 * Changed parameter type to 'uint8_t' to guarantee mathematical integrity
 * when shifting high-order binary payloads (e.g., 0xB5) down to the line.
 */
void rossWrite(uint8_t c)
{
  const uint16_t bitPeriod = 104; // 9600 Baud Timing Edge
  digitalWrite(GPS_TX_PIN, LOW);  // Start Bit
  delayMicroseconds(bitPeriod);

  for (int i = 0; i < 8; i++)
  {
    digitalWrite(GPS_TX_PIN, (c >> i) & 0x01);
    delayMicroseconds(bitPeriod);
  }

  digitalWrite(GPS_TX_PIN, HIGH); // Stop Bit
  delayMicroseconds(bitPeriod);
}

void rossPrint(const char *str)
{
  while (*str)
    rossWrite((uint8_t)(*str++));
}

/**
 * THE BIT-READER: Samples incoming asynchronous serial stream edges.
 * Moves the line timing to the exact center of every bit window.
 */
char readRossByte()
{
  // We arrive here exactly when the Start Bit (LOW) is detected
  noInterrupts();

  // Jump to the middle of Bit 0 (~150us from leading edge)
  delayMicroseconds(135);

  char incomingByte = 0;
  for (int i = 0; i < 8; i++)
  {
    // Strike the bit
    if (digitalRead(GPS_RX_PIN) == HIGH)
    {
      incomingByte |= (1 << i);
    }

    // Delay to reach the next bit center (~104us total with overhead)
    delayMicroseconds(101);
  }

  interrupts();
  return incomingByte;
}

// ==========================================
// 3. LOGICAL DATA FILTERING & ANALYSIS
// ==========================================
bool isChecksumValid(char *sentence)
{
  char *start = strchr(sentence, '$');
  char *end = strchr(sentence, '*');

  if (!start || !end || end < start)
    return false;

  byte calculatedSum = 0;
  for (char *p = start + 1; p < end; p++)
  {
    calculatedSum ^= *p;
  }

  char hex[3] = {*(end + 1), *(end + 2), '\0'};
  byte providedSum = (byte)strtol(hex, NULL, 16);

  return (calculatedSum == providedSum);
}

// ==========================================
// 4. CORE STATE INGESTION MACHINES
// ==========================================
void captureGpsBurst()
{
  // MANTRA: Check for GPS Start Bit. If HIGH, no data is ready.
  if (digitalRead(GPS_RX_PIN) == HIGH)
  {
    return;
  }

  // Pin is LOW. A byte is arriving.
  char c = readRossByte();

  // Guard against overflow
  if (siloIndex < SILO_SIZE)
  {
    gpsSilo[siloIndex++] = c;
  }

  // Detection: End of burst (Look for the newline)
  if (c == '\n')
  {
    char *rmcStart = strstr(gpsSilo, "$GPRMC");
    char *ggaStart = strstr(gpsSilo, "$GPGGA");

    if (rmcStart && ggaStart)
    {
      if (isChecksumValid(rmcStart) && isChecksumValid(ggaStart))
      {
        siloReady = true;
        Serial.println(F("GPS_SIP_COMPLETE"));
      }
      else
      {
        siloReady = false;
      }
      siloIndex = 0; // Reset bucket only when a complete set is processed
    }
    else if (siloIndex >= (SILO_SIZE - 20))
    {
      // Safety release valve to prevent memory corruption
      siloIndex = 0;
    }
  }
}

// ==========================================
// 5. HARDWARE CONFIGURATION ROOT METHOD
// ==========================================

// --- SELECTIVE HARVEST MODE (1Hz Standard NMEA Filters) ---
void setupGPS()
{
  pinMode(GPS_RX_PIN, INPUT_PULLUP);
  pinMode(GPS_TX_PIN, OUTPUT);
  digitalWrite(GPS_TX_PIN, HIGH);

  // Silent mode for unused NMEA
  rossPrint("$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n");
  rossPrint("$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n");
  rossPrint("$PUBX,40,GSV,0,0,0,0,0,0*59\r\n");
  rossPrint("$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n");
  rossPrint("$PUBX,40,RMC,0,1,0,0,0,0*47\r\n");
  rossPrint("$PUBX,40,GGA,0,1,0,0,0,0*5A\r\n");

  currentState = STATE_NEX_LISTENING;
}

/*
// ============================================================================
// ALTERNATE CONFIGURATION: 5Hz DATA STORM EXPERIMENT (Diagnostic Use Only)
// ============================================================================
void setupGPS()
{
  pinMode(GPS_RX_PIN, INPUT_PULLUP);
  pinMode(GPS_TX_PIN, OUTPUT);
  digitalWrite(GPS_TX_PIN, HIGH);

  // Send the proprietary UBX-CFG-RATE binary payload to reconfigure hardware
  uint8_t cfgRate5Hz[] = {
      0xB5, 0x62, // UBX Sync Chars
      0x06, 0x08, // Class: CFG, ID: RATE
      0x06, 0x00, // Payload Length: 6 bytes
      0xC8, 0x00, // Measurement Period: 200ms (0x00C8)
      0x01, 0x00, // Navigation Cycle: 1
      0x01, 0x00, // Time Reference: UTC (1)
      0xDE, 0x6A  // Checksum A and B
  };

  for (uint8_t i = 0; i < sizeof(cfgRate5Hz); i++)
  {
    rossWrite(cfgRate5Hz[i]);
  }

  currentState = STATE_NEX_LISTENING;
}
*/

#endif