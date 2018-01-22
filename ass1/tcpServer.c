/*
** server.c -- a stream socket server demo
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

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

//
#define STORAGE 25
#define MAXDATASIZE 100

int pos = 0;

char* keys[STORAGE];
char* values[STORAGE];


int initStorage(){
	int i = 0;
	//malloc arrays
	for(i=0; i< STORAGE; i++){
		keys[i] 	= malloc(sizeof(char)*8);
		values[i]	= malloc(sizeof(char)*128);
	}
	
	return 0;	
}

int  addKeyValue(char* newKey,char* newValue){
	//check if the new key and value are the correct size
	if( sizeof(newKey) > sizeof(char)*8 || sizeof(newValue) > sizeof(char)*128 ){
		return -1;
	}
	//check if there is enough room
	if(pos >= STORAGE){
		return -1;
	}
	
	keys[pos] 	= newKey;
	values[pos] 	= newValue;

	pos++;

	return 0;
}

int removeKey(char* key){
	int i;
	
	for(i=0; i<STORAGE; i++){
		if(strcmp(keys[i],key)==0){
			for( ; i<STORAGE -1; i++){
				keys[i] 	= keys[i+1];
				values[i] 	= values[i+1];
			}
			pos--;
			return 0;
		}
	}
		
	return -1;
}

int getValue(char* key,char* value){
	int i=0;

	for(i=0; i<STORAGE; i++){
		if(strcmp(keys[i],key)==0){
			value = values[i];
			return 0;
		}
	}
	return -1;
}

/*
int main(){
	char *k = "12345678";
	char *v = "frick the police";
	char *nv = malloc(sizeof(char)*128);
	initStorage();
	addKeyValue(k,v);
	getValue(k,&nv);
	printf("(%s,%s)\n",keys[pos-1],values[pos-1]);
	printf("(%s,%s)\n",k,nv);


}
*/
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

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    int numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
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

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
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

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
	    //close(sockfd); child doesn't need the listener
            
	    if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
                perror("send");

	    
	    if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
        	perror("recv");
        	exit(1);
    	    }

            buf[numbytes] = '\0';

            printf("server: received '%s'\n",buf);

	    if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

