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
#include <getopt.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "utils.h"

#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif

#if 1
#define 	UtilPrint(fmt,args...)  do{ \
			char _PrtBuf[1000]; \
			sprintf(_PrtBuf,":" fmt , ## args); \
			Log_MsgLine("util.log",_PrtBuf); \
			}while(0)
#else
#define 	UtilPrint(fmt,args...)  
#endif

int FifoOpen(char *fifo_name,int fifo_mode)
{
	int ret=0;
	int Fifo_fd=-1;

	if (!fifo_name)
		return -1;

	if(access(fifo_name, F_OK) == -1){
		ret = mkfifo(fifo_name, 0777);
		if (ret<0)	{
			printf("create fifo:%s error:%s[errno=%d]\n",fifo_name,strerror(errno),errno);
			ret=-2;
			goto End;
		}
	}
	
	Fifo_fd=open(fifo_name, fifo_mode);
	if (Fifo_fd<0){
		ret=-3;
		goto End;
	}	

	ret=Fifo_fd;
End:
	UtilPrint("%s(%s,%d),ret=%d\n",__FUNCTION__,fifo_name,fifo_mode,ret);
	return ret;
}

int FifoClose(int fifo_fd)
{
	int ret=0;

	if (fifo_fd>=0)
		ret=close(fifo_fd);

	UtilPrint("%s(%d),ret=%d\n",__FUNCTION__,fifo_fd,ret);
	return ret;
}

int FifoRevMsg(int fifo_fd,MSG_S *MsgBuf,unsigned int MsgBodyLen)
{
	int ret=0;
	int len=MsgBodyLen;

	if (len>=PIPE_BUF){
		ret= -1;
		goto End;
	}

	MsgBuf->mtype=0xFFFF;
	ret=read(fifo_fd,(char *)MsgBuf,len);
	if (ret<=0){
		ret=-4;
		goto End;
	}

End:
//	if (ret>0)
//		UtilPrint("%s(%d,0x%lx,),ret=%d\n",__FUNCTION__,fifo_fd,MsgBuf->mtype,ret);
//	else if (ret<0)
//		UtilPrint("%s(%d),ret=%d\n",__FUNCTION__,fifo_fd,ret);
	
	return ret;
}

int FifoSend(char *fifo_name,MSG_S *MsgBuf,unsigned int MsgBodyLen)
{
	int ret=0;
	int Fifo_fd=-1;
	int len=MsgBodyLen+8;
	//int fifo_mode=FIFO_WRITE_ASYNC;
	int fifo_mode=FIFO_WRITE_SYNC;

	if ( (!fifo_name)||(!MsgBuf) )
		return -1;

	if(access(fifo_name, F_OK) == -1){
		ret = mkfifo(fifo_name, 0777);
		if (ret<0)	{
			printf("create fifo:%s error:%s[errno=%d]\n",fifo_name,strerror(errno),errno);
			ret=-2;
			goto End;
		}
	}
	
	Fifo_fd=open(fifo_name, fifo_mode);
	if (Fifo_fd<0){
		ret=-3;
		goto End;
	}	

	if (len>=PIPE_BUF){
		ret= -4;
		goto End;
	}

	int cnt=0;
	char *p=(char *)&MsgBuf->mtype;

	while(len>0)
	{
		ret=write(Fifo_fd,p,len);
		if(ret<1)
			break;
		cnt+=ret;
		p+=ret;
		len-=ret;
	} //while

	ret=cnt;

//	ret=write(Fifo_fd,(char *)&MsgBuf->mtype,len);

End:
	//UtilPrint("%s(%s,0x%lx,len=%d),ret=%d\n",__FUNCTION__,fifo_name,MsgBuf->mtype,MsgBodyLen,ret);
	if (Fifo_fd>=0)
		close(Fifo_fd);
	return ret;
}

int utl_KillForced(int pid)
{
	int i=0;
	
    if (pid <= 0) {
        return -1;
    }
    
    // first, try kill by SIGTERM.
    if (kill(pid, SIGTERM) < 0) {
        return -2;
    }
    
    // wait to quit.
    for (i = 0; i < 100; i++) {
        int status = 0;
        pid_t qpid = -1;
        if ((qpid = waitpid(pid, &status, WNOHANG)) < 0) {
            return -3;
        }
        
        // 0 is not quit yet.
        if (qpid == 0) {
            usleep(10 * 1000);
            continue;
        }
        
        // killed, set pid to -1.
        printf("SIGTERM stop process pid=%d done.\n", pid);
        
        return 0;
    }

    // then, try kill by SIGKILL.
    if (kill(pid, SIGKILL) < 0) {
        return -5;
    }
    
    // wait for the process to quit.
    // for example, ffmpeg will gracefully quit if signal is:
    //         1) SIGHUP     2) SIGINT     3) SIGQUIT
    // other signals, directly exit(123), for example:
    //        9) SIGKILL    15) SIGTERM
    int status = 0;
    // @remark when we use SIGKILL to kill process, it must be killed,
    //      so we always wait it to quit by infinite loop.
    while (waitpid(pid, &status, 0) < 0) {
        usleep(10 * 1000);
        continue;
    }
    
    printf("SIGKILL stop process pid=%d done.\n", pid);
    
    return 1;
}


unsigned long utl_FileGetSize(const char *filename)
{
	struct stat buf;

	if(stat(filename, &buf)<0)
	{
		return 0;
	}
	return (unsigned long)buf.st_size;
}

int utl_DoesExist(char *cmd, char *key)
{
	int ret=0;
	char sCmd[128]={0};
	int fd=-1;
	char buf[256]={0};
	char *p=NULL;
	int cnt=0;

	sprintf(sCmd,"%s |/bin/grep %s >/tmp/.tmp 2>/dev/null",cmd,key);
	system(sCmd);
	usleep(300000);	//wait a while

	fd=open("/tmp/.tmp",O_RDONLY);
	if (fd>=0)
	{
		ret=read(fd,buf,sizeof(buf));
		if (ret>0){
			p=strstr(buf,key);
			while (p){
				cnt++;
				p+=strlen(key);
				if (!p)
					break;
				p=strstr(p,key);
			}
		}
		
		close(fd);
	}
	//system("rm /tmp/.tmp -rf 2>/dev/null");
	
	ret=cnt;
	return ret;
}

