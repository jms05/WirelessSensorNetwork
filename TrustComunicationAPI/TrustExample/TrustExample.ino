#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
//Include eeprom.h for AVR (Uno, Nano) etc. except ATTiny
#include <EEPROM.h>

/***** Configure the chosen CE,CS pins *****/
RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio,network);

//FOR THE LIBRERY

int sendACK(uint8_t num, uint8_t remoteNode);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


#define BUFFERSIZE 12
#define ACKMAXSIZE 50
#define DATATYPE_DATA 1
#define DATATYPE_ACK 0
#define DATATYPE_ANY -1
#define WAIT_ACK_R 0 
#define WAIT_ACK_U 1 
#define DEFAULT_DELAY 7
#define ACTUALVERSION 0
typedef struct 
  {
    RF24NetworkHeader *header;
    byte trustHeader;
    byte data[20];
  }PktL2, *PktL2PRT;

  typedef struct 
  {
    uint16_t nodeADDR;
    uint8_t num;
  }PairRecived,*PairRecivedPTR;
  
typedef struct node {
    PairRecivedPTR data;
    struct node * next;
  } node_t,*listRecived;


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



uint16_t incInt16Bit(uint16_t counter, int nbits){
   return (counter+1)%(2^nbits);
}

uint8_t incInt8Bit(uint8_t counter, int nbits){
   return (counter+1)%(2^nbits);
} 

bool queueIsFull(){
  return (BUFFERSIZE==buffer_pos+1);
}

bool queueIsEmpty(){
  return (0==buffer_pos);
}

uint8_t getPktSize(char data){
  return 2;
}

uint8_t getPktNum(byte data){
  return 2;
}
uint8_t getPktType(char data){
  return DATATYPE_DATA;
}

uint8_t getPktVersion(byte data){
  return ACTUALVERSION;
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

PktL2PRT checkNextReception(){
  return dequeue();
}

bool checkPendingReception(){
  return !queueIsEmpty();
}

void enqueue(RF24NetworkHeader header, byte* data){
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
    memcpy(pacote->data, data+1, MAX(20,dataSize));
    //pacote esta criado, meter-lo na queue
    bufferPkt[buffer_pos] = pacote;
    buffer_pos++;
    
  } 
}


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
  if(list==NULL){
    return NULL;
  }
  list->next=deleteList(list->next);
  free(list->data);
  free(list);
  list=NULL;
  return list;
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

void processReccivedData(RF24NetworkHeader header, byte* data){
  //TODO SEND ACK
  sendACK(getPktNum(data[0]),header.from_node);
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
     head->next=recivedFrom;
     recivedFrom=head;
     pruneList(recivedFrom,0,ACKMAXSIZE/2); //ver se nao sonseguir meter em fila o que faço 
     enqueue(header,data);
  }else{ //duplicado
    free(novo);
    free(head);
  }
 
}


int reciveData(uint8_t expectedData,uint8_t num, long timeout=DEFAULT_DELAY){ //num so faz sentido se esperarmos um ACK
  bool done= false;
  long startTry = millis();
  long now =startTry;
  if(DATATYPE_ANY==expectedData){ //para poder receber qualquer tipo de pacote
    done =true;
  }
  while(!network.available() && startTry+timeout>now){
    now=millis();
    delay(DEFAULT_DELAY);
  }
  if(network.available()){//novo pacote
    RF24NetworkHeader header;
    byte payload[21];  //pode dar merda
    network.read(header,&payload,sizeof(payload));
    if(getPktType(header.type)==DATATYPE_ACK){ //recebi um ACK
      if(expectedData ==DATATYPE_ACK && num==getPktNum(payload[0])){ //é o ACK estaparado update stats
        done=true;
        if(waitACKType ==WAIT_ACK_R ) R_confirmed =incInt16Bit(R_confirmed,16);
        else U_confirmed =incInt16Bit(R_confirmed,16);
      }else{
        //recebo um ack nao esperado 
      }
    }else if(getPktType(header.type)==DATATYPE_DATA){
      processReccivedData(header,payload); //Pode dar merda ver &payload
      if(expectedData==DATATYPE_DATA){
        done=true;
      }
    }
    if(done){
      return 1; 
    }else{
      //nao era o esperado tento preceber pelo tempo que me falta
      now=millis();
      long leftTime = timeout-(now-startTry);
      return reciveData(expectedData, num, leftTime); 
    }
  }else{ //timeout
     return -1;
  }
  
}

void buildHeader(uint8_t type,uint8_t len, uint8_t vers, uint8_t num, char *headerType, byte *headerMy){
  //TODO
  *headerType = 127;
  *headerMy = 0x80;
  
}

bool sendData(uint8_t type, uint8_t num, byte* payload, uint8_t dataSize, uint8_t remoteNode=0){
  char headerType;
  byte headerMy;
  buildHeader(type,dataSize,ACTUALVERSION,num,&headerType,&headerMy);
  byte dataTosend[21];
  dataTosend[0]= headerMy;
  memcpy(dataTosend+1, payload, MAX(20,dataSize)); //pode dar bosta
  return mesh.write(payload,headerType,sizeof(dataTosend),remoteNode); 
}

bool sendRdata( byte* payload, uint8_t dataSize,int maxTent, uint8_t remoteNode=0){
  my_seq_num = incInt8Bit(my_seq_num,8);
  Rpkt_transmited =incInt16Bit(Rpkt_transmited,16);
  int nTent=0;
  bool sent = false;
  waitACKType =WAIT_ACK_R;
  while(nTent<maxTent && !sent){
    bool leave_me = sendData(DATATYPE_DATA,my_seq_num,payload,dataSize,remoteNode);
    nTent++;
    if(leave_me){
      int aCKRecived = reciveData(DATATYPE_ACK,my_seq_num);
      if(aCKRecived==1){
        sent=true;
      }
    }
  }
  return sent;
}
int sendUData( byte* payload, uint8_t dataSize, uint8_t remoteNode=0){
  my_seq_num = incInt8Bit(my_seq_num,8);
  Upkt_transmited =incInt16Bit(Upkt_transmited,16);
  waitACKType =WAIT_ACK_U;
  bool leave_me = sendData(DATATYPE_DATA,my_seq_num,payload,dataSize,remoteNode);
  if(leave_me){
    reciveData(DATATYPE_ACK,my_seq_num);
  }
}

int sendACK(uint8_t num, uint8_t remoteNode){
  byte data[1];
  return sendData(DATATYPE_ACK,num,data,0,remoteNode);
}


void update(){
  network.update();
  mesh.update();
  reciveData(DATATYPE_ANY,0);
}



void setup() {
  my_seq_num =0;
  version_pt=0;
  Rpkt_transmited=0;
  Upkt_transmited=0;
  R_confirmed=0;
  U_confirmed=0;
  buffer_pos=0;
  recivedFrom=NULL;
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
