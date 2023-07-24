#ifdef __cplusplus
       extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <signal.h>

#include <fcntl.h>
#include <malloc.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <semaphore.h>
#include <pthread.h>
#include <memory.h>
#include <dirent.h> 
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/statfs.h>
#include <sys/msg.h>
#include <sys/ipc.h>
//#include <sys/signal.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <linux/if_ether.h>
#include <net/if.h>

#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/rtc.h>

#include "vos.h"

extern int Log_MsgLine(char *LogFileName,char *sLine);

#if 1
#define 	VosPrint(fmt,args...)  do{ \
			char _PrtBuf[1000]; \
			sprintf(_PrtBuf,":" fmt , ## args); \
			Log_MsgLine("vos.log",_PrtBuf); \
			}while(0)
#else
#define 	VosPrint(fmt,args...)  
#endif


void osInit()
{
	//srand(GetTickCount());
	//signal(SIGPIPE,SIG_IGN);

}

/*
NOTE: some embeded platform does not support pthread_attr_t operation func.
     
*/
void* osCreateThread(int prior,pfnThread route,void* args)
{
	pthread_t* ret=NULL;
	//pthread_attr_t attr;
	//struct   sched_param   param; 
	
	ret = (pthread_t*)malloc(sizeof(pthread_t));
	if(ret)
	{
		//pthread_attr_init(&attr);
		//pthread_attr_setstacksize(&attr,MAX_STATCK_SIZE);
		//pthread_attr_setschedpolicy(&attr,   SCHED_RR);  
		//param.sched_priority   =   prior;
		//pthread_attr_setschedparam(&attr,&param);  
		
		if(0 == pthread_create(ret,NULL,route,args))
		{
			//pthread_attr_destroy(&attr);
			return (void*)ret;
		}
		//pthread_attr_destroy(&attr);
	}
	return NULL;
}


void* osCreateThreadJoinable(int prior,pfnThread route,void* args)
{
	pthread_t* ret=NULL;
	pthread_attr_t attr;
	struct   sched_param   param; 
	
	ret = (pthread_t*)malloc(sizeof(pthread_t));
	if(ret)
	{
		pthread_attr_init(&attr);
		//pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
		pthread_attr_setstacksize(&attr,MAX_STATCK_SIZE);
		pthread_attr_setschedpolicy(&attr,   SCHED_RR);  
		param.sched_priority   =   prior;
		pthread_attr_setschedparam(&attr,&param);  
		
		if(0 == pthread_create(ret,NULL,route,args))
		{
			pthread_attr_destroy(&attr);
			return (void*)ret;
		}
		pthread_attr_destroy(&attr);
	}
	return NULL;
}


/*
pthread_join 编辑
函数pthread_join用来等待一个线程的结束,线程间同步的操作。头文件 ： #include <pthread.h>
函数定义： int pthread_join(pthread_t thread, void **retval);
描述 ：pthread_join()函数，以阻塞的方式等待thread指定的线程结束。当函数返回时，被等待线程的资源被收回
。如果线程已经结束，那么该函数会立即返回。并且thread指定的线程必须是joinable的。
参数 ：thread: 线程标识符，即线程ID，标识唯一线程。retval: 用户定义的指针，用来存储被等待线程的返回值。
返回值 ： 0代表成功。 失败，返回的则是错误号。
可以通过pthread_join()函数来使主线程阻塞等待其他线程退出，这样主线程可以清理其他线程的环境。
但是还有一些线程，更喜欢自己来清理退出的状态，他们也不愿意主线程调用pthread_join来等待他们
。我们将这一类线程的属性称为detached。如果我们在调用pthread_create()
函数的时候将属性设置为NULL，则表明我们希望所创建的线程采用默认的属性，也就是joinable。
*/
int osJoinThread(void* thread)
{
	if (thread)
	{
		pthread_t tid = *((pthread_t*)thread);
		pthread_join(tid,NULL);
		free(thread);
	}
	return 0;
}

void* osCreateSemaphore(char* name,int level)
{
	sem_t* ret;
	ret = (sem_t*)malloc(sizeof(sem_t) + strlen(name));
	if(ret)
	{
		sem_init(ret,0,level);
	}
	return ret;
}

int osTryWaitSemaphore(void* sem)
{
	if (sem)
		return sem_trywait((sem_t*)sem);
	else
		return 0;
}

