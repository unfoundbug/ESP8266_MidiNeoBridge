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

static void UpdateRemoteAPDetails();
static void UpdateLocalAPDetails();

static void Reboot();

static void loop(os_event_t *events);

#define pinHigh() GPIO_OUTPUT_SET(2,1);
#define pinLow() GPIO_OUTPUT_SET(2,0);

#define pinBit(bByte, oSet) if(bByte&oSet) {pinHigh();} else{ pinLow();}

#define midiBit(bByte, oSet) pinBit(bByte, oSet); os_delay_us(29);
#define sendMidiByte(bByte)	pinLow(); midiBit(bByte, 0x01); midiBit(bByte, 0x02); midiBit(bByte, 0x04); midiBit(bByte, 0x08); \
									  midiBit(bByte, 0x10); midiBit(bByte, 0x20); midiBit(bByte, 0x40); midiBit(bByte, 0x80); \
							pinHigh();

#define neoBit(bByte, oSet) pinHigh(); \
								\
								if(bByte&oSet)\
								{}\
							pinLow();
#define neoByte(byte)     neoBit(bByte, 0x01); neoBit(bByte, 0x02); neoBit(bByte, 0x04); neoBit(bByte, 0x08); \
						  neoBit(bByte, 0x10); neoBit(bByte, 0x20); neoBit(bByte, 0x40); neoBit(bByte, 0x80);
						  
#define scheduleCall(Function, sig, par) { system_os_task(Function, 1, user_procTaskQueue, user_procTaskQueueLen); system_os_post(1, sig, par); }

os_timer_t tScheduler;
#define scheduleCallTime(Function, time) { 	os_timer_disarm(&tScheduler);	os_timer_setfn(&tScheduler, (os_timer_func_t*) Function, 0);	os_timer_arm(&tScheduler, time, true);}

//Gets the current run time in S
#define currentRunTime() { return (system_get_rtc_time()/(1000/system_rtc_clock_cali_proc());}

uint32 gi_RemoteStation_EndTime;
uint32 gi_Local_AP_EndTime;

enum {
	STATE_STARTREMOTE,
	STATE_REMOTE_AWAITING,
	STATE_REMOTE_CONNECTED,
	STATE_STARTLOCAL,
	STATE_LOCAL_NOCOMMMS,
	STATE_LOCAL_COMMS
} eCurrentLaunchState;

