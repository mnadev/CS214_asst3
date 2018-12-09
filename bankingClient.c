#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<pthread.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define STDOUT 0
#define STDIN 1
#define STDERR 2

int shutdownMess = 0;

int isNumeric(char * string){
	while(*string != '\0' || *string != '\n'){
		if(*string == '\0' || *string == '\n') {
			break;
		}
		if(isdigit(*string) == 0) {
			return 0;
		}
		string++;
	}
	return 1;
}

char* parseInput(char * input) {
	int len = strlen(input)-1;
	if (input[len] == '\n'){
		input[len] = '\0';
	}
  
	if(strcmp(input,"query") == 0) {
		char * retStr = (char*) malloc(sizeof(char)*6);
		snprintf(retStr, 6, "query\0"); 
		return retStr;
	}
		
	if(strcmp(input, "end") == 0){
		char * retStr = (char*) malloc(sizeof(char)*4);
		snprintf(retStr, 4, "end\0");
		return retStr;
	}

	if(strcmp(input, "quit") == 0) {
		char * retStr = (char*) malloc(sizeof(char)*4);
		snprintf(retStr, 5, "quit\0"); 
		return retStr;
	}


	char * tok = strtok(input, " ");
	//char * tok = NULL;
	int i = 0;
	while(tok != NULL) {


		if(strcmp(tok, "create") == 0) {
			tok = strtok(NULL, " ");
			if(tok != NULL) {
				char* retStr = (char*) malloc(sizeof(char)*(8 + strlen(tok)));
				snprintf(retStr, 8 + strlen(tok), "create %s\0", tok); 		
				return retStr;
			}
		}


		if(strcmp(tok, "serve") == 0) {
			tok = strtok(NULL, " ");
			if(tok != NULL) {
				char* retStr = (char*) malloc(sizeof(char)*(7 + strlen(tok)));
				snprintf(retStr, 7 + strlen(tok), "serve %s\0", tok); 
				return retStr;
			}
		}


		if(strcmp(tok, "deposit") == 0) {
			tok = strtok(NULL, " ");
			if(isNumeric(tok) == 0) {
				return NULL;
			} 

			if(tok != NULL) {
				char* retStr = (char*) malloc(sizeof(char)*(9 + strlen(tok)));
				snprintf(retStr, 9 + strlen(tok), "deposit %s\0", tok); 
				return retStr;
			}
		}

		if(strcmp(tok, "withdraw") == 0) {
			tok = strtok(NULL, " ");
			if(isNumeric(tok) == 0) {
				return NULL;
			}

			if(tok != NULL) {
				char* retStr = (char*) malloc(sizeof(char)*(10 + strlen(tok)));
				snprintf(retStr, 10 + strlen(tok), "serve %s\0", tok); 
				return retStr;
			}
		}
		return NULL;
	}
	
	return NULL;

}

void* get_and_print(void *sf_p) {
	int socketF = *( (int * ) sf_p);
	while(shutdownMess == 0) {
		int noAttempts = 0;
		// get input from server, idk how big message will be so i set it at 1000 chars
		char * output = (char*) malloc(sizeof(char)*1000);
		while(recv(socketF, output, 100, 0) == -1) {
			noAttempts++;
			if(noAttempts > 10) {
				//write(STDERR, "Failed to recieve data.\n", 26);
				//return -1;
			}
		}
		
		// check for shutdown message from server. we have to change this
		// depending on what is sent
		if(strcmp(output,"Server shutting down. Terminating Connection.") == 0){
			shutdownMess = 1;
			break;
		}
		
		
		// print output
		write(STDOUT, output, strlen(output));
		free(output);
	}
		   
	pthread_exit(0);
}

void* get_and_send(void *sf_p) {
	int socketF = *( ( int * ) sf_p);
	// get input and do stuff
	while(shutdownMess == 0) {
		// get input from user, maybe chnage fgets but not sure.
		char * input = (char*) malloc(sizeof(char) * 100);
		
		// parse the input, and keep asking for input until we get something
		char* parsedInput = NULL;
		do{
			fgets(input, 100, stdin);
			parsedInput = parseInput(input);
			if(parsedInput == NULL) {
				write(STDOUT, "Illegal Command\n", 16);
			}
		} while(parsedInput == NULL);
		
		if(strcmp(parsedInput, "quit") == 0) {
			//write(socketF,"quit\0", 5);
			//break;
		}
		
		int noAttempts = 0;
		// write to server
		while(send(socketF,input, strlen(input), 0) == -1) {
			noAttempts++;
			if(noAttempts > 10) {
				//write(STDERR, "Failed to send data.\n",23);
				//return -1;
			}
		}
		free(input);
		sleep(2);
	}
	
	pthread_exit(0);
}

int main(int argc, char** argv) {
	if(argc != 3) {
		write(STDERR, "Illegal number of arguments.\n",29);
		return -1;
	}
	
	//TODO: make the command line arguments location independent
	//getting command line args
	char * machineName = argv[1];
	char * portNoStr = argv[2];
	
	if(isNumeric(portNoStr) == 0) {
		write(STDERR, "Illegal arguments.\n", 19);
		return -1;
	}
	
	int portNo = strtol(portNoStr,NULL, 10);


	// get ip address, using ipv4 can change to ipv6 if necessary
	struct addrinfo dnsInfo, *ptrAI;
	memset(&dnsInfo, 0, sizeof(dnsInfo));
	dnsInfo.ai_family = AF_UNSPEC;
	dnsInfo.ai_socktype = SOCK_STREAM;
	
	// try to resolve hostname to ip address
	int try_addr = getaddrinfo(machineName, portNoStr, &dnsInfo, &ptrAI);
	if(try_addr < 0) {
		write(STDERR, "Could not resolve hostname.\n", 28);
		return -1;
	}
	
	// create socket descriptor
	int socketF = socket(ptrAI -> ai_family, ptrAI -> ai_socktype, ptrAI -> ai_protocol);
	while(socketF < 0) {
		socketF = socket(ptrAI -> ai_family, ptrAI -> ai_socktype, ptrAI -> ai_protocol);
	}	

	/*
	int try_bind = bind(socketF, (struct sockaddr *)addr, sizeof(*addr));
	if(try_bind < 0 ) {
		write(STDERR, "Failed at binding, exiting now.\n", 34);
		return -1;
	}*/
	
	// try to connect to socket, prob wont work
	int try_conn = connect(socketF, ptrAI -> ai_addr, ptrAI -> ai_addrlen);
	while (try_conn < 0) {
		try_conn = connect(socketF, ptrAI -> ai_addr, ptrAI -> ai_addrlen);
	}
	pthread_t* get_thread = (pthread_t*)malloc(sizeof(pthread_t)); 
	pthread_t* print_thread = (pthread_t*)malloc(sizeof(pthread_t)); 
	pthread_create(get_thread, NULL, get_and_send, (void*)&socketF);
	pthread_create(print_thread, NULL, get_and_print, (void*)&socketF);
	
	
	pthread_join(*get_thread, NULL);
	pthread_join(*print_thread, NULL);
	
	
	write(STDOUT, "Client Shutting down. R.I.P. your money.\n",41);
	close(socketF);
	return 0;
}
