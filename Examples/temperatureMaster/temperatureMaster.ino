#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
//Include eeprom.h for AVR (Uno, Nano) etc. except ATTiny
#include <EEPROM.h>
#include "RF24TrustComunication.h"

#include <Narcoleptic.h>

//#include <Sleep_n0m1.h>
#include "DHT.h"
#include <SPI.h>
#include <RF24.h>


#define NODE 0
RF24 radio(9,10);
RF24Network network(radio);
RF24Mesh mesh(radio,network);
RF24TrustComunication trustnet(network,mesh,NODE,true);


void setup() {
  
}

typedef struct payloadS{
  float humidade;
  float temperature;
}payload,*payloadStruct;

void loop() {
  Serial.println("Wait recive ");
  uint8_t t = 0;
  int recived = trustnet.reciveData(DATATYPE_DATA,t);
  Serial.print("Recived code ");
  Serial.println(recived);
    bool onFila = trustnet.checkPendingReception();
    Serial.print("onFila code ");
    Serial.println(onFila);
    if(onFila){
      PktL2PRT pacote = trustnet.checkNextReception();
      Serial.println("\theader ");
      Serial.print("\tFrom: ");
      Serial.println(pacote->header->from_node);
      Serial.print("\tTO: ");
      Serial.println(pacote->header->to_node);
      Serial.print("\tNUM: ");
      Serial.println(trustnet.getPktNum(pacote->trustHeader));
      Serial.print("\tSize: ");
      Serial.println(trustnet.getPktSize(pacote->header->type));
      Serial.print("\tType: ");
      Serial.println(trustnet.getPktType(pacote->header->type));
      //pacote->trustHeader
      payloadStruct message = (payloadStruct)pacote->data;
      Serial.print("humidade: ");
      Serial.println(message->humidade);
      Serial.print("temperature: ");
      Serial.println(message->temperature);
      free(pacote);
    }
    trustnet.update();
    delay(500);
}
