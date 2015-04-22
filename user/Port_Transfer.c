#include "Port_Transfer.h"
#include "application.h"
#define sendMidiByte(bByte)	pinLow(); midiBit(bByte, 0x01); midiBit(bByte, 0x02); midiBit(bByte, 0x04); midiBit(bByte, 0x08); \
									  midiBit(bByte, 0x10); midiBit(bByte, 0x20); midiBit(bByte, 0x40); midiBit(bByte, 0x80); \
							pinHigh();


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
		gpDataBuffer = (char*)os_malloc(100);
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
		if(giDataMax == 0)
		{
			giDataMax = ((uint16*)pData)[0];
			os_printf("New max length: %d\n\r", giDataMax);
			pDataToRead = pData + 2;
			iToRead -= 2;
		}
		os_memcpy(gpDataBuffer+giDataLen, pDataToRead, iToRead);
		giDataLen+=iToRead;
		if(giDataLen == giDataMax)
		{
			os_printf("SendingData");
			uint16 i;
			for(i = 0; i < giDataMax; ++i)
			{
				sendMidiByte(gpDataBuffer[i]);
			}

			giDataLen = 0;
			giDataMax = 0;
			os_printf("Packet Complete\n\r");
		}
		os_printf("Recieved data length recieved: %d\n\r", iLength);
}