int osWaitSemaphore(void* sem)
{
	if (sem)
		return sem_wait((sem_t*)sem);
	else
		return 0;
}

int osPostSemaphore(void* sem)
{
	if (sem)
		return sem_post((sem_t*)sem);
	else
		return 0;
}

int osDestroySemaphore(void* sem)
{
	if (sem){
		sem_destroy((sem_t*)sem);
		free(sem);
	}
	return 0;
}

void* osMalloc(int size)
{
	void *p=NULL;
	
	if(size & 0xfff)	// min size=4KB
	{
		size += 4096 - (size & 0xfff);
	}
	p=calloc(size,1);

	return p;
}

void osFree(void* mem)
{
	if (mem)
		free(mem);
}

int osRecGetTime(char *localTime)
{
	time_t now;
	struct tm *timenow;
	
	time(&now);
	timenow   =   localtime(&now);
	sprintf(localTime,"%04d%02d%02d%02d%02d%02d"
		,timenow->tm_year+1900,timenow->tm_mon+1,timenow->tm_mday
		,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);

	return 0;
}

int osRecCreateDir(char *Dir,char *NewDir)
{
	int ret=0;
	
	time_t now;
	struct tm *timenow;
	char sCmd[128]={0};
	char LocalDir[128]={0};

	if ( (!Dir)||(!NewDir) )
		return -1;
	
	time(&now);
	timenow   =   localtime(&now);
	sprintf(LocalDir,"%s%04d%02d%02d",Dir,
		timenow->tm_year+1900,timenow->tm_mon+1,timenow->tm_mday);

	ret=access(LocalDir, F_OK);
	if (ret!=0){	//no exist
		sprintf(sCmd,"mkdir %s",LocalDir);
		ret=osSystem(sCmd);
		if (!ret)
			strcpy(NewDir,LocalDir);
	}
	else
		strcpy(NewDir,LocalDir);

	//VosPrint("%s(%s),ret=%d\n",__FUNCTION__,Dir,ret);
	return ret;
}


int osSYS_GetTime(char *localTime)
{
	time_t now;
	struct tm *timenow;
	
	time(&now);
	timenow   =   localtime(&now);
	sprintf(localTime,"%04dd-%02d-%02d %02d:%02d:%02d",timenow->tm_year+1900,timenow->tm_mon+1
		,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);

	return 0;
}

int osNetMountNAS(const char *user,const char *pwd,const char *smbsrv,const char *sharepath)
{
	int ret=0;
	char buf[256]={0};
	char *mountpoint="/mnt/nas/";

	if(user&&pwd&&smbsrv&&sharepath&&mountpoint)
	{
		strcat(buf,"mount -t cifs -o ");
		strcat(buf,"username=");
		strcat(buf, user);
		strcat(buf,",");
		strcat(buf,"password=");
		strcat(buf,pwd);
		strcat(buf," //");
		strcat(buf,smbsrv);
		if(sharepath[0]!='/')
		{
			strcat(buf,"/");
		}
		strcat(buf,sharepath);
		strcat(buf," ");
		strcat(buf,mountpoint);

		ret=system(buf);
	}

	return ret;
}


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
int osNetIsMounted(char *MountPoint)
{
	int ret=0;
	char sCmd[128]={0};
	int fd=-1;
	char buf[256]={0};
	char *p=NULL;

	if (!MountPoint)
		return -1;
	
	//MountPoint[strlen(MountPoint)-1]='\0';
	sprintf(sCmd,"/bin/mount |/bin/grep %s >/tmp/mount.txt 2>/dev/null",MountPoint);
	osSystem(sCmd);
	usleep(300000);	//wait 200ms

	fd=open("/tmp/mount.txt",O_RDONLY);
	if (fd>=0)
	{
		ret=read(fd,buf,sizeof(buf));
		if (ret>0)
			p=strstr(buf,MountPoint);
		
		close(fd);
	}
	
	if (p!=NULL)
		ret=1;
	else
		ret=0;

	VosPrint("%s(%s),ret=%d\n",__FUNCTION__,MountPoint,ret);
	return ret;
}


