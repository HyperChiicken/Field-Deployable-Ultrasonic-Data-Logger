/*
  This program is for the start node. The algorithm waits for an object within a given distance
  and executes the data collection. The code is based on TMRh20's RF communication using interrupts
  and 1Sheeld's sample template. 

  Augmented by: HyperChiicken
*/

#define CUSTOM_SETTINGS
#define INCLUDE_DATA_LOGGER_SHIELD
#define INCLUDE_ACCELEROMETER_SENSOR_SHIELD
#define INCLUDE_ORIENTATION_SENSOR_SHIELD
#define INCLUDE_TERMINAL_SHIELD

/* Include 1Sheeld library. */
#include <OneSheeld.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);                          // set up nRF24L01 radio on SPI bus plus pins 9 & 10

uint8_t address[] = { "radio" };            // use the same address for both devices

/* Simple messages to represent a 'ping' and 'pong' */
uint8_t ping = 111;
uint8_t pong = 222;
int trigPin = 4;
int echoPin = 5;
int greenLed = 7;
int redLed = 8;
volatile uint32_t round_trip_timer = 0;
bool startFlag = false;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  Serial.begin(115200);

  /* Start communication. */
  OneSheeld.begin();
  /* Save any previous logged values. */
  Logger.stop();

  //printf_begin();
  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, HIGH);
  
  radio.begin();                               // setup and configure rf radio

  // Use dynamic payloads to improve response time
  radio.enableDynamicPayloads();
  radio.openWritingPipe(address);             // communicate with devices with the same address
  radio.openReadingPipe(1, address);
  radio.startListening();

  //radio.printDetails();                             // Dump the configuration of the rf unit for debugging

  attachInterrupt(0, check_radio, LOW);             // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver
}

void loop() {

  int duration;
  int distance;

  digitalWrite(trigPin, HIGH);
  delay(1);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;                 //in centimeters (search ultrasonic distance calculation for details)

  /* Start data collection when object is detected within 45 cm */
  if (distance <= 45 && distance >= 0) {
    {
      /* First insure to save previous logged values. */
      Logger.stop();
      /* Set a delay. */
      OneSheeld.delay(500);
      /* Start logging in a new CSV file. */
      Logger.start("TEST");
      /* Set startFlag. */
      startFlag = true;
    }

    /* Check logging started. */
    while (startFlag)
    {
      collectStart();
      if (!startFlag) {
        collectStop();
      }
    }
  }
}

/* Set data to be taken from the phone */
void collectStart() {
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, LOW);
  /* Add orientation values as a column in the CSV file. */
  Logger.add("Degrees", OrientationSensor.getX());
  /* Add x-acceleration values as a column in the CSV file. */
  Logger.add("X-axis m/s^2", AccelerometerSensor.getX());
  /* Add y-acceleration values as a column in the CSV file. */
  Logger.add("Y-axis m/s^2", AccelerometerSensor.getY());
  /* Log the row in the file. */
  Logger.log();
  /* Delay for 1 second. */
  OneSheeld.delay(200);
}

void collectStop() {
  Logger.stop();
  OneSheeld.delay(500);
  
  /* Start logging in a new CSV file. */
  Logger.stop();
  Logger.start("TEST");
    
  Terminal.println(startFlag);
}

/* Interrupt */
void check_radio(void)                                // Receiver role: Does nothing!  All the work is in IRQ
{
  bool tx, fail, rx;
  radio.whatHappened(tx, fail, rx);                   // What happened?

  /* If data is available, handle it accordingly */
  if ( rx ) {

    if (radio.getDynamicPayloadSize() < 1) {
      /* Corrupt payload has been flushed */
      return;
    }
    /* Read in the data */
    uint8_t received;
    radio.read(&received, sizeof(received));

    /* If this is a ping, send back a pong */
    if (received == ping) {
      radio.stopListening();
      /* Normal delay will not work here, so cycle through some no-operations (16nops @16mhz = 1us delay) */
      for (uint32_t i = 0; i < 130; i++) {
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
      }
      radio.startWrite(&pong, sizeof(pong), 0);
      digitalWrite(redLed, HIGH);
      digitalWrite(greenLed, LOW);
      startFlag = false;
    } else
      /* If this is a pong, get the current micros() */
      if (received == pong) {
        round_trip_timer = micros() - round_trip_timer;
      }
  }

  /* Start listening if transmission is complete */
  if ( tx || fail ) {
    radio.startListening();
  }
}
