#include "proxy.h"

int main(int argc, char *argv[]){
    if (argc !=2){
        return 1;
    }
    getBadwords();
    initializeServer(atoi(argv[1]));//initialize server
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


    sock_listen = socket(AF_INET, SOCK_STREAM, 0);//create socket
    if (sock_listen < 0) {
        perror("socket() failed");
        exit(0);
    }

    memset(&addr_mine, 0, sizeof (addr_mine));//set struct fields
    addr_mine.sin_family = AF_INET;
    addr_mine.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_mine.sin_port = htons(port);

    if (bind(sock_listen, (struct sockaddr *) &addr_mine, sizeof(addr_mine)) <0) {//bind
        perror("bind() failed");
        close(sock_listen);
        exit(1);
    }

    status = listen(sock_listen, 5);//listen
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
        
    	sock_aClient = accept(sock_listen, (struct sockaddr *) &addr_remote,&addr_size);//accept incoming connections
    	if (sock_aClient == -1){
    		close(sock_listen);
        	exit(1);
    	}
    	sock_tmp = malloc(1);
    	*sock_tmp = sock_aClient;//pointer to socket
    	printf("thread count = %d\n", threadCount);
    	threadCount--;
 		status = pthread_create(&a_thread, NULL, client_handler,(void *) sock_tmp);//create thread 

 		if (status != 0) {
 			perror("Thread creation failed");
 			close(sock_listen);
 			close(sock_aClient);
 			free(sock_tmp);
        	exit(1);
 		}
    }
}

void *client_handler(void *sock_desc) {//runs when thread is created
	int msg_size,i,j,bytes,sent,received;
	char buf[BUF_SIZE];
    char hostname[500];
    char path[500];
    char httprequest[500];
    char request[BUF_SIZE];
	int sock = *(int*)sock_desc;
    struct hostent *ser;
    struct sockaddr_in addr;
    int ret =1;

	printf("In client_handler\n");
    memset(buf,0,BUF_SIZE);
	while((msg_size = recv(sock, buf, BUF_SIZE, 0)) > 0){//recieve message from web browser

        memset(httprequest, 0, 500);
        memset(hostname, 0, 500);
        printf("Received: %s\n", buf);
        i=0;
        while(buf[i] != '\n'){//parse the http request
            httprequest[i]=buf[i];
            i++;
        }
        i=5;
        if(hostname[0] == '\0'){//get domain
        for(i=5,j=0; i<strlen(httprequest); i++){
            if(isspace(httprequest[i]) || httprequest[i]=='/'){
               break;
            }
            hostname[j]=httprequest[i];
                j++;
        }
        }
        if(httprequest[i]=='/'){//get path
            for(i,j=0; i<strlen(httprequest); i++){
                if(isspace(httprequest[i])){
                    break;
                }
                path[j]=httprequest[i];
                j++;
            }
        }
        else{
            path[0]='/';
        }
        //output to screen
        printf("httprequest:%s\n", httprequest);
        printf("hostname:%s\n", hostname);
        printf("path:%s\n",path);
        memset(request, 0, BUF_SIZE);

        //check for favicon
        if(strstr(httprequest, "favicon")!=NULL){
            strcpy(request, "HTTP/1.0 404 Not Found\r\n");
            send(sock,request,strlen(request),0);
            break;
        }
        //check if URL is blacklisted
        if(checkBlackList(hostname)){
            strcpy(request, "HTTP/1.0 200 OK\r\nContent-Type: text/html\n\n<html>\n<head>\n<title>Success</title>\n</head>\n<body>\nBlocked\n</body>\n</html>\n");
            send(sock, request, strlen(request),0);
            break;
        }
        //check if website is cached
        char *req = cached(hostname, path);
        if(req == NULL){//if not create a connection to domain and request http
            free(req);     
            int newSock = socket(AF_INET, SOCK_STREAM, 0);//create socket

            ser = gethostbyname(hostname);//get hostname
            if(ser==NULL){
                printf("Error no such host\n");
                close(newSock);
                pthread_exit(&ret);
            }
            printf("%s\n", ser->h_name);

            //set struct fields
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            memcpy(&addr.sin_addr.s_addr, ser->h_addr_list[0], ser->h_length);
            addr.sin_port = htons(80);

            //connect to server
            if(connect(newSock,(struct sockaddr *) &addr, sizeof(addr))<0)
                printf("error connecting\n");
            else
                printf("connected\n");

            memset(httprequest,0,500);
            sprintf(httprequest, "GET %s HTTP/1.0\nHost:%s\n\n",path,hostname);
            printf("\n%s", httprequest);
            
            //send HTTP request
            if(send(newSock,httprequest, strlen(httprequest),0)<0){
                printf("Error sending\n");
                close(newSock);
                pthread_exit(&ret);
            }
            //receive http censor & write to file
            while(recv(newSock,request,BUF_SIZE,0)>0){
                //censor(request);
                newCacheFile(hostname, path, request);

                memset(request,0,BUF_SIZE);
            }
            printf("DONE RECEIVING\n");
            close(newSock);
        }
        //send http back to web browser
        readfromCache(sock, hostname);

        printf("done sending\n");     
        memset(buf,0,BUF_SIZE);

    }
    //close connections and exit thread
    printf("Connection closed\n");
    close(sock);
	free(sock_desc);
	threadCount++;
    return NULL;
}

