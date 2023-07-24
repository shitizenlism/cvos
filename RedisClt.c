#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include <sys/socket.h>
#include <sys/types.h>      
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>   //inet_addr,inet_aton
#include <netdb.h>         //gethostbyname
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <error.h>         //perror
#include <errno.h>         //errno

#include "vos.h"
#include "RedisClt.h"

extern int Log_MsgLine(char *LogFileName,char *sLine);
#if 1
#define 	CachePrint(fmt,args...)  do{ \
			char _PrtBuf[1000]; \
			sprintf(_PrtBuf,":" fmt , ## args); \
			Log_MsgLine("redis.log",_PrtBuf); \
			}while(0)
#else
#define 	CachePrint(fmt,args...)  
#endif

/*
127.0.0.1:3389[1]> HGETALL 38001170301SN0000041
1) "InsId"
2) "0"
3) "TcpFd"
4) "7"
5) "InsCnt"
6) "5"
127.0.0.1:3389[1]> 
除非内存掉电，否则数据将一直保存，哪怕redis-server restart也不影响。
*/
/*
TS:10:31:57 :DevStatusNotifyUrl(devId=W20000011705SN000054&devToken=9&online=1),ret=1
TS:10:32:17 :DevStatusNotifyUrl(devId=W20000011705SN000054&devToken=12&online=0),ret=1
还是一样的同一个设备多个连接问题，后删除旧连接，导致设备状态错误！
解决方案：
针对每个devid，统计连接数量InsCnt。在改变devid的状态为offline之前，看InsCnt是否为0。
如果<=0，则说明Ins都被delete了。应该设置状态为offline。
建立一个new Ins就要将InsCnt加1。为了快速检索，还是用redis作为cache。
进程重启（未掉电），初始化redis时，要将cache里的InsCnt全部置为0。

方案: 多进程共用同一个connection, 同一个db。

*/
static DB_CONN_T gRedisConn;
static pthread_mutex_t gThrdLock;

void *REDIS_Init(char *hostname,int port,char *authpass,int db)
{
	int ret=0;
	redisReply *reply=NULL;

	if (gRedisConn.hdl!=NULL){
		CachePrint("redis server already has connection!\n");
		printf("redis server already has connection!\n");
		gRedisConn.InsCnt++;
		return (void *)gRedisConn.hdl;
	}

    struct timeval timeout = { 2, 500000 }; // 1.5 seconds
    gRedisConn.hdl= redisConnectWithTimeout(hostname, port, timeout);
    if (gRedisConn.hdl) {
		if (gRedisConn.hdl->err){
			CachePrint("connect error: %s\n", gRedisConn.hdl->errstr);
			ret=-3;goto End;
		}

	    reply = (redisReply*)redisCommand(gRedisConn.hdl,"AUTH %s",authpass);
		if (!reply){
			ret=-4;goto End;
		}
		if (reply->type == REDIS_REPLY_ERROR){
   			CachePrint("AUTH error response: %s\n", reply->str);
		}
		freeReplyObject(reply);

	    reply = (redisReply*)redisCommand(gRedisConn.hdl,"SELECT %d",db);
		if (!reply){
			ret=-5;goto End;
		}
		if (reply->type == REDIS_REPLY_ERROR){
   			CachePrint("SELECT error response: %s\n", reply->str);
		}
		freeReplyObject(reply);

	    reply = (redisReply*)redisCommand(gRedisConn.hdl,"FLUSHDB");
		if (reply){
			if (reply->type!=REDIS_REPLY_ERROR){
				CachePrint("FLUSHDB success!\n");
			}
			freeReplyObject(reply);
		}

		gRedisConn.InsCnt=1;
    }
	else{
		ret=-2;goto End;
	}

End:
	CachePrint("%s(db=%d),ret=%d\n",__FUNCTION__,db,ret);
	if (ret<0){
		REDIS_UnInit(gRedisConn.hdl);
		gRedisConn.hdl = NULL;
		gRedisConn.InsCnt=0;
	}

	pthread_mutex_init(&gThrdLock, NULL);	
	return (void *)gRedisConn.hdl;
}

int REDIS_UnInit(void *hdl)
{
	gRedisConn.InsCnt--;
	if (gRedisConn.InsCnt == 0)
	{
		if (gRedisConn.hdl){
			redisFree(gRedisConn.hdl);gRedisConn.hdl=NULL;
			pthread_mutex_destroy(&gThrdLock);
		}
	}

	return 0;
}

int REDIS_SelectDb(void *hdl,int db)
{
	redisReply *reply=NULL;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"SELECT %d",db);
	if ((reply) && (reply->type == REDIS_REPLY_ERROR)){
			CachePrint("SELECT error response: %s\n", reply->str);
	}
	if (reply)
		freeReplyObject(reply);
	pthread_mutex_unlock(&gThrdLock);
	
	return 0;
}