int utl_TimeGetStamp(char *TimeStamp)
{
	time_t now;
	struct tm *timenow;
	
	time(&now);
	timenow   = localtime(&now);
	sprintf(TimeStamp,"%04d-%02d-%02d %02d:%02d:%02d",timenow->tm_year+1900,timenow->tm_mon+1
		,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);

	return 0;
}

int utl_IsInDuration(char *pBeginTime,char *pEndTime)
{
	int ret=0;
	struct tm *pLocalTime=NULL;
	time_t timep;
	SETTIME_S BeginTime,EndTime;
	char *p1=NULL,*p2=NULL,str[32]={0};

	if ((!pBeginTime)||(!pEndTime))
		return -1;

	p1=pBeginTime;
	p2=strchr(p1,':');	// "hh:mm"
	strncpy(str,p1,p2-p1);
	BeginTime.hh=atoi(str);
	BeginTime.mm=atoi(p2+1);
	
	p1=pEndTime;
	p2=strchr(p1,':');	// "hh:mm"
	strncpy(str,p1,p2-p1);
	EndTime.hh=atoi(str);
	EndTime.mm=atoi(p2+1);

	time(&timep);		//get current time
	pLocalTime=localtime(&timep);
	
	if  ( (((pLocalTime->tm_hour==BeginTime.hh)&&(pLocalTime->tm_min>=BeginTime.mm))
			||(pLocalTime->tm_hour>BeginTime.hh))
		&&
		 (((pLocalTime->tm_hour==EndTime.hh)&&(pLocalTime->tm_min<EndTime.mm))
			||(pLocalTime->tm_hour<EndTime.hh)) 
		)
	{
		ret=1;
	}

	UtilPrint("%s([%s,%s]),%02d:%02d to %02d:%02d ,ret=%d\n",__FUNCTION__,pBeginTime,pEndTime
		,BeginTime.hh,BeginTime.mm,EndTime.hh,EndTime.mm
		,ret);
	return ret;
}

//"ftp://admin:ipc123@120.77.206.22:10029"
int utl_NetSetFtpSvr(char *iFtpUrl,FTPSVR_S *oFtpConf)
{
	int ret=0;
	char *p1=NULL,*p2=NULL,str[32]={0};
	
	if (iFtpUrl&&oFtpConf){
		p1=strchr(iFtpUrl,'/');
		p2=strchr(p1,':');
		strncpy(str,p1+2,p2-p1-2);
		strcpy(oFtpConf->user,str);

		p1=p2;
		p2=strchr(p1,'@');
		if ((p1+1)!=NULL)
			strncpy(str,p1+1,p2-p1-1);
		strcpy(oFtpConf->passwd,str);

		p1=p2;
		p2=strchr(p1,':');
		if ((p1+1)!=NULL)
			strncpy(str,p1+1,p2-p1-1);
		strcpy(oFtpConf->server,str);

		oFtpConf->port=atoi(p2+1);
	}

	UtilPrint("%s(%s),%s:%s@%s:%d\n",__FUNCTION__,iFtpUrl
		,oFtpConf->user,oFtpConf->passwd,oFtpConf->server,oFtpConf->port);
	
	return ret;
}

int utl_TokenCreate(char *str,unsigned char *md5,char *ssum)
{
	MD5CTX_S ctx;
	time_t seconds;
	unsigned char sum[16]={0};
	
	md5_init(&ctx);
	seconds = time(NULL); 

	md5_update(&ctx,str,strlen(str));
	md5_update(&ctx,(char*)&seconds,sizeof(seconds));

	md5_final(&ctx,sum);

	if (md5)
		memcpy(md5,sum,sizeof(sum));
	if (ssum){
		sprintf(ssum
			,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
			,sum[0],sum[1],sum[2],sum[3],sum[4],sum[5],sum[6]
			,sum[7],sum[8],sum[9],sum[10],sum[11],sum[12],sum[13]
			,sum[14],sum[15]);
	}

	return 0;
}

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
	char sMac[32]={0};
	ret=utl_NetGetMacAddr("eth0",4,sMac);
*/
int utl_NetGetMacAddr(const char *netcard,const size_t nlen,char *mac)
{
	int ret=0;
	int sock=-1;
	struct ifreq ifr;
	unsigned char	ClientHwAddr[ETH_ALEN]={0};
	
	if(netcard&&mac)
	{
		memset(&ifr,0,sizeof(struct ifreq));
		memcpy(ifr.ifr_name,netcard,nlen);

		//sock= socket(AF_PACKET,SOCK_PACKET,htons(ETH_P_ALL));
		sock=socket(AF_INET,SOCK_STREAM,0);
		if(sock){
			if (ioctl(sock,SIOCGIFHWADDR,&ifr)==0){
				memcpy(ClientHwAddr,ifr.ifr_hwaddr.sa_data,ETH_ALEN);
				sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",ClientHwAddr[0], ClientHwAddr[1], ClientHwAddr[2],
				ClientHwAddr[3], ClientHwAddr[4], ClientHwAddr[5]);
				ret=0;
			}
			else{
				ret=-3;
			}
			
			close(sock);sock=-1;
		}
		else{
			ret=-2;
		}
	}
	else{
		ret=-1;
	}

	return ret;
}

int utl_NewDir(char *dir)
{
	int ret=0;
	char sCmd[128]={0};

	ret=access(dir, F_OK);
	if(ret != 0){
		sprintf(sCmd,"mkdir %s 2>/dev/null",dir);
		system(sCmd);
		ret=access(dir, F_OK);
	}

	return ret;
}

int utl_SetupDateAsDir(char *TopDir)
{
	int ret=0;
	time_t now;
	struct tm *timenow;
	char subdir[256]={0};
	int i=0;
	
	time(&now);
	timenow   = localtime(&now);

	sprintf(subdir,"%s/%04d",TopDir,timenow->tm_year+1900);
	ret=utl_NewDir(subdir);
	if (!ret){
		for (i=0; i<12; i++){
			sprintf(subdir,"%s/%04d/%02d",TopDir,timenow->tm_year+1900,timenow->tm_mon+1+i);
			ret=utl_NewDir(subdir);
		}
	}
	
	return ret;
}
int utl_LogMyPid(char *pidfile)
{
	int ret=0;
	pid_t  pid;
	
	pid=getpid();
	ret=Cfg_WriteInt(pidfile,"pid","pid",pid);

	return ret;
}
int utl_GetMyPid(char *pidfile)
{
	int pid=-1;

	Cfg_ReadInt(pidfile,"pid","pid",(int *)&pid, -1);

	return pid;
}

