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


#define NODE 1
RF24 radio(9,10);
RF24Network network(radio);
RF24Mesh mesh(radio,network);
RF24TrustComunication trustnet(network,mesh,NODE,true);



void setup() {

}

void loop() {
  trustnet.update();
  delay(1000);
}
