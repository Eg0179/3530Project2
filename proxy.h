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

#define BACKLOG	5
#define BUF_SIZE	4096
#define LISTEN_PORT	8888

int threadCount = BACKLOG;
void *client_handler(void *arg);
void initializeServer(int portNumber);
char *cached(char *hostname);
void newCacheFile(char *hostname, char *response);

#endif
