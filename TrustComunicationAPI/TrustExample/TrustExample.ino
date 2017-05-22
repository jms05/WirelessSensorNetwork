#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
//Include eeprom.h for AVR (Uno, Nano) etc. except ATTiny
#include <EEPROM.h>

/***** Configure the chosen CE,CS pins *****/
RF24 radio(9,10);
RF24Network network(radio);
RF24Mesh mesh(radio,network);

long displayTimer=0;


//FOR THE LIBRERY

int sendACK(uint8_t num, uint8_t remoteNode);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


#define BUFFERSIZE 8
#define ACKMAXSIZE 18
#define PAYLOADSIZE 22

#define DATATYPE_DATA 1
#define DATATYPE_ACK 0
#define DATATYPE_ANY -1

#define WAIT_ACK_R 0 
#define WAIT_ACK_U 1 

#define DEFAULT_DELAY 85
#define DEFAULT_DELAY_LOOP 10

#define ACTUALVERSION 2

#define MASTERNODE 0

#define THISNODE 0 //Change

typedef struct {
    RF24NetworkHeader *header;
    byte trustHeader;
    byte data[PAYLOADSIZE-1];
}PktL2, *PktL2PRT;

typedef struct{
    uint16_t nodeADDR;
    uint8_t num;
  }PairRecived,*PairRecivedPTR;
  
typedef struct node {
    PairRecivedPTR data;
    struct node * next;
  } node_t,*listRecived;

//
bool doDHCP;
uint8_t my_seq_num;
uint8_t version_pt;
//stats
uint16_t Rpkt_transmited;
uint16_t Upkt_transmited;
uint16_t R_confirmed;
uint16_t U_confirmed;
char waitACKType;
//recieve buffer
uint8_t buffer_pos;
PktL2PRT bufferPkt[BUFFERSIZE];
//confirmations
listRecived recivedFrom;


//incretemnter Methods 

int powint(int x, int y)
{
 int val=1;
 for(int z=0;z<y;z++)
 {
     val=val*x;
 }
 return val;
}

uint16_t incInt16Bit(uint16_t counter, int nbits){
   return (counter+1)%powint(2,nbits);
}

uint8_t incInt8Bit(uint8_t counter, int nbits){
   return (counter+1)%powint(2,nbits);
} 


//Header processing methods
uint8_t getPktType(char data){
  char ret = (data & 0x60) >> 5;
  return ((uint8_t)ret);
}


uint8_t getPktSize(char data){
  char ret = data & 0x1F;
  return ((uint8_t)ret);
}

uint8_t getPktVersion(byte data){
  byte ret = (data & 0xC0 )>>6;
  return (uint8_t)ret;
}

uint8_t getPktNum(byte data){
  byte ret = (data & 0x3F );
  return (uint8_t)ret;
}



void buildHeader(uint8_t type,uint8_t len, uint8_t vers, uint8_t num, char *headerType, byte *headerMy){
  byte byteType = ((byte)type << 5) & 0x60;
  byte byteSize = (byte)len & 0x1F;
  byte header = byteType | byteSize;
  *headerType = (char)header;

  byte byteVer = ((byte)vers << 6) & 0xC0;
  byte byteNum = (byte)num & 0x3F;
  byte myHeader = byteVer | byteNum;
  *headerMy = myHeader;
  
}

//QUEUE Methods
bool queueIsFull(){
  return (BUFFERSIZE==buffer_pos+1);
}

bool queueIsEmpty(){
  return (0==buffer_pos);
}

PktL2PRT dequeue(){
  PktL2PRT ret  =NULL;
  if(!queueIsEmpty()){
    ret = bufferPkt[0];
    for(int i =1;i<buffer_pos;i++){
      bufferPkt[i-1]=bufferPkt[i];
    }
    buffer_pos--;
  }
  return ret;
}

