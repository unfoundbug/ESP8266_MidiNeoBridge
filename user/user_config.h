//Function definitions
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "NonVol.h"

void user_init();

static void checkConnection();
static void setupLocalAP();
static void connectToRemoteAP();

static void enableTCPServer(uint32 iTCPPort, struct espconn* pConnection);
static void disableTCPServer(uint32 iTCPPort, struct espconn* pConnection);

static void handleConnectionEstablished(void* pArg);
static void handleRecievedData(void* arg, char* pData, unsigned short iDataLen);
static void handleConnectionDropped(void* pArg);

static void processCommand(struct espconn* pTarget, char* pData, uint16 iLength);
static void processTransfer(struct espconn* pTarget, char* pData, uint16 iLength);

static void Reboot();

static void loop(os_event_t *events);

#define pinHigh() GPIO_OUTPUT_SET(2,1); os_delay_us(29);
#define pinLow() GPIO_OUTPUT_SET(2,0); os_delay_us(29);

#define midiBit(bByte, oSet) if(bByte&oSet) {pinHigh();} else{ pinLow();}

#define scheduleCall(Function, sig, par) { system_os_task(Function, 1, user_procTaskQueue, user_procTaskQueueLen); system_os_post(1, sig, par); }
