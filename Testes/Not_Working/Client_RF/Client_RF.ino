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
struct payload_t_response
{
  unsigned long ms;
  unsigned long counter;
  uint16_t idSender;
  char message[10];
};

struct payload_t_question
{
  uint16_t idSender;
  unsigned int type;
  char message[10];
  int tim;
};

//*genneric to recieve a packet
void recivePacket(void* payload,size_t tamanho){
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
  Serial.println("RF24Network/examples/Cliente_RF/");
 
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  Serial.println("End Setup");
}

void loop(void)
{
  int c = 0;
  // Pump the network regularly
  network.update();
  payload_t_question payload;
  bool recvid =false;
  uint16_t remoteNode = -1;
  while(network.available()){
    Serial.print("Have To me ");
    recivePacket(&payload,sizeof(struct payload_t_question));
    recvid=true;
    remoteNode = payload.idSender;
    Serial.print("Received packet #");
    Serial.print(payload.tim);
    Serial.print(" From #");
    Serial.print(payload.idSender);
    Serial.print(" tipo ");
    Serial.print(payload.type);
    Serial.print(" Message: ");
    Serial.println(payload.message);
    
  }
  int tipo = payload.type;
  while(recvid && tipo!=1){
    //if(/* &payload!=NULL*/recvid){
      uint16_t remote = payload.idSender;
      Serial.print("Remote addr ");
      Serial.println(remote);
      tipo=payload.type; 
      while(!network.available()){
        //recebi o 1espero pelo seguinte
        Serial.println("Espera rede");
        delay(1000);
      }
      recivePacket(&payload,sizeof(struct payload_t_question));
      Serial.print("Received packet #");
      Serial.print(payload.tim);
      Serial.print(" From #");
      Serial.print(payload.idSender);
      Serial.print(" tipo ");
      Serial.print(payload.type);
      tipo=payload.type;
      Serial.print(" Message: ");
      Serial.println(payload.message);
      //recebi uma hello
      if(tipo!=1){
        payload_t_response payloadR = {millis() , c++ ,this_node,"HI"};
        bool ok =false;
        while(!ok){
          Serial.print("Sending... ");
          Serial.print(c);
          ok = sendPacket(remoteNode,&payloadR,sizeof(payload_t_response));
          if (ok)
            Serial.println(" ok.");
          else
            Serial.println(" failed.");
          }
      }
    } 
      
}
// vim:ai:cin:sts=2 sw=2 ft=cpp
