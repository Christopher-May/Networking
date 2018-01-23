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
	
	strcpy(keys[pos] ,newKey);
	strcpy(values[pos] ,newValue);

	pos++;

	return 0;
}

int removeKey(char* key){
	int i;
	int j;

	for(i=0; i<STORAGE; i++){
		if(strcmp(keys[i],key)==0){
			for(j=i; j<STORAGE -1; j++){
				keys[j] 	= keys[j+1];
				values[j] 	= values[j+1];
			}
			pos--;
			return 0;
		}
	}
		
	return -1;
}

int getValue(char* key,char* value){
	int i=0;
	printf("get value! \n");
	for(i=0; i<STORAGE; i++){
		if(strcmp(keys[i],key)==0){
			strcpy(value,values[i]);
			return 0;
		}
	}
	return -1;
}
int getallkeys(char* value,char* result){
	int i;

	for(i=0; i<STORAGE; i++){
		if(strcmp(values[i],value)==0){
			strcat(result,keys[i]);
			strcat(result," ");
		}
	}
	return 1;
}
int tokenize (char* sp, char *tokens[]){
	char s[256];
	int i=0;
	strncpy(s,sp,sizeof(s));
	s[sizeof(s)-1] = '\0';

	printf("\ntokens: %s\n",s);
	char * token = strtok(s, " ");

	while(token != NULL){	
		strcpy(tokens[i++],token);
		printf("i = %i",i);
		token = strtok(NULL," ");
	}
	
	printf("\n finished \n");
	return 1;
}

int doCommand(char* tokens[],char* result){
	char *commands[5] = {"add","getvalue","getallkey","getall","remove"};
	int i;
	int match =1;

	for(i=0; i<5;i++){
		if(strcmp(tokens[0],commands[i]) !=0){
			match =0;
		}else{
			match =1;
			break;
		}
	}
	if(match < 1){
		return -1;
	}

	if(strcmp(tokens[0],"add") ==0){
		addKeyValue(tokens[1],tokens[2]);
		result = "added value!\n"
	}
	else if(strcmp(tokens[0],"getvalue") ==0){
		getValue(tokens[1],result);
        }
	else if(strcmp(tokens[0],"getallkey") ==0){
        	getallkeys(tokens[1],result);
	}	
	else if(strcmp(tokens[0],"getall") ==0){
		for(int i=0; i<pos ;i++){

			strcat(result,keys[i]);
			strcat(result,",");
			strcat(result,values[i]);
			strcat(result," ");
		}

        }
	else if(strcmp(tokens[0],"remove") ==0){
		removeKey(tokens[1]);
		result = "removed value!\n";
        }
	else{
		return -1;
	}
	return 1;

}
int recieve(char* s,char* result){
	char *tk[3];
	
	for(int i =0; i<3; i++){
                tk[i] = malloc(sizeof(char)*128)
        }
	tokenize(s,tk);
	
	r= doCommand(tk,result);

	return r;
}
/*
int main(){
	char *k = "12345678";
	char *v = "frick the police";
	char *nv = malloc(sizeof(char)*128);
	char *tk0[3];
	char *tk1[3];
	char *tk2[3];
        char *tk3[3];
	char *tk4[3];
	char *result0 = malloc( ( (sizeof(char)*128) + (sizeof(char)*8) )*10 );
	char *result1 = malloc( ( (sizeof(char)*128) + (sizeof(char)*8) )*10 );
	char *result2 = malloc( ( (sizeof(char)*128) + (sizeof(char)*8) )*10 );
	char *result = malloc( ( (sizeof(char)*128) + (sizeof(char)*8) )*10 );

	initStorage();
	addKeyValue(k,v);
	getValue(k,nv);
	printf("(%s,%s)\n",keys[pos-1],values[pos-1]);
	printf("(%s,%s)\n",k,nv);
	
	char *s0 = "add 12345679 helloWorld";
	char *s1 = "getvalue 12345679";
	char *s2 = "remove 12345678";
	char *s3 = "getallkey helloWorld";
	char *s4 = "getall";

	for(int i =0; i<3; i++){
		tk0[i] = malloc(sizeof(char)*128);
		tk1[i] = malloc(sizeof(char)*128);
		tk2[i] = malloc(sizeof(char)*128);
		tk3[i] = malloc(sizeof(char)*128);
		tk4[i] = malloc(sizeof(char)*128);

	}
	printf("\ntokenizing\n");	
	tokenize(s0,tk0);
	tokenize(s1,tk1);
	tokenize(s2,tk2);
	tokenize(s3,tk3);
	tokenize(s4,tk4);

	printf("...\ndone\n");

	int r = doCommand(tk0,result);
	printf("\nadd result: %i\n",r);	

	doCommand(tk1,result0);
	printf("\ngetvalue result: %s\n",result0);
	
	r= doCommand(tk2,result);
	printf("\nremove result: %i\n",r);

	doCommand(tk3,result1);
	printf("\ngetallkey result: %s\n",result1);

	doCommand(tk4,result2);
	printf("\ngetall result: %s\n",result2);

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