int enqueue(RF24NetworkHeader header, byte* data){
  if (!queueIsFull()){
    RF24NetworkHeader *myHeader = (RF24NetworkHeader*) malloc(sizeof(RF24NetworkHeader));
    if(myHeader==NULL){
      exit -1;
    }
    memcpy(myHeader, &header, sizeof(header));
    uint8_t dataSize = getPktSize(header.type);
    PktL2PRT pacote = (PktL2PRT)malloc(sizeof(PktL2));
    if(pacote==NULL){
      exit -2;
    }
    pacote->header= myHeader;
    pacote->trustHeader=data[0];
    for(int i = 1;i<PAYLOADSIZE;i++){
      pacote->data[i]=0x00;
    }
    memcpy(pacote->data, data+1, MIN(PAYLOADSIZE,dataSize)); //
    //pacote esta criado, meter-lo na queue
    bufferPkt[buffer_pos] = pacote;
    buffer_pos++;
    
  }else{ //queue is full
    return -1;
  }
  return 0;
}


//ACK LIST METHODS
bool inInList(listRecived nodo, listRecived list){
  if(list==NULL){
    return false;
  }
  if(nodo->data->nodeADDR==list->data->nodeADDR && nodo->data->num==list->data->num){
    return true;
  }
  return inInList(nodo,list->next);
}

listRecived deleteList(listRecived list){
  if(list!=NULL){
    list->next=deleteList(list->next);
    free(list->data);
    free(list);
    list=NULL;
  }
  return NULL;
}

listRecived pruneList(listRecived list, int actual, int level){
  if(actual>=level){
    return deleteList(list);
  }
  list->next = pruneList(list->next,actual+1,level);
  return list;
}

int sizeList(listRecived list){
  if(list==NULL) return 0;
  return 1 +sizeList(list->next);
}

listRecived addToList(listRecived novo, listRecived list){
  novo->next=list;
  return novo;
}

//OUR Library Methods

//private
int processReccivedData(RF24NetworkHeader header, byte* data){ //-1 recebido mas descartado, 0 recebido e na fila, 1 recebido mas duplicado 
  PairRecivedPTR novo= (PairRecivedPTR)malloc(sizeof(PairRecived));
  if(novo==NULL){
      exit -3;
  }
  novo->nodeADDR=header.from_node;
  novo->num = getPktNum(data[0]);
  listRecived head = malloc(sizeof(node_t));
  if (head == NULL) {
    exit -4;
  }
  
  head->data=novo;

  if(!inInList(head,recivedFrom)){ //novo pacote recebido
     Serial.println("NOVO ");
     int onfila = enqueue(header,data); //0 se ficou na fila
     if(onfila==0){
      recivedFrom=addToList(head,recivedFrom);
      pruneList(recivedFrom,0,ACKMAXSIZE/2);
      // SEND ACK
      sendACK(novo);
      return 0;
     }else{ //sem espaço em buffer 
        return -1;
     }
  }else{ //duplicado
    // SEND ACK
    sendACK(novo);
    Serial.println("DUP ");
    free(novo);
    free(head);
    return 1;
  } 
}





