#ifndef PROXY_H
#define PROXY_H

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */

#define BACKLOG	20
#define BUF_SIZE	4096
#define LISTEN_PORT	8888

int threadCount = BACKLOG;
char badWords[200][30];
int totalbadWords;
void *client_handler(void *arg);
void initializeServer(int portNumber);
char *cached(char *hostname, char *path);
void newCacheFile(char *hostname, char *path, char *response);
int checkBlackList(char *hostname);
void readfromCache(int sock,char *hostname);
void censor(char *response);
void getBadwords();

#endif
