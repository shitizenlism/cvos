#ifndef __REBOOT_H__
#define __REBOOT_H__

#ifdef __cplusplus
       extern "C" {
#endif

#define	IPC_RESET_LOG		"/mnt/mtd/reset.log"
#define	IPC_RESET_LOG_SIZE	(100*1024)

void  FLASH_AddOneResetLog(char *line);
int WdtReboot(int ResetCode);
int WdtKeepAlive(void);

#define RESETCODE_FWUPDATE	10
#define RESETCODE_RESETCMD	11
#define RESETCODE_CONFCHANGE	12
#define RESETCODE_RESETKEY	13
#define RESETCODE_CMDLINE	14
#define RESETCODE_TASKDEAD	15
#define RESETCODE_LOSTCONTACT   16
#define RESETCODE_SysException	17
#define RESETCODE_LicenseErr	18

#ifdef __cplusplus
}
#endif 

#endif			//__REBOOT_H__

