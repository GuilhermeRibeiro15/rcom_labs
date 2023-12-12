#ifndef GETIP_H
#define GETIP_H

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

typedef struct {
    char host[128];
    char user[128];
    char password[256];
    char path[240];
    char fileName[128];
    char host_name[128];
    char ip[128];
} url_struct;

int parseUrl(char * url, url_struct * parsedUrl);

char * getFilename(char * path);

int hasLogin(char * args);

#endif // GETIP_H