//private
int reciveDataPrivate(RF24NetworkHeader header, byte payload[], size_t size_payload, uint8_t expectedData,uint8_t num, long timeout=DEFAULT_DELAY){ //num so faz sentido se esperarmos um ACK
  // 0  pacote ACK recebido é o esparado, ou de dados esta guardado
  // -1 timeout
  // -2 quero dados mas a fila está cheia
  int returnMessage=-1;
  bool done= false;
  long startTry = millis();
  long now =startTry;
  if(DATATYPE_ANY==expectedData){ //para poder receber qualquer tipo de pacote
    done =true;
  }
  while(!network.available() && startTry+timeout>now){
    mesh.update();
    now=millis();
    delay(DEFAULT_DELAY_LOOP);
  }
  if(network.available()){//novo pacote
    Serial.print("NOVO PACOTE ");
    network.read(header,payload,size_payload);
    if(getPktType(header.type)==DATATYPE_ACK){ //recebi um ACK
      Serial.println(" ACK ");
      if(expectedData ==DATATYPE_ACK && num==getPktNum(payload[0])){ //é o ACK estaparado update stats
        Serial.println("Esperado ");
        if(waitACKType ==WAIT_ACK_R ) R_confirmed =incInt16Bit(R_confirmed,16);
        else U_confirmed =incInt16Bit(R_confirmed,16);
        done=true;
        returnMessage= 0; 
      }else{
        //recebo um ack nao esperado 
      }
    }else if(getPktType(header.type)==DATATYPE_DATA){
      Serial.print("DATA ");
      int saved = processReccivedData(header,payload); //Pode dar merda ver &payload
      if(expectedData==DATATYPE_DATA){
        done=true;
        switch(saved){
          case(-1) :{ //sem espaço na fila
            returnMessage= -2;
          }
          case(0):{ //tudo ok
            returnMessage= 0;
          }
          case(1):{ //duplicado
            returnMessage= 0;
          }
        }  
      }
    }
    if(done){
      return returnMessage;
    }else{
      //nao era o esperado tento preceber pelo tempo que me falta
      now=millis();
      long leftTime = timeout-(now-startTry);
      return reciveDataPrivate(header, payload,size_payload,expectedData, num, leftTime); 
    }
  }else{ //timeout
     Serial.println("TIMEOUT");
     return -1;
  }
  
}

//public
int reciveData( uint8_t expectedData,uint8_t num, long timeout=DEFAULT_DELAY){ //num so faz sentido se esperarmos um ACK
  // 0  pacote ACK recebido é o esparado, ou de dados esta guardado
  // -1 timeout
  // -2 quero dados mas a fila está cheia
  RF24NetworkHeader header;
  byte payload[PAYLOADSIZE];
  return reciveDataPrivate(header,payload,sizeof(payload),expectedData,num,timeout); //ver se tenho de passar &payload
  
}
//private
bool sendDataNetworkADDR(uint8_t type, uint8_t num, byte* payload, uint8_t dataSize, uint16_t remoteNode){
  char headerType;
  byte headerMy;
  buildHeader(type,dataSize,ACTUALVERSION,num,&headerType,&headerMy);
  byte dataTosend[PAYLOADSIZE];
  for(int i = 1;i<PAYLOADSIZE;i++){
      dataTosend[i]=0x00;
  }
  dataTosend[0]= headerMy;
  memcpy(dataTosend+1, payload, MIN(PAYLOADSIZE-1,dataSize)); //pode dar bosta
  return mesh.write(remoteNode,dataTosend,headerType,sizeof(dataTosend)); 
}


//private
bool sendData(uint8_t type, uint8_t num, byte* payload, uint8_t dataSize, uint8_t remoteNode=0){
  char headerType;
  byte headerMy;
  buildHeader(type,dataSize,ACTUALVERSION,num,&headerType,&headerMy);
  byte dataTosend[PAYLOADSIZE];
  for(int i = 1;i<PAYLOADSIZE;i++){
      dataTosend[i]=0x00;
  }
  dataTosend[0]= headerMy;
  memcpy(dataTosend+1, payload, MIN(PAYLOADSIZE-1,dataSize)); //pode dar bosta
  return mesh.write(dataTosend,headerType,sizeof(dataTosend),remoteNode); 
}

//private
int sendACK(PairRecivedPTR pair){
  Serial.print("SEND ACK ");
  Serial.print(pair->num);
  Serial.print(" To ");
  Serial.println(pair->nodeADDR);
  byte data[1];
  data[0]=0x00;
  bool result=false;
  result=sendDataNetworkADDR(DATATYPE_ACK,pair->num,data,sizeof(data),pair->nodeADDR);
  Serial.println(result);
  return result;
}

