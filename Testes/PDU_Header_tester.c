#include <time.h>
#include <string.h>
#include<stdio.h>
#include<stdlib.h>

//Header processing methods
//TODO
unsigned char getPktType( char data){
  unsigned char ret = (data & 0x60) >> 5;
  return ((unsigned char)ret);
}


unsigned char getPktSize(char data){
  char ret = data & 0x1F;
  return ((unsigned char)ret);
}

unsigned char getPktVersion(unsigned char data){
  unsigned char ret = (data & 0xC0 )>>6;
  return (unsigned char)ret;
}

unsigned char getPktNum(unsigned char data){
  unsigned char ret = (data & 0x3F );
  return (unsigned char)ret;
}

int ipow(int base, int exp)
{
    int result = 1;
    int i= 0;
    while (i<exp)
    {
        result *= base;
        i++;
    }

    return result;
}

unsigned char incInt8Bit(unsigned char counter, int nbits){
  printf("%d\n",nbits);
  printf("%d\n",ipow(2,nbits));
   return (counter+1) % ipow(2,nbits);
} 


void buildHeader(unsigned char type,unsigned char len, unsigned char vers, unsigned char num, char *headerType, unsigned char *headerMy){
  //TODO
  unsigned char byteType = ((unsigned char)type << 5) & 0x60;
  unsigned char byteSize = (unsigned char)len & 0x1F;
  unsigned char header = byteType | byteSize;
  *headerType = (char)header;

  unsigned char byteVer = ((unsigned char)vers << 6) & 0xC0;
  unsigned char byteNum = (unsigned char)num & 0x3F;
  unsigned char myHeader = byteVer | byteNum;
  *headerMy = myHeader;
  
}

int main(){
  char headerType;
  unsigned char headerMy;
  unsigned char type = 2;
  unsigned char dataSize = 10;
  unsigned char my_seq_num=0;
  unsigned char ACTUALVERSION=3;
  char* string = "OLA MUNDO AQUI";
  unsigned char spointer[30];
  char c[10];
  while(1){
    memcpy(spointer,string,strlen(string));
    printf("%lu==%lu\t-%s-\n",strlen(string),sizeof(spointer),spointer);
    buildHeader(type,dataSize,ACTUALVERSION,my_seq_num,&headerType,&headerMy);

    printf("Values: ");
    printf("type=%d",type);
    printf(" ");
    printf("dataSize=%d",dataSize);
    printf(" ");
    printf("Version=%d",ACTUALVERSION);
    printf(" ");
    printf("seqNum=%d",my_seq_num);
    printf("\nBuild: ");
    printf("%d",headerType);
    printf(" ");
    printf("%d\n",headerMy);

    printf("UNPACK ");
    printf("type=%d",getPktType(headerType));
    printf(" ");
    printf("dataSize=%d",getPktSize(headerType));
    printf(" ");
    printf("Version=%d",getPktVersion(headerMy));
    printf(" ");
    printf("seqNum=%d\n",getPktNum(headerMy));
   my_seq_num=incInt8Bit(my_seq_num,6);
    scanf("%s",c);
  }
}