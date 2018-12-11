#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<pthread.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#define STDOUT 0
#define STDIN 1
#define STDERR 2

int shutdownMess = 0;

pthread_t* get_thread; 
pthread_t* print_thread; 

int isNumeric(char * string){
	while(*string != '\0' || *string != '\n'){
		if(*string == '\0' || *string == '\n') {
			break;
		}
		if(isdigit(*string) == 0 && *string != '.') {
			return 0;
		}
		string++;
	}
	return 1;
}

char* parseInput(char * input, int length) {
	int len = strlen(input)-1;
	if(len == 0 || len == 1) {
		return NULL;
	}
	if (input[len] == '\n'){
		input[len] = '\0';
	}
 	char * copy = (char * ) malloc(sizeof(char) * length);
	memset(copy, '\0', length);
	strcpy(copy, input);
	 
	if(strcmp(input,"query") == 0) {
		char * retStr = (char*) malloc(sizeof(char)*6);
		memset(retStr, 0, 6);
		snprintf(retStr, 6, "query\0"); 
		return retStr;
	}
		
	if(strcmp(input, "end") == 0){
		char * retStr = (char*) malloc(sizeof(char)*4);
		memset(retStr, 0, 4);
		snprintf(retStr, 4, "end\0");
		return retStr;
	}

	if(strcmp(input, "quit") == 0) {
		char * retStr = (char*) malloc(sizeof(char)*4);
		memset(retStr, 0, 6);
		snprintf(retStr, 5, "quit\0"); 
		return retStr;
	}

	if(strcmp(input,"create ") == 0) {
		char * retStr = (char*) malloc(sizeof(char)*8);
		memset(retStr, 0, 6);
		snprintf(retStr, 8, "create \0"); 
		return retStr;
	}
	
	if(strcmp(input,"serve ") == 0) {
		char * retStr = (char*) malloc(sizeof(char)*7);
		memset(retStr, 0, 6);
		snprintf(retStr, 7, "serve \0"); 
		return retStr;
	}
	
	if(strstr(input, " ") == NULL) {
		return NULL;
	}	

	char * tok = strtok(input, " ");
	//char * tok = NULL;
	int i = 0;
	while(tok != NULL) {

		char* retStr = (char*) malloc(length+ 2);
		memset(retStr, 0, strlen(input)+ 2);

		if(strcmp(tok, "create") == 0) {
			snprintf(retStr, strlen(copy) + 1, "%s\0", copy); 		
			return retStr;	
		}


		if(strcmp(tok, "serve") == 0) {
			snprintf(retStr, strlen(copy) + 1, "%s\0", copy); 	 
			return retStr;
		}


		if(strcmp(tok, "deposit") == 0) {
			if(length - strlen(tok) <= 2) {
				return NULL;
			}
			if(strstr(copy, "  ") != NULL) {
				return NULL;
			}
			
			tok = strtok(NULL, " ");
			if(tok != NULL) {
				if((*tok) ==  '\n' || (*tok) ==  '\0') {
					return NULL;
				}
				if(isNumeric(tok) == 0) {
					return NULL;
				} 
					snprintf(retStr, 9 + strlen(tok), "deposit %s\0", tok); 
					return retStr;
			}

		}

		if(strcmp(tok, "withdraw") == 0) {
			if(length - strlen(tok) <= 2) {
				return NULL;
			}
	
			if(strstr(copy, "  ") != NULL) {
				return NULL;
			}		

			tok = strtok(NULL, " ");

			if(strstr(tok, " ") != NULL) {
				return NULL;
			}

			if(tok != NULL) {
				if(*tok == '\n' || *tok ==  '\0' || *tok == ' ') {
					return NULL;
				}
				if(isNumeric(tok) == 0) {
					return NULL;
				}
				snprintf(retStr, 10 + strlen(tok), "withdraw %s\0", tok); 
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
		
		// get input from server, idk how big message will be so i set it at 1000 chars
		char * output = (char*) malloc(sizeof(char)*1000);
		memset(output, 0, 1000); 
		int rec = recv(socketF, output, 100, 0);
		// check for shutdown message from server. we have to change this
		// depending on what is sent
		if(strstr(output,"Server shutting down. Terminating Connection.") != NULL){
			shutdownMess = 1;
			//escape from fgets
			pthread_cancel(*get_thread);
			//pthread_kill(*get_thread, SIGKILL);
			write(STDIN, "SHUTDOWN\n", 9);
			pthread_exit(0);
		}
		
		// print output
		if(rec > 0) {
			write(STDOUT, output, strlen(output));
		}
		free(output);
	}
		   
	pthread_exit(0);
}

void* get_and_send(void *sf_p) {
	int socketF = *( ( int * ) sf_p);
	// get input and do stuff
	while(shutdownMess == 0) {
		// get input from user, maybe chnage fgets but not sure.
		char * input = (char*) malloc(sizeof(char) * 300);
		memset(input, 0, 300*sizeof(char));
		// parse the input, and keep asking for input until we get something
		char* parsedInput = NULL;
		do{
			fgets(input, 300, stdin);
			if(shutdownMess == 1) {
				pthread_exit(0);
			}
			if(*input != '\0' && * input != '\n') {
				parsedInput = parseInput(input, strlen(input));
				if(parsedInput == NULL) {
					write(STDOUT, "Illegal Command\n", 16);
				}
			}
		} while(parsedInput == NULL);
		if(strcmp(parsedInput, "quit") == 0){
			shutdownMess = 1;
		}
		// write to server
		send(socketF,parsedInput, strlen(parsedInput), 0);
		
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
	dnsInfo.ai_family = AF_INET;
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
	write(STDOUT, "Successfully Connected.\n", 24);
	get_thread = (pthread_t*)malloc(sizeof(pthread_t)); 
	print_thread = (pthread_t*)malloc(sizeof(pthread_t)); 
	pthread_create(get_thread, NULL, get_and_send, (void*)&socketF);
	pthread_create(print_thread, NULL, get_and_print, (void*)&socketF);
	
	
	pthread_join(*get_thread, NULL);
	pthread_join(*print_thread, NULL);
	
	
	write(STDOUT, "Client Shutting down. R.I.P. your money.\n",41);
	close(socketF);
	return 0;
}
