#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include "RF24TrustComunication.h"

#define resetFunc ((void (*)(void))0x0)

RF24TrustComunication::RF24TrustComunication(RF24Network& _network,RF24Mesh& _mesh,int nodeID=0,bool AllowChild=true): network(_network),mesh(_mesh){
    _my_seq_num=0;
    _version_pt=0;
    _Rpkt_transmited=0;
    _Upkt_transmited=0;
    _R_confirmed=0;
    _U_confirmed=0;
    _waitACKType=0;
    _buffer_pos=0;
    _bufferPkt[BUFFERSIZE];
    _recivedFrom=false;
    if(nodeID==0){
      _doDHCP=true;
    }else{
      _doDHCP=false;
    }
    
    mesh.setNodeID(nodeID);
    mesh.begin();
    this->setChildMesh(AllowChild);
}
int RF24TrustComunication::reciveData( uint8_t expectedData,uint8_t num, long timeout=DEFAULT_DELAY){
  // 0  pacote ACK recebido é o esparado, ou de dados esta guardado
  // -1 timeout
  // -2 quero dados mas a fila está cheia
  /*RF24NetworkHeader header;
  byte payload[PAYLOADSIZE];*/
  return this->reciveDataPrivate(/*header,payload,sizeof(payload),*/expectedData,num,timeout); //ver se tenho de passar &payload
}
int RF24TrustComunication::sendRData( byte* payload, uint8_t dataSize,int maxTent, uint8_t remoteNode=0){
    _my_seq_num = this->incInt8Bit(_my_seq_num,8);
    _Rpkt_transmited =this->incInt16Bit(_Rpkt_transmited,16);
    int nTent=0;
    bool sent = false;
    _waitACKType =WAIT_ACK_R;
    int returnMessage = -1;
    while(nTent<maxTent && !sent){
      bool leave_me = this->sendData(DATATYPE_DATA,_my_seq_num,payload,dataSize,remoteNode);
      nTent++;
      if(leave_me){
        returnMessage=1;
        int aCKRecived = !this->reciveData(DATATYPE_ACK,_my_seq_num,2000);
        if(aCKRecived==1){
          sent=true;
          returnMessage=0;
        }
      }
    }
    return returnMessage;
}
int RF24TrustComunication::sendUData( byte* payload, uint8_t dataSize, uint8_t remoteNode=0){
    _my_seq_num = incInt8Bit(_my_seq_num,8);
    _Upkt_transmited =incInt16Bit(_Upkt_transmited,16);
    _waitACKType =WAIT_ACK_U;
    int returnMessage = -1;
    bool leave_me = this->sendData(DATATYPE_DATA,_my_seq_num,payload,dataSize,remoteNode);
    if(leave_me){
        returnMessage=1;
        int aCKRecived = !this->reciveData(DATATYPE_ACK,_my_seq_num,2000);
        if(aCKRecived==1){
          returnMessage=0;
        }
    }
  return returnMessage;
}
void RF24TrustComunication::DHCPMesh(){
    mesh.DHCP();
}
void RF24TrustComunication::setNodeIDMesh(uint8_t nodeID){
    mesh.setNodeID(nodeID);
}
void RF24TrustComunication::setChildMesh(bool allow){
    mesh.setChild(allow);
}
void RF24TrustComunication::update(){
    mesh.update();
    if(_doDHCP){
      this->DHCPMesh();
    }
    int s = this->reciveData(DATATYPE_ANY,0,100);
}
PktL2PRT RF24TrustComunication::checkNextReception(){
    return this->dequeue();

}
bool RF24TrustComunication::checkPendingReception(){
    return !this->queueIsEmpty();

}
bool RF24TrustComunication::checkConnectionMesh(){
    return mesh.checkConnection();
}
int16_t RF24TrustComunication::getNodeIDMesh(uint16_t address=MESH_BLANK_ID){
    return mesh.getNodeID(address); 

}
bool RF24TrustComunication::releaseAddressMesh(){
    return mesh.releaseAddress();
}
uint16_t RF24TrustComunication::renewAddressMesh(uint32_t timeout=MESH_RENEWAL_TIMEOUT){
    return mesh.renewAddress(timeout);
}
int16_t RF24TrustComunication::getAddressMesh(uint8_t nodeID){
    return mesh.getAddress(nodeID);
}
int16_t RF24TrustComunication::getMyAddressMesh(){
    return mesh.mesh_address;
}
void RF24TrustComunication::setChannelMesh (uint8_t _channel=MESH_DEFAULT_CHANNEL){
    mesh.setChannel(_channel);
}
void RF24TrustComunication::setAddressMesh(uint8_t nodeID, uint16_t address){
    mesh.setAddress(nodeID,address);
}
void RF24TrustComunication::saveDHCPMesh(){
    mesh.saveDHCP();
}
void RF24TrustComunication::loadDHCPMesh(){
    mesh.loadDHCP();
}
//private


