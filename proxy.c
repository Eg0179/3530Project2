#include "proxy.h"

int main(int argc, char *argv[]){
    if (argc !=2){
        return 1;
    }
    initializeServer(atoi(argv[1]));
    return 0;
}    
void initializeServer(int port){
    int status, *sock_tmp;
    pthread_t a_thread;
    void *thread_result;

    struct sockaddr_in addr_mine;
    struct sockaddr_in addr_remote;
    int sock_listen;
    int sock_aClient;
    int addr_size;
    int reuseaddr = 1;


    sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listen < 0) {
        perror("socket() failed");
        exit(0);
    }

    memset(&addr_mine, 0, sizeof (addr_mine));
    addr_mine.sin_family = AF_INET;
    addr_mine.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_mine.sin_port = htons(port);

    if (bind(sock_listen, (struct sockaddr *) &addr_mine, sizeof(addr_mine)) <0) {
        perror("bind() failed");
        close(sock_listen);
        exit(1);
    }

    status = listen(sock_listen, 5);
    if (status < 0) {
        perror("listen() failed");
        close(sock_listen);
        exit(1);
    }

    addr_size = sizeof(struct sockaddr_in);
    printf("waiting for a client\n");

    while(1) {
    	if (threadCount < 1) {
    		sleep(1);
    	}
        
    	sock_aClient = accept(sock_listen, (struct sockaddr *) &addr_remote,&addr_size);
    	if (sock_aClient == -1){
    		close(sock_listen);
        	exit(1);
    	}

    //	printf("Got a connection from %s on port %d\n",inet_ntoa(addr_remote.sin_addr),htons(addr_remote.sin_port));
    	sock_tmp = malloc(1);
    	*sock_tmp = sock_aClient;
    	printf("thread count = %d\n", threadCount);
    	threadCount--;
 		status = pthread_create(&a_thread, NULL, client_handler,(void *) sock_tmp);// returns 0 on success

 		if (status != 0) {
 			perror("Thread creation failed");
 			close(sock_listen);
 			close(sock_aClient);
 			free(sock_tmp);
        	exit(1);
 		}
    }
}

void *client_handler(void *sock_desc) {
	int msg_size,i,j,bytes,sent,received;
	char buf[BUF_SIZE];
    char hostname[500];
    char httprequest[500];
    char request[BUF_SIZE];
	int sock = *(int*)sock_desc;
    struct hostent *ser;
    struct sockaddr_in addr;
    int ret =1;

	printf("In client_handler\n");
    memset(buf,0,BUF_SIZE);
	while((msg_size = recv(sock, buf, BUF_SIZE, 0)) > 0){

        memset(httprequest, 0, 500);
        memset(hostname, 0, 500);
        //buf[msg_size] = '\0';
        printf("Received: %s\n", buf);
        i=0;
        while(buf[i] != '\n'){
            httprequest[i]=buf[i];
            i++;
        }
        for(i=5,j=0; i<strlen(httprequest); i++){
            if(isspace(httprequest[i])){
               break;
            }
            hostname[j]=httprequest[i];
                j++;
        }

        printf("httprequest:%s\n", httprequest);
        printf("hostname:%s\n", hostname);

        memset(request, 0, BUF_SIZE);
        char *req = cached(hostname);
        if(req == NULL){
            free(req);     
            int newSock = socket(AF_INET, SOCK_STREAM, 0);
            ser = gethostbyname(hostname);
            if(ser==NULL){
                printf("Error no such host\n");
                close(newSock);
                pthread_exit(&ret);
            }

            printf("%s\n", ser->h_name);
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            memcpy(&addr.sin_addr.s_addr, ser->h_addr_list[0], ser->h_length);
            addr.sin_port = htons(80);

            if(connect(newSock,(struct sockaddr *) &addr, sizeof(addr))<0)
                printf("error connecting\n");
            else
                printf("connected\n");

            memset(httprequest,0,500);
            sprintf(httprequest, "GET http://%s HTTP/1.0\n\n",hostname);
            printf("\n%s", httprequest);

            if(send(newSock,httprequest, strlen(httprequest),0)<0){
                printf("Error sending\n");
                close(newSock);
                pthread_exit(&ret);
            }

            if(recv(newSock, request, BUF_SIZE,0) <0){
                printf("No response\n");
                close(newSock);
                pthread_exit(&ret);
            }
            close(newSock);
            newCacheFile(hostname, request);
        
            if(send(sock, request, BUF_SIZE,0)<0){
                printf("Error sending to client\n");
            }
        }
        else{
            if(send(sock, req, BUF_SIZE,0)<0){
                printf("Error sending to client\n");
            }
        }

        printf("done sending\n");     
        memset(buf,0,BUF_SIZE);

    }
    printf("Connection closed\n");
    close(sock);
	free(sock_desc);
	threadCount++;
    return NULL;
}

char *cached(char *hostname){
    char *cachefile = (char *) malloc(strlen(hostname)+5);
    sprintf(cachefile, "%s.txt", hostname);

    FILE *f = fopen(cachefile, "r");
    if(f!= NULL){
        char *buffer = (char *)malloc(BUF_SIZE);
        char tmp[60];
        while(fgets(tmp,60,f)!= NULL){
            strcat(buffer, tmp);
        }
        fclose(f);
        return buffer;
    }
    else{
        return NULL;
    }
}
void newCacheFile(char *hostname, char *response){
    char *cachefile = (char *)malloc(strlen(hostname)+5);
    sprintf(cachefile, "%s.txt", hostname);

    FILE *f = fopen(cachefile, "w");

    fprintf(f, "%s\n", response);
    fclose(f);
}
