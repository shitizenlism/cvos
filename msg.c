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
#include <semaphore.h>
#include <pthread.h>
#include <memory.h>

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

#include "msg.h"

extern int Log_MsgLine(char *LogFileName,char *sLine);
#if 1
#define 	MsgPrint(fmt,args...)  do{ \
			char _PrtBuf[1000]; \
			sprintf(_PrtBuf,":" fmt , ## args); \
			Log_MsgLine("msg.log",_PrtBuf); \
			}while(0)
#else
#define 	MsgPrint(fmt,args...)  
#endif

static pthread_mutex_t  gMsgThrdLock;

//////////////////////////////////////////////////////////////////////////     
// 函数功能: CRC16效验(CCITT的0XFFFF效验)     
// 输入参数: pDataIn: 数据地址     
//           iLenIn: 数据长度                
// 输出参数: pCRCOut: 2字节校验值       
static void _SumCRC16(unsigned char* pDataIn, unsigned int iLenIn, unsigned short* pCRCOut)     
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

int MSG_PackMsg(char ver,char* body,char* MsgBuf)
{
	int len=0;
	unsigned short crc16=0;

	if ((!body)||(!MsgBuf))
		return -1;

	MsgBuf[0]='@';
	MsgBuf[1]='@';
	MsgBuf[2]=(char)ver;
	strcpy(&MsgBuf[3],body);
	len=strlen(body);

	if (ver=='2'){
		_SumCRC16(body,len,&crc16);
		MsgBuf[len+3]=(crc16>>8)&0xFF;
		MsgBuf[len+4]=(crc16)&0xFF;
		
		//MsgPrint("crc16=0x%04x,len=%d,0x%02x,0x%02x\n",crc16,len+2,(unsigned char)MsgBuf[len]
		//	,(unsigned char)MsgBuf[len+1]);
		MsgBuf[len+5]='#';
		MsgBuf[len+6]='#';
		len+=7;
	}
	else{
		MsgBuf[len+3]='#';
		MsgBuf[len+4]='#';
		len+=5;
	}

	return len;
}

int MSG_UnPackMsg(char *ver,char* body,char* MsgBuf)
{
	int ret=0;
	int len=0;
	unsigned short crc16=0,sum=0;
	char protocol=0;

	if ((!ver)||(!body)||(!MsgBuf))
		return -1;

	if ((MsgBuf[0] != '@')||(MsgBuf[1] != '@')){
		ret=-2;goto End;
	}
	protocol=MsgBuf[2];
	if (protocol=='2'){	
		len=strlen(MsgBuf)-7;
		_SumCRC16(&MsgBuf[3],len,&sum);
		crc16=(MsgBuf[len+3]<<8)&0xFF00;
		crc16|=MsgBuf[len+4]&0x00FF;
		if (sum!=crc16){
			MsgPrint("sum=0x%04x,crc16=0x%04x\n",sum,crc16);
			ret=-3;goto End;
		}
	}
	else{
		len=strlen(MsgBuf)-5;
	}

	if (body){
		strncpy(body,MsgBuf+3,len);
		ret=len;
	}
	if (ver){
		*ver=protocol;
	}
	
End:
	if (ret<0)
		MsgPrint("%s() failed!ret=%d\n",__FUNCTION__,ret);
	return ret;
}

/*
when ver=1: no crc16, msg format: 0xAA(1)+ver(1)+sn(1)+len(1)+body(<=255)
when ver=2: has crc16, msg format: 0xAA(1)+ver(1)+sn(1)+len(1)+body(<=255)+crc16(2)
*/
unsigned int MSG_PackHex(unsigned char ver, unsigned char sn,
	unsigned char* body,unsigned int len, unsigned char MsgBuf[])
{
	unsigned int total=0;
	unsigned short crc16=0;

	if ((!body)||(!MsgBuf))
		return total;

	MsgBuf[0]=MSG_HEX_HEAD;
	MsgBuf[1]=ver;
	MsgBuf[2]=sn;
	MsgBuf[3]=len;
	memcpy(&MsgBuf[4],body,len);
	total = len + 4;

	if (ver==2){
		_SumCRC16(body,len,&crc16);
		MsgBuf[len+4]=(crc16>>8)&0xFF;
		MsgBuf[len+5]=(crc16)&0xFF;
		total += 2;
	}

	return total;
}

int MSG_UnPackHex(unsigned char *ver, unsigned char *sn,
	unsigned char* body,unsigned char *MsgBuf)
{
	int ret=0;
	unsigned int len=0;
	unsigned short crc16=0,sum=0;
	unsigned char protocol=0;

	if ((!ver)||(!sn)||(!body)||(!MsgBuf))
		return -1;

	if (MsgBuf[0] != MSG_HEX_HEAD){
		ret=-2;goto End;
	}
	*ver=protocol=MsgBuf[1];
	*sn=MsgBuf[2];
	len=MsgBuf[3];
	if (protocol==2){	
		_SumCRC16(&MsgBuf[4],len,&sum);
		crc16=(MsgBuf[len+4]<<8)&0xFF00;
		crc16|=MsgBuf[len+5]&0x00FF;
		if (sum!=crc16){
			MsgPrint("sum=0x%04x,crc16=0x%04x\n",sum,crc16);
			ret=-3;goto End;
		}
	}

	memcpy(body,MsgBuf+4,len);
	ret=len;
	
End:
	if (ret<0)
		MsgPrint("%s() failed!ret=%d\n",__FUNCTION__,ret);
	return ret;
}


