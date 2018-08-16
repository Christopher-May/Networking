/*
 * Christopher May
 * CJM325 11158165
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>

#define _GNU_SOURCE
#define BACKLOG   10 
#define LOCAL_PORT  "33333"

#define MAX_TEXT_LENGTH  256
#define MAX_RECEIVERS 10

typedef struct  
{
    int seq;
    int startSeq;
    char text[MAX_TEXT_LENGTH];
} transmission;
 
typedef struct  
{
    char ip[45];
    char port[6];
    int seq;
    int lastAwk;
    transmission *window;
    int numWindow;
} receiver ;

receiver recvs[MAX_RECEIVERS];
int numReceivers = 0;
int max_window_size;
int base;

/* tokens string into array of string */
int tokenize (char* sp, char *ip, char *port){
	char s[300];
	int i=0;
	char *tokens[2];
	
	for(int i =0; i<=2; i++){
		tokens[i] = malloc(sizeof(char)*128);
	}	
	
	strncpy(s,sp,sizeof(s));
	s[sizeof(s)-1] = '\0';

	/* magic of strtok */
	char * token = strtok(s, " ");
	while(token != NULL){	
		strcpy(tokens[i++],token);
		token = strtok(NULL," ");
	}
	
	strcpy(ip,tokens[0]);
	strcpy(port,tokens[1]);
	
	return 1;
}

