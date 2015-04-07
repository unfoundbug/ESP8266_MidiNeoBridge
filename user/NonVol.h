
#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_
#include "os_type.h"
#include "user_config.h"

#define CFG_HOLDER	0x6A8D73A5	/* Random 32 bit integer, magic marker for structure validation */

#define CFGLOC_C1 40
#define CFGLOC_C2 41
#define CFGLOC_Fl 42



typedef struct{
	//4,4
	uint32_t cfg_holder; //magic number holder, if this doesn't match assume un-configured
	
	//256,260
	uint8_t station_ssid[64]; //station to connect to
	uint8_t station_pwd[64];
	uint8_t localAP_ssid[64]; //access point to host if no station
	uint8_t localAP_pwd[64];
	
	//4, 264
	uint32_t cfg_BaudRate;
	
	//4, 268
	uint8_t conbOutputMode;
	uint8_t conTCPTimeout;
	uint8_t bReserved[2];
	
} SYSCFG;

typedef struct {
    uint8 flag;
    uint8 pad[3];
} SAVE_FLAG;

void ICACHE_FLASH_ATTR CFG_Save();
void ICACHE_FLASH_ATTR CFG_Load();

extern SYSCFG sysCfg;

#endif /* USER_CONFIG_H_ */
