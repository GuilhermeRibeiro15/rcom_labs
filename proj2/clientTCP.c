#include "clientTCP.h"

int download(url_struct url) {


    // Connect to socket
    int socketFd;

    if ((socketFd = createSocket(url.ip, 21)) < 0) {
        fprintf(stderr, "Error initializing connection!\n");
        return -1;
    }
    
    socketFile = fdopen(socketFd, "r");
	readResponse();

    if(login(socketFd, url.user, url.password)<0){
        fprintf(stderr, "Authentication failed!\n");
        return -1;
    }
    char command[256];
    char ip[32]; int port;
    sprintf(command, "pasv\r\n");
    sendCommand(socketFd, command);
        readResponsePassive(&ip, &port);
        printf("ip: %s\nport: %d\n", ip, port);

    int socketFd_rec;
    if ((socketFd_rec = createSocket(ip, port)) < 0) {
        fprintf(stderr, "Error initializing connection!\n");
        return -1;
    }
       


    sprintf(command, "retr %s\r\n", url.path);
    sendCommand(socketFd, command);
    if (readResponse() != 0) return 1;

    saveFile(url.fileName, socketFd_rec);
    return 0;
}

int saveFile(char* filename, int socketfd){
  printf("> filename: %s\n", filename);
  int filefd = open(filename, O_WRONLY | O_CREAT, 0777);
  if (filefd < 0){
    printf("Error: saveFile\n");
    return 1;
  }
  
  int bytes_read;
  char buf[1];
  do {
    bytes_read = read(socketfd, buf, 1);
    if (bytes_read > 0) write(filefd, buf, bytes_read);
  } while (bytes_read != 0);
  
  close(filefd);
  return 0;
}