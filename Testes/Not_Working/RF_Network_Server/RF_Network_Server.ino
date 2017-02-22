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
  char message;
};

struct payload_t_question
{
  uint16_t idSender;
  unsigned long type;
  char message;
  int tim;
};

void setup(void)
{
  Serial.begin(57600);
  Serial.println("RF24Network/examples/RF_Network_Server/");
 
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 89, /*node address*/ this_node);
  other_node[0]=1;
  other_node[1]=2;
  other_node[2]=3;
  Serial.println("End Setup");
  
}

bool startConversation(uint16_t remoteNode){
  payload_t_question payload = { this_node, 0 ,'0',0};
  RF24NetworkHeader header(remoteNode);
  bool ok = network.write(header,&payload,sizeof(payload));
  return ok;
}

void reciveAnswer(void* payload){
  RF24NetworkHeader header;
  network.read(header,&payload,sizeof(payload));
}

void talk(int times,uint16_t remoteNode ){
  int tim=0;
  while(tim<times){
    tim++;
    payload_t_question payload = { this_node, 2 ,'A',tim};
    RF24NetworkHeader header(remoteNode);
    bool ok = network.write(header,&payload,sizeof(payload));
    if (ok){
      Serial.print("Message #");
      Serial.print(tim);
      Serial.println(" ok.");
      payload_t_response payload;
      reciveAnswer(&payload);
      Serial.print("Received packet #");
      Serial.print(payload.counter);
      Serial.print(" From #");
      Serial.print(payload.idSender);
      Serial.print(" at ");
      Serial.print(payload.ms);
      Serial.print(" Message: ");
      Serial.println(payload.message);
      
    }
    else{
      Serial.print("Message #");
      Serial.print(tim);
      Serial.println(" failed.");
    }
  }
}

bool stopTalk(uint16_t remoteNode){
  payload_t_question payload = { this_node, 1 ,'1',0};
  RF24NetworkHeader header(remoteNode);
  bool ok = network.write(header,&payload,sizeof(payload));
  return ok;
}

void loop(void)
{
  // Pump the network regularly
  network.update();
  uint16_t remoteNode = other_node[0];
  // Is there anything ready for us?
  while ( network.available() )
  {
    bool started = startConversation(remoteNode);
    if (started){
      Serial.print("Conversion Started with # ");
      Serial.println(remoteNode);
      talk(5,remoteNode);
      Serial.print("Vill stop with # ");
      Serial.println(remoteNode);
      bool stoped=false;
      while(!stoped){
        stoped=stopTalk(remoteNode);
      }
      Serial.print("Stopped stop with # ");
      Serial.println(remoteNode);
    }else{
      Serial.print("Cannot start with # ");
      Serial.println(remoteNode);
      delay(5000);
    }
    delay(2000);
    // If so, grab it and print it out
    /*RF24NetworkHeader header;
    payload_t payload;
    network.read(header,&payload,sizeof(payload));
    Serial.print("Received packet #");
    Serial.print(payload.counter);
    Serial.print(" From #");
    Serial.print(payload.idSender);
    Serial.print(" at ");
    Serial.print(payload.ms);
    Serial.print(" Message: ");
    Serial.println(payload.message);*/
  }
}
// vim:ai:cin:sts=2 sw=2 ft=cpp