void utl_CalcCRC16(unsigned char *p, int len, unsigned char *LByte, unsigned char *HByte)
{
	unsigned char CRC16Lo, CRC16Hi;
	unsigned char CL, CH;
	unsigned char SaveHi, SaveLo;
	int i, j;
	
	CRC16Lo = 0xFF;
	CRC16Hi = 0xFF;
	CL = 0x01; CH = 0xA0; //多项式码0xA001
	for(i = 0; i<len; ++i)
	{
		CRC16Lo = CRC16Lo^(p[i]); //每一个数据与CRC寄存器进行异或
		for(j = 0; j < 8; ++j)
		{
			SaveHi = CRC16Hi;
			SaveLo = CRC16Lo;
			CRC16Hi = (CRC16Hi>>1); //高位右移一位
			CRC16Lo = (CRC16Lo>>1); //低位右移一位
			if ((SaveHi & 0x01) == 0x01) //如果高位字节最后一位为1
			{
				CRC16Lo = CRC16Lo | 0x80; //则低位字节右移后前面补1否则自动补0
			}
			if ((SaveLo & 0x01) == 0x01) //如果LSB为1，则与多项式码进行异或
			{
				CRC16Hi = CRC16Hi^CH;
				CRC16Lo = CRC16Lo^CL;
			}
		}
	}
	*HByte = CRC16Hi;
	*LByte = CRC16Lo;
}


#include "libjson.h"
int JsonFindIndex(char *JsonName,char **NameArray)
{
	int pos=-1;
	int i=0;

	for (i=0; i<MAX_JSON_TOKEN; i++)
	{
		if (!NameArray[i])
			break;
		
		if (!strcmp(JsonName,NameArray[i])){
			pos=i;
			break;
		}
	}
	
	return pos;
}

int JsonParseText(char *MsgBuf,char **pName,char **pValue)
{
	int i=-1;
	char *out;cJSON *json;

	json=cJSON_Parse(MsgBuf);
	if (!json) {printf("Error before: [%s]\n",cJSON_GetErrorPtr());}
	else
	{
		out=cJSON_Print(json);
		printf("%s\n",out);
		i=0;
		while (json!=NULL){
			if (((json->type)&&0xFF)!=cJSON_String){
				printf("Json message error!\n");
				break;
			}
			pName[i]=strdup(json->string);
			pValue[i]=strdup(json->valuestring);
			
			json=json->next;
			i++;
		}
		
		cJSON_Delete(json);
		free(out);
	}

	return i;
}

int JsonCreateObjects(char **pName,char **pValue,int TokenNum,char *MsgBuf)
{
	int ret=0;
	cJSON *root;int i;
	char *out=NULL;

	root=cJSON_CreateObject();
	for (i=0; i<TokenNum; i++)
		cJSON_AddItemToObject(root, pName[i], cJSON_CreateString(pValue[i]));

	out=cJSON_Print(root);	
	cJSON_Delete(root);	
	if (out){
		ret=strlen(out);
		//assert(ret<1024);	//max length
		if (ret>1000)
			ret=1000;
		strncpy(MsgBuf,out,ret);
		free(out);
	}
	return ret;
}

static struct if_ipv4_info *if_info = NULL;
 
/*
** add ip to a linklist
*/
static int  _if_add_to_link (struct ifaddrs *ifap, char *buf, char *raw_buf)
{
  struct if_ipv4_info *p, *q;
 
  if (if_info == NULL) {  
    if_info = malloc (sizeof (struct if_ipv4_info));
    /* init head */
    if_info->next = NULL;
    if_info->ip_addr[0] = '\0';
    if_info->hd_addr[0] = '\0';
 
    if (if_info == NULL) {
      fprintf (stderr, "malloc failed\n");
      return -1;
    }
    strcpy (if_info->if_name, ifap->ifa_name);
    switch (ifap->ifa_addr->sa_family) {
      case AF_INET:
        strcpy (if_info->ip_addr, buf);
     //   printf ("head %s", if_info->ip_addr);
        break;
      case AF_PACKET:
	  	strcpy(if_info->raw_hd_addr, raw_buf);
        strcpy (if_info->hd_addr, buf);
      //  printf ("head %s", if_info->hd_addr);
        break;
      default:
        break;
    } //switch
  }else {
    for (p = if_info; p != NULL; p = p->next) {
      q = p;
      if (strcmp (p->if_name, ifap->ifa_name) == 0) {
        break;
      }
    }
    if (p != NULL) {
      switch (ifap->ifa_addr->sa_family) {
        case AF_INET:
          strcpy (p->ip_addr, buf);
       //   printf ("p %s\n", p->ip_addr);
          break;
        case AF_PACKET:
		  strcpy(p->raw_hd_addr, raw_buf);
          strcpy (p->hd_addr, buf);
        //  printf ("p %s\n", p->hd_addr);
          break;
        default:
          break;
      } //switch
    } else {
      p = malloc (sizeof (struct if_ipv4_info));
      /* init node */
      p->next = NULL;
      p->ip_addr[0] = '\0';
      p->hd_addr[0] = '\0';
 
      if (p == NULL) {
        fprintf (stderr, "malloc failled");
        return -1;
      }
      strcpy (p->if_name, ifap->ifa_name);
      switch (ifap->ifa_addr->sa_family) {
        case AF_INET:
          strcpy (p->ip_addr, buf);
         // printf ( "else p %s\n", p->ip_addr);
          break;
        case AF_PACKET:
		  strcpy(p->raw_hd_addr, raw_buf);
          strcpy (p->hd_addr, buf);
          //printf ( "else p %s\n", p->hd_addr);
          break;
        default:
          break;
    } 
      q->next = p;
  }//else
  }
  return 0;
}
 
