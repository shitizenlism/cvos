#ifndef __REDISCLT_H__
#define __REDISCLT_H__
#include <hiredis.h>

#ifdef __cplusplus
       extern "C" {
#endif

typedef struct{
	redisContext *hdl;
	int InsCnt;
}DB_CONN_T;

#define KEY_FIELD_INSID "InsId"
#define KEY_FIELD_TCPFD "TcpFd"
#define KEY_FIELD_INSCNT "InsCnt"
#define KEY_APP_DEV "app-dev"

void *REDIS_Init(char *hostname,int port,char *authpass,int db);
int REDIS_SelectDb(void *hdl,int db);
int REDIS_UnInit(void *hdl);

int REDIS_HMSET(char* devid,int InsId,int TcpFd);

int REDIS_HGET_InsCnt(char* devid);
int REDIS_HSET_InsCnt(char* devid,int InsCnt);

int REDIS_HGET_InsId(char* devid);
int REDIS_HDEL_InsId(char* devid);

int REDIS_HGET_TcpFd(char* devid);
int REDIS_HDEL_TcpFd(char* devid);

int REDIS_HSET_AppFd(char* field, int sockfd);
int REDIS_HGET_AppFd(char* field);

#ifdef __cplusplus
}
#endif 
#endif

