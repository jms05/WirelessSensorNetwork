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
const unsigned long interval = 3000; //ms


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
  Serial.println("RF24Network/examples/RF_Network_Client/");
 
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 89, /*node address*/ this_node);
  Serial.println("End Setup");
}

void recive_Packet(void* payload){
  while(true){
    Serial.println("TRUE");
    while ( network.available() ){
      Serial.println("Avaible");
      RF24NetworkHeader header;
      network.read(header,payload,sizeof(payload));
      return;
    }
    delay(500);
  }
  
}

int startConversation(){
  int tipo = -1;
  payload_t_question payload;
  Serial.print("Will start talk # ");
  while(tipo!=0){ //while is not start packet
    Serial.print(" Wait recie # ");
    recive_Packet (&payload);
    Serial.print(" Recived # ");
    tipo=payload.type;
    Serial.print("Type # ");
    Serial.println(tipo);
  }
  Serial.print("Conversion Started with # ");
  Serial.print(payload.idSender);
  Serial.print(" Massage # ");
  Serial.print(payload.message);
  return payload.idSender;
  
}

void talk(uint16_t remoteNode){
  int i=0;
  RF24NetworkHeader header(remoteNode);
  payload_t_question payload;
  int tipo =-1;
  while(tipo!=1){
    recive_Packet(&payload);
    tipo = payload.type;
    Serial.print("Tipo # ");
    Serial.print(tipo);
    if(tipo==2){
      Serial.print(" Talking with # ");
      Serial.print(payload.idSender);
      Serial.print(" Message # ");
      Serial.print(payload.message);
      Serial.print(" Time # ");
      Serial.println(payload.tim);
      i++;
      unsigned long now = millis();
      payload_t_response payload__R = { now, i ,this_node,'R'};
      
      bool ok = network.write(header,&payload__R,sizeof(payload__R));
      if(ok){
        Serial.print("Message #");
        Serial.print(ok);
        Serial.println(" ok.");
      }else{
        Serial.print("Message #");
        Serial.print(i);
        Serial.println(" failed.");
        i--;
      }
    }
  }
  Serial.print(" End Talk with # ");
  Serial.println(payload.idSender);
}

void loop(void)
{

  // Pump the network regularly
  network.update();
  Serial.println("Updated");
  while ( network.available() ){
  Serial.println("while");
    int remote =  startConversation();
    talk(remote);
    delay(2000);
  }
  delay(5000);
    
}
// vim:ai:cin:sts=2 sw=2 ft=cpp
