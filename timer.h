#ifndef __TIMER_H__
#define __TIMER_H__
#include <sys/time.h>
#include <time.h>

#ifdef __cplusplus
       extern "C" {
#endif

#define TIMER_ONCE 		0
#define TIMER_INTERVAL 	1

typedef void (*fpTimerFunc)(int sival);
typedef struct{
	int flag;		// 0:free, 1:used
	int timerId;	//timer id

	int timerMode;	// 0:once, 1:repeat
	char ChkTimeStr[32];	//expire time or due time in string format.
//	struct tm ChkTimeVal;	//expire time or due time in unix time format.
//	struct timeval timeout;	//D-value , interval=(expiretime - presenttime)
	int TimeoutSec;
	fpTimerFunc TimeoutFunc;
	int args;	//something like gpio_val
}TIMER_S;
#define MAX_TIMER_NUM 100
typedef struct{
	int timerMode;	// 0:once, 1:repeat
	char ChkTimeStr[32];	//expire time or due time in string format.
	fpTimerFunc TimeoutFunc;
	int args;	//something like gpio_val
}TIMER_USER_S;

int TM_NewDeltaTimer(int mode,int TimeoutSec,fpTimerFunc TimeoutFunc,int sival);
int TM_NewAbsTimer(TIMER_USER_S *pTimerPara);
int TM_Delete(int idx);
int TM_Reset(int idx);

#define TM_VERSION 2
int TM_Init(void);
int TM_UnInit(void);

#ifdef __cplusplus
}
#endif 

#endif
