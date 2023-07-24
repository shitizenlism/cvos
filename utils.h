#ifndef __UTILS_H__
#define __UTILS_H__
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
       extern "C" {
#endif

#define MAX(a,b) ((a>b)?a:b)
#define MIN(a,b) ((a>b)?b:a)

#define	MAX_QUEBUF_LEN	1024
typedef struct{
	long mtype;
    union
	{	
		char mtext[MAX_QUEBUF_LEN];
		int mBuf[MAX_QUEBUF_LEN>>2];
    };
}MSG_S;

#define	FIFO_DIR "/var/pipe/"
#define	RtxpRxFIFO	FIFO_DIR"rtxp_rx"
#define	FIFO_READ_SYNC O_RDONLY
#define	FIFO_READ_ASYNC (O_RDONLY|O_NONBLOCK)
#define	FIFO_WRITE_SYNC O_WRONLY
#define	FIFO_WRITE_ASYNC (O_WRONLY|O_NONBLOCK)

int FifoOpen(char *fifo_name,int fifo_mode);
int FifoClose(int fifo_fd);
int FifoRevMsg(int fifo_fd,MSG_S *MsgBuf,unsigned int MsgBodyLen);
int FifoSend(char *fifo_name,MSG_S *MsgBuf,unsigned int MsgBodyLen);

typedef struct{
	int hh;
	int mm;
}SETTIME_S;
int utl_IsInDuration(char *pBeginTime,char *pEndTime);
typedef struct{
	char user[32];
	char passwd[32];
	char server[32];
	int port;
}FTPSVR_S;
int utl_NetSetFtpSvr(char *iFtpUrl,FTPSVR_S *oFtpConf);

int utl_KillForced(int pid);
unsigned long utl_FileGetSize(const char *filename);
int utl_DoesExist(char *cmd, char *key);
int utl_TimeGetStamp(char *TimeStamp);
int utl_TokenCreate(char *str,unsigned char *md5,char *ssum);
int utl_NewDir(char *dir);
int utl_SetupDateAsDir(char *TopDir);
int utl_LogMyPid(char *pidfile);
int utl_GetMyPid(char *pidfile);
int utl_NetGetMacAddr(const char *netcard,const size_t nlen,char *mac);

int JsonFindIndex(char *JsonName,char **NameArray);
int JsonParseText(char *MsgBuf,char **pName,char **pValue);
int JsonCreateObjects(char **pName,char **pValue,int TokenNum,char *MsgBuf);

#if 0
	typedef struct TCFGENTRY
	{
	    char *section;
	    char *id;
	    char *value;
	    char *comment;
	    unsigned short flags;
	}
	TCFGENTRY, *PCFGENTRY;

	/* configuration file */
	typedef struct TCFGDATA
	{
	    char *fileName;		// Current file name
	    int fd;			//file handler, add by yingcai

	    int dirty;			// Did we make modifications?

	    char *image;		// In-memory copy of the file 
	    size_t size;		// Size of this copy (excl. \0) 
	    time_t mtime;		// Modification time 

	    unsigned int numEntries;
	    unsigned int maxEntries;
	    PCFGENTRY entries;

	    /* Compatibility */
	    unsigned int cursor;
	    char *section;
	    char *id;
	    char *value;
	    char *comment;
	    unsigned short flags;

	}
	TCONFIG, *PCONFIG;

	int cfg_init (PCONFIG *ppconf, const char *filename, int doCreate);
	int cfg_done (PCONFIG pconfig);
	int cfg_getstring (PCONFIG pconfig, char *section, char *id, char *valptr);
	int cfg_refresh (PCONFIG pconfig);
	int cfg_getint (PCONFIG pconfig, char *section, char *id, int *valptr);
	int cfg_writeint( PCONFIG pconfig,char *section,char *id,int value);
	int cfg_write ( PCONFIG pconfig,char *section,char *id,char *value);
	int cfg_commit (PCONFIG pconfig);
	
	int Cfg_ReadStr(char *File,char *Section,char *Entry,char *valptr);
	int Cfg_WriteStr(char *File,char *Section,char *Entry,char *valptr);
	int Cfg_ReadInt(char *File,char *Section,char *Entry,int *valptr);
	int Cfg_WriteInt(char *File,char *Section,char *Entry,int Value);

	int utl_fputs (char *fileName,char *line, int bLock);
	int utl_fgets (char *fileName,char *line, int bLock);

#endif	//#ifndef __CFGFILE_H__


/* define a get if info struct */
struct if_ipv4_info {
  char if_name[16];
  unsigned char ip_addr[20];
  unsigned char hd_addr[20];
  unsigned char raw_hd_addr[20];
  
  struct if_ipv4_info *next;
};
int utl_IfGetInfo(const char *if_name,char *ip,char *mac,char *raw_mac);

#ifdef __cplusplus
}
#endif 

#endif