/*
FUNC description: use to get netcard mac address without ':'
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
int osNetGetMacAddr(const char *netcard,const size_t nlen,char *mac)
{
	int ret=-1;
	int sock=-1;
	struct ifreq ifr;
	unsigned char	ClientHwAddr[ETH_ALEN]={0};
	
	if(netcard&&mac)
	{
		memset(&ifr,0,sizeof(struct ifreq));
		memcpy(ifr.ifr_name,netcard,nlen);

		sock= socket(AF_PACKET,SOCK_PACKET,htons(ETH_P_ALL));
		if(sock)
		{
			if ( ioctl(sock,SIOCGIFHWADDR,&ifr)==0)
			{
				//memcpy((unsigned char*)mac,ifr.ifr_hwaddr.sa_data,ETH_ALEN);
				
				memcpy(ClientHwAddr,ifr.ifr_hwaddr.sa_data,ETH_ALEN);
				sprintf(mac,"%02x%02x%02x%02x%02x%02x",ClientHwAddr[0], ClientHwAddr[1], ClientHwAddr[2],
					ClientHwAddr[3], ClientHwAddr[4], ClientHwAddr[5]);
				
				ret=0;
			}
			else
			{
				ret=-3;
			}
			
			close(sock);sock=-1;
		}
		else
		{
			ret=-2;
		}
	}
	else
	{
		ret=-1;
	}

	VosPrint("%s(%s),ret=%d,mac=%s\n",__FUNCTION__,netcard,ret,mac);
	return ret;
}

/*
FUNC description: use to get IP address
INPUT:
	char *ifname,		//"eth0" or "wlan0" or "ra0"
	char *ip			//pointer to get
OUTPUT:
	char *ip			//net card ip
RETURN: 0:success, -N: error number
NOTE: 
example:
	ret=osNetGetIP("eth0",&ip);
*/
int osNetGetIP(char *ifname,char *ip)
{
	int ret=0;

	if ((!ifname)||(!ip))
		return -1;
	
	static const int families[] = 
	{
	AF_INET, AF_IPX, AF_AX25, AF_APPLETALK
	};
	unsigned int	i;	
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;
	
	//sock = socket(AF_INET, SOCK_DGRAM, 0);
	/*
	* Now pick any (exisiting) useful socket family for generic queries
	* Note : don't open all the socket, only returns when one matches,
	* all protocols might not be valid.
	* Workaround by Jim Kaba <jkaba@sarnoff.com>
	* Note : in 99% of the case, we will just open the inet_sock.
	* The remaining 1% case are not fully correct...
	*/

	// Try all families we support
	for(i = 0; i < sizeof(families)/sizeof(int); ++i)
	{
		sock = socket(families[i], SOCK_DGRAM, 0);
		if(sock >= 0)
			break;
	}
	
	if (sock<0)
	{
		ret= -1;
		goto End;
	}
	
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		ret=-2;
		goto End;
	}

	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	memcpy(ip,inet_ntoa(sin.sin_addr),strlen(inet_ntoa(sin.sin_addr)));

End:
	if (sock>=0)
		close(sock);
	VosPrint("%s(%s),ret=%d,ip=%s\n",__FUNCTION__,ifname,ret,ip);
	return ret;
}


static unsigned long kscale(unsigned long b, unsigned long bs)
{
	return (b * (unsigned long long) bs + 1024/2) / 1024;
}

/*
FUNC description: use to get sd card capacity(volume)
INPUT:char * MountPoint	//sd card mounting point
OUTPUT:int *totalKiloBytes, int *freeKiloBytes	//unit:KB
RETURN: 0:success, -nnnn:errno
NOTE:
example:
*/
int osGetStorageInfo(char * MountPoint,int *totalKiloBytes, int *freeKiloBytes)
{
	int ret=0;
	struct statfs statFS;

	if ((!MountPoint)||(!totalKiloBytes)||(!freeKiloBytes))
		return -1;
	
	if (statfs(MountPoint, &statFS) == -1)
	{
		printf("statfs failed for path->[%s]\n", MountPoint);
		ret=-2;
		goto End;
	}

	*totalKiloBytes =kscale(statFS.f_blocks, statFS.f_bsize); //(statFS.f_blocks * statFS.f_frsize)/1024;
	*freeKiloBytes = kscale(statFS.f_bavail, statFS.f_bsize); //(statFS.f_bfree * statFS.f_frsize)/1024;

End:
	VosPrint("%s(%s,1K-blocks=%9lu,Available=%9lu),ret=%d\n",__FUNCTION__,MountPoint
		,(long)*totalKiloBytes,(long)*freeKiloBytes,ret);
	return ret;
}

