#include "user_config.h"
#include "application.h"

//Tasks
#define user_procTaskPrio        0
#define user_procTaskQueueLen    2
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

//BulkTransfer socket
struct espconn espconnTransfer;

//UDP Broadcast host
struct espconn espconnBroadcast;

//CrossCallData
//Client Socket
uint8 pBuffer[128]; //Temporary buffer for structures

//Timers
os_timer_t tConnectionTimer;
os_timer_t tStatusTimer;

char rgcIpAddress[4];

//Called on a timers
static void ICACHE_FLASH_ATTR
StateEngine(os_event_t *events)
{
	struct ip_info addrInfo;
	uint32 uiCurrentTime = currentRunTime();
	switch (eCurrentLaunchState)
	{
		case STATE_STARTREMOTE:
		{
			os_printf("Starting connection to remote wifi\n\r");
			connectToRemoteAP();
			gi_RemoteStation_EndTime = uiCurrentTime + 30 + CONNECTION_TIMEOUT;
			eCurrentLaunchState = STATE_REMOTE_AWAITING;
			
		}break;
		case STATE_REMOTE_AWAITING:
		{
			addrInfo.ip.addr = 0;
			if(!wifi_get_ip_info( 0, &addrInfo))
			{
				os_printf("Unable to get IP details\n\r");
				return;
			}
			if(addrInfo.ip.addr != 0) //Connection
			{
				startServers();
				eCurrentLaunchState = STATE_REMOTE_CONNECTED;
				gi_RemoteStation_EndTime = uiCurrentTime + CONNECTION_TIMEOUT;
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
				else
					os_printf(".");
			}
		}break;
		case STATE_REMOTE_CONNECTED:
		{
			//If we have lost connection, restart
		}break;
		case STATE_STARTLOCAL:
		{
			os_printf("Starting local wifi host\n\r");
			setupLocalAP();
			startServers();
			eCurrentLaunchState = STATE_LOCAL_NOCOMMMS;
			gi_Local_AP_EndTime = uiCurrentTime + SETUP_TIMEOUT; //After 10 minutes reset if there we NO comms
		}break;
		case STATE_LOCAL_NOCOMMMS:
		{
			if(gi_Local_AP_EndTime < uiCurrentTime)
			{
				//scheduleCallTime(Reboot, 250);
			}
		}break;
		case STATE_LOCAL_COMMS:
		{
		}break;
	}
	if(eCurrentLaunchState == STATE_LOCAL_COMMS || eCurrentLaunchState == STATE_LOCAL_NOCOMMMS || eCurrentLaunchState == STATE_REMOTE_CONNECTED)
	{
		if(gi_Broadcast_NextTime < uiCurrentTime)
		{
			os_printf("Sending Broadcast\n\r");
			//Send broadcast
			gi_Broadcast_NextTime = uiCurrentTime + BROADCAST_TIMEOUT;
			
			if(!espconnBroadcast.proto.udp)
			{
				espconnBroadcast.proto.udp = (struct _esp_udp *) os_malloc(sizeof(struct _esp_udp));
				espconnBroadcast.type = ESPCONN_UDP;
				espconnBroadcast.proto.udp->remote_port = 8282;
				espconnBroadcast.proto.udp->remote_ip[0] = 255;
				espconnBroadcast.proto.udp->remote_ip[1] = 255;
				espconnBroadcast.proto.udp->remote_ip[2] = 255;
				espconnBroadcast.proto.udp->remote_ip[3] = 255;			
				
			}
			if(espconn_create(&espconnBroadcast))
			{
				os_printf("Created\n\r");
				char rgcBuffer[64];
				os_sprintf(rgcBuffer, "ESP_M%d_%s", sysCfg.conbOutputMode, sysCfg.identifier);
				espconn_sent(&espconnBroadcast, rgcBuffer, strlen(rgcBuffer));
				os_printf("Sent\n\r");
			}
			
		}
	}
}

static void Reboot()
{
	system_restart();
}

//WifiMode
int iRetryCount;
static void ICACHE_FLASH_ATTR
startServers()
{
	os_printf("Starting servers\n\r");
	
	InitCommandServer(8080);
	InitTransferServer(8081);
	InitHTTPServer();
	espconn_tcp_set_max_con(1);
	os_printf("Servers started\n\r");
	os_printf("Memory left: %d\n\r", system_get_free_heap_size());
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
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	//Enable UART at 115200 BAUD
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	ets_wdt_disable();
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
		
	//Setup GPIO and data buffer
	gpio_init();
	PIN_DIR_OUTPUT = 0x04;
	os_printf("GPIO Enabled\n\r");
	memset(&espconnBroadcast, 0, sizeof( struct espconn ) );
	PIN_OUT_CLEAR = 0x04; //Init NeoLine;
	ProcessNeo(0,0);
		
	os_timer_disarm(&tStatusTimer);
	os_timer_setfn(&tStatusTimer, (os_timer_func_t*) StateEngine, 0);
	os_timer_arm(&tStatusTimer, 250, true);
	os_printf("Timer set\n\r");
	os_printf("RTC calibration at %d\n\r", system_rtc_clock_cali_proc());
	eCurrentLaunchState = STATE_STARTREMOTE;
}
