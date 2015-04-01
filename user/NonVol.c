
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "mqtt.h"
#include "config.h"
#include "user_config.h"
#include "debug.h"

SYSCFG sysCfg;
SAVE_FLAG saveFlag;

void ICACHE_FLASH_ATTR
CFG_Save()
{
	 spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
	                   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));

	if (saveFlag.flag == 0) {
		spi_flash_erase_sector(CFG_LOCATION + 1);
		spi_flash_write((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 1;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	} else {
		spi_flash_erase_sector(CFG_LOCATION + 0);
		spi_flash_write((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 0;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	}
}

void ICACHE_FLASH_ATTR
CFG_Load()
{

	INFO("\r\nload ...\r\n");
	spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
				   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	if (saveFlag.flag == 0) {
		spi_flash_read((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&sysCfg, sizeof(SYSCFG));
	} else {
		spi_flash_read((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&sysCfg, sizeof(SYSCFG));
	}
	if(sysCfg.cfg_holder != CFG_HOLDER){
		os_memset(&sysCfg, 0x00, sizeof sysCfg);

	
		sysCfg.cfg_BaudRate = 115200; //magic number holder, if this doesn't match assume un-configured

		//Leave target station blank by default
		os_sprintf(localAP_ssid, "ESPStation_%02x", system_get_chip_id());
		//Default to no password
		
		//4, 268
		sysCfg.conbOutputMode = 0; //No output
		sysCfg.conTCPTimeout = 30; //30s delay
		
		sysCfg.cfg_holder = CFG_HOLDER;

		CFG_Save();
	}

}
