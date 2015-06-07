#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define BAUDRATE 57600

const int led_pin = 8;

ArduinoNunchuk nunchuk = ArduinoNunchuk();

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup()
{
  Serial.begin(BAUDRATE);
  printf_begin();
  pinMode(led_pin, OUTPUT);
  nunchuk.init();
  radio.begin();
  radio.setChannel(112);
  radio.setPayloadSize(2);  
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);  
  radio.printDetails();
}

void loop()
{
  nunchuk.update();
  
  byte car_data[2];
  car_data[0] = nunchuk.analogX;
  car_data[1] = nunchuk.analogY;
  
  Serial.print(car_data[0], DEC);
  Serial.print(" ");
  Serial.print(car_data[1], DEC);
  Serial.print("   ");
  
  radio.stopListening();
  bool sent;
  sent = radio.write( &car_data, 2 );
  if(sent) {
    Serial.print(" sent ");
  } else {
    Serial.print(" FAIL ");
  }
  
  // Now, continue listening
  radio.startListening();

  // Wait here until we get a response, or timeout (200ms)
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
    byte response[2];
    radio.read( &response, sizeof(response) );
    Serial.print(response[0], DEC);
    Serial.print(" ");
    Serial.print(response[1], DEC);
    Serial.print("   ");
    
    if (response[0] == car_data[0] && response[1] == car_data[1]) 
    {
      digitalWrite(led_pin,HIGH);
      Serial.println("Ok");
    }
    else
    {
      digitalWrite(led_pin,LOW);
      Serial.println("Error?");
    }
  }
  // Try again later
  delay(100);  
  digitalWrite(led_pin,LOW);
}
