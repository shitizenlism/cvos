#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/watchdog.h>

#include <time.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>  // htons 
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "ghttp.h"
#include "http.h"

extern int Log_MsgLine(char *LogFileName,char *sLine);

#if 1
#define 	HttpPrint(fmt,args...)  do{ \
			char _PrtBuf[HTTP_MAX_PKT_LEN + 100]; \
			sprintf(_PrtBuf,":" fmt , ## args); \
			Log_MsgLine("http.log",_PrtBuf); \
			}while(0)
#else
#define 	HttpPrint(fmt,args...)  
#endif

int Http_Get(char *uri,char *outJson)
{
	int ret=0;
	
    ghttp_request *request = NULL;
    ghttp_status status=0;
    char *buf=NULL;
    int bytes_read=0;
	int methid=ghttp_type_get;
	int http_errno = 0;

	if ((!uri))
		return -1;

    request = ghttp_request_new();
    if(ghttp_set_uri(request, uri) == -1){
		ret=-2;goto End;
	}

    if(ghttp_set_type(request, methid) == -1){
		ret=-3;goto End;
	}

    ghttp_prepare(request);
    status = ghttp_process(request);
    if(status == ghttp_error){
		ret=-4;goto End;
	}

	http_errno = ghttp_status_code(request);
//    HttpPrint("http Status code -> %d\n", http_errno);
	if (http_errno != 200){
		ret=-http_errno; goto End;
	}

    buf = ghttp_get_body(request);
    bytes_read = ghttp_get_body_len(request);
	if (bytes_read>HTTP_MAX_PKT_LEN){
		ret=-5;goto End;
	}
	
//    HttpPrint("http response body[%d]=\n%s\n",bytes_read,buf);
	if ((outJson)&&(buf)){
		//strncpy(outJson,buf,MIN(strlen(outJson),bytes_read));	//can NOT cal strlen()
		strcpy(outJson,buf);
	}
	ret=bytes_read;

End:
	if (request)
		ghttp_request_destroy(request);

	if (ret<0)
		HttpPrint("%s(uri=%s),ret=%d\n",__FUNCTION__,uri,ret);
    return ret;
}


int Http_Post(char *uri,char *body,char *outJson)
{
	int ret=0;

#if 0
	char szXML[2048];
	char szVal[256];
 
	ghttp_request *request = NULL;
	ghttp_status status;
	char *buf;
	char retbuf[128];
	int len;
 
	strcpy(szXML, "POSTDATA=");
	sprintf(szVal, "%d", 15);
	strcat(szXML, szVal);
 
	printf("%s\n", szXML);		//test
 
	request = ghttp_request_new();
	if (ghttp_set_uri(request, uri) == -1)
		return -1;
	if (ghttp_set_type(request, ghttp_type_post) == -1)		//post
		return -1;
 
	ghttp_set_header(request, http_hdr_Content_Type,
			"application/x-www-form-urlencoded");
	
	//ghttp_set_sync(request, ghttp_sync); //set sync
	len = strlen(szXML);
	ghttp_set_body(request, szXML, len);	//
	ghttp_prepare(request);
	
	status = ghttp_process(request);
	if (status == ghttp_error)
		return -1;
	buf = ghttp_get_body(request);	//test
	sprintf(retbuf, "%s", buf);
	ghttp_clean(request);	
#endif

    ghttp_request *request = NULL;
    ghttp_status status=0;
    char *buf=NULL;
    int bytes_read=0;
	int methid=ghttp_type_post;

	if ((!uri)||(!body))
		return -1;
	
    request = ghttp_request_new();
    if(ghttp_set_uri(request, uri) == -1){
		ret=-2;goto End;
	}

    if(ghttp_set_type(request, methid) == -1){
		ret=-3;goto End;
	}
	ghttp_set_header(request, http_hdr_Content_Type,
			"application/x-www-form-urlencoded");
    if(ghttp_set_body(request, body,strlen(body)) == -1){
		ret=-4;goto End;
	}

    ghttp_prepare(request);
    status = ghttp_process(request);
    if(status == ghttp_error){
		ret=-5;goto End;
	}

//    HttpPrint("http Status code -> %d\n", ghttp_status_code(request));
    buf = ghttp_get_body(request);
    bytes_read = ghttp_get_body_len(request);
	if (bytes_read>HTTP_MAX_PKT_LEN){
		ret=-6;goto End;
	}
	
//    HttpPrint("http response body[%d]=\n%s\n",bytes_read,buf);
	if ((outJson)&&(buf)){
		//strncpy(outJson,buf,MIN(strlen(outJson),bytes_read));	//can NOT cal strlen()
		strcpy(outJson,buf);
	}
	ret=bytes_read;

End:
	if (request)
		ghttp_request_destroy(request);

	if (ret<0)
		HttpPrint("%s(uri=%s ,body[]=%s),ret=%d\n",__FUNCTION__,uri,body,ret);
    return ret;
}


/*
Content-Type: "application/json" or "application/x-www-form-urlencoded"
*/
int Http_Post2(char *uri,char *body,char *outJson, char *content_type)
{
	int ret=0;

    ghttp_request *request = NULL;
    ghttp_status status=0;
    char *buf=NULL;
    int bytes_read=0;
	int methid=ghttp_type_post;
	int http_errno=0;

	if ((!uri)||(!body))
		return -1;
	
    request = ghttp_request_new();
    if(ghttp_set_uri(request, uri) == -1){
		ret=-2;goto End;
	}

    if(ghttp_set_type(request, methid) == -1){
		ret=-3;goto End;
	}
	ghttp_set_header(request, http_hdr_Content_Type, content_type);
    if(ghttp_set_body(request, body,strlen(body)) == -1){
		ret=-4;goto End;
	}

    ghttp_prepare(request);
    status = ghttp_process(request);
    if(status == ghttp_error){
		ret=-5;goto End;
	}

	http_errno = ghttp_status_code(request);
//    HttpPrint("http Status code -> %d\n", http_errno);
	if (http_errno != 200){
		ret=-http_errno; goto End;
	}
	
    buf = ghttp_get_body(request);
    bytes_read = ghttp_get_body_len(request);
	if (bytes_read>HTTP_MAX_PKT_LEN){
		ret=-6;goto End;
	}
	
  //  HttpPrint("http response body[%d]=\n%s\n",bytes_read,buf);
	if ((outJson)&&(buf)){
		strcpy(outJson,buf);
	}
	ret=bytes_read;

End:
	if (request)
		ghttp_request_destroy(request);
	if (ret<0)
		HttpPrint("%s(uri=%s ,body[]=%s),ret=%d\n",__FUNCTION__,uri,body,ret);

    return ret;
}


