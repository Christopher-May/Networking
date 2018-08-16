/*
 * Christopher May 11158165 cjm325
 * assignment 3 CMPT434
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define BACKLOG  1
#define PORT "3491"
#define MAXDATASIZE 300

int version     =0;
int running     =1;
struct timeval *timeout;
int speed       =1;
int pos         =0;
int maxPos      =0;

int *offset[3];
char *files[3];
FILE * fp[3];

/* stuff from beejs */
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int tokenize (char* sp, char *tokens[]){
	char s[300];
	int i=0;
	strncpy(s,sp,sizeof(s));
	s[sizeof(s)-1] = '\0';

	/* magic of strtok */
	char * token = strtok(s, " ");
	while(token != NULL){	
		strcpy(tokens[i++],token);
		token = strtok(NULL," ");
	}
	
	return 1;
}
/* take array of string in format [COMMAND] [PARAMS] */
int doCommand(char* tokens[]){
    printf("Server: executing command %s \n",tokens[0]);
    int n;
    
    if(strcmp(tokens[0],"pause") ==0){
        running =0;
        printf("paused \n");
    }
    else if(strcmp(tokens[0],"resume") ==0){
        running =1;
        printf("resumed \n");
    }
    else if(strcmp(tokens[0],"rate") ==0){
        n = atoi(tokens[1]);
        if (n >0){
            speed = n;
        }
        printf("speed changed to %s \n",tokens[1]);
    }
    else if (strcmp(tokens[0],"version") ==0){
        n = atoi(tokens[1]);
        if(n >0 && n <4){
            version = n;
        }
        printf("version changed to %s \n",tokens[1]);
    }
    else if(strcmp(tokens[0],"pos") ==0){
        n = atoi(tokens[1]);
        if (n >= 0 && n < maxPos+1){
            pos = n;
        }
        printf("pos changed to %s \n",tokens[1]);
    }
    else{
        printf("command %s was not found \n",tokens[0]);
        return -1;
    }
    return 1;
    
}
/* recieve from from send to to */
int recvControl(int from){
    char buf[MAXDATASIZE];
    char *b;
    int numbytes =0;
    char *tk[3];
    
    /*allocate*/
    for(int i =0; i<3; i++){
         tk[i] = malloc(sizeof(char)*MAXDATASIZE);
    }
    b = malloc(sizeof(char)*MAXDATASIZE);
    /*recv command*/
    if((numbytes = recv(from, buf, MAXDATASIZE-1, 0)) == -1){

        perror("recv");
        return -1;
    }
    /* this should be fixed, there is a better way to do this */
    buf[numbytes-1] = '\0';
    strcpy(b,buf);
    /*tokenize the command*/
    strtok(b, "\n");
    tokenize(b,tk);
    /* do the command */
    doCommand(tk);
    
    return 0;
    
}
/* Socket connection TCP stuff, basically what is shown on Beejs guide */
int connectTo(char* port){
    int sockfd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
	return sockfd;
}
/* handles the logic to send and recv in the correct timing */
int handle(fd_set set, int display, int control, int maxfd, char* msg,int len){
    /* select stuff */    
    if (select(maxfd + 1, &set, NULL, NULL, timeout) == -1) {
            perror("select");
            return -1;
        }
        /* check if we got a message */
        if (FD_ISSET(control, &set)) {
            recvControl(control);
        }
        /*when time is up send line */
        else{
            if(running == 1){
                if (send(display,msg,len, 0) == -1){
                    perror("send");
                }
            }

        }
        
    return 1;
}
/* send lines to handle to send */
int sending(fd_set set,int display, int control, int maxfd){
    char* line;
    size_t len = 0;
    ssize_t read =0;
    int current = version;
    
    
    while (read != -1) {
        if(running ==1){
            read = getline(&line, &len, fp[current]);
        }
        if(strcmp(line,"\n")==0){
            pos++;
            continue;
        }
        timeout->tv_sec = speed;	
        printf("%s \n",line);
        handle(set,display,control,maxfd,line,len);
    }
    return 1;
}
int main(int argc, char **argv){
    int sockfd,sockfd2,control,display;	
    int connected;
    fd_set master,set;
    struct sockaddr_in their_addr;
    struct sockaddr_in their_addr2;
    socklen_t addr_len;
    socklen_t addr_len2;
    char *tk[3];
    FILE * f;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int maxfd;
    int i=0;
    
    /* allocate and set variables*/
    for(int i =0; i<3; i++){
            tk[i] = malloc(sizeof(char)*MAXDATASIZE);
    }
    
    addr_len = sizeof(their_addr);
    addr_len2 = sizeof(their_addr);
    timeout = malloc(sizeof(struct timeval));

    if (argc < 5) {
        printf(" [largest] [mediumest] [smallest] [ chunk file] \n");
        return 1;
    }
    files[0] = argv[1];
    files[1] = argv[2];
    files[2] = argv[3];
    
    f = fopen(argv[4], "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, f)) != -1) {
        
        maxPos++;
    }
    fclose(f);
    
    for(i=0;i<3;i++){
        offset[i]= malloc(maxPos *sizeof(int));
    }
    
    f = fopen(argv[4], "r");
    if (f == NULL)
        exit(EXIT_FAILURE);
    i=0;
    while ((read = getline(&line, &len, f)) != -1) {
        tokenize(line,tk);
        offset[0][i]    = atoi(tk[0]);
        offset[1][i]    = atoi(tk[1]);
        offset[2][i]    = atoi(tk[2]);
        i++;
    }
    fclose(f);
    
    for(i=0; i<3;i++){
        fp[i] = fopen(files[i], "r");
        if (fp[i] == NULL)
            exit(EXIT_FAILURE);
    }
    

    /* connect */
    sockfd = connectTo("3491");
    sockfd2 = connectTo("3490");
    /* Main loop */
   while (1) {

        control = accept(sockfd, (struct sockaddr*)&their_addr, &addr_len); 
        display = accept(sockfd2, (struct sockaddr*)&their_addr2, &addr_len2);
		
        if (control == -1 || display == -1) {
            perror("accept");
            continue;
        }
	
        connected =1;
        
        /*set up for select*/
        maxfd = MAX(control,display);
        FD_ZERO(&set);
        FD_SET(control, &master);
        FD_SET(display, &master);
        if(maxfd == 1){
            //hey
        }
        /*loop while connected*/
        while (connected) {
            /*seek to next chunk*/
            
            if(pos < maxPos){
                fseek(fp[version], offset[version][pos], SEEK_SET);
                set = master;
                sending( set, display, control, maxfd);
            }
            else{
                /* do the sending / receiving stuff */
                recvControl(control);
                printf("%i",pos);
            }
            /*
            while ((read = getline(&line, &len, fp[version])) != -1) {
                if(strcmp(line,"\n")==0){
                    pos++;
                    continue;
                }
                timeout->tv_sec = speed;	
                set = master;
                printf("%s \n",line);
                handle(set,display,control,maxfd,line,len);
            }
            */
    	}
		
    	
	close(display);
	close(control);
    
    }

    close(sockfd);

    return 0;
}