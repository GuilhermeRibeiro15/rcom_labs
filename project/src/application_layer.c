// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename){
    LinkLayer linkLayer;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    strcpy(linkLayer.serialPort, serialPort);

    if(strcmp(role, "tx") == 0){
        linkLayer.role = LlTx;
        int open = llopen(linkLayer);
    }
    else if(strcmp(role, "rx") == 0){
        linkLayer.role = LlRx;
        int open = llopen(linkLayer);
    }
    else{
        printf("Invalid role\n");
        return;
    }

}