//public
int sendRData( byte* payload, uint8_t dataSize,int maxTent, uint8_t remoteNode=0){ //return 0 sent OK, 1 JUMP To next, -1 NOT Leave US
  Serial.println("SEND DATA R ");
  my_seq_num = incInt8Bit(my_seq_num,8);
  Rpkt_transmited =incInt16Bit(Rpkt_transmited,16);
  int nTent=0;
  bool sent = false;
  waitACKType =WAIT_ACK_R;
  int returnMessage = -1;
  while(nTent<maxTent && !sent){
    bool leave_me = sendData(DATATYPE_DATA,my_seq_num,payload,dataSize,remoteNode);
    nTent++;
    if(leave_me){
      returnMessage=1;
      Serial.print("Recive ACK?? ");
      int aCKRecived = !reciveData(DATATYPE_ACK,my_seq_num,2000);
      Serial.println(aCKRecived);
      if(aCKRecived==1){
        sent=true;
        returnMessage=0;
      }
    }
  }
  return returnMessage;
}
//public
int sendUData( byte* payload, uint8_t dataSize, uint8_t remoteNode=0){ //return 0 sent OK, 1 JUMP To next, -1 NOT Leave US
  Serial.println("SEND DATA U ");

  my_seq_num = incInt8Bit(my_seq_num,8);
  Upkt_transmited =incInt16Bit(Upkt_transmited,16);
  waitACKType =WAIT_ACK_U;
  int returnMessage = -1;
  bool leave_me = sendData(DATATYPE_DATA,my_seq_num,payload,dataSize,remoteNode);
  if(leave_me){
      returnMessage=1;
      int aCKRecived = !reciveData(DATATYPE_ACK,my_seq_num,2000);
      if(aCKRecived==1){
        returnMessage=0;
      }
  }
  return returnMessage;
}

//public
void DHCPMesh(){
  mesh.DHCP();
}
//public
void setNodeIDMesh(uint8_t nodeID){
  mesh.setNodeID(nodeID);
}


//public
void setChildMesh(bool allow){
  mesh.setChild(allow);
}


//public
void update(){
  mesh.update();
  if(doDHCP){
    DHCPMesh();
    if(millis() - displayTimer > 5000){
    displayTimer = millis();
    Serial.println(" ");
    Serial.println(F("********Assigned Addresses********"));
     for(int i=0; i<mesh.addrListTop; i++){
       Serial.print("NodeID: ");
       Serial.print(mesh.addrList[i].nodeID);
       Serial.print(" RF24Network Address: 0");
       Serial.println(mesh.addrList[i].address,OCT);
     }
    Serial.println(F("**********************************"));
    }
  }
  Serial.println("RECIVE DATA ANY ");
  int s = reciveData(DATATYPE_ANY,0,100);
  //Serial.println(s);  
}

//public
/*bool begin(uint8_t nodeID,bool allowChild=true ,uint8_t channel = MESH_DEFAULT_CHANNEL, rf24_datarate_e data_rate = RF24_1MBPS, uint32_t timeout=MESH_RENEWAL_TIMEOUT ){
  if(nodeID==0) doDHCP=true;
  setChildMesh(allowChild);
  return  mesh.begin(channel,data_rate,timeout );

}
*/
//public
PktL2PRT checkNextReception(){
  return dequeue();
}
//public
bool checkPendingReception(){
  return !queueIsEmpty();
}

//public
bool checkConnectionMesh(){
  return mesh.checkConnection();
}

//public
int16_t getNodeIDMesh(uint16_t address=MESH_BLANK_ID){
  return mesh.getNodeID(address);  
}

//public
bool releaseAddressMesh(){
  return mesh.releaseAddress();
}

//public 
uint16_t renewAddressMesh(uint32_t timeout=MESH_RENEWAL_TIMEOUT){
  return mesh.renewAddress(timeout);
}

//public
int16_t getAddressMesh(uint8_t nodeID){
  return mesh.getAddress(nodeID);
}

//public
int16_t getMyAddressMesh(){
  return mesh.mesh_address;
}


//public 
void setChannelMesh (uint8_t _channel=MESH_DEFAULT_CHANNEL){
  mesh.setChannel(_channel);
}

