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

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Deadman's Switch
unsigned long last_contact;

void setup()
{
  Serial.begin(57600);
  printf_begin();
  Serial.println("Starting car.");
  pinMode(obstacle_pin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // motors
  Serial.println("Defining motors...");
  pinMode(drive_ia, OUTPUT);
  pinMode(drive_ib, OUTPUT);
  pinMode(steer_ia, OUTPUT);
  pinMode(steer_ib, OUTPUT);
  //drive.run(RELEASE);
  //drive.setSpeed(0);
  //steer.run(RELEASE);
  //steer.setSpeed(0);
  
  // radio
  Serial.println("Enabling radio...");
  radio.begin();  
  Serial.println("setting channel");
  radio.setChannel(112);
  Serial.println("setting payload size");
  radio.setPayloadSize(2);
  Serial.println("Opening pipes");
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  Serial.println("start listening");
  radio.startListening();  
  Serial.println("radio debug");
  radio.printDetails();
  
  last_contact = millis();
}

void loop()
{
  // if there is data ready
  if ( radio.available() ) {
    byte transmission[2];
    bool done = false;
    
    // Fetch the payload, and see if this was the last one.
    while (!done) {
      done = radio.read( &transmission, sizeof(transmission) );
    }

    // Spew it
    Serial.print(transmission[0]);
    Serial.print(" ");
    Serial.print(transmission[1]);
    Serial.print("   ");
    
    byte speedValue = transmission[1];
    byte turnValue = transmission[0];
    
//        if (obstacle()) 
//        {
//          speed(speedValue, true);          
//        }
//        else
//        {
    speed(speedValue, false);
//        }

    turn(turnValue);
 
    // Delay just a little bit to let the other unit
    // make the transition to receiver
    delay(20);

    // First, stop listening so we can talk
    radio.stopListening();

    // Send the original transmission back
    radio.write( &transmission, sizeof(transmission) );
    Serial.println("Sent response.");

    // Now, resume listening so we catch the next packets.
    radio.startListening();
    last_contact = millis();
  } else if (millis() - last_contact > 1000) {
    Serial.println("No contact");
    speed(128, false); //If there is no contact with the controller, stop the car
    turn(128);
    last_contact = millis();
    
    radio.startListening();
  }  
}

void speed(byte speedValue, bool obstacle)
{
  if ((int)speedValue < 127) {
    //int mappedVal = map((int)speedValue,0,126,127,191);
    // reverse
    digitalWrite(drive_ib, HIGH);
    analogWrite(drive_ia, 0);
    //drive.run(FORWARD|RELEASE);
    //drive.setSpeed(mappedVal);
  } else if ((int)speedValue > 129 && !obstacle ) {
    //int mappedVal = map((int)speedValue,130,255,63,127);
    // forward
    digitalWrite(drive_ib, LOW);
    analogWrite(drive_ia, 255);
    //drive.run(BACKWARD|RELEASE);
    //drive.setSpeed(mappedVal);
  } else {
    // stopping
    digitalWrite(drive_ib, LOW);
    analogWrite(drive_ia, 0);
    //drive.run(RELEASE);
    //drive.setSpeed(0);
  }
}

void turn(byte turnValue)
{
  if (turnValue < 125) {
    // left
    digitalWrite(steer_ib, HIGH);
    analogWrite(steer_ia, 0);
    //steer.run(FORWARD|RELEASE);
    //steer.setSpeed(255);
  } else if (turnValue > 130) {
    // right
    digitalWrite(steer_ib, LOW);
    analogWrite(steer_ia, 255);
    //steer.run(BACKWARD|RELEASE);         
    //steer.setSpeed(255);
  } else {
    // straight
    digitalWrite(steer_ib, LOW);
    analogWrite(steer_ia, 0);
    //steer.run(RELEASE);
    //steer.setSpeed(0);
  }
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
  distance = (duration/2) / 29.1;

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
