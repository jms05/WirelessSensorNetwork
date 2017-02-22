/*
 Copyright (C) 2012 James Coliz, Jr. <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Simplest possible example of using RF24Network 
 *
 * TRANSMITTER NODE
 * Every 2 seconds, send a payload to the receiver node.
 */

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

// nRF24L01(+) radio attached using Getting Started board 
RF24 radio(9,10);

// Network uses that radio
RF24Network network(radio);

// Address of our node
const uint16_t this_node = 1;

// Address of the other node
const uint16_t other_node = 0;

// How often to send 'hello world to the other unit
const unsigned long interval = 2000; //ms

// When did we last send?
unsigned long last_sent;

// How many have we sent already
unsigned long packets_sent;

// Structure of our payload
// Structure of our payload
struct payload_t
{
  unsigned long ms;
  unsigned long counter;
  uint16_t idSender;
  char message[12];
};


//*genneric to recieve a packet
void recivePacket(void* payload,size_t tamanho){
  while (! network.available() ){
      network.update();
     Serial.println("Wait network");
     delay(500);
  }
  Serial.println("Recived");
  RF24NetworkHeader header;
  network.read(header,payload,tamanho);

}

//*genneric to send a packet
bool sendPacket(uint16_t remoteNode,void* payload,size_t tamanho){
  RF24NetworkHeader header(remoteNode);
  bool ok = network.write(header,payload,tamanho); 
  return ok;
}


void setup(void)
{
  Serial.begin(57600);
  Serial.println("RF24Network/examples/helloworld_tx_2_way/");
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  Serial.println("END SETUP");
}


void loop(void)
{

  // Pump the network regularly
  network.update();

  // If it's time to send a message, send it!
  unsigned long now = millis();
  if ( now - last_sent >= interval  )
  {
    last_sent = now;
    Serial.print("Sending...");
    payload_t payload = { millis(), packets_sent ,this_node,"HI From TX"};
    bool ok = sendPacket(other_node,&payload,sizeof(payload));
    if (ok){
      packets_sent++;
      Serial.println("ok.");
      payload_t payloadR; 
      recivePacket(&payloadR,sizeof(payloadR));
      Serial.print("Received packet #");
      Serial.print(payloadR.counter);
      Serial.print(" From #");
      Serial.print(payloadR.idSender);
      Serial.print(" at ");
      Serial.print(payloadR.ms);
      Serial.print(" Message: ");
      Serial.println(payloadR.message);
    } 
    else{
      Serial.println("failed.");
    }

  }
  
}
// vim:ai:cin:sts=2 sw=2 ft=cpp
