#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    2
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
	struct ip_info addrInfo;
	char outString[65];
	if(false)//wifi_get_ip_info( 1, &addrInfo))
	{
		os_sprintf(outString, "IP: %d.%d.%d.%d\n\r",  IP2STR(&addrInfo.ip.addr)); 
		os_printf(outString);

		os_sprintf(outString, "netmask: %d.%d.%d.%d\n\r", IP2STR(&addrInfo.netmask.addr));
		os_printf(outString);

		os_sprintf(outString, "gw: %d.%d.%d.%d\n\r\n\r",  IP2STR(&addrInfo.gw.addr));
		os_printf(outString);
	}
   // os_printf("Hello\n\r");
    os_delay_us(1000);
    system_os_post(user_procTaskPrio, 0, 0 );
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

	system_os_post(0,0,0);
}
//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	os_printf("SDK Version: %d.%d.%d/n/r", 1, 2, 3);

	//Start os task
	system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
	system_os_task(setupLocalAP, 1, user_procTaskQueue, user_procTaskQueueLen);
	system_os_post(1, 0, 0);
}
