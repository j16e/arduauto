#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define BAUDRATE 57600

const int led_pin = 13;

ArduinoNunchuk nunchuk = ArduinoNunchuk();

class RF24Test: public RF24
{
public: RF24Test(int a, int b): RF24(a,b) {}
};

RF24Test radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup()
{
  Serial.begin(BAUDRATE);
  pinMode(led_pin, OUTPUT);
  nunchuk.init();
  radio.begin();
  radio.setPayloadSize(2);  
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);  
  radio.printDetails();
}

void loop()
{
  nunchuk.update();
  
//  byte analogX = nunchuk.analogX;
//  byte analogY = nunchuk.analogY;
//  byte zButton = nunchuk.zButton;
//  byte cButton = nunchuk.cButton;
  
  byte car_data[2];
  car_data[0] = nunchuk.analogX;
  car_data[1] = nunchuk.analogY;
//  car_data[2] = zButton;
//  car_data[3] = cButton;
  
  Serial.print(car_data[0], DEC);
  Serial.print(" ");
  Serial.print(car_data[1], DEC);
  Serial.print("   ");
//  Serial.print(car_data[2], DEC);
//  Serial.print(' ');
//  Serial.print(car_data[3], DEC);
  
  radio.stopListening();
  radio.write( &car_data, 2 );
  // Now, continue listening
  radio.startListening();

  // Wait here until we get a response, or timeout (250ms)
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  while ( ! radio.available() && ! timeout ) {
    if (millis() - started_waiting_at > 200 ) {
        timeout = true;
    }
  }

  // Describe the results
  if ( timeout )
  {
    Serial.println("Failed, response timed out.");     
  }
  else
  {
    // Grab the response, compare, and send to debugging spew
    byte response;
    radio.read( &response, sizeof(response) );
    Serial.println(response,BIN);
    
    if (response == B0) 
    {
      digitalWrite(led_pin,HIGH);
      Serial.println("Ok");
    }
    else
    {
      digitalWrite(led_pin,LOW);
      Serial.println("No connection");
    }
  }
  // Try again  later
  delay(150);  
  digitalWrite(led_pin,LOW);
}
