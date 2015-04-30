#include "Port_Transfer.h"
#include "application.h"

#define midiHigh() GPIO_OUTPUT_SET(2,1); os_delay_us(32);
#define midiLow() GPIO_OUTPUT_SET(2,0); os_delay_us(32);

#define midiBit(bByte, oSet) if(bByte&oSet) {midiHigh();} else{ midiLow();}

#define sendMidiByte(bByte)	midiLow(); midiBit(bByte, 0x01); midiBit(bByte, 0x02); midiBit(bByte, 0x04); midiBit(bByte, 0x08); \
									  midiBit(bByte, 0x10); midiBit(bByte, 0x20); midiBit(bByte, 0x40); midiBit(bByte, 0x80); \
							midiHigh();


#define neoBit(bByte, oSet) pinHigh(); \
								\
								if(bByte&oSet)\
								{}\
							pinLow();
#define neoByte(byte)     neoBit(bByte, 0x01); neoBit(bByte, 0x02); neoBit(bByte, 0x04); neoBit(bByte, 0x08); \
						  neoBit(bByte, 0x10); neoBit(bByte, 0x20); neoBit(bByte, 0x40); neoBit(bByte, 0x80);
					
//CommandTransfer socket
struct espconn espconnTransfer;

//TransferBuffer
char* gpDataBuffer;
uint32 giDataLen;
uint32 giDataMax;
struct espconn* espconnClient;

void ICACHE_FLASH_ATTR InitTransferServer(uint32 Port)
{
	memset(&espconnTransfer, 0, sizeof( struct espconn ) );
	enableTransferServer(Port);
}

void ICACHE_FLASH_ATTR
enableTransferServer(uint32 iTCPPort)
{
	if(espconnTransfer.proto.tcp) //If we had something already
	{
		os_free(espconnTransfer.proto.tcp);
		espconn_delete(&espconnTransfer);
		memset(&espconnTransfer, 0, sizeof( struct espconn ) );
	}
	else
	{
		gpDataBuffer = (char*)os_malloc(2048 + 2 + 1); //Data + size + tempo
		os_printf("Buffer Allocated\n\r");
	}
	espconn_create(&espconnTransfer);
	espconnTransfer.type  = ESPCONN_TCP;
	espconnTransfer.state = ESPCONN_NONE; 

	espconnTransfer.proto.tcp = (esp_tcp*)os_malloc(sizeof(esp_tcp));
	espconnTransfer.proto.tcp->local_port = iTCPPort;

	espconn_regist_connectcb(&espconnTransfer, TransferConnectionEstablished);
	espconn_accept(&espconnTransfer);
	espconn_regist_time(&espconnTransfer, 120, 0);
}


//Handles a connection from a remote target on any port
void ICACHE_FLASH_ATTR
TransferConnectionEstablished(void* pArg)
{
	struct espconn * pConnection = (struct espconn*) pArg;
	espconnClient = pConnection;
	espconn_regist_time(pConnection, 120, 0);
	os_printf("Connection established to Transfer server\n\r");
	giDataLen = 0;
	giDataMax = 0;
	espconn_regist_recvcb(pConnection, TransferDataRecieved);
	espconn_regist_disconcb(pConnection, TransferConnectionClosed);
}
//Handles remote client disconnection
void ICACHE_FLASH_ATTR
TransferConnectionClosed(void* pArg)
{
	struct espconn * pConnection =  (struct espconn*) pArg;
	os_printf("Connection dropped from command server\n\r");
}


void ICACHE_FLASH_ATTR
TransferDataRecieved(void* pTarget, char* pData, unsigned short iLength)
{
		char* pDataToRead = pData;
		unsigned short iToRead = iLength;
		uint32_t usPerBeat;
		if(giDataMax == 0)
		{
			giDataMax = (pData[0] << 8) | pData[1];
			os_printf("New max length: %d\n\r", giDataMax);
			pDataToRead = pData + 2;
			iToRead -= 2;
		}
		os_memcpy(gpDataBuffer+giDataLen, pDataToRead, iToRead);
		giDataLen+=iToRead;
		if(giDataLen == giDataMax)
		{
			os_printf("SendingData");
			char iBPM = gpDataBuffer[0];
			usPerBeat = 1000000 / iBPM;
			
			os_printf("Starting to play %d bytes at %dbpm, with %d us per beat\n\r", giDataMax - 1, iBPM, usPerBeat);
			/*
			for(i = 0; i < giDataMax; ++i)
			{
				sendMidiByte(gpDataBuffer[i]);
			}
			*/
			giDataLen = 0;
			giDataMax = 0;
			os_printf("Packet Complete\n\r");
		}
		os_printf("Recieved data length recieved: %d\n\r", iLength);
}
char* ProcessMidi(char* pcNMidi)
{
	uint16_t uiTimeToWait = *((uint16_t*)pcNMidi);
	char* pcCurCmd = pcNMidi + 2;
	
	if(uiTimeToWait)
	{
	}
	sendMidiByte(pcCurCmd[0]);
	sendMidiByte(pcCurCmd[1]);
	if(pcCurCmd[0] < 0xB0 || pcCurCmd[0] >= 0xe0)
	{
		sendMidiByte(pcCurCmd[2]);
		pcCurCmd += 3;
	}
	else
		pcCurCmd += 2;
}