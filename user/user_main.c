#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

//Tasks
#define user_procTaskPrio        0
#define user_procTaskQueueLen    2
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

//TransferBuffer
char* gpDataBuffer;
uint32 giDataLen;
uint32 giDataMax;

//CommandTransfer socket
struct espconn espconnCommand;
//BulkTransfer socket
struct espconn espconnTransfer;

//Timers
os_timer_t tConnectionTimer;
os_timer_t tStatusTimer;

//Function definitions
static void loop(os_event_t *events);

static void checkConnection();
static void setupLocalAP();
static void connectToRemoteAP();

static void enableTCPServer(uint32 iTCPPort, struct espconn* pConnection);
static void disableTCPServer(uint32 iTCPPort, struct espconn* pConnection);

static void handleConnectionEstablished(void* pArg);
static void handleRecievedData(void* arg, char* pData, unsigned short iDataLen);
static void handleConnectionDropped(void* pArg);

static void processCommand(struct espconn* pTarget, char* pData, uint16 iLength);
static void processTransfer(struct espconn* pTarget, char* pData, uint16 iLength);

#define pinHigh() GPIO_OUTPUT_SET(2,1); os_delay_us(29);
#define pinLow() GPIO_OUTPUT_SET(2,0); os_delay_us(29);

#define midiBit(bByte, oSet) if(bByte&oSet) {pinHigh();} else{ pinLow();}

void sendMidiByte(char bByte) {
	//StartBit
	pinLow();

	midiBit(bByte, 0x01);
	midiBit(bByte, 0x02);
	midiBit(bByte, 0x04);
	midiBit(bByte, 0x08);
	midiBit(bByte, 0x10);
	midiBit(bByte, 0x20);
	midiBit(bByte, 0x40);
	midiBit(bByte, 0x80);

	//EndBit
	pinHigh();
}


void user_init();