int RF24TrustComunication::sendACK(uint8_t num, uint8_t remoteNode){

}
int RF24TrustComunication::powint(int x, int y){
    int val=1;
    for(int z=0;z<y;z++)
    {
        val=val*x;
    }
    return val;
}

uint16_t RF24TrustComunication::incInt16Bit(uint16_t counter, int nbits){
    return (counter+1)%powint(2,nbits);
}
uint8_t RF24TrustComunication::incInt8Bit(uint8_t counter, int nbits){
    return (counter+1)%powint(2,nbits);
}
uint8_t RF24TrustComunication::getPktType(char data){
    char ret = (data & 0x60) >> 5; 
    return ((uint8_t)ret);
}

uint8_t RF24TrustComunication::getPktSize(char data){
    char ret = data & 0x1F;
    return ((uint8_t)ret);
}
uint8_t RF24TrustComunication::getPktVersion(byte data){
    byte ret = (data & 0xC0 )>>6;
    return (uint8_t)ret;
}
uint8_t RF24TrustComunication::getPktNum(byte data){
    byte ret = (data & 0x3F );
    return (uint8_t)ret;
}
void RF24TrustComunication::buildHeader(uint8_t type,uint8_t len, uint8_t vers, uint8_t num, char *headerType, byte *headerMy){
    byte byteType = ((byte)type << 5) & 0x60;
    byte byteSize = (byte)len & 0x1F;
    byte header = byteType | byteSize;
    *headerType = (char)header;

    byte byteVer = ((byte)vers << 6) & 0xC0;
    byte byteNum = (byte)num & 0x3F;
    byte myHeader = byteVer | byteNum;
    *headerMy = myHeader;
}
bool RF24TrustComunication::queueIsFull(){
    return (BUFFERSIZE==_buffer_pos+1);
}
bool RF24TrustComunication::queueIsEmpty(){
    return (0==_buffer_pos);
}
PktL2PRT RF24TrustComunication::dequeue(){
    PktL2PRT ret  =NULL;
    if(!this->queueIsEmpty()){
      ret = _bufferPkt[0];
      for(int i =1;i<_buffer_pos;i++){
        _bufferPkt[i-1]=_bufferPkt[i];
      }
      _buffer_pos--;
    }
    return ret;
}
int RF24TrustComunication::enqueue(RF24NetworkHeader header, byte* data){
    if (!queueIsFull()){
        RF24NetworkHeader *myHeader = (RF24NetworkHeader*) malloc(sizeof(RF24NetworkHeader));
        if(myHeader==NULL){
          resetFunc();
        }
        memcpy(myHeader, &header, sizeof(header));
        uint8_t dataSize = this->getPktSize(header.type);
        PktL2PRT pacote = (PktL2PRT)malloc(sizeof(PktL2));
        if(pacote==NULL){
          free(myHeader);
          resetFunc();
        }
        pacote->header= myHeader;
        pacote->trustHeader=data[0];
        for(int i = 1;i<PAYLOADSIZE;i++){
          pacote->data[i]=0x00;
        }
        memcpy(pacote->data, data+1, MIN(PAYLOADSIZE,dataSize)); //
        //pacote esta criado, meter-lo na queue
        _bufferPkt[_buffer_pos] = pacote;
        _buffer_pos++;
    }else{ //queue is full
      return -1;
    }
  return 0;
}
bool RF24TrustComunication::inInList(listRecived nodo, listRecived list){
    if(list==NULL){
        return false;
    }
    if(nodo->data->nodeADDR==list->data->nodeADDR && nodo->data->num==list->data->num){
        return true;
    }
    return this->inInList(nodo,list->next);
}

