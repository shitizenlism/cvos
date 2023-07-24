#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
 
#include "mylib.h"
 
#define DEBUG 0
 
struct if_ipv4_info *if_info = NULL;
 
/*
** add ip to a linklist
*/
static int  add_to_link (struct ifaddrs *ifap, char *buf)
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
      exit (EXIT_FAILURE);
    }
    strcpy (if_info->if_name, ifap->ifa_name);
    switch (ifap->ifa_addr->sa_family) {
      case AF_INET:
        strcpy (if_info->ip_addr, buf);
     //   printf ("head %s", if_info->ip_addr);
        break;
      case AF_PACKET:
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
        exit (EXIT_FAILURE);
      }
      strcpy (p->if_name, ifap->ifa_name);
      switch (ifap->ifa_addr->sa_family) {
        case AF_INET:
          strcpy (p->ip_addr, buf);
         // printf ( "else p %s\n", p->ip_addr);
          break;
        case AF_PACKET:
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
 
int prt_if_info (struct sockaddr *ifa_addr, char *addrbuf, int bufsize)
{
  memset (addrbuf, 0, bufsize);
  struct sockaddr_ll *s;
  int i;
  int len;
  int family = ifa_addr->sa_family;
 
  int ret = 0;
 
  switch (family) {
    case AF_INET: 
      inet_ntop (ifa_addr->sa_family, &((struct sockaddr_in *)ifa_addr)->sin_addr,
                  addrbuf, sizeof (struct sockaddr_in));
      break;
    case AF_PACKET:
      s = (struct sockaddr_ll *)ifa_addr;
      for (i=0,len=0; i<6; i++) {
        len += sprintf (addrbuf+len, "%02x%s", s->sll_addr[i], i<5?":":" ");
      }
      break;
    default:
      ret = -1;
      break;
  }
    return ret;
}
 
int get_if_info (void)
{
  struct ifaddrs *ifa, *ifap;
  int family;  /* protocl family */
 
  if (getifaddrs (&ifa) == -1) {
    perror (" getifaddrs\n");
    exit (EXIT_FAILURE);
  }
  char buf[20];
 
  for (ifap = ifa; ifap != NULL; ifap = ifap->ifa_next) {
    if (strcmp (ifap->ifa_name, "lo") == 0) {
      continue; /* skip the lookback card */
    }
 
    if (ifap->ifa_addr == NULL)
      continue; /* if addr is NULL, this must be no ip address */
 
    if (prt_if_info (ifap->ifa_addr, buf, 20) == 0) {
     // printf ("%s: %s\n", ifap->ifa_name, buf);
      add_to_link (ifap, buf);
    }
  }
  
  
  /* printf all of them */
  struct if_ipv4_info *p = NULL;
  for (p = if_info; p != NULL; p = p->next) {
    printf ("%s: %s\t%s\n", p->if_name, strlen(p->ip_addr)>0?p->ip_addr: "No ip addr", p->hd_addr);
  }
 
  /* free link list */
  struct if_ipv4_info *q = NULL;
  for (p = if_info; p != NULL; ) {
    q = p;
    p = p->next;
    free (q);
  } 
  freeifaddrs (ifa);
  return 0;
}
 
 
/*
** test code
*/
 
int main (int argc, char **argv)
{
  get_if_info ();
   return 0;
}

