#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <ctype.h>	
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <linux/watchdog.h>

#include "reboot.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

static unsigned long _get_file_size(const char *filename)
{
	struct stat buf;

	if(stat(filename, &buf)<0)
	{
		return 0;
	}
	return (unsigned long)buf.st_size;
}

void  FLASH_AddOneResetLog(char *line)
{
	FILE *fp=NULL;
	unsigned long fSize;

	fSize=_get_file_size(IPC_RESET_LOG);
	
	if ((fSize > IPC_RESET_LOG_SIZE) ||(fSize==0))		//too large or no logfile exist, create it
		fp = fopen(IPC_RESET_LOG, "w+");		//reset to zero
	else
		fp = fopen(IPC_RESET_LOG, "r+");	//add log to reset.log
	if (fp)
	{
		fseek(fp,0,2);		//move to the end
		
		fputs(line,fp);		//write a line to file
		fflush(fp);		//only block device has buffer(cache) requiring flush operation.
		fclose(fp);
	}

	return ;
}


int WdtReboot(int ResetCode)
{
	int ret=0;
	
	char line[128]={0};
	time_t timep=0;
	struct tm *pLocalTime;

	int fd=-1;
	unsigned int ulTemp=0;
//	int Temp=0;

	time(&timep);		//get current time, how many senconds
	pLocalTime=localtime(&timep);	//convert to date/time format
		
	sprintf(line,"%s(%d) is called at %4d-%02d-%02d %02d:%02d:%02d. \n",(char*)__FUNCTION__,ResetCode
		,pLocalTime->tm_year+1900,pLocalTime->tm_mon+1,pLocalTime->tm_mday,pLocalTime->tm_hour,pLocalTime->tm_min,pLocalTime->tm_sec);
	FLASH_AddOneResetLog(line);

	usleep(200000);		//waiting file operation done.

	fd = open("/dev/watchdog", O_RDWR);	//try one
	if (fd<0)
		fd=open("/dev/wdt", O_RDWR);	//try another one if fail

	if (fd<0) 
	{
		//printf("watchdog device not found! Trying busybox reboot... \n");

		//system("rm /sbin/reboot;ln -s /bin/busybox /sbin/reboot;/sbin/reboot");
		system("ln -s /bin/busybox /tmp/reboot;/tmp/reboot");

		sleep(1);
		system("reboot");
	}
	else
	{
		ulTemp=1;		//let wdt timeout at once
		ret=ioctl(fd, WDIOC_SETTIMEOUT, &ulTemp);
		close(fd);

		sleep(1);
		system("reboot");
		
		for (;;);	//dead loop to waiting wdt reset
	}

	return ret;
}


#ifdef __cplusplus
}
#endif //__cplusplus

#ifdef SMAIN
//usage: reboot 51
int main(int argc,char *argv[])
{
	int ret=0;

	if (argc==1)
	{
		printf("usage: reboot reset_code[1:999] NOTE:0 is reserved!This is wdt reset!\n");
		usleep(200000);	//wait 200ms
		ret=WdtReboot(0);
	}
	else
	{
		printf("reboot %s\n",argv[1]);
		ret=WdtReboot(atoi(argv[1]));
	}

	return ret;
}

#endif	//#ifdef SMAIN