listRecived RF24TrustComunication::deleteList(listRecived list){
    if(list!=NULL){
        list->next=deleteList(list->next);
        free(list->data);
        free(list);
        list=NULL;
  }
  return NULL;

}
listRecived RF24TrustComunication::pruneList(listRecived list, int actual, int level){
    if(list!=NULL){
        if(actual>=level){
            return deleteList(list);
        }
        list->next = pruneList(list->next,actual+1,level);
    }
    return list;
}
int RF24TrustComunication::sizeList(listRecived list){
    if(list==NULL) return 0;
    return 1 +sizeList(list->next);
}
listRecived RF24TrustComunication::addToList(listRecived novo, listRecived list){
    novo->next=list;
    return novo;
}
int RF24TrustComunication::processReccivedData(RF24NetworkHeader header, byte* data){
    PairRecivedPTR novo= (PairRecivedPTR)malloc(sizeof(PairRecived));
    if(novo==NULL){
        //exit -3;
        resetFunc();
    }
    novo->nodeADDR=header.from_node;
    novo->num = this->getPktNum(data[0]);
    listRecived head = malloc(sizeof(node_t));
    if (head == NULL) {
      free(novo);
      //exit -4;
      resetFunc();
    }
    
    head->data=novo;

    if(!this->inInList(head,_recivedFrom)){ //novo pacote recebido
       int onfila = this->enqueue(header,data); //0 se ficou na fila
       if(onfila==0){
        _recivedFrom=this->addToList(head,_recivedFrom);
        this->pruneList(_recivedFrom,0,ACKMAXSIZE/2);
        // SEND ACK
        this->sendACK(novo);
        return 0;
       }else{ //sem espaço em buffer
          free(novo);
          free(head); 
          return -1;
       }
    }else{ //duplicado
      // SEND ACK
      this->sendACK(novo);
      free(novo);
      free(head);
      return 1;
    } 
}
int RF24TrustComunication::reciveDataPrivate(/*RF24NetworkHeader header, byte payload[], size_t size_payload,*/ uint8_t expectedData,uint8_t num, long timeout=DEFAULT_DELAY){
    // 0  pacote ACK recebido é o esparado, ou de dados esta guardado
    // -1 timeout
    // -2 quero dados mas a fila está cheia
    /*
     Rmover depois
     */
    RF24NetworkHeader header;
    byte payload[PAYLOADSIZE];
    size_t size_payload = sizeof(payload);
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
      network.read(header,payload,size_payload);
      if(this->getPktType(header.type)==DATATYPE_ACK){ //recebi um ACK
        if(expectedData ==DATATYPE_ACK && num==this->getPktNum(payload[0])){ //é o ACK estaparado update stats
          if(_waitACKType ==WAIT_ACK_R ) _R_confirmed=this->incInt16Bit(_R_confirmed,16);
          else _U_confirmed =this->incInt16Bit(_U_confirmed,16);
          done=true;
          returnMessage= 0; 
        }else{
          //recebo um ack nao esperado 
        }
      }else if(this->getPktType(header.type)==DATATYPE_DATA){
        int saved = this->processReccivedData(header,payload); //Pode dar merda ver &payload
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
        return this->reciveDataPrivate(/*header, payload,size_payload,*/expectedData, num, leftTime); 
      }
    }else{ //timeout
       return -1;
    }
  
}

bool RF24TrustComunication::sendDataNetworkADDR(uint8_t type, uint8_t num, byte* payload, uint8_t dataSize, uint16_t remoteNode){
    char headerType;
    byte headerMy;
    this->buildHeader(type,dataSize,ACTUALVERSION,num,&headerType,&headerMy);
    byte dataTosend[PAYLOADSIZE];
    for(int i = 1;i<PAYLOADSIZE;i++){
        dataTosend[i]=0x00;
    }
    dataTosend[0]= headerMy;
    memcpy(dataTosend+1, payload, MIN(PAYLOADSIZE-1,dataSize)); //pode dar bosta
    return mesh.write(remoteNode,dataTosend,headerType,sizeof(dataTosend)); 
}
bool RF24TrustComunication::sendData(uint8_t type, uint8_t num, byte* payload, uint8_t dataSize, uint8_t remoteNode=0){
    char headerType;
    byte headerMy;
    this->buildHeader(type,dataSize,ACTUALVERSION,num,&headerType,&headerMy);
    byte dataTosend[PAYLOADSIZE];
    for(int i = 1;i<PAYLOADSIZE;i++){
        dataTosend[i]=0x00;
    }
    dataTosend[0]= headerMy;
    memcpy(dataTosend+1, payload, MIN(PAYLOADSIZE-1,dataSize)); //pode dar bosta
    return mesh.write(dataTosend,headerType,sizeof(dataTosend),remoteNode); 
}
int RF24TrustComunication::sendACK(PairRecivedPTR pair){
    byte data[1];
    data[0]=0x00;
    bool result=false;
    result=this->sendDataNetworkADDR(DATATYPE_ACK,pair->num,data,sizeof(data),pair->nodeADDR);
    return result;
}

