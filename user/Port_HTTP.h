#ifndef PORT_HTTP_H_
#define PORT_HTTP_H_
#include "user_config.h"
void ICACHE_FLASH_ATTR enableHTTPServer(uint32 iTCPPort);
void ICACHE_FLASH_ATTR InitHTTPServer();
void ICACHE_FLASH_ATTR HTTPConnectionEstablished(void* pArg);
void ICACHE_FLASH_ATTR HTTPDataRecieved(void* arg, char* pData, unsigned short iDataLen);
void ICACHE_FLASH_ATTR HTTPConnectionClosed(void* pArg);


#endif
