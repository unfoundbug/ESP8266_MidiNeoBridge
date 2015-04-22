#include "NonVol.h"
SYSCFG sysCfg;
SAVE_FLAG saveFlag;

void ICACHE_FLASH_ATTR
CFG_Save()
{
	 spi_flash_read((CFGLOC_Fl) * SPI_FLASH_SEC_SIZE,
	                   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));

	if (saveFlag.flag == 0) {
		spi_flash_erase_sector(CFGLOC_C1);
		spi_flash_write((CFGLOC_C1) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 1;
		spi_flash_erase_sector(CFGLOC_Fl);
		spi_flash_write((CFGLOC_Fl) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	} else {
		spi_flash_erase_sector(CFGLOC_C2);
		spi_flash_write((CFGLOC_C2) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 0;
		spi_flash_erase_sector(CFGLOC_Fl);
		spi_flash_write((CFGLOC_Fl) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	}
}
void ICACHE_FLASH_ATTR
CFG_Load()
{
	os_printf("About to read %d bytes at %d, prepping for %d\n\r", sizeof(SAVE_FLAG), (CFGLOC_Fl) * SPI_FLASH_SEC_SIZE, sizeof(SYSCFG));
	os_printf("Starting CFG Load\n\r");

	spi_flash_read((CFGLOC_Fl) * SPI_FLASH_SEC_SIZE,
				   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	if (saveFlag.flag == 0) {
		os_printf("About to read %d bytes at %d\n\r", sizeof(SYSCFG), (CFGLOC_C2) * SPI_FLASH_SEC_SIZE);
		spi_flash_read((CFGLOC_C2) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&sysCfg, sizeof(SYSCFG));
	} else {
		os_printf("About to read %d bytes at %d\n\r", sizeof(SYSCFG), (CFGLOC_C1) * SPI_FLASH_SEC_SIZE);
		spi_flash_read((CFGLOC_C1) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&sysCfg, sizeof(SYSCFG));
	}
	if(sysCfg.cfg_holder != CFG_HOLDER){
		os_printf("Loading default settings\n\r");
	
		os_memset(&sysCfg, 0x00, sizeof sysCfg);
	
	
		sysCfg.cfg_BaudRate = 115200; //magic number holder, if this doesn't match assume un-configured

		//Leave target station blank by default
		os_sprintf(sysCfg.localAP_ssid, "ESPStation_%02x", system_get_chip_id());
		os_sprintf(sysCfg.identifier, "ESP8266_%02x" , system_get_chip_id());
		//Default to no password
		os_printf("Station SSID set\n\r");
	
		//4, 268
		sysCfg.conbOutputMode = 0; //No output
		sysCfg.conTCPTimeout = 30; //30s delay
		os_printf("Default settings\n\r");
	
		sysCfg.cfg_holder = CFG_HOLDER;

		CFG_Save();
		os_printf("Saved to flash Pass 1\n\r");
		CFG_Save();
		os_printf("Saved to flash Pass 2\n\r");
	
	}

}
