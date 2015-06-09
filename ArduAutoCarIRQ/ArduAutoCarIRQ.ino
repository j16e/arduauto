//#include <L9110Driver.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

// pins for ping sensor
#define trigPin 2
#define echoPin 3
const int obstacle_pin = 8;

// Define the radio
RF24 radio(9,10);

// Define the motors on the L9110 bridge
//L9110_Motor drive(5, 4);
//L9110_Motor steer(6, 7);
const int drive_ia = 5;
const int drive_ib = 4;
const int steer_ia = 6;
const int steer_ib = 7;

// Radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

// Interrupt handler, check the radio because we got an IRQ
void check_radio(void);

// Deadman's Switch
unsigned long last_contact;

// Software reset function
void(* resetFunc) (void) = 0; //declare reset function @ address 0

// track current speed and direction so we can transition direction safely
byte currentVector[2] = {127,127};
byte newVector[2] = {127,127};

void setup()
{
  Serial.begin(57600);
  printf_begin();
  printf("Starting car.\n\r");
  pinMode(obstacle_pin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // motors
  printf("Defining motors...");
  pinMode(drive_ia, OUTPUT);
  pinMode(drive_ib, OUTPUT);
  pinMode(steer_ia, OUTPUT);
  pinMode(steer_ib, OUTPUT);
  
  // radio
  printf("Enabling radio...\n\r");
  radio.begin();
  radio.setChannel(112);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.setAutoAck(1);
  radio.setCRCLength(RF24_CRC_8);
  radio.setPayloadSize(2);
  radio.openReadingPipe(1,pipe);
  radio.startListening();
  radio.writeAckPayload(1, &currentVector, sizeof(currentVector));
  radio.printDetails();
  
  // Set the Deadman's switch
  last_contact = millis();
  
  // attach interrupt handler on pin 2
  attachInterrupt(0, check_radio, FALLING);
}

void loop()
{
  // Lost connection checks
  if (millis() - last_contact > 3000) {
    printf("No contact for 3 seconds, resetting...\n\r");
    resetFunc();
  } else if (millis() - last_contact > 200) {
    printf("Lost contact, stop!\n\r");
    newVector[0] = 127; // reset to neutral steering position
    newVector[1] = 127; // reset drive position to stop
  }  

  /*
   * TODO: Add back in obstacle detection.
   */
  speed(newVector[1], false);
  turn(newVector[0]);
}

void speed(byte speedValue, bool obstacle)
{
  if (currentVector[1] == speedValue) {
    // same as previous, no change :)
    return;
  }
  
  if ((int)speedValue < 127) {
    //int mappedVal = map((int)speedValue,0,126,127,191);
    if (currentVector[1] > 129) {
      // direction change! protect the motors
      digitalWrite(drive_ib, LOW);
      analogWrite(drive_ia, 0);
      delay(100);
    }
    // reverse
    digitalWrite(drive_ib, HIGH);
    analogWrite(drive_ia, 0);
  } else if ((int)speedValue > 129 && !obstacle ) {
    //int mappedVal = map((int)speedValue,130,255,63,127);
    if (currentVector[1] < 127) {
      // direction change! protect the motors
      digitalWrite(drive_ib, LOW);
      analogWrite(drive_ia, 0);
      delay(100);
    }
    // forward
    digitalWrite(drive_ib, LOW);
    analogWrite(drive_ia, 255);
  } else {
    // stopping
    digitalWrite(drive_ib, LOW);
    analogWrite(drive_ia, 0);
  }
  currentVector[1] = speedValue;
}

void turn(byte turnValue)
{
  if (currentVector[0] == turnValue) {
    // same as previous, no change :)
    return;
  }
  
  if (turnValue < 125) {
    // left
    digitalWrite(steer_ib, HIGH);
    analogWrite(steer_ia, 0);
  } else if (turnValue > 130) {
    // right
    digitalWrite(steer_ib, LOW);
    analogWrite(steer_ia, 255);
  } else {
    // straight
    digitalWrite(steer_ib, LOW);
    analogWrite(steer_ia, 0);
  }
  currentVector[0] = turnValue;
}

boolean obstacle()
{
  long duration, distance;
  digitalWrite(trigPin, LOW);  // Added this line
  delayMicroseconds(2); // Added this line
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); // Added this line
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration / 58;

  if (distance >= 30 || distance <= 0){
    Serial.println("No obstacle");
    digitalWrite(obstacle_pin,LOW);
    return false;
  }
  else {
//    Serial.print("DANGER! ");
    Serial.print(distance);
    Serial.println(" cm");
    digitalWrite(obstacle_pin,HIGH);
    return true;
  }
}

// interrupt handler
void check_radio(void)
{
  // What happened?
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);

  // Have we successfully transmitted?
  if ( tx )
  {
    printf("Ack Payload:Sent\n\r");
  }

  // Have we failed to transmit?
  if ( fail )
  {
    printf("Ack Payload:Failed\n\r");
  }

  // Did we receive a message?
  if ( rx )
  {
    // Get this payload and dump it
    radio.read( &newVector, sizeof(newVector) );
    printf("New Vector [%3d,%3d]\n\r", newVector[0], newVector[1]);

    // Add an ack packet for the next time around.
    radio.writeAckPayload(1, &currentVector, sizeof(currentVector));
    
    // reset the deadman's switch
    last_contact = millis();
  }
}

