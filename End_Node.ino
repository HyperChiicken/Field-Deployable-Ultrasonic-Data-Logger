/*
  This program is for the stop node. The algorithm sends a stop command 
  to the start node to end data collection. The code is based on TMRh20's 
  RF communication using interrupts. 

  Augmented by: HyperChiicken
*/

#include <SPI.h>
#include "RF24.h"
#include <printf.h>

RF24 radio(9, 10);                  // set up nRF24L01 radio on SPI bus plus pins 9 & 10

uint8_t address[] = { "radio" };    // use the same address for both devices

/* Simple messages to represent a 'ping' and 'pong' */
uint8_t ping = 111;
uint8_t pong = 222;
int trigPin = 3;
int echoPin = 4;
int builtinLed = 13;
volatile uint32_t round_trip_timer = 0;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(builtinLed, OUTPUT);
  Serial.begin(115200);
  Serial.println(F("STOP NODE"));
  //printf_begin();
  
  radio.begin();                  // Setup and configure rf radio

  /* Use dynamic payloads to improve response time */
  radio.enableDynamicPayloads();
  radio.openWritingPipe(address);             // communicates with devices with the same address
  radio.openReadingPipe(1, address);
  radio.startListening();
}

void loop() {
  int duration;
  int distance;

  digitalWrite(builtinLed, LOW);
  
  digitalWrite(trigPin, HIGH);
  delay(1);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;           // in centimeters (search ultrasonic distance calculation for details)

  /* Send stop command if an object is detected within 45 cm. */
  if (distance <= 45 && distance >= 0) {
    digitalWrite(builtinLed, HIGH);
    radio.stopListening();
    round_trip_timer = micros();
    radio.write( &ping, sizeof(uint8_t), 0 );
  }
}
