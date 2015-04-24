#include "Port_HTTP.h"
#include "NonVol.h"
//HTTPTransfer socket
struct espconn espconnHTTP;
struct espconn* espconnClient;
extern SYSCFG sysCfg;

#include "response.c"

uint32 strpos(char* rgcSrc, char* rgcTar)
{
	uint32 uiSrcPos = 0;
	uint32 uiTarPos = 0;
	while(rgcSrc[uiSrcPos] != 0)
	{
		if(rgcTar[uiTarPos] != 0)
		{
			if(rgcSrc[uiSrcPos] == rgcTar[uiTarPos])
				++uiTarPos;
			else
				uiTarPos = 0;
		}
		else
		{
			return uiSrcPos - uiTarPos;
		}
		++uiSrcPos;
	}
	return -1;
}

void ICACHE_FLASH_ATTR
InitHTTPServer()
{
	memset(&espconnHTTP, 0, sizeof( struct espconn ) );
	enableHTTPServer(80);
}
void ICACHE_FLASH_ATTR
enableHTTPServer(uint32 iTCPPort)
{
	if(espconnHTTP.proto.tcp) //If we had something already
	{
		os_free(espconnHTTP.proto.tcp);
		espconn_delete(&espconnHTTP);
		memset(&espconnHTTP, 0, sizeof( struct espconn ) );
	}

	espconn_create(&espconnHTTP);
	espconnHTTP.type  = ESPCONN_TCP;
	espconnHTTP.state = ESPCONN_NONE; 

	espconnHTTP.proto.tcp = (esp_tcp*)os_malloc(sizeof(esp_tcp));
	espconnHTTP.proto.tcp->local_port = iTCPPort;

	espconn_regist_connectcb(&espconnHTTP, HTTPConnectionEstablished);
	espconn_accept(&espconnHTTP);
	espconn_regist_time(&espconnHTTP, 120, 0);
}


//Handles a connection from a remote target on any port
void ICACHE_FLASH_ATTR
HTTPConnectionEstablished(void* pArg)
{
	struct espconn * pConnection = (struct espconn*) pArg;
	espconn_regist_time(pConnection, 120, 0);
	os_printf("Connection established to HTTP server\n\r");
	espconn_regist_recvcb(pConnection, HTTPDataRecieved);
	espconn_regist_disconcb(pConnection, HTTPConnectionClosed);
}
//Handles remote client disconnection
void ICACHE_FLASH_ATTR
HTTPConnectionClosed(void* pArg)
{
	struct espconn * pConnection =  (struct espconn*) pArg;
	os_printf("Connection dropped from HTTP server\n\r");
}
void ICACHE_FLASH_ATTR HTTPHandleDataSent(void* arg)
{
	struct espconn * pConnection = (struct espconn*) arg;
	espconn_disconnect(pConnection);
	espconn_delete(pConnection);
	os_printf("Send Done\n\r");
}
void ICACHE_FLASH_ATTR
HTTPDataRecieved(void* arg, char* pData, uint16 iLength)
{
	struct espconn * pConnection = (struct espconn*) arg;
	os_printf("Recieved HTTP length recieved: %d\n\r", iLength);
	os_printf("Recieved a HTTP to : %s\n\r", pData);
	char rgcOutputString[1200];
	
	if(strpos(pData, "POST /") != -1)
	{
		os_printf("Handled Submit!\n\r");
		os_sprintf(rgcOutputString, "HTTP/1.1 200 OK\n\rConnection: close\n\r\n\r<html><head/><body><h1>DONE</body></html>");
	}
	else if(strpos(pData, "GET / ") != -1)
	{
		os_sprintf(rgcOutputString, rgcDefaultConnection, sysCfg.station_ssid, sysCfg.station_pwd, sysCfg.localAP_ssid, sysCfg.localAP_pwd, sysCfg.identifier);
	}
	else
	{
		os_printf("Handled Fail!\n\r");
		os_sprintf(rgcOutputString, "HTTP/1.1 200 OK\n\rConnection: close\n\r\n\r<html><head/><body><h1>Whoops, wrong link</body></html>");
	}
	
	espconn_regist_sentcb(pConnection, HTTPHandleDataSent);
	espconn_sent(pConnection, rgcOutputString, strlen(rgcOutputString));
	os_printf("Send Started\n\r");
}