//Called on a timer
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
	struct ip_info addrInfo;
	if(wifi_get_ip_info( 1, &addrInfo))
	{
		os_printf("Connection Status: \n\r");
		os_printf("IP: %d.%d.%d.%d\n\r",  IP2STR(&addrInfo.ip.addr)); 
		os_printf("netmask: %d.%d.%d.%d\n\r", IP2STR(&addrInfo.netmask.addr));
		os_printf("gw: %d.%d.%d.%d\n\r\n\r",  IP2STR(&addrInfo.gw.addr));

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


//Handles a connection from a remote target on any port
static void ICACHE_FLASH_ATTR
handleConnectionEstablished(void* pArg)
{
	struct espconn * pConnection = (struct espconn*) pArg;
	os_printf("Connection established for port: %d\n\r", pConnection->proto.tcp->local_port);
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
	os_printf("Connection dropped from port: %d\n\r", pConnection->proto.tcp->local_port);
}
//Recieve data from a connection
static void ICACHE_FLASH_ATTR
handleRecievedData(void* arg, char* pData, unsigned short iLength)
{
	os_printf("Handling data\n\r");
	os_printf("Current State: Max:%d Cur:%d\n\r", giDataMax, giDataLen);

	struct espconn * pConnection = (struct espconn*) arg;
	if(pConnection->proto.tcp->local_port == espconnCommand.proto.tcp->local_port)
	{
		processCommand(pConnection, pData, iLength);
	}
	else
	{
		processTransfer(pConnection, pData, iLength);
	}
}

static void ICACHE_FLASH_ATTR
processCommand(struct espconn* pTarget, char* pData, uint16 iLength)
{
	os_printf("Recieved command length recieved: %d\n\r", iLength);
	os_printf("Recieved a command to : %s\n\r", pData);
	
	struct softap_config wifiLocal;
	struct station_config stationConf;
	
	char cCommand = pData[0]; //Get, Set, Reset
	char cTarget = pData[1]; //Station, AP
	char cEntry = pData[2]; //SSID, Password
	char* cValue = pData+3;
	
	char* pcTargetSSID;
	char* pcTargetPassword;
	if(cTarget== 's' || cTarget == 'S')
	{
		wifi_station_get_config(&stationConf);
		pcTargetSSID = stationConf.ssid;
		pcTargetPassword = stationConf.password;
	}
	else
	{
		wifi_softap_get_config(&wifiLocal);
		pcTargetSSID = wifiLocal.ssid;
		pcTargetPassword = wifiLocal.password;
	}
	if(cCommand == 'g' || cCommand == 'G')
	{
		char* pcTarget = cEntry == 's' ? pcTargetSSID : pcTargetPassword;
		char rgcOutputMessage[32];
		os_sprintf(rgcOutputMessage, "%s\n\r", pcTarget);
		espconn_sent(pTarget, rgcOutputMessage, strlen(rgcOutputMessage));
	}
	else if(cCommand == 's' || cCommand == 'S')
	{
		
		char* pcTarget = cEntry == 's' ? pcTargetSSID : pcTargetPassword;
		os_printf("Setting %c changed from %s to %s\n\r", cEntry, pcTarget, cValue);
		os_sprintf(pcTarget, "%s", cValue);
		if(cTarget== 's' || cTarget == 'S')
		{
			wifi_station_set_config(&stationConf);
		}
		else
		{
			wifi_softap_set_config(&wifiLocal);
		}
		
	}
}
static void ICACHE_FLASH_ATTR
processTransfer(struct espconn* pTarget, char* pData, uint16 iLength)
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


//WifiMode
int iRetryCount;
static void ICACHE_FLASH_ATTR
checkConnection()
{
	if(wifi_station_get_connect_status() == STATION_GOT_IP)  //Connected
	{
		os_printf("Connected OK\n\r");
	}
	else if(iRetryCount < 2) //Any retries left?
	{
		++iRetryCount;
		os_printf("Waiting for connection\n\r");
		os_timer_arm(&tConnectionTimer,2000, false);
		return;
	}
	else
	{
		wifi_station_dhcpc_stop();
		wifi_station_disconnect();
		setupLocalAP();
	}
	
	enableTCPServer(8080, &espconnCommand);
	enableTCPServer(8081, &espconnTransfer);
	espconn_tcp_set_max_con(1);
}
static void ICACHE_FLASH_ATTR
setupLocalAP()
{
	struct softap_config wifiLocal;
	char wifiChar[33];

	wifi_softap_get_config(&wifiLocal);

	os_sprintf(wifiChar, "ESPHost-%d", system_get_chip_id());
	memcpy(wifiLocal.ssid, wifiChar, strlen(wifiChar));
	wifiLocal.ssid_len = strlen(wifiChar);

	wifiLocal.authmode = AUTH_WPA_WPA2_PSK;
	wifi_softap_set_config(&wifiLocal);

	wifi_softap_dhcps_stop();
	{
		struct dhcps_lease sDHCP;
		sDHCP.start_ip = 0xC0A80464; //192.168.4.100
		sDHCP.end_ip   = 0xC0A804C8; //192.168.4.200
		wifi_softap_set_dhcps_lease(&sDHCP);
	}
	wifi_softap_dhcps_start();

	wifi_set_opmode(0x02);
}
static void ICACHE_FLASH_ATTR
connectToRemoteAP()
{
	wifi_set_opmode(0x01);
	struct station_config stationConf;
	wifi_station_get_config(&stationConf);
	stationConf.bssid_set = 0;
	wifi_station_set_config(&stationConf);
	wifi_station_connect();
	wifi_station_dhcpc_start();
	os_timer_disarm(&tConnectionTimer);
	os_timer_setfn(&tConnectionTimer, (os_timer_func_t*) checkConnection, 0);
	os_timer_arm(&tConnectionTimer, 4000, false);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	//Enable UART at 115200 BAUD
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	
	//Setup GPIO and data buffer
	gpio_init();
	gpDataBuffer = (char*)os_malloc(10000);
	
	os_timer_disarm(&tStatusTimer);
	os_timer_setfn(&tStatusTimer, (os_timer_func_t*) loop, 0);
	os_timer_arm(&tStatusTimer, 10000, true);

	//Start os task
	system_os_task(connectToRemoteAP, 1, user_procTaskQueue, user_procTaskQueueLen);
	iRetryCount = 0;
	system_os_post(1, 0, 0);
}