static int _if_prt_if_info (struct sockaddr *ifa_addr, char *buf, char *raw_buf)
{
  struct sockaddr_ll *s;
  int i;
  int len;
  int family = ifa_addr->sa_family;
 
  int ret = 0;
 
  switch (family) {
    case AF_INET: 
      inet_ntop (ifa_addr->sa_family, &((struct sockaddr_in *)ifa_addr)->sin_addr,
                  buf, sizeof (struct sockaddr_in));
      break;
    case AF_PACKET:
      s = (struct sockaddr_ll *)ifa_addr;
      for (i=0,len=0; i<6; i++) {
	  	sprintf (raw_buf+(i<<1), "%02x", s->sll_addr[i]);
        len += sprintf (buf+len, "%02x%s", s->sll_addr[i], i<5?":":"");
      }
	  //printf("raw_buf=%s,buf=%s\n",raw_buf,buf);
      break;
    default:
      ret = -1;
      break;
  }
    return ret;
}
 
static int _if_get_if_info (const char *if_name)
{
  struct ifaddrs *ifa, *ifap;
  int family;  /* protocl family */
 
  if (getifaddrs (&ifa) == -1) {
    perror (" getifaddrs\n");
    return -1;
  }
  char buf[20]={0},raw_buf[20]={0};
 
  for (ifap = ifa; ifap != NULL; ifap = ifap->ifa_next) {
    if (strcmp (ifap->ifa_name, "lo") == 0) {
      continue; /* skip the lookback card */
    }
 
    if (ifap->ifa_addr == NULL)
      continue; /* if addr is NULL, this must be no ip address */

    if (!(strcmp (ifap->ifa_name, if_name) == 0)) {
      continue;
    }
 
    if (_if_prt_if_info (ifap->ifa_addr, buf, raw_buf) == 0) {
	  
      _if_add_to_link (ifap, buf, raw_buf);
    }
  }
  
  freeifaddrs (ifa);
  return 0;
}
 
 
int utl_IfGetInfo(const char *if_name,char *ip,char *mac,char *raw_mac)
{
	int ret=0;
  
	ret=_if_get_if_info(if_name);

  
  /* printf all of them */
  struct if_ipv4_info *p = NULL;
//  for (p = if_info; p != NULL; p = p->next) {
//    printf ("%s: %s\t%s\n", p->if_name, strlen(p->ip_addr)>0?p->ip_addr: "No ip addr", p->hd_addr);
//  }

	if (!ret){
		if (mac)
			strcpy(mac, if_info->hd_addr);
		if (ip)
			strcpy(ip, if_info->ip_addr);
		if (raw_mac)
			strcpy(raw_mac, if_info->raw_hd_addr);
	}

  /* free link list */
  struct if_ipv4_info *q = NULL;
  for (p = if_info; p != NULL; ) {
    q = p;
    p = p->next;
    free (q);
  } 
	
	return ret;
}

#if 0
#include <sys/stat.h>
#include <ctype.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define CFG_VALID		0x8000
#define CFG_EOF			0x4000
#define CFG_ERROR		0x0000
#define CFG_SECTION		0x0001
#define CFG_DEFINE		0x0002
#define CFG_CONTINUE	0x0003
#define CFG_TYPEMASK	0x000F

#define CFG_TYPE(X)		((X) & CFG_TYPEMASK)
#define cfg_valid(X)	((X) != NULL && ((X)->flags & CFG_VALID))
#define cfg_eof(X)	((X)->flags & CFG_EOF)
#define cfg_section(X)	(CFG_TYPE((X)->flags) == CFG_SECTION)
#define cfg_define(X)	(CFG_TYPE((X)->flags) == CFG_DEFINE)


#define CFE_MUST_FREE_SECTION	0x8000
#define CFE_MUST_FREE_ID	0x4000
#define CFE_MUST_FREE_VALUE	0x2000
#define CFE_MUST_FREE_COMMENT	0x1000

static int _rewind (PCONFIG pconfig)
{
	if (!cfg_valid (pconfig))
	return -1;

	pconfig->flags = CFG_VALID;
	pconfig->cursor = 0;

	return 0;
}

/*
 *  Initialize a configuration, read file into memory
 */
int cfg_init (PCONFIG *ppconf, const char *filename, int doCreate)
{
	PCONFIG pconfig=NULL;
	int ret=0;
	int fd=-1;
	int exist=-1;

	if ( (!filename)||(ppconf==NULL) )		//check input parameters
	{
		return -1;
	}
	
	*ppconf = NULL;	//init output

	ret=access(filename, F_OK);	//check if exist or not
	if (ret!=0)	//no exist
	{
		if (!doCreate)	//not create new file
		{
			return -2;
		}
		else
		{
			exist=2;		//need to create
		}
	}
	else
	{
		exist=1;		//
	}

	if ((pconfig = (PCONFIG) calloc (1, sizeof (TCONFIG))) == NULL)
	{
		ret=-3;
		goto End;
	}	

	pconfig->fileName = strdup (filename);
	pconfig->size=0;		//add by yingcai
	pconfig->fd=-1;

	// If the file does not exist, try to create it 
	if (exist==2)
	{
		fd = creat (filename, 0644);		//create new file
		if (fd>=0){close(fd);}
	}

	//add here

	pconfig->fd=-1;

	ret=cfg_refresh (pconfig);
	if (ret !=0)		//read in
	{
		ret=-11;
		goto End;
	}

	ret=0;
	*ppconf = pconfig;

End:  
	if (ret<0)
	{
		cfg_done (pconfig);
		*ppconf=NULL;
	}

	return ret;
}


char * remove_quotes(const char *szString)
{
	char *szWork, *szPtr;

	while (*szString == '\'' || *szString == '\"')
		szString += 1;

	if (!*szString)
		return NULL;
	szWork = strdup (szString);		//string duplicate
	szPtr = strchr (szWork, '\'');		//find the first substring
	if (szPtr)
		*szPtr = 0;
	szPtr = strchr (szWork, '\"');
	if (szPtr)
		*szPtr = 0;

	return szWork;
}

/*
 *  returns:
 *	 0 success
 *	-1 no next entry
 *
 *	section	id	value	flags		meaning
 *	!0	0	!0	SECTION		[value]
 *	!0	!0	!0	DEFINE		id = value|id="value"|id='value'
 *	!0	0	!0	0		value
 *	0	0	0	EOF		end of file encountered
 */
