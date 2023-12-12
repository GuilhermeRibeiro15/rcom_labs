/**
 * Example code for getting the IP address from hostname.
 * tidy up includes
 */

#include "getip.h"

int parseUrl(char * url, url_struct * parsedUrl) {
    char * ftp = strtok(url, "/");
    char * args = strtok(NULL, "/");
    char * path = strtok(NULL, "");

    if (ftp == NULL || args == NULL || path == NULL) {
        fprintf(stderr, "Invalid URL!\n");
        return -1;
    }

    if (hasLogin(args)) {
        char * login = strtok(args, "@");
        char * host = strtok(NULL, "@");

        char * user = strtok(login, ":");
        char * password = strtok(NULL, ":");

        strcpy(parsedUrl->user, user);

        if (password == NULL)
            strcpy(parsedUrl->password, "pass");
        else
            strcpy(parsedUrl->password, password);

        strcpy(parsedUrl->host, host);
    }
    else {
        strcpy(parsedUrl->user, "anonymous");
        strcpy(parsedUrl->password, "pass");
        strcpy(parsedUrl->host, args);
    }

    char * fileName = getFilename(path);
    strcpy(parsedUrl->path, path);
    strcpy(parsedUrl->fileName, fileName);

    if (!strcmp(parsedUrl->host, "") || !strcmp(parsedUrl->path, "")) {
        fprintf(stderr, "Invalid URL!\n");
        return -1;
    }

    struct hostent * h;

    if ((h = gethostbyname(parsedUrl->host)) == NULL) {  
        herror("gethostbyname");
        return -1;
    }

    char * host_name = h->h_name;
    strcpy(parsedUrl->host_name, host_name);

    char * ip = inet_ntoa(*((struct in_addr *)h->h_addr));
    strcpy(parsedUrl->ip, ip);
    printf("\nUser: %s\n", parsedUrl->user);
    printf("Password: %s\n", parsedUrl->password);
    printf("Host: %s\n", parsedUrl->host);
    printf("Path: %s\n", parsedUrl->path);
    printf("File name: %s\n", parsedUrl->fileName);
    printf("Host name  : %s\n", parsedUrl->host_name);
    printf("IP Address : %s\n\n", parsedUrl->ip);
    return 0;
}

char * getFilename(char * path) {
    char * filename = path, *p;
    for (p = path; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ':') {
            filename = p + 1;
        }
    }
    return filename;
}

int hasLogin(char * args) {
    return strchr(args, '@') != NULL ? 1 : 0;
}