#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "user_config.h"
#include "NonVol.h"
#include "Port_Transfer.h"
#include "Port_Command.h"

void user_init();

static void startServers();
static void setupLocalAP();
static void connectToRemoteAP();

static void UpdateRemoteAPDetails();
static void UpdateLocalAPDetails();

static void Reboot();

static void StateEngine(os_event_t *events);

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
#define currentRunTime() (system_get_time()/ (uint32_t)1000000)

//Time to wait before giving up on the remote AP
#define CONNECTION_TIMEOUT 15
//Time to host the local AP to accept changes before retrying the remote
#define SETUP_TIMEOUT 60

//Time between Broadcast Attempts
#define BROADCAST_TIMEOUT 15

uint32 gi_RemoteStation_EndTime;
uint32 gi_Local_AP_EndTime;
uint32 gi_Broadcast_NextTime;

enum {
	STATE_STARTREMOTE,
	STATE_REMOTE_AWAITING,
	STATE_REMOTE_CONNECTED,
	STATE_STARTLOCAL,
	STATE_LOCAL_NOCOMMMS,
	STATE_LOCAL_COMMS
} eCurrentLaunchState;

#endif //APPLICATION_H_