/* get replies from the receiver*/
int getReplies(int *replySeq, int recvfd, struct addrinfo *addr,struct timeval *timeout){
    int numbytes;
    int rv;
    int maxfd;
    fd_set readfds;
    /*set ip select*/
    FD_ZERO(&readfds);
    FD_SET(recvfd, &readfds);
    
    maxfd = recvfd +1;
    if( (rv =select(maxfd,&readfds,NULL,NULL,timeout)) == -1){
        perror("recvfrom");
        exit(1);
    }
    /* if something is broken */
    if( FD_ISSET(recvfd,&readfds) ){
        if ((numbytes = recvfrom(recvfd, (char *)replySeq, sizeof(*replySeq) , 0, addr->ai_addr, &addr->ai_addrlen)) == -1){
            perror("recvfrom");
            exit(1);
        }
    }
    /* we timed out */
    else if(rv == 0){
        printf("Timed Out \n");
        return -1;
    }
    /* actually got a reply */
    return 1;
}
/* connects to receiver then sends transmission and then deals with the response if any */
void handle(int r,transmission * tran, struct timeval *timeout){
    int recvfd;
    struct addrinfo hints;
    struct addrinfo *serv_info;
    struct addrinfo *p;
    int rv;
    int *replySeq;
    int result=-1;
    int numBytes;
    int i = 0;
    int j = 0;
    int rp;
    int remainder = 0;
    
    
    replySeq = malloc( sizeof(*replySeq));
    
    /* connect */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; 
    
    /* Get the receiver's address information */
    if ((rv = getaddrinfo(recvs[r].ip, recvs[r].port, &hints, &serv_info)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        
        exit(1);
    }

    /* Loop through the results and make the first available socket */
    for(p = serv_info; p != NULL; p = p->ai_next){
        if ((recvfd = socket(p->ai_family, p->ai_socktype,
                              p->ai_protocol)) == -1)
        {
            perror("sender: socket");
            
            continue;
        }

        break;
    }

    if (p == NULL){
        fprintf(stderr, "sender: failed to create socket\n");
        exit(1);
    }
    /* send transaction */
    if ( (numBytes = sendto(recvfd, tran, sizeof(transmission), 0, p->ai_addr, p->ai_addrlen)) == -1 ){
        perror("recvfrom");
        exit(1);
    }
    
    /* Get a reply from the receiver */
    result = getReplies(replySeq, recvfd, p, timeout);
    /* if we actually got a reply update window */
    if ( result>0){
        rp = *replySeq;

        /* find where response is bigger */
        for(i=0; i < recvs[r].numWindow; i++){

            if (recvs[r].window[i].seq > rp){
                break;
            }
            else{
                memset(&recvs[r].window[i], 0, sizeof(transmission));
                
                remainder++;
            }
        }
        /*shift the rest to the beginning*/
        for(i = i; i<recvs[r].numWindow; i++){
            recvs[r].window[j].seq = recvs[r].window[i].seq;
            strcpy(recvs[r].window[j].text,recvs[r].window[i].text);
            j++;
        }
        
        recvs[r].numWindow -= remainder;
        
        if (rp < tran->seq && (recvs[r].numWindow == 0 || recvs[r].window[recvs[r].numWindow-1].seq < tran->seq)){
            memcpy(&recvs[r].window[recvs[r].numWindow], tran, sizeof(transmission));
            
            recvs[r].numWindow++;
        }
        
        /* Update the last reply seqnum */
        if (rp > recvs[r].lastAwk){
            recvs[r].lastAwk = rp;
        }
        
    }
    /* no reply so we add transaction to window */
    else{
        *replySeq = -1;
        if (recvs[r].numWindow == 0 || recvs[r].window[recvs[r].numWindow-1].seq < tran->seq){
            memcpy(&recvs[r].window[recvs[r].numWindow], tran, sizeof(transmission));
            recvs[r].numWindow++;
       }
    }
    
    printf("Window %i: %s:%s response: %i \n \t",r,recvs[r].ip, recvs[r].port, *replySeq);
    for (j = 0; j< recvs[r].numWindow; j++){
        printf(" [ %i ] ", recvs[r].window[j].seq);
    }
    printf("\n");
    
    close(recvfd);
}

/* goes through the receivers and manages the transmissions to send them */
void processRecievers(transmission *tran, int t, int seqNum ){
    char *buf;
    size_t len = 0;
    int overflow =1;
    struct timeval *timeout;
    int i=0;
    
    timeout = malloc(sizeof(struct timeval));
    buf = malloc(sizeof(char)*256);
    
    /* keep looping until no receiver has a full window */
    while (1){
        
        overflow =1;
        
        /* go through all recvs */
        for(int r=0; r<numReceivers; r++){
            /* find recvs with max buffer */
            if(recvs[r].numWindow >= max_window_size){
                /* resend */
                for (i = 0; i < recvs[r].numWindow; i++){
                    timeout->tv_usec = 0;
                    timeout->tv_sec = t;
                    handle(r,&recvs[r].window[i], timeout);
                    overflow =0;    

                }
            }
        }
        /* if we didn't have to resend we can send a new transmission */
        if(overflow == 1){
            break;   
        }
    }
    /* get transmission from user */
    printf("Enter a transmission: \n");
    getline(&buf, &len, stdin);
    /* if user wants to quit */
    if( strcmp(buf,"QUIT\n") ==0 ){
        printf("Success \n");
        
        /* loop until there are no transactions in any window */
        while (1){
            overflow =1;

            /* go through all recvs */
            for(int r=0; r<numReceivers; r++){
                /* find recvs with max buffer */
                if(recvs[r].numWindow > 0){
                    /* resend */
                    for (i = 0; i < recvs[r].numWindow; i++){
                        timeout->tv_usec = 0;
                        timeout->tv_sec = t;
                        handle(r,&recvs[r].window[i], timeout);
                        overflow =0;    

                    }
                }
            }
            /* if we didn't have to resend we can send a new transmission */
            if(overflow == 1){
                break;   
            }
        }
        /* wait to startup again */
        printf("continue... \n");
        getline(&buf, &len, stdin);
    }

    tran->seq = seqNum;
    tran->startSeq = base;
    memcpy(tran->text, buf, MAX_TEXT_LENGTH);
    /* send it to all receivers as well as the contents of their window */
    for(int r=0; r<numReceivers; r++){
        for (i = 0; i < recvs[r].numWindow; i++){

            timeout->tv_usec = 0;
            timeout->tv_sec = t;
            handle(r,&recvs[r].window[i], timeout);
            overflow =0;    

        }
        timeout->tv_usec = 0;
        timeout->tv_sec = t;
        handle(r, tran, timeout);
    }

    free(buf);
}

int main(int argc, char **argv){
    int timeout_sec;
    int seqNum = 0; 
    transmission *tran;
    int i=0;
    
    FILE * fp;
    char * line = NULL;
    size_t l = 0;
    ssize_t read;

    /* Get the receiver host and port as well as window size and timeout from the command line */
    if (argc < 4){
        fprintf(stderr, "<max window size> <starting number> <timeout>\n");
        
        exit(1);
    }
    max_window_size = atoi(argv[1]);
    base = atoi(argv[2]);
    timeout_sec = atoi(argv[3]);
    /* set the starting seqNum */
    
    seqNum = base;
    
    /* not useful now, if I looped through a set instead of just increasing seq it would be though */
    if (max_window_size < seqNum){
    /*    printf("win size should be less the seq num \n"); */
    }
    
    /*make space*/
    for(i=0; i< MAX_RECEIVERS; i++){
            recvs[i].window = calloc(max_window_size, sizeof(transmission));
            recvs[i].lastAwk = UINT32_MAX;
            recvs[i].numWindow = 0;

    }
    
    /* read receivers */
    fp = fopen("receivers.txt", "r");
    if (fp == NULL){
        exit(1);
    }
    
    i=0;
    while ((read = getline(&line, &l, fp)) != -1) {
        strtok(line, "\n"); /* get rid of newline */
        tokenize(line,recvs[i].ip,recvs[i].port); /* tokenize ip and port */
        printf("%s : %s:%s\n", line,recvs[i].ip,recvs[i].port);
        i++;
    }
    numReceivers = i;
    fclose(fp);
    /* check if we have to many receivers, if so tell user about it */
    if (numReceivers >MAX_RECEIVERS){
        printf("To many receivers, either remove some or change MAX_RECEIVERS \n");
        exit(1);
    }
    printf(" initialized \n");
    
    while (1){   

        /*make transmission*/
        tran = malloc(sizeof(transmission));
        
        processRecievers(tran,timeout_sec,seqNum);
        seqNum++; 
    }
 
    return 0;
}