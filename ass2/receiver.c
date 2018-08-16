/*
 * Christopher May
 * CJM325 11158165
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <limits.h>

#define MAX_TEXT_LENGTH  256

typedef struct  
{
    int seq;
    int startSeq;
    char text[MAX_TEXT_LENGTH];
} transmission;

int currSeq = -1;

void *get_in_addr(struct sockaddr *sa)
{
    /* IPv4 case */
    if (sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    /* IPv6 case */
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]){
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *serv_info;
    struct addrinfo *p;
    int rv;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    
    double lossProbRange;
    int numBytes;
    transmission  *tran;
    char *buf;
    size_t len = 0;
    
    /*allocate mem*/
    tran = malloc(sizeof(transmission));
    
    /*check if we got the right amount of variables*/
    if (argc < 3){
        printf("<portnumber> <probability of loss>\n");
        exit(1);
    }
    /*set probabilty of loss*/
    lossProbRange = atof(argv[2]);
    
    if(lossProbRange > 1 || lossProbRange < 0){
        printf("we want probability to be between 0-1\n");
        exit(1);
    }
    /* socket setup */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; 
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &serv_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        
        exit(1);
    }
    
    for(p = serv_info; p != NULL; p = p->ai_next){
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
        {
            perror("UDP server: socket");
            
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("UDP server: bind");
            
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "UDP server: failed to bind socket\n");
        
        return 2;
    }
    
    addr_len = sizeof(their_addr);

    freeaddrinfo(serv_info);
    
    printf("receiver: waiting to recvfrom\n");
    
    while (1){
       
        /* Recv mesg */
        if ((numBytes = recvfrom(sockfd, tran, sizeof(transmission ) , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1){
            perror("recvfrom");      
            exit(1);
        }

        /* Print it */
        printf("\n Received transmission %i: %s \n", tran->seq, tran->text);

        if(currSeq == -1){
            currSeq = tran->startSeq -1;
        }

        /* check if we lost it */
        if( !( ((double)rand()/(double)RAND_MAX) < lossProbRange) ){
            /* we got an old transmission */
            if (currSeq == tran->seq){
                
                /* ask if you want to resend the transmission */
                printf("Previously received transmission do you want it? (y/n)\n");
                getline(&buf, &len, stdin);
                
                /* lost it */
                if (buf[0] == 'n'){
                    printf("Ack was corrupted by command\n");

                }
                /*resend awk*/
                else if(buf[0] == 'y') {
                    currSeq = tran->seq;

                    if ( (numBytes = sendto(sockfd, (char *)&currSeq, sizeof(currSeq), 0,(struct sockaddr *) &their_addr, addr_len)) < 0) {
                       perror("sendto");
                       exit(1);
                    }

                    printf("Ack sent\n");                   
                }
                /*whatcha doin?*/
                else{
                    printf("Wrong Command, the commands are(y/n). For a punishment I corrupted the transmission \n");
                }
            }
            /* next transmission */
            else if ( ( (currSeq+1) == tran->seq) ){
                
                printf("The next transmission do you want it? (y/n)\n");
                getline(&buf, &len, stdin);

                /* lost it */
                if (buf[0] == 'n'){
                    printf("Ack was corrupted by command\n");

                }
                /* send awk */
                else if(buf[0] == 'y') {
                    currSeq = tran->seq;

                    if ( (numBytes = sendto(sockfd, (char *)&currSeq, sizeof(currSeq), 0,(struct sockaddr *) &their_addr, addr_len)) < 0) {
                       perror("sendto");
                       exit(1);
                    }

                    printf("Ack sent\n");                   
                }
                /* whatcha doin?! */
                else{
                    printf("Wrong Command, the commands are(y/n). For a punishment I corrupted the transmission \n");
                }
            /* the transmission is out of order */
            }
            else{
                printf("This transmission is an out of order. Do nothing \n");
            }
        }
        /* lost it */
        else{
            printf("Ack was corrupted by probablity \n");
        }
        /* clean up */
        memset(tran, 0, sizeof(transmission ));
        memset(buf, 0, len);
    }
    
    close(sockfd);
    
    return 0;
}