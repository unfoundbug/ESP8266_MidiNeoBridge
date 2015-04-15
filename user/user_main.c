#include "user_config.h"
#include "NonVol.h"
#include "application.h"

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

#define sendMidiByte(bByte)	pinLow(); midiBit(bByte, 0x01); midiBit(bByte, 0x02); midiBit(bByte, 0x04); midiBit(bByte, 0x08); \
									  midiBit(bByte, 0x10); midiBit(bByte, 0x20); midiBit(bByte, 0x40); midiBit(bByte, 0x80); \
							pinHigh();

//Called on a timer
static void ICACHE_FLASH_ATTR
StateEngine(os_event_t *events)
{
	struct ip_info addrInfo;
	os_printf("State engine cycling\n\r");
	switch (eCurrentLaunchState)
	{
		case STATE_STARTREMOTE:
		{
			os_printf("Starting connection to remote wifi\n\r");
			connectToRemoteAP();
			os_printf("Connection Started\n\r");
			gi_RemoteStation_EndTime = currentRunTime() + CONNECTION_TIMEOUT;
			os_printf("End time got\n\r");
			eCurrentLaunchState = STATE_REMOTE_AWAITING;
			os_printf("State Done\n\r");
		}break;
		case STATE_REMOTE_AWAITING:
		{
			addrInfo.ip.addr = 0;
			if(!wifi_get_ip_info( 0, &addrInfo))
			{
				os_printf("Unable to get IP details\n\r");
				return;
			}
			else
			{
				os_printf("Got IP Details\n\r");				
			}
			if(addrInfo.ip.addr != 0) //Connection
			{
				eCurrentLaunchState = STATE_REMOTE_CONNECTED;
				gi_RemoteStation_EndTime = currentRunTime() + CONNECTION_TIMEOUT;
				os_printf("Connected to remote wifi\n\r");
				os_printf("IP: %d.%d.%d.%d\n\r",  IP2STR(&addrInfo.ip.addr)); 
				os_printf("netmask: %d.%d.%d.%d\n\r", IP2STR(&addrInfo.netmask.addr));
				os_printf("gw: %d.%d.%d.%d\n\r\n\r",  IP2STR(&addrInfo.gw.addr));
			}
			else //No Connection
			{
				if(gi_RemoteStation_EndTime < currentRunTime())
				{
					eCurrentLaunchState = STATE_STARTLOCAL;
				}
			}
		}break;
		case STATE_REMOTE_CONNECTED:
		{
			os_printf("Connection Established\n\r");
			if(	gi_RemoteStation_EndTime < currentRunTime())
			{
				
			}
			//If we have lost connection, restart
		}break;
		case STATE_STARTLOCAL:
		{
			os_printf("Starting local wifi host\n\r");
			setupLocalAP();
			eCurrentLaunchState = STATE_LOCAL_NOCOMMMS;
			gi_Local_AP_EndTime = currentRunTime() + SETUP_TIMEOUT; //After 10 minutes reset if there we NO comms
		}break;
		case STATE_LOCAL_NOCOMMMS:
		{
			if(gi_Local_AP_EndTime < currentRunTime())
			{
				scheduleCallTime(Reboot, 250);
			}
		}break;
		case STATE_LOCAL_COMMS:
		{
		}break;
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
	espconn_regist_time(pConnection, 120, 0);
}


//Handles a connection from a remote target on any port
static void ICACHE_FLASH_ATTR
handleConnectionEstablished(void* pArg)
{
	struct espconn * pConnection = (struct espconn*) pArg;
	espconnClient = pConnection;
	espconn_regist_time(pConnection, 120, 0);
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
	char cTarget = 0; //Station, AP
	char cEntry = 0; //SSID, Password
	if(iLength >= 3)
	{
		cTarget = pData[1]; //Station, AP
		cEntry = pData[2]; //SSID, Password
	}
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
		scheduleCallTime(Reboot, 250);
	}
	else if(cCommand == 'c' || cCommand == 'C')
	{
		sysCfg.cfg_holder = 0; //Forcibly reset all settings
		CFG_Save();
		CFG_Save();
	}
}

