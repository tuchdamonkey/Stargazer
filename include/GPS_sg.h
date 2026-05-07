#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h" // This defines GPS_RX_PIN as 3
#include "Diagnostics.h"

// --- 1. CONFIG & GLOBALS ---
#define SILO_SIZE 160
char gpsSilo[SILO_SIZE];
int siloIndex = 0;
bool siloReady = false;

unsigned long lastCarryTime = 0;
const unsigned long carryInterval = 10000;

extern char goldenPacket[85];
extern volatile int bufIndex;
extern volatile SystemState currentState;

// --- 2. THE VALIDATION FILTER ---
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

// --- 3. PRECISION TIMING (The Ross Magic) ---
char readRossByte()
{
  noInterrupts(); // Pause for timing perfection
  delayMicroseconds(146); // Calibrated jump to first bit center

  char incomingByte = 0;
  for (int i = 0; i < 8; i++)
  {
    SYNC_HIGH(); // D7 Visual Pulse Start
    if (digitalRead(GPS_RX_PIN) == HIGH)
    {
      incomingByte |= (1 << i);
    }
    SYNC_LOW(); // D7 Visual Pulse End
    delayMicroseconds(98); // Calibrated bit width
  }
  interrupts();
  return incomingByte;
}

// --- 4. THE ATOMIC CAPTURE ---
void captureGpsBurst()
{
  siloIndex = 0;
  siloReady = false;
  memset(gpsSilo, 0, SILO_SIZE);

  unsigned long startWait = millis();
  while (digitalRead(GPS_RX_PIN) == HIGH)
  {
    if (millis() - startWait > 1500)
      return;
  }

  while (siloIndex < SILO_SIZE)
  {
    char c = readRossByte();
    gpsSilo[siloIndex++] = c;

    toggleDiagnostic(); 

    if (c == '\n' && siloIndex > 100)
    {
      char *rmcStart = strstr(gpsSilo, "$GPRMC");
      char *ggaStart = strstr(gpsSilo, "$GPGGA");

      if (rmcStart && ggaStart)
      {
        if (isChecksumValid(rmcStart) && isChecksumValid(ggaStart))
        {
          siloReady = true;
        }
        else
        {
          for (int i = 0; i < 6; i++) { toggleDiagnostic(); delayMicroseconds(500); }
          siloReady = false;
        }
      }
      break;
    }
  }
}

// --- 5. HARDWARE UTILITIES ---
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

void rossPrint(const char *str) {
  while (*str) rossWrite(*str++);
}

void setupGPS()
{
  pinMode(GPS_RX_PIN, INPUT_PULLUP);
  pinMode(GPS_TX_PIN, OUTPUT);
  digitalWrite(GPS_TX_PIN, HIGH);

  // Muzzle unused NMEA sentences
  rossPrint("$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n");
  rossPrint("$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n");
  rossPrint("$PUBX,40,GSV,0,0,0,0,0,0*59\r\n");
  rossPrint("$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n");
  rossPrint("$PUBX,40,RMC,0,1,0,0,0,0*47\r\n");
  rossPrint("$PUBX,40,GGA,0,1,0,0,0,0*5A\r\n");

  // PCINT Configuration for Pin D3
  cli();
  PCICR |= (1 << PCIE2);    
  PCMSK2 |= (1 << PCINT19); 
  sei();

  currentState = STATE_IDLE;
}

ISR(PCINT2_vect)
{
  if (digitalRead(GPS_RX_PIN) == LOW && currentState == STATE_IDLE)
  {
    currentState = STATE_ACQUIRE;
  }
}

#endif