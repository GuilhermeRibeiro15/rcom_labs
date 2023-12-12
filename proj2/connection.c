#include "connection.h"

int *socketFile;

/*função dada pelos professores*/
int createSocket(char * ip, int port) {
	struct sockaddr_in server_addr;
    int sockfd;
    bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Error opening socket!\n");
        return -1;
  }

	   if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

/*escrever comandos atualizado, ler ainda não percebi bem*/
int login(int socketFd, char * username, char * password) {
    char command[256];
    sprintf(command, "user %s\r\n", username);
    sendCommand(socketFd, command);
    if (readResponse() != 0) return 1;
    sprintf(command, "pass %s\r\n", password);
    sendCommand(socketFd, command);
	  if (readResponse() != 0) return 1;
    return 0;
}


int sendCommand(int socketfd, char * command){
  //printf(" about to send command: \n> %s", command);
  int sent = send(socketfd, command, strlen(command), 0);
  if (sent == 0){
    printf("sendCommand: Connection closed");
    return 1;
  }
  if (sent == -1){
    printf("sendCommand: error");
    return 2;
  }
  printf("> command sent\n");
  return 0;
}

int readResponse(){
  char * buf;
  size_t bytesRead = 0;

  while (1){
    getline(&buf, &bytesRead, socketFile);
    printf("< %s", buf);
    if (buf[3] == ' '){
      long code = strtol(buf, &buf, 10);
      if (code == 550 || code == 530)
      {
        printf("Command error\n");
        return 1;
      }
      break;
    }
  }
  return 0;
}

int readResponsePassive(char (*ip)[], int *port){
  char * buf;
	size_t bytesRead = 0;

  while (1){
    getline(&buf, &bytesRead, socketFile);
    printf("< %s", buf);
    if (buf[3] == ' '){
      break;
    }
  }

  strtok(buf, "(");       
  char* ip1 = strtok(NULL, ",");       // 193
  char* ip2 = strtok(NULL, ",");       // 137
  char* ip3 = strtok(NULL, ",");       // 29
  char* ip4 = strtok(NULL, ",");       // 15

  sprintf(*ip, "%s.%s.%s.%s", ip1, ip2, ip3, ip4);
  
  char* p1 = strtok(NULL, ",");       // 199
  char* p2 = strtok(NULL, ")");       // 78

  *port = atoi(p1)*256 + atoi(p2);
  
  return 0;
}
