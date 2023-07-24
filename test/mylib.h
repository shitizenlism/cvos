#ifndef _MYLIB_H
#define _MYLIB_H
#include <stdlib.h>
#include <errno.h>
 
#define error_report(msg) \
	do { perror(msg); \
		exit (EXIT_FAILURE) \
	}while (0)
 
//clean the stdin cach
void clean_cach (void);
 
//get a string from stdin
int getstring (char *str, int num);
 
/* define a get if info struct */
struct if_ipv4_info {
  char if_name[10];;
  unsigned char ip_addr[20];
  unsigned char hd_addr[20];
 
  struct if_ipv4_info *next;
};
#endif