static int _nextentry (PCONFIG pconfig)
{
  PCFGENTRY e;

  if (!cfg_valid (pconfig) || cfg_eof (pconfig))
    return -1;

  pconfig->flags &= ~(CFG_TYPEMASK);
  pconfig->id = pconfig->value = NULL;

  while (1)
    {
      if (pconfig->cursor >= pconfig->numEntries)
	{
	  pconfig->flags |= CFG_EOF;
	  return -1;
	}
      e = &pconfig->entries[pconfig->cursor++];

      if (e->section)
	{
	  pconfig->section = e->section;
	  pconfig->flags |= CFG_SECTION;
	  return 0;
	}
      if (e->value)
	{
	  pconfig->value = e->value;
	  if (e->id)
	    {
	      pconfig->id = e->id;
	      pconfig->flags |= CFG_DEFINE;
	    }
	  else
	    pconfig->flags |= CFG_CONTINUE;
	  return 0;
	}
    }
}

static int _find (PCONFIG pconfig, char *section, char *id)
{
  int atsection;

  if (!cfg_valid (pconfig) || _rewind (pconfig))
    return -1;

  atsection = 0;
  while (_nextentry (pconfig) == 0)
    {
      if (atsection)
	{
	  if (cfg_section (pconfig))
	    return -1;
	  else if (cfg_define (pconfig))
	    {
	      char *szId = remove_quotes (pconfig->id);
	      int bSame;
	      if (szId)
		{
		  //bSame = !strcasecmp (szId, id);
		  bSame = !strcmp (szId, id);
		  free (szId);
		  if (bSame)
		    return 0;
		}
	    }
	}
      else if (cfg_section (pconfig)
	  //&& !strcasecmp (pconfig->section, section))
	  && !strcmp (pconfig->section, section))
	{
	  if (id == NULL)
	    return 0;
	  atsection = 1;
	}
    }
  return -1;
}

int cfg_getstring (PCONFIG pconfig, char *section, char *id, char *valptr)
{
	if(!pconfig || !section || !id || !valptr) return -1;
	if(_find(pconfig,section,id) <0)
	{ 
		return -1;
	}
	strcpy(valptr,pconfig->value);
	return 0;
}

int cfg_getint (PCONFIG pconfig, char *section, char *id, int *valptr)
{
	if(!pconfig || !section || !id) 
		return -1;
	if(_find(pconfig,section,id) <0)
		return -2;
	*valptr = atoi(pconfig->value);
	return 0;
}

static int _SimpleSetPosixLock(int fd,int type)
{
	struct flock lock;
	
	lock.l_type=type;
	lock.l_pid=getpid();
	lock.l_whence=SEEK_SET;
	lock.l_start=0;
	lock.l_len=0;
	return (fcntl(fd, F_SETLKW, &lock));		//block if busy
}

/*
 *  Free the content specific data of a configuration
 */
static int _freeimage (PCONFIG pconfig)
{
  char *saveName;
  PCFGENTRY e;
  unsigned int i;

  if (pconfig->image)
  {
    	free (pconfig->image);
	pconfig->image=NULL;
  }
  if (pconfig->entries)
  {
      e = pconfig->entries;
      for (i = 0; i < pconfig->numEntries; i++, e++)
	{
	  if (e->flags & CFE_MUST_FREE_SECTION)
	    free (e->section);
	  if (e->flags & CFE_MUST_FREE_ID)
	    free (e->id);
	  if (e->flags & CFE_MUST_FREE_VALUE)
	    free (e->value);
	  if (e->flags & CFE_MUST_FREE_COMMENT)
	    free (e->comment);
	}
      free (pconfig->entries);
  }

  saveName = pconfig->fileName;
  memset (pconfig, 0, sizeof (TCONFIG));
  pconfig->fileName = saveName;

  return 0;
}

/*
strchr()功能：查找字符串s中首次出现字符c的位置
*/
#define iseolchar(C) (strchr ("\n\r\x1a", C) != NULL)
#define iswhite(C) (strchr ("\f\t ", C) != NULL)

static char *_cfg_skipwhite (char *s)
{
  while (*s && iswhite (*s))
    s++;
  return s;
}


static int _cfg_getline (char **pCp, char **pLinePtr)
{
  char *start;
  char *cp = *pCp;

  while (*cp && iseolchar (*cp))
    cp++;
  start = cp;
  if (pLinePtr)
    *pLinePtr = cp;

  while (*cp && !iseolchar (*cp))
    cp++;
  if (*cp)
    {
      *cp++ = 0;
      *pCp = cp;

      while (--cp >= start && iswhite (*cp));
      cp[1] = 0;
    }
  else
    *pCp = cp;

  return *start ? 1 : 0;
}

static char *rtrim (char *str)
{
  char *endPtr;

  if (str == NULL || *str == '\0')
    return NULL;

  for (endPtr = &str[strlen (str) - 1]; endPtr >= str && isspace (*endPtr);endPtr--)
  	;
  endPtr[1] = 0;		//should be endPrt[1]='\0' ??

  return endPtr >= str ? endPtr : NULL;
}

static PCFGENTRY _cfg_poolalloc (PCONFIG p, unsigned int count)
{
  PCFGENTRY newBase;
  unsigned int newMax;

  if (p->numEntries + count > p->maxEntries)
    {
      newMax =
	  p->maxEntries ? count + p->maxEntries + p->maxEntries / 2 : count +
	  4096 / sizeof (TCFGENTRY);
      newBase = (PCFGENTRY) malloc (newMax * sizeof (TCFGENTRY));
      if (newBase == NULL)
	return NULL;
      if (p->entries)
	{
	  memcpy (newBase, p->entries, p->numEntries * sizeof (TCFGENTRY));
	  free (p->entries);
	}
      p->entries = newBase;
      p->maxEntries = newMax;
    }

  newBase = &p->entries[p->numEntries];
  p->numEntries += count;

  return newBase;
}

