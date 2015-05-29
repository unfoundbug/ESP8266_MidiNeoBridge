#include "Port_Transfer.h"
#include "application.h"
#include "eagle_soc.h"
#define midiHigh() PIN_OUT_SET = 0x04; os_delay_us(38);
#define midiLow() PIN_OUT_CLEAR = 0x04; os_delay_us(38);

#define midiBit(bByte, oSet) if(bByte&oSet) {midiHigh();} else{ midiLow();}

#define sendMidiByte(bByte)	midiLow(); midiBit(bByte, 0x01); midiBit(bByte, 0x02); midiBit(bByte, 0x04); midiBit(bByte, 0x08); \
									  midiBit(bByte, 0x10); midiBit(bByte, 0x20); midiBit(bByte, 0x40); midiBit(bByte, 0x80); \
							midiHigh()os_delay_us(10);					
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
		gpDataBuffer = (char*)os_malloc((30*1024) + 2 + 1); //Data + size + tempo
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
uint32_t usPerBeat;

void ICACHE_FLASH_ATTR
TransferDataRecieved(void* pTarget, char* pData, unsigned short iLength)
{
		char* pDataToRead = pData;
		unsigned short iToRead = iLength;
		char i;
		if(giDataMax == 0)
		{
			for(i = 0; i < 8; ++i)
				os_printf("%d ", pData[i]);
			giDataMax = (pData[0] << 8) | pData[1];
			os_printf("\n\rNew max length: %d\n\r", giDataMax);
			pDataToRead = pData + 2;
			iToRead -= 2;
		}
		os_memcpy(gpDataBuffer+giDataLen, pDataToRead, iToRead);
		giDataLen+=iToRead;
		if(giDataLen == giDataMax)
		{
			os_printf("SendingData");
			usPerBeat = (gpDataBuffer[0] << 8)| gpDataBuffer[1];
			os_printf("Starting to play %d bytes at %dus per beat\n\r", giDataMax, usPerBeat);
			ProcessMidi(gpDataBuffer+2, giDataMax - 2);
			giDataLen = 0;
			giDataMax = 0;
			os_printf("Packet Complete\n\r");
		}
		os_printf("Recieved data length recieved: %d\n\r", iLength);
}
void ProcessMidi(char* pcNMidi, uint32 uiLen)
{
	os_printf("Sending bytes\n\r");
	uint32 uiCurPoint = 0;
	while(uiLen > uiCurPoint)
	{
		uint16_t uiTimeToWait = pcNMidi[uiCurPoint];
		uiTimeToWait = uiTimeToWait << 8;
		uiTimeToWait =uiTimeToWait + pcNMidi[uiCurPoint + 1];
		char* pcCurCmd = pcNMidi + uiCurPoint + 2;
		
		if(uiTimeToWait)
		{
			os_delay_us(uiTimeToWait * usPerBeat);
		}
		sendMidiByte(pcCurCmd[0]);
		sendMidiByte(pcCurCmd[1]);
		if(pcCurCmd[2] != 0xFF)
		{
			sendMidiByte(pcCurCmd[2]);
			uiCurPoint += 3;
		}
		else
		{
			uiCurPoint += 2;
		}
	}
}