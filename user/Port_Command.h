#ifndef PORT_COMMAND_H_
#define PORT_COMMAND_H_
#include "user_config.h"
void ICACHE_FLASH_ATTR enableCommandServer(uint32 iTCPPort);
void ICACHE_FLASH_ATTR InitCommandServer(uint32 port);
void ICACHE_FLASH_ATTR CommandConnectionEstablished(void* pArg);
void ICACHE_FLASH_ATTR CommandDataRecieved(void* arg, char* pData, unsigned short iDataLen);
void ICACHE_FLASH_ATTR CommandConnectionClosed(void* pArg);


#endif
