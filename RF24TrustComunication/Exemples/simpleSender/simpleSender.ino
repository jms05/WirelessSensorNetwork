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
   char* payload = "OLA MUNDO CONF";
  Serial.println("Will Send ");
  int sentCode = trustnet.sendRData((byte*)payload,strlen(payload),3);
  Serial.print("Sent code ");
  Serial.println(sentCode);
  delay(100);
  trustnet.update();
  delay(500);
}
