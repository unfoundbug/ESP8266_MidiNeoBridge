#include "Port_HTTP.h"
#include "NonVol.h"
//HTTPTransfer socket
struct espconn espconnHTTP;
struct espconn* espconnClient;
extern SYSCFG sysCfg;

#include "response.c"

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
	espconn_regist_sentcb(pConnection, HTTPHandleDataSent);
	espconn_sent(pConnection, rgcDefaultConnection, strlen(rgcDefaultConnection));
	os_printf("Send Started\n\r");
}
