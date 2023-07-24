/*
Func: use to bind MAC and limit expired days.
*/

#ifndef __AUTH_H__
#define __AUTH_H__

#ifdef __cplusplus
       extern "C" {
#endif

//docker container has different MAC with host.
#define CDIEU_ETH0MACADDR "52:54:00:e8:76:db"
#define CDIEU_ETH0NAME		"eth0"

#define SZUDP_ETH0MACADDR "70:7b:e8:74:91:f2"		//SZ udp<---->hls
#define SZUDP_ETH0NAME	"enp1s0f0"

#define MYDOCKER_ETH0MACADDR  "02:42:ac:11:00:02"	//docker eth0
#define MYDOCKER_ETH0NAME            "eth0" 
#define MYVM_ETH0MACADDR  "00:0c:29:18:94:fa"	//pc-vm
#define MYVM_ETH0NAME		"eth9"	//pc-vm

#define SHSL_1ETH0MACADDR "08:00:27:97:c6:ec"
#define SHSL_1ETH0NAME	"enp0s3"

#define SHSL_2ETH0MACADDR "00:f1:f3:1e:f3:e7"		
#define SHSL_2ETH0NAME	"ens34"

#define DIQI_ETH0MACADDR "00:16:3e:04:71:22"
#define DIQI_ETH0NAME		"eth0"

#define USER_SERVER_ETHMAC 	DIQI_ETH0MACADDR
#define USER_SERVER_ETHNAME DIQI_ETH0NAME	

#define EXPIRE_DAYS_COUNT_FILE "/usr/local/.livevod"
#define MAX_EXPIRE_DAYS (365*10)


int Auth_VerifyDateAuth(char *statFile,int expireDays);
int Auth_VerifyMacAuth(char *ethName,char *ethAddr);
int Auth_ForkAuthTask(void);
int Auth_RunIndependentTask(int ParentPid);


#ifdef __cplusplus
}
#endif 

#endif
