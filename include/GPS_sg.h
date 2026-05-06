#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h"

// --- Global Buffer & State (Volatile is required for ISR safety) ---
extern char goldenPacket[85];
extern volatile int bufIndex;
extern volatile SystemState currentState;

// --- 1. The Core Utilities (Precision Timing) ---

char readRossByte()
{
  // 1. Initial jump to Bit 0 center
  delayMicroseconds(135); 

  char incomingByte = 0;
  for (int i = 0; i < 8; i++)
  {
    // --- DIAGNOSTIC PING ---
    PORTD |= (1 << PD7);  // Set D7 HIGH (Direct Port Manipulation is faster)
    
    if (digitalRead(GPS_RX_PIN) == HIGH)
      incomingByte |= (1 << i);
      
    PORTD &= ~(1 << PD7); // Set D7 LOW
    // -----------------------

    delayMicroseconds(94); 
  }
  return incomingByte;
}

void rossWrite(char c)
{
  const uint16_t bitPeriod = 104;
  digitalWrite(GPS_TX_PIN, LOW); // Start Bit
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
    rossWrite(*str++);
}

// --- 2. The Bridge-Guard Filter ---

void processGPSByte(char c)
{
  if (c == '$')
  {
    bufIndex = 0;
  }

  if (bufIndex < 84)
  {
    goldenPacket[bufIndex++] = c;
    goldenPacket[bufIndex] = '\0';
  }

  // HEADER FILTER: Reject non-RMC/GGA sentences after first 6 chars
  if (bufIndex == 6)
  {
    if (strstr(goldenPacket, "RMC") == NULL && strstr(goldenPacket, "GGA") == NULL)
    {
      bufIndex = 0;
      currentState = STATE_IDLE; // Kill the sentence early
      return;
    }
  }

  // END OF SENTENCE: Trigger validation
  if (c == '\n')
  {
    currentState = STATE_VALIDATE;
  }
  else
  {
    // STAGE 2 FIX: Stay in IDLE while waiting for the NEXT character's start bit.
    // The ISR will flip us back to ACQUIRE when the next byte starts falling.
    currentState = STATE_IDLE;
  }
}

// --- 3. The Setup & Interrupt Architecture ---

void setupGPS()
{
  pinMode(GPS_RX_PIN, INPUT_PULLUP);
  pinMode(GPS_TX_PIN, OUTPUT);
  digitalWrite(GPS_TX_PIN, HIGH);

  // Initial hardware muzzle (UBX commands for NEO-7M)
  rossPrint("$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n");
  rossPrint("$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n");
  rossPrint("$PUBX,40,GSV,0,0,0,0,0,0*59\r\n");
  rossPrint("$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n");
  rossPrint("$PUBX,40,RMC,0,1,0,0,0,0*47\r\n");
  rossPrint("$PUBX,40,GGA,0,1,0,0,0,0*5A\r\n");

  // ENABLE PCINT (Pin Change Interrupt) for D3
  cli();
  PCICR |= (1 << PCIE2);    // Enable Port D interrupts
  PCMSK2 |= (1 << PCINT19); // Trigger on Digital Pin 3
  sei();

  currentState = STATE_IDLE;
}

// THE SENTRY: This ISR catches the GPS talking in the background
ISR(PCINT2_vect)
{
  // If the pin dropped LOW and we are IDLE, start bit detected
  if (digitalRead(GPS_RX_PIN) == LOW && currentState == STATE_IDLE)
  {
    currentState = STATE_ACQUIRE;
  }
}

#endif