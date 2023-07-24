#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include <sys/time.h>
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

#include "cfgfile.h"
#include "utils.h"
#include "auth.h"

/*
return: 0:valid, -n:expired
*/
static int gBaseDays=0,gLostDays=0;
int Auth_VerifyDateAuth(char *statFile,int expireDays)
{
	int ret=0;
	int diff=0;
	long goingDays=0;
	int oneday=(24*3600);
	
#if 0
    struct timeval tv;
    gettimeofday(&tv,NULL);
	int nowDays=tv.tv_sec/oneday;
#else
	time_t timep;  
	struct tm *p;  
	time(&timep);  
	p=localtime(&timep);  
	timep = mktime(p);
	int nowDays=timep/oneday;
#endif

	if (gBaseDays<=0){
		gBaseDays=nowDays;	//init
		Cfg_ReadInt(statFile,"stat", "days",&goingDays);
	}

	diff=nowDays-gBaseDays;
	if (diff<0)
		gBaseDays=nowDays;	//re-init
	if (diff>gLostDays){
		gLostDays=diff;	//keep it as last value
		
		Cfg_WriteInt(statFile,"stat", "days",gLostDays+goingDays);
		if ((gLostDays + goingDays)>expireDays){
			ret=-1;	//days due
		}
	}

	return ret;
}


int Auth_VerifyMacAuth(char *ethName,char *ethAddr)
{
	int ret=0;
	char sMac[32]={0};
	
	ret=utl_NetGetMacAddr(ethName,strlen(ethName),sMac);
	if (!ret){
		if (!strcmp(sMac,ethAddr))
			ret=1;	//pass
		else
			ret=-1;
	}

	return ret;
}

int Auth_ForkAuthTask(void)
{
	int ret=-1;
	int pid=-1;
	MSG_S MsgBuf;

	pid=fork();
	if (pid==0){
		while(1){
			ret=Auth_VerifyMacAuth(USER_SERVER_ETHNAME,USER_SERVER_ETHMAC);
			if (ret != 1){
				MsgBuf.mtype=0x50;
				MsgBuf.mBuf[0]=-33;
				FifoSend(RtxpRxFIFO,&MsgBuf,8);
			}

			ret=Auth_VerifyDateAuth(EXPIRE_DAYS_COUNT_FILE,MAX_EXPIRE_DAYS);
			if (ret == -1){
				MsgBuf.mtype=0x51;
				MsgBuf.mBuf[0]=-34;
				FifoSend(RtxpRxFIFO,&MsgBuf,8);
			}

			sleep(30);
		}
	}
	return pid;
}

int Auth_RunIndependentTask(int ParentPid)
{
	int ret=-1;
	int pid=-1;

	pid=fork();
	if (pid==0){
		while(1){
			ret=Auth_VerifyMacAuth(USER_SERVER_ETHNAME,USER_SERVER_ETHMAC);
			if (ret != 1){
				ret=utl_KillForced(ParentPid);
			}

			ret=Auth_VerifyDateAuth(EXPIRE_DAYS_COUNT_FILE,MAX_EXPIRE_DAYS);
			if (ret == -1){
				ret=utl_KillForced(ParentPid);
			}

			sleep(30);
		}
	}
	return pid;
}