//WifiMode
int iRetryCount;
static void ICACHE_FLASH_ATTR
startServers()
{
	os_printf("Starting servers\n\r");
	enableTCPServer(8080, &espconnCommand);
	enableTCPServer(8081, &espconnTransfer);
	espconn_tcp_set_max_con(1);
	os_printf("Servers started\n\r");
}

static void ICACHE_FLASH_ATTR
setupLocalAP()
{
	struct softap_config wifiLocal;
	char wifiChar[33];
	wifi_set_opmode(0x02);
	wifi_softap_get_config(&wifiLocal);
	
	if(sysCfg.localAP_pwd[0])
	{
		os_printf("LocalPassword: %s\n\r", sysCfg.localAP_pwd);
		os_sprintf(wifiLocal.password, sysCfg.localAP_pwd);
		wifiLocal.authmode = AUTH_WPA_WPA2_PSK;
	}
	else
	{
		wifiLocal.authmode = AUTH_OPEN;
		os_printf("Default Password\n\r");
		wifiLocal.password[0] = 0; //No Password
	}
	
	if(sysCfg.localAP_ssid[0])
	{
		os_printf("LocalSSID: %s\n\r", sysCfg.localAP_ssid);
		os_sprintf(wifiLocal.ssid, sysCfg.localAP_ssid);
		wifiLocal.ssid_len = strlen(wifiLocal.ssid);
	}
	else
	{
		os_printf("Default SSID\n\r");
	}
	
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
	startServers();
}

static void ICACHE_FLASH_ATTR
connectToRemoteAP()
{
	os_printf("Starting station mode\n\r");
	//Set Remote mode, to allow settings update
	wifi_set_opmode(0x01);
	os_printf("Starting station configuration\n\r");
	struct station_config stationConf;
	wifi_station_get_config(&stationConf);
	if(sysCfg.station_pwd[0])
	{
		os_printf("Remote Password: %s\n\r", sysCfg.station_pwd);
		os_sprintf(stationConf.password, sysCfg.station_pwd);
	}
	else
	{
		os_printf("Default remote Password\n\r");
		stationConf.password[0] = 0; //No Password
	}
	
	if(sysCfg.station_ssid[0])
	{
		os_printf("Remote SSID: %s\n\r", sysCfg.station_ssid);
		os_sprintf(stationConf.ssid, sysCfg.station_ssid);
	}
	stationConf.bssid_set = 0;
	wifi_station_set_config(&stationConf);
	os_printf("Starting Connection\n\r");
	wifi_station_connect();
	os_printf("Starting DHCP Service\n\r");
	wifi_station_dhcpc_start();
	//Restart mode, not sure if this helps
	wifi_set_opmode(0x01);
	startServers();
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	//Enable UART at 115200 BAUD
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	CFG_Load();
	
	os_printf("Loading complete\n\r");
	if(sysCfg.station_ssid[0])
		os_printf("Station SSID: %s\n\r", sysCfg.station_ssid);
	if(sysCfg.station_pwd[0])
		os_printf("Station PWD: %s\n\r", sysCfg.station_pwd);
	if(sysCfg.localAP_ssid[0])
		os_printf("LocalAP SSID: %s\n\r", sysCfg.localAP_ssid);
	if(sysCfg.localAP_pwd[0])
		os_printf("LocalAP PWD: %s\n\r", sysCfg.localAP_pwd);
	os_printf("Baud: %d\n\r", sysCfg.cfg_BaudRate);
	os_printf("OutMode: %d\n\r", sysCfg.conbOutputMode);
	os_printf("Timeout: %d\n\r", sysCfg.conTCPTimeout);
	
	os_printf("UART Enabled\n\r");
	
	//Setup GPIO and data buffer
	gpio_init();
	os_printf("GPIO Enabled\n\r");
	gpDataBuffer = (char*)os_malloc(100);
	os_printf("Buffer Allocated\n\r");
	os_timer_disarm(&tStatusTimer);
	os_timer_setfn(&tStatusTimer, (os_timer_func_t*) StateEngine, 0);
	os_timer_arm(&tStatusTimer, 2000, true);
	os_printf("Timer set\n\r");
	os_printf("RTC calibration at %d\n\r", system_rtc_clock_cali_proc());
	eCurrentLaunchState = STATE_STARTREMOTE;
}