int osFileNew(char *filename,char *data,int len)
{
	int ret=0;
	int wd=-1;
	
	if ((!filename)||(!data))
		return -1;

	wd=open(filename,O_CREAT|O_RDWR|O_TRUNC);
	if (wd<0){
		ret=-3;goto End;
	}

	ret=write(wd,data,len);
	if(len!=ret)
	{
		ret=-4;
		goto End;
	}

End:
	if (wd>=0)
		close(wd);
	VosPrint("%s(%s),ret=%d\n",__FUNCTION__,filename,ret);
	return ret;
}

int osFileCopy(char *infile,char *outfile)
{
	int ret=0;
	int rd=-1;
	int wd=-1;
	struct stat st;
	char buf[0x1000]={0};
	int len=0;
	int size=0;
	
	if ((!infile)||(!outfile))
		return -1;

	rd=open(infile,O_RDONLY);
	if (rd<0){
		ret=-2;goto End;
	}
	wd=open(outfile,O_CREAT|O_RDWR);
	if (wd<0){
		ret=-3;goto End;
	}

	memset(&st,0,sizeof(st));
	ret=fstat(rd,&st);
	if (ret<0){
		ret=-4;goto End;
	}

	size=0;
	while(1)
	{
		len=read(rd,buf,0x1000);
		if(len<=0)
			break;
		
		ret=write(wd,buf,len);
		if(len!=ret)
		{
			ret=-10;
			break;
		}

		size+=len;
	}

	if (st.st_size!=size)
		ret=-5;
	else
		ret=0;
	
End:
	if (rd>=0)
		close(rd);
	if (wd>=0)
		close(wd);
	VosPrint("%s(%s,%s),ret=%d\n",__FUNCTION__,infile,outfile,ret);
	return ret;
}