/*
int msgget ( key_t key, int msgflg )
第二个参数，msgflg 控制的。它可以取下面的几个值：
IPC_CREAT ：
如果消息队列对象不存在，则创建之，否则则进行打开操作;

系统建立IPC通讯 （消息队列、信号量和共享内存） 时必须指定一个ID值。通常情况下，该id值通过ftok函数得到。

    int key = ftok("msg.tmp", 0x01 ) ;
    if ( key < 0 )
    {
        perror("ftok key error") ;
        return -1 ;
    }
 
    msqid = msgget( key, 0 ) ;
    
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);

*/

/*
key_t ftok( const char * fname, int id )
*/
static int gIpcIdBase=0,gIpcId=0;;
int MSG_GetIpcId(char ipckey)
{
	pthread_mutex_lock(&gMsgThrdLock);
	if (gIpcIdBase==0){
		gIpcIdBase=ftok(".", ipckey);
		gIpcId=gIpcIdBase;
	}
	else
		gIpcId++;

	pthread_mutex_unlock(&gMsgThrdLock);
	return gIpcId;
}

int MSG_QueCreate(int IpcId)
{
	int QueId=-1;

	pthread_mutex_lock(&gMsgThrdLock);
	QueId=msgget(IpcId, 0666 | IPC_CREAT);
	pthread_mutex_unlock(&gMsgThrdLock);
	
	if (QueId<0)
	{
		MsgPrint("create queue err:%s[errno=%d]\n",strerror(errno),errno);
		QueId=-errno;
	}	
	return QueId;
}

//receive with NOWAIT mode
int MSG_QueRevMsg(int QueId,MSGBUF_T *MsgBuf,unsigned int MsgBodyLen)
{
	int ret=0;

	if (QueId<0)
		return -1;

	//If msgtyp is 0, the first message on the queue shall be received. 
	//return: Number of bytes copied into message buffer
	pthread_mutex_lock(&gMsgThrdLock);
	ret=msgrcv(QueId,MsgBuf, MsgBodyLen, 0, MSG_NOERROR|IPC_NOWAIT);
	pthread_mutex_unlock(&gMsgThrdLock);
	
	if (ret<0)
	{
		//MsgPrint("rev queue err:%s[errno=%d]\n",strerror(errno),errno);
		ret=-errno;
	}	
	return ret;
}

int MSG_QueDestroy(int QueId)
{
	int ret=0;

	if (QueId<0)
		return -1;

	pthread_mutex_lock(&gMsgThrdLock);
	if(msgctl(QueId,IPC_RMID,0)==-1)
	{
		ret=-3;
		goto End;
	}

End:
	pthread_mutex_unlock(&gMsgThrdLock);
	
	if (ret<0)
	{
		MsgPrint("destory queue err:%s[errno=%d]\n",strerror(errno),errno);
		ret=-errno;
	}	
	return ret;
}

//MsgBodyLen=sizeof(msg)-sizeof(long). 0:success. -nnnn:errno
int MSG_QueSndMsg(int IpcId,MSGBUF_T *MsgBuf,unsigned int MsgBodyLen)
{
	int ret=0;
	int QueId=0;

	pthread_mutex_lock(&gMsgThrdLock);
	if ((QueId = msgget(IpcId, 0666)) < 0) 
	{
		ret=-2;
		goto End;
	}

	ret=msgsnd(QueId, MsgBuf,MsgBodyLen,IPC_NOWAIT);

	msgctl(QueId,IPC_RMID,0);	//destory this IPC(QueId) on 2017/8/4

End:
	pthread_mutex_unlock(&gMsgThrdLock);
	if (ret<0)
	{
		MsgPrint("send queue[IpcId=0x%x] err:%s[errno=%d]\n",IpcId,strerror(errno),errno);
		ret=-errno;
	}	
	return ret;
}

int MSG_AppSendtoKeeper(long mType,APPMSG_S *pAppMsg)
{
	int ret=0;
	MSGBUF_T MsgBuf;
	int len=0;

	memset(&MsgBuf.mType,0,sizeof(MSG_S));
	MsgBuf.mType=mType;
	MsgBuf.mBuf[0]=pAppMsg->AppPid;
	len=sizeof(pAppMsg->AppPid);
	strcpy(&MsgBuf.mText[len],pAppMsg->AppName);
	len+=sizeof(pAppMsg->AppName);
	strcpy(&MsgBuf.mText[len],pAppMsg->AppStartScript);
	len+=sizeof(pAppMsg->AppStartScript);
	
	ret=MSG_QueSndMsg(IpcId_SysKeeper,&MsgBuf,len);

	MsgPrint("%s(),AppPid=%d,AppName=%s,AppStartScript=%s\n\r",__FUNCTION__
		,pAppMsg->AppPid,pAppMsg->AppName,pAppMsg->AppStartScript);
	return ret;
}

/*
do NOT support multi-thread. pls refer to ringbuf to use mutex.
*/
int MSG_Init(void)
{
//	MsgPrint("msg version=%d,buildtime=%s\n",MSG_VERSION,BNO);
	pthread_mutex_init(&gMsgThrdLock, NULL);

	return 0;
}

int MSG_UnInit(void)
{
	pthread_mutex_destroy(&gMsgThrdLock);
	
	return 0;	
}