static int _storeentry (PCONFIG pconfig,char *section,char *id,char *value,char *comment, int dynamic)
{
	PCFGENTRY data;

	if ((data = _cfg_poolalloc (pconfig, 1)) == NULL)
		return -1;

	data->flags = 0;
	if (dynamic)
	{
		if (section)
			section = strdup (section);
		if (id)
			id = strdup (id);
		if (value)
			value = strdup (value);
		if (comment)
			comment = strdup (value);

		if (section)
			data->flags |= CFE_MUST_FREE_SECTION;
		if (id)
			data->flags |= CFE_MUST_FREE_ID;
		if (value)
			data->flags |= CFE_MUST_FREE_VALUE;
		if (comment)
			data->flags |= CFE_MUST_FREE_COMMENT;
	}

	data->section = section;
	data->id = id;
	data->value = value;
	data->comment = comment;

	return 0;
}

/*
 *  Parse the in-memory copy of the configuration data
 */
static int _cfg_parse (PCONFIG pconfig)
{
  int ret=0;
  int isContinue, inString;
  char *imgPtr;
  char *endPtr;
  char *lp;
  char *section;
  char *id;
  char *value;
  char *comment;

  if (cfg_valid (pconfig))
    return 0;

  endPtr = pconfig->image + pconfig->size;
  for (imgPtr = pconfig->image; imgPtr < endPtr;)
    {
      if (!_cfg_getline (&imgPtr, &lp))
	continue;

      section = id = value = comment = NULL;

      /*
         *  Skip leading spaces
       */
      if (iswhite (*lp))
	{
	  lp = _cfg_skipwhite (lp);
	  isContinue = 1;
	}
      else
	isContinue = 0;

      /*
       *  Parse Section
       */
      if (*lp == '[')
	{
	  section = _cfg_skipwhite (lp + 1);
	  if ((lp = strchr (section, ']')) == NULL)
	    continue;
	  *lp++ = 0;
	  if (rtrim (section) == NULL)
	    {
	      section = NULL;
	      continue;
	    }
	  lp = _cfg_skipwhite (lp);
	}
      else if (*lp != ';')
	{
	  /* Try to parse
	   *   1. Key = Value
	   *   2. Value (iff isContinue)
	   */
	  if (!isContinue)
	    {
	      /* Parse `<Key> = ..' */
	      id = lp;
	      if ((lp = strchr (id, '=')) == NULL)
		continue;
	      *lp++ = 0;
	      rtrim (id);
	      lp = _cfg_skipwhite (lp);
	    }

	  /* Parse value */
	  inString = 0;
	  value = lp;
	  while (*lp)
	    {
	      if (inString)
		{
		  if (*lp == inString)
		    inString = 0;
		}
	      else if (*lp == '"' || *lp == '\'')
		inString = *lp;
	      else if (*lp == ';' && iswhite (lp[-1]))
		{
		  *lp = 0;
		  comment = lp + 1;
		  rtrim (value);
		  break;
		}
	      lp++;
	    }
	}

      /*
       *  Parse Comment
       */
      if (*lp == ';')
	comment = lp + 1;

      if (_storeentry (pconfig, section, id, value, comment,0) <0)
	{
	  pconfig->dirty = 1;
	  return -1;
	}
    }

  pconfig->flags |= CFG_VALID;

  return 0;
}


/*
 *  This procedure reads an copy of the file into memory
 *  catching the content based on stat
 */
int cfg_refresh (PCONFIG pconfig)
{
	int ret=0;
	struct stat sb;
	char *mem;
	int fd=-1;

	if (pconfig == NULL)
		return -1;
	
	if ((fd = open (pconfig->fileName, O_RDONLY | O_BINARY))<0)
  	{
  		ret=-2;
		goto End;
  	}

	ret=_SimpleSetPosixLock(fd,F_RDLCK);
	if (ret<0)		//can not get R_lock when writting
	{
		ret=-100;
		goto End;
	}

	//stat (pconfig->fileName, &sb);		//it will open twice, do not use this func.
	ret=fstat(fd,&sb);
	if (ret)
	{
		ret=-3;
		goto End;
	}

	/*
	*  Check to see if our incore image is still valid
	*/
	if (pconfig->image && sb.st_size == pconfig->size
	&& sb.st_mtime == pconfig->mtime)
	{
		ret=0;		//still valid, do NOT need to read in
		goto End;
	}

	/*
	*  If our image is dirty, ignore all local changes
	*  and force a reread of the image, thus ignoring all mods
	*/
	if (pconfig->dirty)
		_freeimage (pconfig);

	/*
	*  Now read the full image
	*/
	mem = (char *) malloc (sb.st_size + 1);
	if (mem == NULL || read (fd, mem, sb.st_size) != sb.st_size)		//read into RAM
	{
		free (mem);mem=NULL;
		ret=-4;
		goto End;
	}
	mem[sb.st_size] = 0;

	/*
	*  Store the new copy
	*/
	_freeimage (pconfig);
	pconfig->image = mem;
	pconfig->size = sb.st_size;
	pconfig->mtime = sb.st_mtime;

	if (_cfg_parse (pconfig)<0)
	{
		_freeimage (pconfig);
		ret=-5;
		goto End;
	}

	ret=0;	//success!

End:
	if (fd>=0)
	{
		_SimpleSetPosixLock(fd,F_UNLCK);
		close (fd);
	}
	if (ret<0)		//failed
	{
		pconfig->image = NULL;
	  	pconfig->size = 0;
	}
  	return ret;
}

/*
 *  Change the configuration
 *
 *  section id    value		action
 *  --------------------------------------------------------------------------
 *   value  value value		update '<entry>=<string>' in section <section>
 *   value  value NULL		delete '<entry>' from section <section>
 *   value  NULL  NULL		delete section <section>
 */
