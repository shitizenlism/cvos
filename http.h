#ifndef _HTTP_H_
#define _HTTP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define HTTP_MAX_PKT_LEN 2000

int Http_Get(char *uri,char *outJson);
int Http_Post(char *uri,char *body,char *outJson);
/*
Content-Type: "application/json" or "application/x-www-form-urlencoded"
*/
int Http_Post2(char *uri,char *body,char *outJson, char *content_type);


#ifdef __cplusplus
}
#endif

#endif
