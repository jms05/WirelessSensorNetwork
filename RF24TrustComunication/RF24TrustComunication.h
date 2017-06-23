#ifndef RF24TrustComunication_h
#define RF24TrustComunication_h

#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"

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



class RF24TrustComunication
{
  public:
  	uint8_t getPktType(char data);
	uint8_t getPktSize(char data);
	uint8_t getPktVersion(byte data);
	uint8_t getPktNum(byte data);
    RF24TrustComunication(RF24Network& network,RF24Mesh& mesh,int nodeID=0,bool AllowChild=true);
    int reciveData( uint8_t expectedData,uint8_t num, long timeout=DEFAULT_DELAY);
    int sendRData( byte* payload, uint8_t dataSize,int maxTent, uint8_t remoteNode=0);
    int sendUData( byte* payload, uint8_t dataSize, uint8_t remoteNode=0);
    void DHCPMesh();
    void setNodeIDMesh(uint8_t nodeID);
    void setChildMesh(bool allow);
    void update();
    PktL2PRT checkNextReception();
    bool checkPendingReception();
    bool checkConnectionMesh();
    int16_t getNodeIDMesh(uint16_t address=MESH_BLANK_ID);
    bool releaseAddressMesh();
    uint16_t renewAddressMesh(uint32_t timeout=MESH_RENEWAL_TIMEOUT);
    int16_t getAddressMesh(uint8_t nodeID);
    int16_t getMyAddressMesh();
    void setChannelMesh (uint8_t _channel=MESH_DEFAULT_CHANNEL);
    void setAddressMesh(uint8_t nodeID, uint16_t address);
    void saveDHCPMesh();
    void loadDHCPMesh();
  private:
  	RF24Network& network;
  	RF24Mesh& mesh;
  	uint8_t _my_seq_num;
	uint8_t _version_pt;
	//stats
	uint16_t _Rpkt_transmited;
	uint16_t _Upkt_transmited;
	uint16_t _R_confirmed;
	uint16_t _U_confirmed;
	char _waitACKType;
	bool _doDHCP;
	//recieve buffer
	uint8_t _buffer_pos;
	PktL2PRT _bufferPkt[BUFFERSIZE];
	//confirmations
	listRecived _recivedFrom;
	//Functions
	int sendACK(uint8_t num, uint8_t remoteNode);
	int powint(int x, int y);
	uint16_t incInt16Bit(uint16_t counter, int nbits);
	uint8_t incInt8Bit(uint8_t counter, int nbits);
	void buildHeader(uint8_t type,uint8_t len, uint8_t vers, uint8_t num, char *headerType, byte *headerMy);
	bool queueIsFull();
	bool queueIsEmpty();
	PktL2PRT dequeue();
	int enqueue(RF24NetworkHeader header, byte* data);
	bool inInList(listRecived nodo, listRecived list);
	listRecived deleteList(listRecived list);
	listRecived pruneList(listRecived list, int actual, int level);
	int sizeList(listRecived list);
	listRecived addToList(listRecived novo, listRecived list);
	int processReccivedData(RF24NetworkHeader header, byte* data);
	int reciveDataPrivate(/*RF24NetworkHeader header, byte payload[], size_t size_payload,*/ uint8_t expectedData,uint8_t num, long timeout=DEFAULT_DELAY);
	bool sendDataNetworkADDR(uint8_t type, uint8_t num, byte* payload, uint8_t dataSize, uint16_t remoteNode);
	bool sendData(uint8_t type, uint8_t num, byte* payload, uint8_t dataSize, uint8_t remoteNode=0);
	int sendACK(PairRecivedPTR pair);

};
 #endif