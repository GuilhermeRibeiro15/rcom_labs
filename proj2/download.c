#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include "clientTCP.h"

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address to get IP address>\n", argv[0]);
        exit(-1);
    }

    url_struct url;

    if (parseUrl(argv[1], &url) < 0) {
        fprintf(stderr, "Error parsing url!\n");
        exit(2);
    }
    printf("will download");

    if (download(url) < 0) {
        fprintf(stderr, "Error downloading file!\n");
        exit(3);
    }

    return 0;
}
