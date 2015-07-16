#include "Port_Transfer.h"
#include "application.h"
#include "eagle_soc.h"
#define midiHigh() PIN_OUT_SET = 0x04; os_delay_us(38);
#define midiLow() PIN_OUT_CLEAR = 0x04; os_delay_us(38);

#define midiBit(bByte, oSet) if(bByte&oSet) {midiHigh();} else{ midiLow();}

#define sendMidiByte(bByte)	midiLow(); midiBit(bByte, 0x01); midiBit(bByte, 0x02); midiBit(bByte, 0x04); midiBit(bByte, 0x08); \
									  midiBit(bByte, 0x10); midiBit(bByte, 0x20); midiBit(bByte, 0x40); midiBit(bByte, 0x80); \
							midiHigh()os_delay_us(10);	

uint32_t ccount;
uint32_t targetCount;
#define UpdateCycleCount() {__asm__ __volatile__("rsr %0,ccount":"=a" (ccount));}
#define waitTillEnd() {do {UpdateCycleCount();} while(ccount < targetCount);};

#define T0H 5
#define T0L 96
#define T1H 160
#define T1L 104
uint16_t m_ui16OutputPins = 0x04;							
#define pinHigh() PIN_OUT_SET = 0x04;
#define pinLow()  PIN_OUT_CLEAR= 0x04;
//40 high, 96 low
#define neo0() { UpdateCycleCount(); pinHigh(); targetCount = T0H + ccount; waitTillEnd(); \
                 UpdateCycleCount(); pinLow();  targetCount = T0L + ccount; waitTillEnd();}
//160 high 104 low
#define neo1() { UpdateCycleCount(); pinHigh(); targetCount = T1H + ccount; waitTillEnd(); \
                  UpdateCycleCount();pinLow();  targetCount = T1L + ccount; waitTillEnd();}
				 
				 
#define neoBit(bByte, oSet) {if(bByte&oSet) {neo1();} else{ neo0();} }

#define sendneoByte(bByte)	{   neoBit(bByte, 0x80); neoBit(bByte, 0x40); neoBit(bByte, 0x20); neoBit(bByte, 0x10); \
								neoBit(bByte, 0x08); neoBit(bByte, 0x04); neoBit(bByte, 0x02); neoBit(bByte, 0x01);}
#define neoLatch()	pinLow(); ccount = 100; while(--ccount);

#define SendWheel(Point) { char r,g,b, p; p = Point % 255; \
							if(p < 85) \
							{ r= (255- (p * 3)); g = (0); b = (p*3);} \
							else if(p < 170) \
							{ p-= 85; r = (0); g = (p*3); b = (255- (p * 3));} \
							else \
							{ p-= 170;r = (p * 3); g = (255- (p * 3)); b = (0);} \
							sendneoByte(r); sendneoByte(g); sendneoByte(b);\
						}

							
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
	os_printf("Connection Established\n\r");
	struct espconn * pConnection = (struct espconn*) pArg;
	espconnClient = pConnection;
	espconn_regist_time(pConnection, 120, 0);
	giDataLen = 0;
	giDataMax = 0;
	espconn_regist_recvcb(pConnection, TransferDataRecieved);
	espconn_regist_disconcb(pConnection, TransferConnectionClosed);
}
//Handles remote client disconnection
void ICACHE_FLASH_ATTR
TransferConnectionClosed(void* pArg)
{
	os_printf("Connection closed\n\r");
	struct espconn * pConnection =  (struct espconn*) pArg;
}
uint32_t usPerBeat;
uint8_t bCounter = 0;
void TransferDataRecieved(void* pTarget, char* pData, unsigned short iLength)
{
	if(bCounter == 0)
	{
		bCounter = 250;
		os_printf("Memory left: %d\n\r", system_get_free_heap_size());
	}
	else
		--bCounter;
		char* pDataToRead = pData;
		unsigned short iToRead = iLength;
		char i;
		if(giDataMax == 0)
		{
			giDataMax = (pData[0] << 8) | pData[1];
			pDataToRead = pData + 2;
			iToRead -= 2;
		}
		os_memcpy(gpDataBuffer+giDataLen, pDataToRead, iToRead);
		giDataLen+=iToRead;
		if(giDataLen == giDataMax)
		{
			switch (sysCfg.conbOutputMode)
			{
				case  1:
				{
					usPerBeat = (gpDataBuffer[0] << 8)| gpDataBuffer[1];
					os_printf("Starting to play %d bytes at %dus per beat\n\r", giDataMax - 2, usPerBeat);
					ProcessMidi(gpDataBuffer+2, giDataMax - 2);
				}break;
				default:
				{
					
					m_ui16OutputPins = gpDataBuffer[0];
					m_ui16OutputPins = m_ui16OutputPins << 8;
					m_ui16OutputPins |= gpDataBuffer[1];
					m_ui16OutputPins &= 0x1111000000110101;
					//os_printf("Starting to send %d bytes over ports %d\n\r", giDataMax - 2, m_ui16OutputPins);
					ProcessNeo(gpDataBuffer+2, giDataMax - 2);
					//os_printf("Done\n\r", giDataMax - 2);
				}break;
			}
			espconn_sent(pTarget, "1", 1);
			giDataLen = 0;
			giDataMax = 0;
		}
}
void ProcessNeo(char* pcNPixel, uint32 uiLen)
{
	/*while(1)
	{
		int i;
		int j;
		int k;
		for(j = 0; j < 255; ++j)
		{
				for(i = 0; i < 29; ++i)
				{
					SendWheel((i*8)+(j*8));
				}
				neoLatch();
				os_delay_us(90000);
		}
	}*/
	uint32 iCount;
	ets_wdt_disable();
	os_intr_lock();
	for(iCount = 0; iCount < uiLen; iCount += 3)
	{
		sendneoByte(pcNPixel[iCount+1]);
		sendneoByte(pcNPixel[iCount]);
		sendneoByte(pcNPixel[iCount+2]);
	}
	os_intr_unlock();
}


void ProcessMidi(char* pcNMidi, uint32 uiLen)
{
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