//checks if the website is already cached
char *cached(char *hostname, char *path){
    char *cachefile = (char *) malloc((strlen(hostname))+(strlen(path))+5);
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

//create a new cache file 
void newCacheFile(char *hostname, char *path,char *response){
    char *cachefile = (char *)malloc((strlen(hostname))+(strlen(path))+5);
    sprintf(cachefile, "%s.txt", hostname);

    FILE *f = fopen(cachefile, "a");//"a" append to file

    fprintf(f, "%s\n", response);
    fclose(f);
}

//check if the URL is blacklisted
int checkBlackList(char *hostname){
    FILE *f = fopen("blacklist.txt", "r");
    char tmp[60];
    while(fgets(tmp,60,f)!=NULL){
        if(strstr(tmp,hostname)!=NULL){
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

//read cached file and send it to the web browser
void readfromCache(int sock, char *hostname){
    char *cachefile = (char *)malloc(strlen(hostname)+5);
    sprintf(cachefile, "%s.txt", hostname);

    FILE *f = fopen(cachefile, "r");
    char tmp[60];
    while(fgets(tmp,60,f)!=NULL){
        send(sock, tmp, strlen(tmp), 0);
        memset(tmp,0,60);
    }
    fclose(f);
}

//read curse words from file and change them to * if they appear
void censor(char *response){
    int i;
    char bufc[BUF_SIZE],star[22];

    strcpy(bufc, response);
    //FILE *f = fopen("badWords.txt", "r");
    char tmp[60]= "";

    printf("CENSORING\n");
    /*
    while(fgets(tmp,60,f)!=NULL){
        memset(star,0,22);
        tmp[strlen(tmp)-1] = '\0';
        char *found = strstr(bufc, tmp);//strstr returns the position of the found word
        if(found != NULL){
            for(i=0; i<strlen(tmp);i++){
            strcat(star, "*");
            
            }
            strncpy(found, star,strlen(star));//censor the word with ****
            //censor(bufc);
        }
    }*/
    for(int i = 0; i< totalbadWords; i++){
        char *found = strstr(response, badWords[i]);
    }
    strcpy(response, bufc);
    //fclose(f);
    printf("ok done censoring\n");

}
void getBadwords(){
    FILE *f = fopen("badWords.txt", "r");
    char tmp[30];
    int i =0;
    while(fgets(tmp,30,f)!=NULL){
        tmp[strlen(tmp)-1]= '\0';
        strcpy(badWords[i], tmp);
        i++;

    }
    totalbadWords =i;
    fclose(f);

}
