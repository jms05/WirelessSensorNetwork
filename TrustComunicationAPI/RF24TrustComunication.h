#ifndef __RF24TRUSTCOMUNICATION_H__
#define __RF24TRUSTCOMUNICATION_H__

#include <stdio.h>
#include <stdlib.h>
#define BUFFERSIZE 12

 
/**
 * @file RF24TRUSTCOMUNICATION.h
 *
 * Class declaration for RF24TRUSTCOMUNICATION
 */

#if defined (__AVR_ATtiny85__) || defined (__AVR_ATtiny84__)
	#define RF24_TRUST
#endif


//#include "RF24Mesh_config.h"

#if defined (__linux) && !defined(__ARDUINO_X86__)
  #include <RF24Network/RF24Network.h>
  #include <RF24/RF24Mesh.h>
  #define RF24_LINUX
#else
  #include <RF24Network.h>
  #include <RF24Mesh.h>
#endif
  #include <stddef.h>
  #include <stdint.h>


class RF24Mesh;
class RF24;



class RF24TrustComunication
{

public:


  RF24TrustComunication(RF24Network& _network,RF24Mesh& _mesh );


  void* checkNextReception();


  bool begin(uint8_t channel = MESH_DEFAULT_CHANNEL, rf24_datarate_e data_rate = RF24_1MBPS, uint32_t timeout=MESH_RENEWAL_TIMEOUT );
  
  
  uint8_t update();
  

  bool write(const void* data, uint8_t msg_type, size_t size, uint8_t nodeID=0);
  

  void setNodeID(uint8_t nodeID);
 
  void DHCP();
  

  int16_t getNodeID(uint16_t address=MESH_BLANK_ID);
  

  bool checkConnection();
  

  uint16_t renewAddress(uint32_t timeout=MESH_RENEWAL_TIMEOUT);
  

  bool releaseAddress();
  

  uint16_t getSelfMesh_address(); 
  

  int16_t getAddress(uint8_t nodeID);


  bool write(uint16_t to_node, const void* data, uint8_t msg_type, size_t size );
  

  void setChannel(uint8_t _channel);
  

  void setChild(bool allow);
  

  void setAddress(uint8_t nodeID, uint16_t address);
  
  void saveDHCP();
  void loadDHCP();
  

  uint8_t _nodeID;

  
#if !defined RF24_TRUST  
  typedef struct{
	uint8_t nodeID;       /**< NodeIDs and addresses are stored in the addrList array using this structure */
	uint16_t address;  /**< NodeIDs and addresses are stored in the addrList array using this structure */
  }addrListStruct;
  
  // Pointer used for dynamic memory allocation of address list
  addrListStruct *addrList;  /**< See the addrListStruct class reference */
  uint8_t addrListTop;       /**< The number of entries in the assigned address list */
#endif

  typedef struct 
  {
    RF24NetworkHeader *header;
    byte data[21];
  }PktL2PRT, *PktL2PRT;

  typedef struct 
  {
    uint16_t nodeADDR;
    uint8_t num;
  }PairRecived,*PairRecivedPTR;
  
  typedef struct node {
    PairRecived data;
    struct node * next;
  } node_t,*listRecived;

  private:
  RF24Network& network;
  RF24Mesh& mesh;  
  uint8_t my_seq_num;
  uint8_t version;
  //stats
  uint16_t Rpkt_transmited;
  uint16_t Upkt_transmited;
  uint16_t R_confirmed;
  uint16_t U_confirmed;
  //recieve buffer
  uint8_t buffer_pos;
  PktL2 bufferPkt[BUFFERSIZE]

  listRecived recivedFrom;

 };
 
 #endif