#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"


#define user_procTaskPrio        0
#define user_procTaskQueueLen    2
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

char* gpDataBuffer;
uint32 giDataLen;
uint32 giDataMax;

//CommandTransfer
struct espconn espconnCommand;

//BulkTransfer
struct espconn espconnTransfer;

char rgcOutputString[65];


static void loop(os_event_t *events);
static void setupLocalAP();

static void enableTCPServer(uint32 iTCPPort, struct espconn* pConnection);
static void disableTCPServer(uint32 iTCPPort, struct espconn* pConnection);

static void handleConnectionEstablished(void* pArg);
static void handleConnectionDropped(void* pArg);

static void handleRecievedData(void* arg, char* pData, unsigned short iDataLen);

void user_init();


//Called on a timer
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
	struct ip_info addrInfo;
	if(false)//wifi_get_ip_info( 1, &addrInfo))
	{
		os_sprintf(rgcOutputString, "IP: %d.%d.%d.%d\n\r",  IP2STR(&addrInfo.ip.addr)); 
		os_printf(rgcOutputString);

		os_sprintf(rgcOutputString, "netmask: %d.%d.%d.%d\n\r", IP2STR(&addrInfo.netmask.addr));
		os_printf(rgcOutputString);

		os_sprintf(rgcOutputString, "gw: %d.%d.%d.%d\n\r\n\r",  IP2STR(&addrInfo.gw.addr));
		os_printf(rgcOutputString);
	}
	system_os_post(user_procTaskPrio, 0, 0 );
}

//Handles a connection from a remote target on any port
static void ICACHE_FLASH_ATTR
handleConnectionEstablished(void* pArg)
{
	struct espconn * pConnection = (struct espconn*) pArg;
	os_sprintf(rgcOutputString, "Connection established for port: %d\n\r", pConnection->proto.tcp->local_port);
	os_printf(rgcOutputString);
	giDataLen = 0;
	giDataMax = 0;
	espconn_regist_recvcb(pConnection, handleRecievedData);
	espconn_regist_disconcb(pConnection, handleConnectionDropped);
}
//Handles remote client disconnection
static void ICACHE_FLASH_ATTR
handleConnectionDropped(void* pArg)
{
	struct espconn * pConnection =  (struct espconn*) pArg;
	os_sprintf(rgcOutputString, "Connection dropped from port: %d\n\r", pConnection->proto.tcp->local_port);
	os_printf(rgcOutputString);
}
//Recieve data from a connection
static void ICACHE_FLASH_ATTR
handleRecievedData(void* arg, char* pData, unsigned short iLength)
{
	struct espconn * pConnection = (struct espconn*) arg;
	if(pConnection->proto.tcp->local_port == espconnCommand.proto.tcp->local_port)
	{
		os_printf("Recieved command length recieved: %d\n\r", iLength);
		os_printf("Recieved a command to : %s\n\r", pData);
	}
	else
	{
		os_printf(rgcOutputString, "Recieved data length recieved: %d\n\r", iLength);
	}

}
static void ICACHE_FLASH_ATTR
enableTCPServer(uint32 iTCPPort, struct espconn * pConnection)
{
	memset(pConnection, 0, sizeof( struct espconn ) );

	espconn_create(pConnection);
	pConnection->type  = ESPCONN_TCP;
	pConnection->state = ESPCONN_NONE; 

	pConnection->proto.tcp = (esp_tcp*)os_malloc(sizeof(esp_tcp));
	pConnection->proto.tcp->local_port = iTCPPort;

	espconn_regist_connectcb(pConnection, handleConnectionEstablished);

	espconn_accept(pConnection);
}
//Init localAP
static void ICACHE_FLASH_ATTR
setupLocalAP()
{
	struct softap_config wifiLocal;
	char wifiChar[33];

	wifi_softap_get_config(&wifiLocal);

	memset(wifiLocal.password, 0, sizeof(wifiLocal.password));
	memset(wifiLocal.ssid, 0, sizeof(wifiLocal.ssid));

	os_sprintf(wifiChar, "TestPass");
	memcpy(wifiLocal.password, wifiChar, strlen(wifiChar));

	os_sprintf(wifiChar, "ESPHost-%d", system_get_chip_id());
	memcpy(wifiLocal.ssid, wifiChar, strlen(wifiChar));
	wifiLocal.ssid_len = strlen(wifiChar);

	wifiLocal.authmode = AUTH_WPA_WPA2_PSK;
	wifi_softap_set_config(&wifiLocal);

	wifi_station_dhcpc_stop();
	{
		struct dhcps_lease sDHCP;
		sDHCP.start_ip = 0xC0A80464; //192.168.4.100
		sDHCP.end_ip   = 0xC0A804C8; //192.168.4.200
		wifi_softap_set_dhcps_lease(&sDHCP);
	}
	wifi_station_dhcpc_start();

	wifi_set_opmode(0x02);


	enableTCPServer(8080, &espconnCommand);
	enableTCPServer(8081, &espconnTransfer);
	espconn_tcp_set_max_con(1);

	system_os_post(0,0,0);
}

//Handle TCP Packet

//Init local TCP Server


//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	os_printf("SDK Version: %d.%d.%d/n/r", 1, 2, 3);

	gpDataBuffer = (char*)os_malloc(20480);

	//Start os task
	system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
	system_os_task(setupLocalAP, 1, user_procTaskQueue, user_procTaskQueueLen);
	system_os_post(1, 0, 0);
}
