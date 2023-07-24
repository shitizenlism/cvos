#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>  // htons 
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "vos.h"
#include "vos_test.h"

void test_msgpack(void)
{
	int ret=0,i;
	unsigned char msg[6]={1,2,3,4,5,6};
	unsigned char MsgBuf[128]={0};
	unsigned char ver=0;
	unsigned char sn=0;
	unsigned char body[100]={0};

	ret=MSG_PackHex(1,20,msg,6,MsgBuf);
	printf("packed.len=%d.msg[]=\n",ret);
	for (i=0; i<ret; i++)
		printf(" 0x%02x",MsgBuf[i]);
	printf("\n");

	ret=MSG_UnPackHex(&ver, &sn, body, MsgBuf);
	printf("unpack. len=%d\n",ret);
	if (ret>0){
		printf("ver=%d,sn=%d\n",ver,sn);
		printf("body=\n");
		for (i=0; i<ret; i++)
			printf(" 0x%02x",body[i]);
		printf("\n");
	}
}

void test_ringbuf(void)
{
	int ret=0;
	ringbuf r;
	datum ringbuf_arr[9], item;
	char tmp[10]="FuckIt";
	memset(&r, 0, sizeof(r));
	memset(ringbuf_arr[0].buf, 0, sizeof(ringbuf_arr));
	memset(item.buf, 0, sizeof(item));

	ringbuf_init(&r, VOS_itemof(ringbuf_arr), ringbuf_arr);

	strcpy((char *)item.buf, "tony123");
	ret=ringbuf_enqueue(&r, &item);
	printf("put one. ret=%d,num=%d\n", ret, r.num);
	
	memcpy(item.mBuf.data,tmp,7);
	item.mBuf.len=7;
	ret=ringbuf_enqueue(&r, &item);
	printf("put one. ret=%d,num=%d\n", ret, r.num);

	memset(item.buf, 0, sizeof(item));
	ret=ringbuf_dequeue(&r, &item);
	printf("get one. ret=%d,num=%d,buf=%s\n", ret, r.num, item.buf);
	
	memset(item.buf, 0, sizeof(item));
	ret=ringbuf_dequeue(&r, &item);
	printf("get one. ret=%d,num=%d,buf=%s\n", ret, r.num, item.buf);

	memset(item.buf, 0, sizeof(item));
	ret=ringbuf_dequeue(&r, &item);
	printf("get one. ret=%d,num=%d,buf=%s\n", ret, r.num, item.buf);

	ringbuf_fini(&r);
}

/*

*/
void test_json(void)
{
	
}

void test_utils(void)
{
	int ret = 0;
	char *if_name="eth9";
	char ip[32]={0};
	char mac[32]={0};
	char raw_mac[32]={0};

	ret = utl_IfGetInfo(if_name, ip, mac, raw_mac);
	printf("%s, ip=%s,mac=%s,raw_mac=%s\n", if_name, ip, mac, raw_mac);
}


int main(int argc, char** argv) 
{
//	test_ringbuf();
	//test_msgpack();

	test_utils();

	return 0;
}

