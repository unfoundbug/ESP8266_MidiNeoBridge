#include "Port_Command.h"
#include "NonVol.h"
//CommandTransfer socket
struct espconn espconnCommand;
struct espconn* espconnClient;
extern SYSCFG sysCfg;
void ScanCallback(void* arg, STATUS status)
{
	char rgcOutString[64];
	os_printf("Scan callback %d\n\r", status);
	struct bss_info *bss = (struct bss_info*) arg;
	if(status == OK)
	{
			while(bss)
			{
				uint8 bssid[6];
				os_sprintf(rgcOutString, "Found AP ssid: %s(%d)\n\r", bss->ssid, bss->channel);
				espconn_sent(espconnClient, rgcOutString, strlen(rgcOutString));
				os_printf(rgcOutString);
				bss = bss->next.stqe_next;
			}
	}
}
void ICACHE_FLASH_ATTR
InitCommandServer(uint32 port)
{
	memset(&espconnCommand, 0, sizeof( struct espconn ) );
	enableCommandServer(port);
}
void ICACHE_FLASH_ATTR
enableCommandServer(uint32 iTCPPort)
{
	if(espconnCommand.proto.tcp) //If we had something already
	{
		os_free(espconnCommand.proto.tcp);
		espconn_delete(&espconnCommand);
		memset(&espconnCommand, 0, sizeof( struct espconn ) );
	}

	espconn_create(&espconnCommand);
	espconnCommand.type  = ESPCONN_TCP;
	espconnCommand.state = ESPCONN_NONE; 

	espconnCommand.proto.tcp = (esp_tcp*)os_malloc(sizeof(esp_tcp));
	espconnCommand.proto.tcp->local_port = iTCPPort;

	espconn_regist_connectcb(&espconnCommand, CommandConnectionEstablished);
	espconn_accept(&espconnCommand);
	espconn_regist_time(&espconnCommand, 120, 0);
}


//Handles a connection from a remote target on any port
void ICACHE_FLASH_ATTR
CommandConnectionEstablished(void* pArg)
{
	struct espconn * pConnection = (struct espconn*) pArg;
	espconn_regist_time(pConnection, 120, 0);
	os_printf("Connection established to command server\n\r");
	espconn_regist_recvcb(pConnection, CommandDataRecieved);
	espconn_regist_disconcb(pConnection, CommandConnectionClosed);
}
//Handles remote client disconnection
void ICACHE_FLASH_ATTR
CommandConnectionClosed(void* pArg)
{
	struct espconn * pConnection =  (struct espconn*) pArg;
	os_printf("Connection dropped from command server\n\r");
}

void ICACHE_FLASH_ATTR
CommandDataRecieved(void* arg, char* pData, uint16 iLength)
{
	struct espconn * pConnection = (struct espconn*) arg;
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
		espconn_sent(pConnection, rgcOutputMessage, strlen(rgcOutputMessage));
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
		espconn_sent(pConnection, "OK\n\r", 4);
		CFG_Save();
	}
	else if(cCommand == 'c' || cCommand == 'C')
	{
		sysCfg.cfg_holder = 0; //Forcibly reset all settings
		CFG_Save();
		CFG_Save();
	}
	else if(cCommand == 'a' || cCommand == 'A')
	{
		espconnClient = pConnection;
		if(!wifi_station_scan(0, ScanCallback))
			os_printf("Failed to start scan\n\r");
	}
	else if(cCommand == 'r' || cCommand == 'R')
	{
	}
}