/*
针对每个devid，我要缓存[InsId,TcpFd]这两个值!
127.0.0.1:6379> HMSET Cam001 InsId 3 TcpFd 5
OK
127.0.0.1:6379> HGET Cam001 InsId
"3"
127.0.0.1:6379> HGET Cam001 TcpFd
"5"
*/
int REDIS_HMSET(char* devid,int InsId, int TcpFd)
{
	int ret=-1;
	redisReply *reply=NULL;

	if ((!devid)||(!gRedisConn.hdl))
		return -1;
	if (strlen(devid)<=3)
		return -2;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HMSET %s %s %d %s %d"
		,devid,KEY_FIELD_INSID,InsId,KEY_FIELD_TCPFD,TcpFd);
	if (reply){
		CachePrint("HMSET: %s\n", reply->str);
		if (reply->type == REDIS_REPLY_ERROR){
			ret=-3;
		}
		else{
			if (reply->str){
				if (!strcmp(reply->str,"OK"))
					ret=0;
				else
					ret=-4;
			}
			else{
				ret=-5;
				// 退出当前进程。因为系统异常发生了。
				exit(ret);
			}
		}
		freeReplyObject(reply);
	}
	pthread_mutex_unlock(&gThrdLock);

	CachePrint("%s(%s = %d),ret=%d\n",__FUNCTION__,devid,TcpFd,ret);
	return ret;
}

/*
TS:10:31:57 :DevStatusNotifyUrl(devId=W20000011705SN000054&devToken=9&online=1),ret=1
TS:10:32:17 :DevStatusNotifyUrl(devId=W20000011705SN000054&devToken=12&online=0),ret=1
还是一样的同一个设备多个连接问题，后删除旧连接，导致设备状态错误！
解决方案：
针对每个devid，统计连接数量InsCnt。在改变devid的状态为offline之前，看InsCnt是否为0。
如果<=0，则说明Ins都被delete了。应该设置状态为offline。
建立一个new Ins就要将InsCnt加1。为了快速检索，还是用redis作为cache。
进程重启（未掉电），初始化redis时，要将cache里的InsCnt全部置为0。

*/
int REDIS_HGET_InsCnt(char* devid)
{
	int ret=-1;
	redisReply *reply=NULL;

	if ((!devid)||(!gRedisConn.hdl))
		return -1;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HGET %s %s",devid,KEY_FIELD_INSCNT);
	if (reply){
		if (reply->type==REDIS_REPLY_ERROR){
			ret=-2;
		}
		else{
			if (!reply->str)
				ret=-3;
			else if (!strcmp(reply->str,"OK"))
				ret=-4;
			else{
				ret=atoi(reply->str);
			}
		}
		
		freeReplyObject(reply);
	}
	else
		CachePrint("%s(%s) failed!!\n",__FUNCTION__,devid);
	pthread_mutex_unlock(&gThrdLock);
	
	CachePrint("HGET InsCnt=ret=%d\n", ret);
	return ret;
}

/*
redis> HSET myhash field 5
(integer) 1
redis> HINCRBY myhash field 1
(integer) 6
redis> HINCRBY myhash field -1
(integer) 5
redis> HINCRBY myhash field -10
(integer) -5
redis> 

127.0.0.1:6379> AUTH foo12345
OK
127.0.0.1:6379> 
127.0.0.1:6379> select 5
OK
127.0.0.1:6379[5]> HLEN Cam001
(integer) 3
127.0.0.1:6379[5]> HINCRBY Cam001 InsCnt 1
(integer) 2
127.0.0.1:6379[5]> HINCRBY Cam001 InsCnt -1
(integer) 1
127.0.0.1:6379[5]> 

*/
int REDIS_HSET_InsCnt(char* devid,int AddInsCnt)
{
	int ret=-1;
	redisReply *reply=NULL;

	if ((!devid)||(!gRedisConn.hdl))
		return -1;
	if (strlen(devid)<=3)
		return -2;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HINCRBY %s %s %d"
		,devid,KEY_FIELD_INSCNT,AddInsCnt);
	if (reply){
		if (reply->type == REDIS_REPLY_ERROR)
			ret=-3;
		else
			ret=reply->integer;
		freeReplyObject(reply);
	}
	else
		ret=-2;
	pthread_mutex_unlock(&gThrdLock);
	
	CachePrint("%s(%s,%d),ret=%d\n",__FUNCTION__,devid,AddInsCnt,ret);
	return ret;
}

int REDIS_HGET_InsId(char* devid)
{
	int ret=-1;
	redisReply *reply=NULL;

	if ((!devid)||(!gRedisConn.hdl))
		return -1;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HGET %s %s",devid,KEY_FIELD_INSID);
	if (reply){
		CachePrint("HGET InsId=ret=%s\n", reply->str);
		ret=atoi(reply->str);
		freeReplyObject(reply);
	}
	else
		CachePrint("%s(%s) failed!!\n",__FUNCTION__,devid);
	pthread_mutex_unlock(&gThrdLock);
	
	return ret;
}

