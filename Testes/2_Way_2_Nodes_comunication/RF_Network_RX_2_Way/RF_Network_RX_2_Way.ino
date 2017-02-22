/*
 Copyright (C) 2012 James Coliz, Jr. <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Simplest possible example of using RF24Network,
 *
 * RECEIVER NODE
 * Listens for messages from the transmitter and prints them out.
 */

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

// nRF24L01(+) radio attached using Getting Started board 
RF24 radio(9,10);

// Network uses that radio
RF24Network network(radio);

// Address of our node
const uint16_t this_node = 0;

// Address of the other node
const uint16_t other_node = 1;

// How often to send 'delay to send a response
const unsigned long interval = 500; //ms


// Structure of our payload
struct payload_t
{
  unsigned long ms;
  unsigned long counter;
  uint16_t idSender;
  char message[12];
};


unsigned long counter;

//*genneric to recieve a packet
void recivePacket(void* payload,size_t tamanho){
  while (!network.available() ){
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
  Serial.println("RF24Network/examples/helloworld_rx_2_way/");
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  Serial.println("end setup");
}

void loop(void)
{
  // Pump the network regularly
  network.update();
  bool aborta=false;
  // Is there anything ready for us?
  
  // If so, grab it and print it out
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
  if(this_node!=payloadR.idSender){
    Serial.println("É meu");
    //recived packet
    delay(interval);
    //resposta
    payload_t payload = {millis() , counter ,this_node,"HI From RX"};
    bool ok = false;
    while(!ok){
      Serial.print("Sending... ");
      Serial.print(counter);
      ok= sendPacket(other_node,&payload,sizeof(payload));
      if (ok)  Serial.println(" ok.");
      else  Serial.println(" failed.");
    }
    counter++;
  }
    //sent whiil wait for another
}

