#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <semaphore.h>
#include <pthread.h>
#include <memory.h>

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <linux/if_ether.h>
#include <net/if.h>

#include "timer.h"

extern int Log_MsgLine(char *LogFileName,char *sLine);
#if 1
#define 	TimerPrint(fmt,args...)  do{ \
			char _PrtBuf[1000]; \
			sprintf(_PrtBuf,":" fmt , ## args); \
			Log_MsgLine("timer.log",_PrtBuf); \
			}while(0)
#else
#define 	TimerPrint(fmt,args...)  
#endif

static pthread_mutex_t  gTMThrdLock;
static TIMER_S gTimerCtrlTable[MAX_TIMER_NUM];

static int  s_iMonthFix[]   = {2, 0, 2, 1, 2, 1, 2, 2, 1, 2, 1, 2}; 
/* date -s "2017-02-09 17:14:11"
Func: tranform to linux systime.
*/
static int _digit2time(char *timeStr, int iLen, struct tm *ptTime)
{ 
	int ret=0;
	char *pChar = timeStr;  
	char sCmd[128]={0};

	memset(ptTime, 0, sizeof(*ptTime));  

	if ( (iLen != 14)||(!timeStr)||(!ptTime) )
	{  
		return -1;
	}  

	ptTime->tm_year = (pChar[0] - '0') * 1000 + (pChar[1] - '0') * 100 +   
		(pChar[2] - '0') * 10 + (pChar[3] - '0');  
	ptTime->tm_mon  = (pChar[4] - '0') * 10 + (pChar[5] - '0');  
	ptTime->tm_mday = (pChar[6] - '0') * 10 + (pChar[7] - '0');  
	ptTime->tm_hour = (pChar[8] - '0') * 10 + (pChar[9] - '0');  
	ptTime->tm_min  = (pChar[10] - '0') * 10 + (pChar[11] - '0');  
	ptTime->tm_sec  = (pChar[12] - '0') * 10 + (pChar[13] - '0');  
	if (ptTime->tm_year >= 1900 && ptTime->tm_year < 2036 &&  
		ptTime->tm_mon < 13 &&   
		ptTime->tm_mday <= 29 + s_iMonthFix[ptTime->tm_mon - 1] &&  
		ptTime->tm_hour < 24 && ptTime->tm_min < 60 && ptTime->tm_sec < 60)  
	{  
		ptTime->tm_year -= 1900;
		ptTime->tm_mon -= 1;
		ret=0;
	}
	else{
		ret= -2;
	}

	TimerPrint("Translated done. %04d-%02d-%02d %02d:%02d:%02d\n",ptTime->tm_year,ptTime->tm_mon
		,ptTime->tm_mday,ptTime->tm_hour,ptTime->tm_min,ptTime->tm_sec);
	
	return ret;
}  


int TM_NewDeltaTimer(int mode,int TimeoutSec,fpTimerFunc TimeoutFunc,int sival)
{  
    int ret=0;
	int i=0;
	int id=-1;

	pthread_mutex_lock(&gTMThrdLock);
	for (i=0; i<MAX_TIMER_NUM; i++){
		if (gTimerCtrlTable[i].flag==0)
			break;
	}
	if (i==MAX_TIMER_NUM){
		ret=-1;
		goto End;
	}
	id=i;
	
    timer_t timerid=(timer_t)(-1);  
    sigevent_t evp;  
    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_signo = SIGALRM; 
    evp.sigev_value.sival_int = sival;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = (void*)TimeoutFunc;
    evp.sigev_notify_attributes =NULL;
  
    if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)  
    {  
       ret=-2;goto End;  
    }
    if (timerid<0)
    {  
       ret=-3;goto End;  
    }

    struct itimerspec it;  
    it.it_value.tv_sec = TimeoutSec;  
    it.it_value.tv_nsec = 0;
	if (mode==TIMER_INTERVAL){
    	it.it_interval.tv_sec = TimeoutSec;
	    it.it_interval.tv_nsec = 0; 
	}
	else{
    	it.it_interval.tv_sec = 0;
	    it.it_interval.tv_nsec = 0; 
	}
    if (timer_settime(timerid, 0, &it, NULL) == -1)  
    {  
        ret=-4;goto End;  
    }

	gTimerCtrlTable[id].timerId=(int)timerid;
//	gTimerCtrlTable[id].timeout.tv_sec=TimeoutSec;
//	gTimerCtrlTable[id].timeout.tv_sec=0;
	gTimerCtrlTable[id].TimeoutSec=TimeoutSec;
	gTimerCtrlTable[id].TimeoutFunc=(void*)TimeoutFunc;
	gTimerCtrlTable[id].args=sival;
	gTimerCtrlTable[id].flag=1;
	ret=id;

	TimerPrint("create Timer.TimerId=%d,timeout=%ds\n"
			,gTimerCtrlTable[id].timerId,gTimerCtrlTable[id].TimeoutSec);
End:
	pthread_mutex_unlock(&gTMThrdLock);
    TimerPrint("%s(interval=%ds),ret=%d\n",__FUNCTION__,TimeoutSec,ret);
    return ret;
}  

