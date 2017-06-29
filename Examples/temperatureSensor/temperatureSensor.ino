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

#define MODE 1 //1 = R data, other = u data
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE);

#define NODE 10
RF24 radio(9,10);
RF24Network network(radio);
RF24Mesh mesh(radio,network);
RF24TrustComunication trustnet(network,mesh,NODE,true);


float humi;
float temperature;

void setup() {
  dht.begin();
  humi=-999;
  temperature=-999;
}

typedef struct payloadS{
  float humidade;
  float temperature;
}payload,*payloadStruct;

void loop() {
  payloadStruct p  =(payloadStruct)malloc(sizeof(payload)) ;
  humi = dht.readHumidity();
  temperature = dht.readTemperature();
   if(isnan(humi)){
        humi=-999;
  }
  if(isnan(temperature)){
         temperature=-999;
  }
  p->humidade=humi;
  p->temperature=temperature;
  
  Serial.println("Will Send ");
  int sentCode;
  if(MODE!=1) sentCode = trustnet.sendUData((byte*)p,sizeof(payload),3);
  else sentCode = trustnet.sendRData((byte*)p,sizeof(payload),3);
  Serial.print("Sent code ");
  Serial.println(sentCode);
  trustnet.update();
  delay(1000);
}
