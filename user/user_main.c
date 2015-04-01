#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "NonVol.h"
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

//CrossCallData
//Client Socket
struct espconn* espconnClient;
uint8 pBuffer[128]; //Temporary buffer for structures


//Timers
os_timer_t tConnectionTimer;
os_timer_t tStatusTimer;

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
	espconn_regist_time(pConnection, sysCfg.conTCPTimeout, 0);
}


//Handles a connection from a remote target on any port
static void ICACHE_FLASH_ATTR
handleConnectionEstablished(void* pArg)
{
	struct espconn * pConnection = (struct espconn*) pArg;
	espconnClient = pConnection;
	espconn_regist_time(pConnection, sysCfg.conTCPTimeout, 0);
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
static void Reboot()
{
	system_restart();
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
	if(cCommand == 'g' || cCommand == 'G')
	{
		char rgcOutputMessage[32];
		if(cTarget == 'a' || cTarget == 'A')
		{
			if(cEntry == 'p' || cEntry == 'P')
			{
				os_sprintf(rgcOutputMessage, "%s\n\r", sysCfg.localAP_pwd);
			}
			else
			{
				os_sprintf(rgcOutputMessage, "%s\n\r", sysCfg.localAP_ssid);
			}
		}
		else if(cTarget == 's' || cTarget == 'S')
		{
			if(cEntry == 'p' || cEntry == 'P')
			{
				os_sprintf(rgcOutputMessage, "%s\n\r", sysCfg.station_pwd);
			}
			else
			{
				os_sprintf(rgcOutputMessage, "%s\n\r", sysCfg.station_ssid);
			}
		}
		else if(cTarget == 'm' || cTarget == 'M')
		{
			os_sprintf(rgcOutputMessage, "%d\n\r", sysCfg.conbOutputMode);
		}
		else
		{
			os_sprintf(rgcOutputMessage, "UNKNOWN\n\r", sysCfg.station_ssid);
		}
		os_printf("Responding with: %s\n\r", rgcOutputMessage);
		espconn_sent(pTarget, rgcOutputMessage, strlen(rgcOutputMessage));
	}
	else if(cCommand == 's' || cCommand == 'S')
	{
		while(cValue[0] == ' ') ++cValue;
		char* cValueToChange;
		if(cTarget == 'm' || cTarget == 'M')
		{
			uint8_t newValue = atoi(cValue);
			os_printf("Setting changed from %d to %d\n\r", sysCfg.conbOutputMode, newValue);
			sysCfg.conbOutputMode = newValue;
		}
		else
		{
			if(cTarget == 'a' || cTarget == 'A')
			{
				if(cEntry == 'p' || cEntry == 'P')
				{
					cValueToChange = sysCfg.localAP_pwd;
				}
				else
				{
					cValueToChange = sysCfg.localAP_ssid;
				}
			}
			else if(cTarget == 's' || cTarget == 'S')
			{
				if(cEntry == 'p' || cEntry == 'P')
				{
					cValueToChange = sysCfg.station_pwd;
				}
				else
				{
					cValueToChange = sysCfg.station_ssid;
				}
			}
			os_printf("Setting changed from %s to %s\n\r", cValueToChange, cValue);
			os_sprintf(cValueToChange, cValue);
		}
		espconn_sent(pTarget, "OK\n\r", 4);
		CFG_Save();
	}
	else if(cCommand == 'r' || cCommand == 'R')
	{
		espconn_disconnect(espconnClient);
		scheduleCall(Reboot, 0, 0);
	}
}

//WifiMode
int iRetryCount;
static void ICACHE_FLASH_ATTR
checkConnection()
{
	os_printf("Checking connection\n\r");
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
		os_printf("Starting AP mode\n\r");
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

	memcpy(wifiLocal.ssid, sysCfg.localAP_ssid, strlen(sysCfg.localAP_ssid));
	wifiLocal.ssid_len = strlen(sysCfg.localAP_ssid);
	
	memcpy(wifiLocal.password, sysCfg.localAP_pwd, strlen(sysCfg.localAP_pwd));
	
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
	os_printf("Starting station mode\n\r");
	wifi_set_opmode(0x01);
	os_printf("Starting station configuration\n\r");
	struct station_config stationConf;
	wifi_station_get_config(&stationConf);
	
	memcpy(stationConf.ssid, sysCfg.localAP_ssid, strlen(sysCfg.localAP_ssid));
	stationConf.ssid_len = strlen(sysCfg.localAP_ssid);
	
	memcpy(stationConf.password, sysCfg.localAP_pwd, strlen(sysCfg.localAP_pwd));
	
	
	stationConf.bssid_set = 0;
	wifi_station_set_config(&stationConf);
	os_printf("Starting Connection\n\r");
	wifi_station_connect();
	os_printf("Starting DHCP Service\n\r");
	wifi_station_dhcpc_start();
	os_timer_disarm(&tConnectionTimer);
	os_timer_setfn(&tConnectionTimer, (os_timer_func_t*) checkConnection, 0);
	os_timer_arm(&tConnectionTimer, 4000, false);
	os_printf("Waiting for connection\n\r");
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	//Enable UART at 115200 BAUD
	
	CFG_Load();
	
	uart_div_modify(0, UART_CLK_FREQ / sysCfg.cfg_Baudrate);
	os_printf("UART Enabled\n\r");
	
	//Setup GPIO and data buffer
	gpio_init();
	os_printf("GPIO Enabled\n\r");
	gpDataBuffer = (char*)os_malloc(100);
	os_printf("Buffer Allocated\n\r");
	os_timer_disarm(&tStatusTimer);
	os_timer_setfn(&tStatusTimer, (os_timer_func_t*) loop, 0);
	os_timer_arm(&tStatusTimer, 10000, true);
	os_printf("Timer set\n\r");
	//Start os task
	system_os_task(connectToRemoteAP, 0, user_procTaskQueue, user_procTaskQueueLen);
	iRetryCount = 0;
	
	//Ensure watchdogs are running
	os_printf("WDT about to Enable\n\r");
	ets_wdt_disable();
	os_printf("OS Starting\n\r");
	system_os_post(0, 0, 0); //START SYSTEM
}
