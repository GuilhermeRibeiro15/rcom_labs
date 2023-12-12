#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <errno.h> 
#include <netdb.h> 
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "getip.h"

extern int * socketFile;

int createSocket(char * ip, int port);

int login(int socketFd, char * username, char * password);

int sendCommand(int socketfd, char * command);

int readResponse();

int readResponsePassive(char (*ip)[], int *port);

#endif 