int TM_NewAbsTimer(TIMER_USER_S *pTimerPara)
{  
    int ret=0;
	int i=0;
	int idx=-1;

	//get idle-timerid
	pthread_mutex_lock(&gTMThrdLock);
	for (i=0; i<MAX_TIMER_NUM; i++){
		if (gTimerCtrlTable[i].flag==0)
			break;
	}
	if (i==MAX_TIMER_NUM){
		ret=-1;
		goto End;
	}
	idx=i;

	//calc D-value between expired-time and present-time.
	time_t preTime,expTime;
	time(&preTime);
	
	struct tm ExpiredTime;
	ret=_digit2time(pTimerPara->ChkTimeStr,strlen(pTimerPara->ChkTimeStr),&ExpiredTime);
	if (!ret)
		expTime=mktime(&ExpiredTime);
	else{
		TimerPrint("TimeStr has wrong format!ret=%d\n",ret);
		ret=-2;goto End;
	}

	int diffSec=0;
	diffSec=expTime-preTime;
	if (diffSec<=0){
		TimerPrint("expTime[%d] - preTime[%d]=%d\n",expTime,preTime,diffSec);
		ret=-3;goto End;
	}
	
    timer_t timerid=(timer_t)(-1);
    sigevent_t evp;  
    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_signo = SIGALRM; 
    evp.sigev_value.sival_int = pTimerPara->args;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = (void*)pTimerPara->TimeoutFunc;
    evp.sigev_notify_attributes =NULL;
  
    if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)  
    {  
       ret=-4;goto End; 
    }
    if (timerid<0)
    {  
       ret=-5;goto End;  
    }

    struct itimerspec it;  
    it.it_value.tv_sec = diffSec;  
    it.it_value.tv_nsec = 0;
	if (pTimerPara->timerMode==TIMER_INTERVAL){
    	it.it_interval.tv_sec = 3600;
	    it.it_interval.tv_nsec = 0;
	}
	else{
    	it.it_interval.tv_sec = 0;
	    it.it_interval.tv_nsec = 0; 
	}
	
    if (timer_settime(timerid, 0, &it, NULL) == -1)  
    {  
        ret=-6;goto End;  
    }  
  
	gTimerCtrlTable[idx].timerMode=pTimerPara->timerMode;
	strcpy(gTimerCtrlTable[idx].ChkTimeStr,pTimerPara->ChkTimeStr);
	gTimerCtrlTable[idx].TimeoutFunc=pTimerPara->TimeoutFunc;
	gTimerCtrlTable[idx].args=pTimerPara->args;
	gTimerCtrlTable[idx].TimeoutSec=diffSec;
//	gTimerCtrlTable[idx].timeout.tv_sec=diffSec;
//	gTimerCtrlTable[idx].timeout.tv_sec=0;

	gTimerCtrlTable[idx].timerId=(int)timerid;
	gTimerCtrlTable[idx].flag=1;
	ret=idx;

	TimerPrint("create Timer.TimerId=%d,timeout=%ds\n"
			,gTimerCtrlTable[idx].timerId,gTimerCtrlTable[idx].TimeoutSec);
	
End:
	pthread_mutex_unlock(&gTMThrdLock);
    TimerPrint("%s(expiredtime=%s),ret=%d\n",__FUNCTION__,pTimerPara->ChkTimeStr,ret);
    return ret;  
}  


int TM_Delete(int idx)
{
	int ret=0;
	if ((idx<0)||(idx>=MAX_TIMER_NUM)){
		return -1;
	}
	
	pthread_mutex_lock(&gTMThrdLock);
	gTimerCtrlTable[idx].flag=0;
	ret=timer_delete((timer_t)gTimerCtrlTable[idx].timerId);
	pthread_mutex_unlock(&gTMThrdLock);

	TimerPrint("%s(idx=%d),ret=%d\n",__FUNCTION__,idx,ret);	
	return ret;
}

int TM_Reset(int idx)
{  
    int ret=0;
    struct itimerspec it;  

	if ((idx<0)||(idx>=MAX_TIMER_NUM)){
		return -1;
	}

	pthread_mutex_lock(&gTMThrdLock);

	if (gTimerCtrlTable[idx].timerMode==TIMER_ONCE){
	    it.it_value.tv_sec = gTimerCtrlTable[idx].TimeoutSec;  
		it.it_value.tv_nsec = 0;
	   	it.it_interval.tv_sec = 0;
    	it.it_interval.tv_nsec = 0; 
    	ret=timer_settime((timer_t)(gTimerCtrlTable[idx].timerId), 0, &it, NULL);
		TimerPrint("reset onceTimer.TimerId=%d,timeout=%ds\n"
			,gTimerCtrlTable[idx].timerId,gTimerCtrlTable[idx].TimeoutSec);
	}
	pthread_mutex_unlock(&gTMThrdLock);

	TimerPrint("%s(idx=%d),ret=%d\n",__FUNCTION__,idx,ret);
    return ret;  
}  

int TM_Init(void)
{
	TimerPrint("timer version=%d,buildtime=%s\n",TM_VERSION,BNO);
	pthread_mutex_init(&gTMThrdLock, NULL);

	return 0;
}

int TM_UnInit(void)
{
	pthread_mutex_destroy(&gTMThrdLock);
	return 0;	
}