int osSystem(char *cmd)  //call system()
{
    int ret=0;
    int status;  
  
    status = system(cmd);  

    if (-1 == status)  
    {  
        perror("system error!");
	 ret=-1;goto End;
    }  
    else  
    {  
        //printf("exit status value = [0x%x],exit code=[0x%x]\n", status,WEXITSTATUS(status));  
  
        if (WIFEXITED(status))  
        {  
            if (WEXITSTATUS(status) >=0)  
            {  
                //printf("run shell script successfully.\n");  
                ret=WEXITSTATUS(status);
            }  
            else  
            {  
                printf("run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
		  ret=-2;
            }  
        }  
        else  
        {  
            printf("exit status = [%d]\n", WEXITSTATUS(status));
	     ret=-3;
        }  
    }  

End:
    //VosPrint("%s(%s),ret=%d[0x%x],exit status=%d\n",__FUNCTION__,cmd,ret,ret,WEXITSTATUS(status));
    return ret;  
}  


/* date -s "2017-02-09 17:14:11"
Func: tranform to linux systime.
*/
int osdigit2time(char *timeStr, int iLen, struct tm *ptTime)  
{ 
	int ret=0;
	char *pChar = timeStr;  
//	char sCmd[128]={0};
	int  s_iMonthFix[]   = {2, 0, 2, 1, 2, 1, 2, 2, 1, 2, 1, 2};
	memset(ptTime, 0, sizeof(*ptTime));  

	if ( (iLen != 14)||(!timeStr)||(!ptTime) )
	{  
		ret=-1;goto End;
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
		ret=0;

		//sprintf(sCmd,"date -s \"%04dd-%02d-%02d %02d:%02d:%02d\"",ptTime->tm_year+1900,ptTime->tm_mon+1
		//	,ptTime->tm_mday,ptTime->tm_hour,ptTime->tm_min,ptTime->tm_sec);
		//ret=system(sCmd);
	}
	else{
		ret= -2;
	}

End:
	VosPrint("%s(%s),ret=%d\n",__FUNCTION__,timeStr,ret);
	return ret;
}  

/*
Func: wget -i download.txt
return: 0: burn flash success! -nnnn: error code
Note: this func will not call reboot

*/
#if 1
int osFwNetUpdate(char* swUpdateUrl,int pid,char *wget)
{
	int ret=0;
	char sCmd[256]={0};
	char *filename=NULL;
	char *postfix=NULL;
	char name[32]={0};
	char TmpDir[64]="/mnt/mtd/ipc/tmpfs";

	ret=access(TmpDir, F_OK);
	if(ret != 0){
		memset(TmpDir,0,sizeof(TmpDir));
		strcpy(TmpDir,"/tmp");
	}
	
	sprintf(sCmd,"mkdir %s/fw;cd %s/fw;rm %s/fw/* -rf;%s -c %s"
		,TmpDir,TmpDir,TmpDir,wget,swUpdateUrl);
	system(sCmd);
	filename=strrchr(swUpdateUrl,'/')+1;
	if (!filename){
		ret=-1;goto End;
	}
	sprintf(sCmd,"%s/fw/%s",TmpDir,filename);
	if(access(sCmd,R_OK) != 0){
		ret=-2;goto End;
	}

	postfix=strchr(filename,'.');
	strncpy(name,filename,postfix-filename);

	if (pid>0)
		ret=utl_KillForced(pid);

	sleep(1);
	sprintf(sCmd,"cd %s/fw;tar xf %s",TmpDir,filename);ret=system(sCmd);
	sleep(1);
	sprintf(sCmd,"cd %s/fw/%s/;sh appins.sh",TmpDir,name);ret=system(sCmd);
	sleep(1);
	sprintf(sCmd,"rm %s/fw -rf",TmpDir);ret=system(sCmd);

	ret=1;

End:
	VosPrint("%s(%s,pid=%d),ret=%d\n",__FUNCTION__,swUpdateUrl,pid,ret);
	return ret;
}
#else
int osFwNetUpdate(char* swUpdateUrl,int pid,char *wget)
{
	int ret=0;
	char sCmd[256]={0};
	char *filename=NULL;
	char *postfix=NULL;
	char name[32]={0};
	
	sprintf(sCmd,"cd /tmp;%s -c %s",wget,swUpdateUrl);system(sCmd);
	
	if (pid>0)
		ret=utl_KillForced(pid);
	
	filename=strrchr(swUpdateUrl,'/')+1;
	if (!filename){
		ret=-1;goto End;
	}
	sprintf(sCmd,"/tmp/fw/%s",filename);
	if(access(sCmd,R_OK) != 0){
		ret=-2;goto End;
	}

	postfix=strchr(filename,'.');
	strncpy(name,filename,postfix-filename);


	sleep(1);
	sprintf(sCmd,"cd /tmp;tar xf %s",filename);ret=system(sCmd);

	sleep(1);
	sprintf(sCmd,"sh /tmp/fw/%s/install.sh",name);ret=system(sCmd);
	sleep(1);
	sprintf(sCmd,"rm /tmp/fw -rf");ret=system(sCmd);

	sleep(1);

	system("reboot");system("/sbin/reboot");

End:
	VosPrint("%s(%s,pid=%d),ret=%d\n",__FUNCTION__,swUpdateUrl,pid,ret);
	return ret;
}

#endif

/*
Func: once timer only
*/
int osTimerCreate(struct timeval *pTimeout,lpTimeoutFunc TimeoutFunc,int sival)
{  
    int ret=0;

    // XXX int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);  
    // clockid--值：CLOCK_REALTIME,CLOCK_MONOTONIC，CLOCK_PROCESS_CPUTIME_ID,CLOCK_THREAD_CPUTIME_ID  
    // evp--存放环境值的地址,结构成员说明了定时器到期的通知方式和处理方式等  
    // timerid--定时器标识符  
    timer_t timerid=(timer_t)(-1);  
    sigevent_t evp;  
    memset(&evp, 0, sizeof(struct sigevent));       //清零初始化  

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
    // XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);  
    // timerid--定时器标识  
    // flags--0表示相对时间，1表示绝对时间  
    // new_value--定时器的新初始值和间隔，如下面的it  
    // old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值  
    struct itimerspec it;  
    it.it_value.tv_sec = pTimeout->tv_sec;  
    it.it_value.tv_nsec = pTimeout->tv_usec*1000;  
    it.it_interval.tv_sec = 0;  //once timer
    it.it_interval.tv_nsec = 0;    
    if (timer_settime(timerid, 0, &it, NULL) == -1)  
    {  
        ret=-4;goto End;  
    }  
  
    ret=(int)timerid;
End:
    VosPrint("%s(),ret=%d\n",__FUNCTION__,ret);
    return ret;  
}  

int osTimerDelete(int timerid)
{
	int ret=0;
	
	ret=timer_delete((timer_t)timerid);

	VosPrint("%s(),ret=%d\n",__FUNCTION__,ret);
	return ret;
}

int osTimerSettime(int timerid,struct timeval *pTimeout)
{  
    int ret=0;
    struct itimerspec it;  

    it.it_value.tv_sec = pTimeout->tv_sec;  
    it.it_value.tv_nsec = pTimeout->tv_usec*1000;  
    it.it_interval.tv_sec = 0;  //once timer
    it.it_interval.tv_nsec = 0;    
    ret=timer_settime((timer_t)timerid, 0, &it, NULL); 

    return ret;  
}  

#ifndef	__MD5_C__
#define	__MD5_C__

/*
 * md5 function from linux kernel
 */
//#include <stdio.h>
//#include <string.h>
#include <endian.h>
//#include "md5.h"

static inline void swab(unsigned char *buf,int size)
{
	unsigned char t;
	int i;
	int pos;

	for(i=0;i<size/2;i++)
	{
		pos=size-i-1;
		t=buf[i];
		buf[i]=buf[pos];
		buf[pos]=t;
	}

	return;
}

#define BYTEORDER_HAS_U64
#if __BYTE_ORDER == __LITTLE_ENDIAN
	#define __le32_to_cpus(x) do { (void)(x); } while (0)
	#define __cpu_to_le32s(x) do { (void)(x); } while (0)
#else
	#define __le32_to_cpus(x) swab(x,4)
	#define __cpu_to_le32s(x) swab(x,4)
#endif

#define F1(x, y, z)	(z ^ (x & (y ^ z)))
#define F2(x, y, z)	F1(z, x, y)
#define F3(x, y, z)	(x ^ y ^ z)
#define F4(x, y, z)	(y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, in, s) \
	(w += f(x, y, z) + in, w = (w<<s | w>>(32-s)) + x)


static void md5_transform(unsigned int *hash, unsigned int const *in)
{
	unsigned int a, b, c, d;

	a = hash[0];
	b = hash[1];
	c = hash[2];
	d = hash[3];

	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	hash[0] += a;
	hash[1] += b;
	hash[2] += c;
	hash[3] += d;
}

/* XXX: this stuff can be optimized */
static inline void le32_to_cpu_array(unsigned int *buf, unsigned int words)
{
	while (words--) {
		__le32_to_cpus(buf);
		buf++;
	}
}

static inline void cpu_to_le32_array(unsigned int *buf, unsigned int words)
{
	while (words--) {
		__cpu_to_le32s(buf);
		buf++;
	}
}

static inline void md5_transform_helper(MD5CTX_S *ctx)
{
	le32_to_cpu_array(ctx->block, sizeof(ctx->block) / sizeof(unsigned int));
	md5_transform(ctx->hash, ctx->block);
}

void md5_init(void *ctx)
{
	MD5CTX_S *mctx = (MD5CTX_S *)ctx;

	mctx->hash[0] = 0x67452301;
	mctx->hash[1] = 0xefcdab89;
	mctx->hash[2] = 0x98badcfe;
	mctx->hash[3] = 0x10325476;
	mctx->byte_count = 0;
}

void md5_update(void *ctx,char *data, int len)
{
	MD5CTX_S *mctx = (MD5CTX_S *)ctx;
	const unsigned int avail = sizeof(mctx->block) - (mctx->byte_count & 0x3f);

	mctx->byte_count += len;

	if (avail > len) {
		memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
		       data, len);
		return;
	}

	memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
	       data, avail);

	md5_transform_helper(mctx);
	data += avail;
	len -= avail;

	while (len >= sizeof(mctx->block)) {
		memcpy(mctx->block, data, sizeof(mctx->block));
		md5_transform_helper(mctx);
		data += sizeof(mctx->block);
		len -= sizeof(mctx->block);
	}

	memcpy(mctx->block, data, len);
}