int cfg_write ( PCONFIG pconfig,char *section,char *id,char *value)
{
  int ret=0;
  PCFGENTRY e, e2, eSect;
  int idx;
  int i;
  char sValue[1024]={0};

  if (!cfg_valid (pconfig) || section == NULL)
    return -1;

//  ret=cfg_getstring(pconfig,section,id,sValue);		//we allow value=NULL to delete key
//  if ((ret==0)&&(!strcmp(value,sValue)))	//if equal, do not need to write, add on 2013-03-27
//  	return 0;

  // find the section 
  e = pconfig->entries;
  i = pconfig->numEntries;
  eSect = 0;
  while (i--)
    {
      //if (e->section && !strcasecmp (e->section, section))
      if (e->section && !strcmp (e->section, section))
	{
	  eSect = e;
	  break;
	}
      e++;
    }

  // did we find the section? 
  if (!eSect)
    {
      // check for delete operation on a nonexisting section 
      if (!id || !value)
	return 0;

      // add section first 
      if (_storeentry (pconfig, section, NULL, NULL, NULL,1) == -1  
	  	|| _storeentry (pconfig, NULL, id, value, NULL,1) == -1)
	return -1;

      pconfig->dirty = 1;
      return 0;
    }

  // ok - we have found the section - let's see what we need to do 

  if (id)
    {
      if (value)
	{
	  /* add / update a key */
	  while (i--)
	    {
	      e++;
	      /* break on next section */
	      if (e->section)
		{
		  /* insert new entry before e */
		  idx = e - pconfig->entries;
		  if (_cfg_poolalloc (pconfig, 1) == NULL)
		    return -1;
		  memmove (e + 1, e,
		      (pconfig->numEntries - idx) * sizeof (TCFGENTRY));
		  e->section = NULL;
		  e->id = strdup (id);
		  e->value = strdup (value);
		  e->comment = NULL;
		  if (e->id == NULL || e->value == NULL)
		    return -1;
		  e->flags |= CFE_MUST_FREE_ID | CFE_MUST_FREE_VALUE;
		  pconfig->dirty = 1;
		  return 0;
		}

	      //if (e->id && !strcasecmp (e->id, id))
	      if (e->id && !strcmp (e->id, id))
		{
		  /* found key - do update */
		  if (e->value && (e->flags & CFE_MUST_FREE_VALUE))
		    {
		      e->flags &= ~CFE_MUST_FREE_VALUE;
		      free (e->value);
		    }
		  pconfig->dirty = 1;
		  if ((e->value = strdup (value)) == NULL)
		    return -1;
		  e->flags |= CFE_MUST_FREE_VALUE;
		  return 0;
		}
	    }

	  /* last section in file - add new entry */
	  if (_storeentry (pconfig, NULL, id, value, NULL,
		  1) == -1)
	    return -1;
	  pconfig->dirty = 1;
	  return 0;
	}
      else
	{
	  /* delete a key */
	  while (i--)
	    {
	      e++;
	      /* break on next section */
	      if (e->section)
		return 0;	/* not found */

	      //if (e->id && !strcasecmp (e->id, id))
	      if (e->id && !strcmp (e->id, id))
		{
		  /* found key - do delete */
		  eSect = e;
		  e++;
		  goto doDelete;
		}
	    }
	  /* key not found - that' ok */
	  return 0;
	}
    }
  else
    {
      /* delete entire section */

      /* find e : next section */
      while (i--)
	{
	  e++;
	  /* break on next section */
	  if (e->section)
	    break;
	}
      if (i < 0)
	e++;

      /* move up e while comment */
      e2 = e - 1;
      while (e2->comment && !e2->section && !e2->id && !e2->value
	  && (iswhite (e2->comment[0]) || e2->comment[0] == ';'))
	e2--;
      e = e2 + 1;

    doDelete:
      /* move up eSect while comment */
      e2 = eSect - 1;
      while (e2->comment && !e2->section && !e2->id && !e2->value
	  && (iswhite (e2->comment[0]) || e2->comment[0] == ';'))
	e2--;
      eSect = e2 + 1;

      /* delete everything between eSect .. e */
      for (e2 = eSect; e2 < e; e2++)
	{
	  if (e2->flags & CFE_MUST_FREE_SECTION)
	    free (e2->section);
	  if (e2->flags & CFE_MUST_FREE_ID)
	    free (e2->id);
	  if (e2->flags & CFE_MUST_FREE_VALUE)
	    free (e2->value);
	  if (e2->flags & CFE_MUST_FREE_COMMENT)
	    free (e2->comment);
	}
      idx = e - pconfig->entries;
      memmove (eSect, e, (pconfig->numEntries - idx) * sizeof (TCFGENTRY));
      pconfig->numEntries -= e - eSect;
      pconfig->dirty = 1;
    }

  return 0;
}

int cfg_writeint( PCONFIG pconfig,char *section,char *id,int value)
{
	int ret=0;
	char sTemp[16]={0};

	sprintf(sTemp,"%d",value);
	ret=cfg_write(pconfig,section,id,sTemp);

	return ret;
}

/*
key=value	//no space before/after = to avoid some comptible problem.
*/
static void
_cfg_output (PCONFIG pconfig, FILE *fd)
{
  PCFGENTRY e = pconfig->entries;
  int i = pconfig->numEntries;
  int m = 0;
  int j, l;
  int skip = 0;

  while (i--)
    {
      if (e->section)
	{
	  /* Add extra line before section, unless comment block found */
	  if (skip)
	    fprintf (fd, "\n");
	  fprintf (fd, "[%s]", e->section);
	  if (e->comment)
	    fprintf (fd, "\t;%s", e->comment);

	  /* Calculate m, which is the length of the longest key */
	  m = 0;
	  for (j = 1; j <= i; j++)
	    {
	      if (e[j].section)
		break;
	      if (e[j].id && (l = strlen (e[j].id)) > m)
		m = l;
	    }

	  /* Add an extra lf next time around */
	  skip = 1;
	}
      /*
       *  Key = value
       */
      else if (e->id && e->value)
	{
	  if (m)
	    fprintf (fd, "%s=%s",e->id, e->value);
	  else
	    fprintf (fd, "%s=%s", e->id, e->value);
	  if (e->comment)
	    fprintf (fd, "\t;%s", e->comment);
	}
      /*
       *  Value only (continuation)
       */
      else if (e->value)
	{
	  fprintf (fd, "%s", e->value);
	  if (e->comment)
	    fprintf (fd, "\t;%s", e->comment);
	}
      /*
       *  Comment only - check if we need an extra lf
       *
       *  1. Comment before section gets an extra blank line before
       *     the comment starts.
       *
       *          previousEntry = value
       *          <<< INSERT BLANK LINE HERE >>>
       *          ; Comment Block
       *          ; Sticks to section below
       *          [new section]
       *
       *  2. Exception on 1. for commented out definitions:
       *     (Immediate nonwhitespace after ;)
       *          [some section]
       *          v1 = 1
       *          ;v2 = 2   << NO EXTRA LINE >>
       *          v3 = 3
       *
       *  3. Exception on 2. for ;; which certainly is a section comment
       *          [some section]
       *          definitions
       *          <<< INSERT BLANK LINE HERE >>>
       *          ;; block comment
       *          [new section]
       */
      else if (e->comment)
	{
	  if (skip && (iswhite (e->comment[0]) || e->comment[0] == ';'))
	    {
	      for (j = 1; j <= i; j++)
		{
		  if (e[j].section)
		    {
		      fprintf (fd, "\n");
		      skip = 0;
		      break;
		    }
		  if (e[j].id || e[j].value)
		    break;
		}
	    }
	  fprintf (fd, ";%s", e->comment);
	}
      fprintf (fd, "\n");
      e++;
    }
}