int REDIS_HDEL_InsId(char* devid)
{
	int ret=0;
	redisReply *reply=NULL;

	if ((!devid)||(!gRedisConn.hdl))
		return -1;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HDEL %s %s",devid,KEY_FIELD_INSID);
	if (reply){
		CachePrint("HDEL InsId: %lld\n", reply->integer);
		freeReplyObject(reply);
	}
	else
		CachePrint("%s(%s) failed!!\n",__FUNCTION__,devid);
	pthread_mutex_unlock(&gThrdLock);
	
	return ret;
}

/*
[root@izwz9jay6aqdkrnnlgscchz bin]# redis-cli 
127.0.0.1:6379> AUTH foo12345
OK
127.0.0.1:6379> select 1
OK
127.0.0.1:6379[1]> HSET app_123456 111
(error) ERR wrong number of arguments for 'hset' command
127.0.0.1:6379[1]> HSET app-dev app-123456 123456
(integer) 1
127.0.0.1:6379[1]> HSET app-dev app-123455 123455
(integer) 1
127.0.0.1:6379[1]> HVALS app-dev
1) "123456"
2) "123455"
127.0.0.1:6379[1]> HGET app-dev app-123455
"123455"
127.0.0.1:6379[1]> HGET app-dev app-12345
(nil)
127.0.0.1:6379[1]> exit

*/
int REDIS_HSET_AppFd(char* field, int sockfd)
{
	int ret=-1;
	redisReply *reply=NULL;

	if ((!field)||(!gRedisConn.hdl))
		return -1;
	if (strlen(field)<=3)
		return -2;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HSET %s %s %d",KEY_APP_DEV,field,sockfd);
	if (reply){
		CachePrint("HSET: %s\n", reply->str);
		if (reply->type == REDIS_REPLY_ERROR){
			ret=-3;
		}
		else{
			ret=0;
		}
		freeReplyObject(reply);
	}
	else
		CachePrint("%s(%s) failed!!\n",__FUNCTION__,field);
	pthread_mutex_unlock(&gThrdLock);
	
	CachePrint("HSET AppFd,ret=%d\n", ret);
	return ret;
}

int REDIS_HGET_AppFd(char* field)
{
	int ret=-1;
	redisReply *reply=NULL;

	if ((!field)||(!gRedisConn.hdl))
		return -1;
	if (strlen(field)<=3)
		return -2;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HGET %s %s",KEY_APP_DEV,field);
	if (reply){
		if (reply->type==REDIS_REPLY_ERROR){
			ret=-2;
		}
		else{
			if (!reply->str)
				ret=-3;
			else if (!strcmp(reply->str,"OK"))
				ret=-4;
			else{
				ret=atoi(reply->str);
			}
		}
		
		freeReplyObject(reply);
	}
	else
		CachePrint("%s(%s) failed!!\n",__FUNCTION__,field);
	pthread_mutex_unlock(&gThrdLock);

	CachePrint("HGET(AppFd=%s). ret=%d\n", field, ret);
	return ret;
}

/*
127.0.0.1:6379[2]> HGET A68000011705SN000015 TcpFd
"6"

why it is so unstabilable?
TS:09:38:23 :HGET TcpFd=ret=-4
TS:09:38:23 :HGET TcpFd=ret=-4

*/
int REDIS_HGET_TcpFd(char* devid)
{
	int ret=-1;
	redisReply *reply=NULL;

	if ((!devid)||(!gRedisConn.hdl))
		return -1;
	if (strlen(devid)<=3)
		return -2;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HGET %s %s",devid,KEY_FIELD_TCPFD);
	if (reply){
		if (reply->type==REDIS_REPLY_ERROR){
			ret=-2;
		}
		else{
			if (!reply->str)
				ret=-3;
			else if (!strcmp(reply->str,"OK"))
				ret=-4;
			else{
				ret=atoi(reply->str);
			}
		}
		
		freeReplyObject(reply);
	}
	else
		CachePrint("%s(%s) failed!!\n",__FUNCTION__,devid);
	pthread_mutex_unlock(&gThrdLock);

	CachePrint("HGET(devid=%s). TcpFd=ret=%d\n", devid, ret);
	return ret;
}

int REDIS_HDEL_TcpFd(char* devid)
{
	int ret=0;
	redisReply *reply=NULL;

	if ((!devid)||(!gRedisConn.hdl))
		return -1;

	pthread_mutex_lock(&gThrdLock);
    reply = (redisReply*)redisCommand(gRedisConn.hdl,"HDEL %s %s",devid,KEY_FIELD_TCPFD);
	if (reply){
		CachePrint("HDEL TcpFd: %lld\n", reply->integer);
		freeReplyObject(reply);
	}
	else
		CachePrint("%s(%s) failed!!\n",__FUNCTION__,devid);
	pthread_mutex_unlock(&gThrdLock);
	
	return ret;
}