void md5_final(void *ctx, unsigned char *out)
{
	MD5CTX_S *mctx = (MD5CTX_S *)ctx;
	const unsigned int offset = mctx->byte_count & 0x3f;
	char *p = (char *)mctx->block + offset;
	int padding = 56 - (offset + 1);

	*p++ = 0x80;
	if (padding < 0) {
		memset(p, 0x00, padding + sizeof (unsigned long long));
		md5_transform_helper(mctx);
		p = (char *)mctx->block;
		padding = 56;
	}

	memset(p, 0, padding);
	mctx->block[14] = mctx->byte_count << 3;
	mctx->block[15] = mctx->byte_count >> 29;
	le32_to_cpu_array(mctx->block, (sizeof(mctx->block) -
	                  sizeof(unsigned long long)) / sizeof(unsigned int));
	md5_transform(mctx->hash, mctx->block);
	cpu_to_le32_array(mctx->hash, sizeof(mctx->hash) / sizeof(unsigned int));
	memcpy(out, mctx->hash, sizeof(mctx->hash));
	memset(mctx, 0, sizeof(*mctx));
}

void md5_show(unsigned char *sum)
{
	int i;

	if(!sum)
		return;

	printf("md5sum:\t");
	for(i=0;i<16;i++)
		printf("%02X",sum[i]);
	printf("\n");

	return;
}

