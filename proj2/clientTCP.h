#ifndef CLIENTTCP_H
#define CLIENTTCP_H

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <errno.h> 
#include <netdb.h> 
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include "getip.h"
#include "connection.h"

int download(url_struct url);

int saveFile(char* filename, int socketfd);

#endif // CLIENTTCP_H