int _FileCopy(char *infile,int wd)
{
	int ret=0;
	int rd=-1;
	struct stat st;
	char buf[0x1000]={0};
	int len=0;
	int size=0;
	
	rd=open(infile,O_RDONLY);
	if (rd<0){
		ret=-2;goto End;
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
	
	return ret;
}


int cfg_commit (PCONFIG pconfig)
{
	int ret=0;
	
	int fd=-1;
	FILE *fp=NULL;

	if (!cfg_valid (pconfig))
	{
		ret=-1;
		goto End;
	}

	if (pconfig->dirty)
	{
		if ((fd = open (pconfig->fileName, O_RDWR | O_BINARY)) <0)
		{
			ret=-4;
			goto End;
		}
		
		ret=_SimpleSetPosixLock(fd,F_WRLCK);
		if (ret<0)		//can not get R_lock when writting
		{
			ret=-100;
			goto End;
		}
	
		fp = fopen ("/tmp/cfgfile.tmp", "w");
		if (fp){
      			_cfg_output(pconfig,fp);
			fclose(fp);
		}
		_FileCopy("/tmp/cfgfile.tmp",fd);
		
		pconfig->dirty = 0;
      		ret=0;	  
    	}
  
End:
	if (fd>=0){
		_SimpleSetPosixLock(fd,F_UNLCK);
		close (fd);
	}

	return ret;
}

/*
 *  Free all data associated with a configuration
 */
int cfg_done (PCONFIG pconfig)
{
	int ret=0;
	
	if (pconfig)
	{
		_freeimage (pconfig);
		if (pconfig->fileName)
		{
			free (pconfig->fileName);
		}
		free (pconfig);
		pconfig=NULL;	  
		ret=0;
	}
	else
		ret=-1;
	
	return ret;
}

int Cfg_ReadStr(char *File,char *Section,char *Entry,char *valptr)
{ 
	PCONFIG pCfg;
	int ret=-1;

	ret=cfg_init (&pCfg, File, 0);		//do not create new file
	if (!ret)
	{
		ret=cfg_getstring(pCfg, Section, Entry,valptr);
		cfg_done(pCfg);
	}

	return ret;
}

//decimal number(not 0xHHHH)
int Cfg_WriteStr(char *File,char *Section,char *Entry,char *valptr)
{ 
	PCONFIG pCfg;
	int ret=-1;

	ret=cfg_init (&pCfg, File, 1);		//create it if not exist
	if (!ret)
	{
		cfg_write (pCfg, Section, Entry, valptr);
		ret = cfg_commit(pCfg);	//save
		cfg_done(pCfg);	//close handle 
	}
	return ret;
}

//decimal number(not 0xHHHH)
int  Cfg_ReadInt(char *File,char *Section,char *Entry,int *valptr)
{ 
	PCONFIG pCfg;
	char sValue[64];
	int ret=-1;

	ret=cfg_init (&pCfg, File, 0);		//do not create new file
	if (!ret)
	{
		ret=cfg_getstring(pCfg, Section, Entry,sValue);
		if (ret==0)	//success
			*valptr = atoi(sValue);		//decimal number 
//		else
//			*valptr = NULL_VAL;
		cfg_done(pCfg);
	}
	return ret;
}

//decimal number(not 0xHHHH)
int Cfg_WriteInt(char *File,char *Section,char *Entry,int Value)
{ 
	PCONFIG pCfg;
	int ret=-1;
	
	char sValue[64];
	sprintf(sValue,"%d",Value);		//convert to string, decimal number

	ret=cfg_init (&pCfg, File, 1);		//create it if not exist
	if (!ret)
	{
		cfg_write (pCfg, Section, Entry, sValue);
		ret = cfg_commit(pCfg);	//save
		cfg_done(pCfg);	//close handle 
	}
	return ret;
}


int utl_fgets (char *fileName,char *line, int bLock)
{
	int ret=0;
	int fd=-1;
	FILE *fp=NULL;

	if ( (!fileName)||(!line) )
		return -1;
	
	if (bLock==1){
		if ((fd = open (fileName, O_RDONLY | O_BINARY)) <0)
		{
			ret=-2;
			goto End;
		}
		
		ret=_SimpleSetPosixLock(fd,F_RDLCK);
		if (ret<0)
		{
			ret=-100;
			goto End;
		}
	}
	
	fp = fopen(fileName, "rb");
	if (fp)
	{
		//fgets(line, 1024, fp);	//max line size
		if (fgets(line, 1024, fp) == NULL)
			ret=-3;
		else
			ret=1;
		fclose(fp);
	}
  
End:
	if (fd>=0){
		_SimpleSetPosixLock(fd,F_UNLCK);
		close (fd);
	}

	return ret;
}

int utl_fputs (char *fileName,char *line, int bLock)
{
	int ret=0;
	int fd=-1;
	FILE *fp=NULL;

	if ( (!fileName)||(!line) )
		return -1;

	if (bLock==1){
		if ((fd = open (fileName, O_RDWR | O_BINARY)) <0)
		{
			ret=-2;
			goto End;
		}
		
		ret=_SimpleSetPosixLock(fd,F_WRLCK);
		if (ret<0)
		{
			ret=-100;
			goto End;
		}
	}
	
	fp = fopen(fileName, "a+");
	if (fp)
	{
		fputs(line, fp);
		fputs("\n", fp);
		fclose(fp);
	}
  
End:
	if (fd>=0){
		_SimpleSetPosixLock(fd,F_UNLCK);
		close (fd);
	}

	return ret;
}

#endif	//#ifndef __CFGFILE_H__