#endif		// __MD5_C__

//////////////////////////////////////////////////////////////////////////     
// 函数功能: CRC16效验(CCITT的0XFFFF效验)     
// 输入参数: pDataIn: 数据地址     
//           iLenIn: 数据长度                
// 输出参数: pCRCOut: 2字节校验值       
void CRC16_CRCCCITT(const char* pDataIn, int iLenIn, unsigned short* pCRCOut)     
{   
	int i=0,j=0;
    unsigned short wTemp = 0;      
    unsigned short wCRC = 0xffff;      
    
    for(i=0; i < iLenIn; i++)      
    {             
        for(j = 0; j < 8; j++)      
        {      
            wTemp = ((pDataIn[i] << j) & 0x80 ) ^ ((wCRC & 0x8000) >> 8);      
    
            wCRC <<= 1;      
    
            if(wTemp != 0)       
            {     
                wCRC ^= 0x1021;      
            }     
        }      
    }      
    
    *pCRCOut = wCRC;     
}  


//#include "auth.c"

#include "logfile.c"

#include "utils.c"

#include "libjson.c"

#include "netsock.c"

#include "reboot.c"

#include "http.c"

#include "msg.c"
#include "timer.c"
#include "cfgfile.c"
#include "ringbuf.c"

// do not support embeded-arm
//#include "RedisClt.c"

#if 0
void lib_init(void) __attribute__((constructor));
void lib_fini(void) __attribute__((destructor));

void lib_init(void)
{
	int ret=0;

	osInit();
	VosPrint("-----------libvos.so load,ret=%d--------------\n",ret);
	
}

void lib_fini(void)
{
	int ret=0;

	VosPrint("-----------libvos.so unload,ret=%d--------------\n",ret);
}
#endif

//pthread_mutex_t  gNetThrdLock;
//#define MAX_THRDLOCK_NUM 10
//pthread_mutex_t  gThrdLock[MAX_THRDLOCK_NUM];

int VOS_Init(char *LogDir,unsigned int MaxLogFileSize)
{
	osInit();
	
	//CFG_Init();
	MSG_Init();
	Log_Init(LogDir,MaxLogFileSize);
	
//	pthread_mutex_init(&gNetThrdLock, NULL);
//	for (i=0; i<MAX_THRDLOCK_NUM; i++)
//		pthread_mutex_init(&gThrdLock, NULL);
	
	VosPrint("%s: vos version=%d,buildtime=%s\n",__FUNCTION__,VOS_VERSION,BNO);
//	printf("%s: vos version=%d,buildtime=%s\n",__FUNCTION__,VOS_VERSION,BNO);
	return 0;
}

int VOS_UnInit(void)
{
	//CFG_UnInit();
	MSG_UnInit();
	
//	for (i=0; i<MAX_THRDLOCK_NUM; i++)
//		pthread_mutex_destroy(&gThrdLock, NULL);

//	pthread_mutex_destroy(&gNetThrdLock);
	VosPrint("%s: vos version=%d,buildtime=%s\n",__FUNCTION__,VOS_VERSION,BNO);
	//printf("%s: vos version=%d,buildtime=%s\n",__FUNCTION__,VOS_VERSION,BNO);
	return 0;
}


#ifdef __cplusplus
}
#endif 

