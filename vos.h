#ifndef __VOS_H__
#define __VOS_H__
#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
       extern "C" {
#endif

#ifndef U64
typedef unsigned long long  U64;
#endif

#define VOS_itemof(arr) (sizeof(arr) / sizeof((arr)[0]))
#define VOS_tcalloc(count, type) ((type *) calloc((count) * sizeof(type)))

#define	THREAD_STATE_INIT  0
#define	THREAD_STATE_RUN   1
#define	THREAD_STATE_EXIT  2
#define	THREAD_STATE_PAUSE 3
typedef struct{
	int pid;
	
	void *pThrd;
	volatile int ThrdState;		//NOTE: can NOT let flag defined here!
	
	int ThrdFifo;
	int ThrdIpcId;
		
	int ThrdSockFd;
	int ThrdSockTimerId;
	struct timeval ThrdSockTimeout;
	int ThrdSockHelloCount;
	int LinkStatus;		// 0:dis-connect, 1:connected.
}THREAD_S;

typedef struct{
	char ProgName[20];
	char ThrdInsDir[128];
	char ThrdName[128];
	char ThrdStartup[128];
}THRDCHK_S;


typedef void* (*pfnThread)(void* args);
#define MAX_STATCK_SIZE		(1024 * 1024)	// = 1MB

void* osCreateThread(int prior,pfnThread route,void* args);
void* osCreateThreadJoinable(int prior,pfnThread route,void* args);
int osJoinThread(void* thread);
void* osCreateSemaphore(char* name,int level);
int osTryWaitSemaphore(void* sem);
int osWaitSemaphore(void* sem);
int osPostSemaphore(void* sem);
int osDestroySemaphore(void* sem);
void* osMalloc(int size);
void osFree(void* mem);

/*
FUNC description: use to mount a share folder to linux local dir. using CIFS protocal.
INPUT:
OUTPUT:
	none.
RETURN: none.
NOTE: the mount point is fixed as /mnt/nas/
example:
*/
int osNetMountNAS(const char *user,const char *pwd,const char *smbsrv,const char *sharepath);
/*
FUNC description: use to check SD-card/NAS-disk mounted or not
INPUT:
OUTPUT:
RETURN: 0:not mounted, 1:mounted
NOTE: 
example:
	ret=osNetIsMounted("/mnt/sd1");
	ret=osNetIsMounted("/mnt/nas");
*/
int osNetIsMounted(char *MountPoint);

int osNetSyncNtpTime(const char *NtpServer);
int osNetFtpPut(char *filename);
/*
FUNC description: use to get netcard mac address
INPUT:
	const char *netcard,	//net card name like eth0
	const size_t nlen,		//net card neme length like 4
	char *mac			//mac buffer
OUTPUT:
	char *mac			//mac address (ipv4)
RETURN: 0:success, -N: error number
NOTE:get mac from driver not config file
example:
	char sMac[16]={0};
	ret=osNetGetMacAddr("eth0",4,sMac);
*/
int osNetGetMacAddr(const char *netcard,const size_t nlen,char *mac);
int osNetGetIP(char *ifname,char *ip);

//Func: transfer "YYYYMMDDHHmmss" to linux system time and take effect.
int osdigit2time(char *timeStr, int iLen, struct tm *ptTime);

int osFwNetUpdate(char* swUpdateUrl,int pid,char *wget);

//FUNC description: get current time for recording use, format is like:20150801101010
int osRecGetTime(char *localTime);

//create a sub-dir use YYYYMMDD
int osRecCreateDir(char *Dir,char *NewDir);

//FUNC description: get current time for osd use, format is like:2014-09-05 18:20:01
int osSYS_GetTime(char *localTime);

int osSystem(char *cmd);  //call system()

int osFileNew(char *filename,char *data,int len);
int osFileCopy(char *infile,char *outfile);

typedef void (*lpTimeoutFunc)(int sival);
int osTimerCreate(struct timeval *pTimeout,lpTimeoutFunc TimeoutFunc,int sival);
int osTimerDelete(int timerid);
int osTimerSettime(int timerid,struct timeval *pTimeout);

/*
FUNC description: use to get sd card capacity(volume)
INPUT:char * MountPoint	//sd card mounting point
OUTPUT:int *totalKiloBytes, int *freeKiloBytes	//unit:KB
RETURN: 0:success, -nnnn:errno
NOTE:
example:
*/
int osGetStorageInfo(char * MountPoint,int *totalKiloBytes, int *freeKiloBytes);

#ifndef _MD5_H_
#define _MD5_H_

#define MD5_DIGEST_SIZE		16
#define MD5_HMAC_BLOCK_SIZE	64
#define MD5_BLOCK_WORDS		16
#define MD5_HASH_WORDS		4

	typedef	struct
	{
		unsigned int hash[MD5_HASH_WORDS];
		unsigned int block[MD5_BLOCK_WORDS];
		unsigned long long byte_count;
	}MD5CTX_S;

	void md5_init(void *ctx);
	void md5_update(void *ctx, char *data,int len);
	void md5_final(void *ctx, unsigned char *out);
	void md5_show(unsigned char *sum);

#endif //_MD5_H_
void CRC16_CRCCCITT(const char* pDataIn, int iLenIn, unsigned short* pCRCOut);

//#include "auth.h"
#include "logfile.h"

#include "utils.h"

#include "libjson.h"

#include "netsock.h"

#include "reboot.h"

#include "http.h"

#include "msg.h"
#include "timer.h"
#include "cfgfile.h"
#include "ringbuf.h"


#define VOS_VERSION 4
int VOS_Init(char *LogDir,unsigned int MaxLogFileSize);
int VOS_UnInit(void);

#ifdef __cplusplus
}
#endif 

#endif

