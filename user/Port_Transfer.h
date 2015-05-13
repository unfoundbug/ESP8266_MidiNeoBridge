#ifndef PORT_TRANSFER_H_
#define PORT_TRANSFER_H_
#include "user_config.h"
void ICACHE_FLASH_ATTR enableTransferServer(uint32 iTCPPort);
void ICACHE_FLASH_ATTR InitTransferServer(uint32 Port);
void ICACHE_FLASH_ATTR TransferConnectionEstablished(void* pArg);
void ICACHE_FLASH_ATTR TransferDataRecieved(void* pTarget, char* pData, unsigned short iDataLen);
void ICACHE_FLASH_ATTR TransferConnectionClosed(void* pArg);
char* ProcessMidi(char* pcNMidi);
#endif
