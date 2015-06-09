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

// Radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

// Interrupt handler, check the radio because we got an IRQ
void check_radio(void);

void setup()
{
  // Setup debug
  Serial.begin(BAUDRATE);
  printf_begin();
  
  // pin mode output for indicator
  pinMode(led_pin, OUTPUT);
  
  // Initalise nunchuk
  nunchuk.init();
  
  // Initialise Radio
  radio.begin();
  radio.setChannel(112);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.setAutoAck(1);
  radio.setCRCLength(RF24_CRC_8);
  radio.setPayloadSize(2);
  radio.openWritingPipe(pipe);
  radio.stopListening();
  radio.printDetails();
  
  // attach interrupt handler on pin 2
  attachInterrupt(0, check_radio, FALLING);
}

void loop()
{
  // get the nunchuck position
  nunchuk.update();
  
  // Setup the data packet
  byte car_data[2];
  car_data[0] = nunchuk.analogX;
  car_data[1] = nunchuk.analogY;
  
  // print debug
  char req[14] = " ";
  sprintf(req, "req [%3d,%3d]", (int)car_data[0], (int)car_data[1]);
  printf(req);
  
  radio.startWrite( &car_data, 2 );
  
  // Try again later
  delay(50);
}

static byte response[2];

void check_radio(void)
{
  // What happened?
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);

  // Have we successfully transmitted?
  if ( tx )
  {
    printf("Send:OK\n\r");
  }

  // Have we failed to transmit?
  if ( fail )
  {
    printf("Send:Failed\n\r");
  }

  // Transmitter can power down for now, because
  // the transmission is done.
  if ( tx || fail ) {
    radio.powerDown();
  }

  // Did we receive a message?
  if ( rx )
  {
    radio.read(&response,sizeof(response));
    char resp[15] = " ";
    sprintf(resp, "resp [%3d,%3d]", response[0], response[1]);
    printf(resp);
  }
}