//public
void setAddressMesh(uint8_t nodeID, uint16_t address){
  mesh.setAddress(nodeID,address);
}

//public
void saveDHCPMesh(){
  mesh.saveDHCP();
}

//public
void loadDHCPMesh(){
  mesh.loadDHCP();
}

void setup() {
  // put your setup code here, to run once:
  my_seq_num =0;
  version_pt=0;
  Rpkt_transmited=0;
  Upkt_transmited=0;
  R_confirmed=0;
  U_confirmed=0;
  buffer_pos=0;
  recivedFrom=NULL;
  doDHCP=false;
  
  Serial.begin(57600);
  Serial.println("SETUP");
  if(MASTERNODE==THISNODE){
    doDHCP=true;
    /*
    mesh.setNodeID(0);
    mesh.begin();
    */
    // Set the nodeID to 0 for the master node
    mesh.setNodeID(0);
    Serial.println(mesh.getNodeID());
    // Connect to the mesh
    mesh.begin();
  }else{
    mesh.setNodeID(THISNODE);
    // Connect to the mesh
    Serial.println(F("Connecting to the mesh..."));
    mesh.begin();
  }
  //setChildMesh(true);
}

/*
void testHeaderBuild(){
  char headerType;
  byte headerMy;
  uint8_t type = DATATYPE_DATA;
  uint8_t dataSize = 10;
  buildHeader(type,dataSize,ACTUALVERSION,my_seq_num,&headerType,&headerMy);

  Serial.print("Values: ");
  Serial.print(type);
  Serial.print(" ");
  Serial.print(dataSize);
  Serial.print(" ");
  Serial.print(ACTUALVERSION);
  Serial.print(" ");
  Serial.println(my_seq_num);
  Serial.print("Build: ");
  Serial.print(headerType);
  Serial.print(" ");
  Serial.println(headerMy);

  Serial.print("UNPACK ");
  Serial.print(getPktType(headerType));
  Serial.print(" ");
  Serial.print(getPktSize(headerType));
  Serial.print(" ");
  Serial.print(getPktVersion(headerMy));
  Serial.print(" ");
  Serial.println(getPktNum(headerMy));
  my_seq_num++;
}
*/
void testReciver(){
  Serial.println("Wait recive ");
  int recived = reciveData(DATATYPE_DATA,0);
  Serial.print("Recived code ");
  Serial.println(recived);
    bool onFila = checkPendingReception();
    Serial.print("onFila code ");
    Serial.println(onFila);
    while(onFila){
      PktL2PRT pacote = dequeue();
      Serial.println("\theader ");
      Serial.print("\tFrom: ");
      Serial.println(pacote->header->from_node);
      Serial.print("\tTO: ");
      Serial.println(pacote->header->to_node);
      Serial.print("\tNUM: ");
      Serial.println(getPktNum(pacote->trustHeader));
      Serial.print("\tSize: ");
      Serial.println(getPktSize(pacote->header->type));
      Serial.print("\tType: ");
      Serial.println(getPktType(pacote->header->type));
      //pacote->trustHeader
      char* message = (char*)pacote->data;
      Serial.print("Message: ");
      Serial.println(message);
      free(pacote);
    }
}

void testSend(){
  char* payload = "OLA MUNDO CONF";
  Serial.println("Will Send ");
  int sentCode = sendRData((byte*)payload,strlen(payload),3);
  Serial.print("Sent code ");
  Serial.println(sentCode);
  delay(100);
}


void loop() {
  /*char* st = "OLA MUNDO CONF";
  byte payload[PAYLOADSIZE];
  for(int i = 1;i<PAYLOADSIZE;i++){
    payload[i]=0x00;
  }
  memcpy(payload+1,st,MIN(strlen(st),PAYLOADSIZE));
  Serial.print("Copy ");
  Serial.println((char*)payload+1);
  delay(2000);*/
  update();
  delay(1000);
  if(MASTERNODE==THISNODE){
    testReciver();
  }else{
    testSend();
  }
  
  update();
  delay(500);

}
