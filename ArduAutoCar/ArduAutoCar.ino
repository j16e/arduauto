#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

const int E1 = 6;  
const int M1 = 7;
const int E2 = 5;  
const int M2 = 4;

#define trigPin 2
#define echoPin 3
//#define trigPin A5
//#define echoPin A4
const int obstacle_pin = 8;

class RF24Test: public RF24
{
public: RF24Test(int a, int b): RF24(a,b) {}
};

RF24Test radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Deadman's Switch
unsigned long last_contact;

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting car.");
  pinMode(obstacle_pin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  radio.begin();  
  radio.setPayloadSize(2);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();  
}

void loop()
{
  // if there is data ready
  if ( radio.available() ) {
    byte transmission[2];
    bool done = false;
    
    // Fetch the payload, and see if this was the last one.
    radio.read( &transmission, sizeof(transmission) );

    // Spew it
    Serial.print(transmission[0]);
    Serial.print(" ");
    Serial.print(transmission[1]);
    Serial.print("   ");
    
    byte speedValue = transmission[0];
    byte turnValue = transmission[1];
    
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
    delay(10);

    // First, stop listening so we can talk
    radio.stopListening();

    // Send the final one back.
    byte response = B0;
    radio.write( &response, sizeof(response) );
//      Serial.println("Sent response.");

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  } else if (millis() - last_contact > 1000) {
    speed(128, false); //If there is no contact with the controller, stop the car
    turn(128);
  }  
}

void speed(byte speedValue, bool obstacle)
{
  if (speedValue < 127)
         {
            int mappedVal = map(speedValue,0,126,0,255);
            //Going reverse
            digitalWrite(M1,HIGH);        
            analogWrite(E1, mappedVal);   //PWM Speed Control
            delay(10); 
         } else if (speedValue > 129 && !obstacle )
         {
            //Going forward
            int mappedVal = map(speedValue,130,255,0,255);
            digitalWrite(M1,LOW);         
            analogWrite(E1, mappedVal);   //PWM Speed Control
            delay(10); 
         } else
         {
             Serial.println("Obstacle - stopping");
             digitalWrite(M1,LOW); 
             analogWrite(E1, 0);
         }
}

void turn(byte turnValue)
{
  if (turnValue < 125) {
    //Turn right
    digitalWrite(M2,LOW);
    analogWrite(E2, 255);
    delay(10); 
  } else if (turnValue > 130) {
    //Turn left
    digitalWrite(M2,HIGH);         
    analogWrite(E2, 0); 
    delay(10); 
  } else {
    digitalWrite(M2,LOW);
    analogWrite(E2, 0); //Set turning wheels to straight.
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
