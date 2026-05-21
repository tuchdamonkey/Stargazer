#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h"
#include "Diagnostics.h"

// --- GLOBALS & CONFIG ---
#define SILO_SIZE 160
char gpsSilo[SILO_SIZE];
int siloIndex = 0;
bool siloReady = false;

unsigned long lastCarryTime = 0;
const unsigned long carryInterval = 10000;

extern char goldenPacket[85];
extern volatile int bufIndex;
extern volatile SystemState currentState;

// === PROTOTYPES ===
void setupGPS();
void captureGpsBurst();

// --- 1. THE BIT-READER (Resyncing on Every Byte) ---
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

// --- 2. THE VALIDATION GATE ---
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

// --- 3. THE ATOMIC SILO CAPTURE ---
void captureGpsBurst()
{
  // MANTRA: Check for GPS Start Bit. If HIGH, no data is ready.
  // Exit immediately to give the NexStar Listener priority.
  if (digitalRead(GPS_RX_PIN) == HIGH)
  {
    return;
  }

  // --- SIP START ---
  // If we are here, the pin is LOW. A byte is arriving.
  char c = readRossByte();

  // Guard against overflow
  if (siloIndex < SILO_SIZE)
  {
    gpsSilo[siloIndex++] = c;
    // toggleDiagnostic(); // Pulse D7 to show "Siphoning" activity
  }

  // Detection: End of burst (Look for the newline)
  // We only run the heavy string analysis when we hit the end of the sentence
  if (c == '\n')
  {
    char *rmcStart = strstr(gpsSilo, "$GPRMC");
    char *ggaStart = strstr(gpsSilo, "$GPGGA");

    if (rmcStart && ggaStart)
    {
      if (isChecksumValid(rmcStart) && isChecksumValid(ggaStart))
      {
        siloReady = true;
        // Optional:
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
      // Safety release valve: If the bucket is getting full but we don't have both
      // sentences yet, reset to prevent an unmanaged buffer overflow.
      siloIndex = 0;
    }
  }
}

// --- 4. HARDWARE SETUP & UTILS ---
void rossWrite(char c)
{
  const uint16_t bitPeriod = 104;
  digitalWrite(GPS_TX_PIN, LOW);
  delayMicroseconds(bitPeriod);
  for (int i = 0; i < 8; i++)
  {
    digitalWrite(GPS_TX_PIN, (c >> i) & 0x01);
    delayMicroseconds(bitPeriod);
  }
  digitalWrite(GPS_TX_PIN, HIGH);
  delayMicroseconds(bitPeriod);
}

void rossPrint(const char *str)
{
  while (*str)
    rossWrite(*str++);
}

//=============ORIGINAL CODE===========
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
//===============FLOOD GATES OPEN==============
//   (  !! for diagnostic testing only !! )
// ============================================================================
// PROVISIONAL CONFIGURATION: 5Hz DATA STORM EXPERIMENT
// ============================================================================
void setupGPS()
{
  pinMode(GPS_RX_PIN, INPUT_PULLUP);
  pinMode(GPS_TX_PIN, OUTPUT);
  digitalWrite(GPS_TX_PIN, HIGH);

  // 1. Send the proprietary UBX-CFG-RATE binary payload to reconfigure
  // the physical GPS engine from 1Hz (1000ms) to 5Hz (200ms updates).
  uint8_t cfgRate5Hz[] = {
      0xB5, 0x62, // UBX Sync Chars
      0x06, 0x08, // Class: CFG, ID: RATE
      0x06, 0x00, // Payload Length: 6 bytes
      0xC8, 0x00, // Measurement Period: 200ms (0x00C8)
      0x01, 0x00, // Navigation Cycle: 1
      0x01, 0x00, // Time Reference: UTC (1)
      0xDE, 0x6A  // Checksum A and B
  };

  // Stream the binary payload directly over the TX line
  for (uint8_t i = 0; i < sizeof(cfgRate5Hz); i++)
  {
    rossWrite(cfgRate5Hz[i]);
  }

  // 2. By leaving all $PUBX lines completely absent from this block,
  // the hardware will flood us with all sentences (RMC, GGA, GLL, GSA, GSV, VTG) at 5Hz.

  currentState = STATE_NEX_LISTENING;
}
// ============================================================================
*/
#endif