#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
//Include eeprom.h for AVR (Uno, Nano) etc. except ATTiny
#include <EEPROM.h>
#include "RF24TrustComunication.h"

#define NODE 0
RF24 radio(9,10);
RF24Network network(radio);
RF24Mesh mesh(radio,network);
RF24TrustComunication trustnet(network,mesh,NODE,true);


void setup() {
  // put your setup code here, to run once:

}

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
      char* message = (char*)pacote->data;
      Serial.print("Message: ");
      Serial.println(message);
      free(pacote);
    }
    trustnet.update();
    delay(500);

}
