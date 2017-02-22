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
uint16_t other_node[3];// = [1,2,3];

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

bool startConversation(uint16_t remoteNode){
  Serial.print("Sending Start...");
  payload_t_question payload = { this_node, 0 ,"Start",0};
  bool ok = sendPacket(remoteNode,&payload,sizeof(payload));
  //RF24NetworkHeader header(/*to node*/ remoteNode);
  //bool ok = network.write(header,&payload,sizeof(payload));
  if (ok)
      Serial.println("ok.");
    else
      Serial.println("failed.");
  return ok;
}


bool endConversation(uint16_t remoteNode){
  Serial.print("Sending Stop...");
  payload_t_question payload = { this_node, 1 ,"STOP",0};
  bool ok = sendPacket(remoteNode,&payload,sizeof(payload));
  //RF24NetworkHeader header(/*to node*/ remoteNode);
  //bool ok = network.write(header,&payload,sizeof(payload));
  if (ok)
      Serial.println("ok.");
    else
      Serial.println("failed.");
  return ok;
}

void setup(void)
{
  Serial.begin(57600);
  Serial.println("RF24Network/examples/Server_RF/");
 
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
   other_node[0]=1;
  other_node[1]=2;
  other_node[2]=3;
  Serial.println("End Setup");
}




void loop(void)
{
  // Pump the network regularly
  network.update();
  uint16_t remoteNode = other_node[0];
  int tim =0;
  bool started = startConversation(remoteNode);
  // have the remote waitinf for me
  delay(2000);
  while (tim < 5 && started)
  {
    tim++;
    payload_t_question  payload = { this_node, 2 ,"Hello",tim};
    bool ok =false;
    while(!ok){
      Serial.print("Sending...");
      ok = sendPacket(remoteNode,&payload,sizeof(payload_t_question));
      if (ok)
        Serial.println("ok.");
      else
        Serial.println("failed.");  
    }
    while(!network.available() ){
      Serial.println("Espera rede");
      delay(1000);
    }
      payload_t_response payloadR;
      recivePacket(&payloadR,sizeof(struct payload_t_response));
      Serial.print("Received packet #");
      Serial.print(payloadR.counter);
      Serial.print(" From #");
      Serial.print(payloadR.idSender);
      Serial.print(" at ");
      Serial.print(payloadR.ms);
      Serial.print(" Message: ");
      Serial.println(payloadR.message);
  }
  bool stoped = false;
  while(started && !stoped){
    stoped  = endConversation(remoteNode);
  }
  
  delay(1000);
}
// vim:ai:cin:sts=2 sw=2 ft=cpp
