#ifndef __MSG_H__
#define __MSG_H__

#ifdef __cplusplus
       extern "C" {
#endif

#define IpcId_SysKeeper 	0x221234	//fixed. only one msgque
#define IpcId_DevMgrKeeper 	0x231234	
#define IpcId_DevMgr 		0x241234

#define IPC_TYPE_DevManagerStartup  	0x51
#define IPC_TYPE_DevManagerHeartbeat  	0x52
#define IPC_TYPE_DevManagerHeartbeatLost  	0x53
#define IPC_TYPE_DevClientStartup  		0x61
#define IPC_TYPE_Heartbeat  	0x55
#define IPC_TYPE_KillMe  		0x71
#define IPC_TYPE_RestartMe  	0x72

typedef struct{
	int AppPid;
	char AppName[16];
	char AppStartScript[64];
}APPMSG_S;

int MSG_AppSendtoKeeper(long mType,APPMSG_S *pAppMsg);

//#define PROTOCOL_MAGIC "@@"
//#define PROCOCOL_VER "2"
//#define PROTOCOL_END "##"

#define	MAX_MSGBUF_SIZE	2048
#if 0
typedef struct{
	long mtype;
	int SrcIpcId;
	int DstIpcId;
	char mtext[1024];
}MSGBUF_T;
#else
typedef struct{
	long mType;
    union
	{	
		char mText[MAX_MSGBUF_SIZE];
		int mBuf[MAX_MSGBUF_SIZE>>2];
    };
}MSGBUF_T;
#endif

#define MSG_HEAD_MAGIC "@@"
#define MSG_TAIL_MAGIC "##"
typedef struct{
	char head[2];
	char ver[1];
	//int seq;
	char *pBody;
	char tail[2];
}MSG_FRAME_S;

int MSG_GetIpcId(char ipckey);
int MSG_PackMsg(char ver,char* body,char* MsgBuf);
int MSG_UnPackMsg(char *ver,char* body,char* MsgBuf);

#define MSG_HEX_HEAD 0xAA
#define MSG_HEX_ACK 0xB1
#define MSG_HEX_NAK 0xB0
/*
when ver=1: no crc16, msg format: 0xAA(1)+ver(1)+sn(1)+len(1)+body(<=255)
when ver=2: has crc16, msg format: 0xAA(1)+ver(1)+sn(1)+len(1)+body(<=255)+crc16(2)
*/
unsigned int MSG_PackHex(unsigned char ver, unsigned char sn,
	unsigned char* body,unsigned int len, unsigned char MsgBuf[]);
int MSG_UnPackHex(unsigned char *ver, unsigned char *sn,
	unsigned char* body,unsigned char* MsgBuf);

int MSG_QueCreate(int IpcId);
int MSG_QueDestroy(int QueId);
int MSG_QueRevMsg(int QueId,MSGBUF_T *MsgBuf,unsigned int MsgBodyLen);
int MSG_QueSndMsg(int IpcId,MSGBUF_T *MsgBuf,unsigned int MsgBodyLen);

#define MSG_VERSION 2
int MSG_Init(void);
int MSG_UnInit(void);

#ifdef __cplusplus
}
#endif 

#endif
