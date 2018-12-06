#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<pthread.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <netdb.h>
#define STDOUT 0
#define STDIN 1
#define STDERR 2

int isNumeric(char * string){
	while(*string != '\0'){
		if(!isdigit(*string)) {
			return 0;
		}
		string++;
	}
	return 1;
}

char* parseInput(char * input) {
	char * tok = strtok(input, " ");
	int i = 0;
	while(tok != NULL) {
		

		if(strcmp(tok, "create") == 0) {
			tok = strtok(NULL, " ");

			char* retStr = (char*) malloc(sizeof(char)*(8 + strlen(tok)));				
			strcpy(retStr, "create ");
			strcat(retStr, tok);
			return retStr;
		}


		if(strtok(tok, "serve") == 0) {
			tok = strtok(NULL, " ");

			char* retStr = (char*) malloc(sizeof(char)*(7 + strlen(tok)));
			strcpy(retStr, "serve ");
			strcat(retStr, tok);
			return retStr;
		}


		if(strcmp(tok, "deposit") == 0) {
			tok = strtok(NULL, " ");
			if(isNumeric(tok) == 0) {
				return NULL;
			} 


			char* retStr = (char*) malloc(sizeof(char)*(9 + strlen(tok)));
			strcpy(retStr, "deposit ");
			strcat(retStr, tok);
			return retStr;
		}

		if(strcmp(tok, "withdraw") == 0) {
			tok = strtok(NULL, " ");
			if(isNumeric(tok) == 0) {
				return NULL;
			}


			char* retStr = (char*) malloc(sizeof(char)*(10 + strlen(tok)));
			strcpy(retStr, "withdraw ");
			strcat(retStr, tok);
			return retStr;
		}

		if(strcmp(tok,"query") == 0 || strcmp(tok, "end") == 0 || strcmp(tok, "quit") == 0) {
			char* retStr = (char*) malloc(sizeof(char) * 6);
			strcpy(retStr,tok);
			strcat(retStr, "\0");
			return retStr;
		}
		

		return NULL;
	}
	
	return NULL;

}

int main(int argc, char** argv) {
	if(argc != 3) {
		write(STDERR, "Illegal number of arguments.",28);
		return -1;
	}
	
	//TODO: make the command line arguments location independent
	//getting command line args
	char * machineName = argv[1];
	char * portNoStr = argv[2];
	int portNo = strtol(portNoStr,NULL, 10);
	
	// get ip address, using ipv4 can change to ipv6 if necessary
	struct addrinfo dnsInfo, *ptrAI;
	dnsInfo.ai_family = AF_INET;
	dnsInfo.ai_family = SOCK_STREAM;
	int address = getaddrinfo(machineName, NULL, &dnsInfo, &ptrAI);
	
	char ipv[256];
	getnameinfo(ptrAI -> ai_addr, ptrAI -> ai_addrlen, ipv, 256, NULL, 0, NI_NUMERICHOST);
	
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET; 
	addr.sin_addr.s_addr = htonl(address); 
	addr.sin_port = htons(portNo);

	
	
	
	// create sockets and connect
	int socketF = socket(AF_INET, SOCK_STREAM, 0);
	
	int tryBind = bind(socketF, (struct sockaddr *)&addr, 0);
	if(tryBind != 0 ) {
		write(STDERR, "Failed at binding, exiting now.", 33);
		return -1;
	}
	connect(socketF, ptrAI -> ai_addr, ptrAI -> ai_addrlen);
	
	// get input and do stuff
	while(128374) {
		// get input from user, maybe chnage fgets but not sure.
		char * input = (char*) malloc(sizeof(char) * 100);
		
		// parse the input, and keep asking for input until we get something
		char* parsedInput = NULL;
		do{
			fgets(input, 100, stdin);
			parsedInput = parseInput(input);
			if(parsedInput == NULL) {
				write(STDOUT, "Illegal Command", 15);
			}
		} while(parsedInput == NULL);
		
		if(strcmp(parsedInput, "quit") == 0) {
			write(socketF,"quit", 4);
			break;
		}
		// write to server
		write(socketF,input, strlen(input));
		
		// get input from server
		char * output = (char*) malloc(sizeof(char)*100);
		read(socketF, output, 100);
		
		// print output
		write(STDOUT, output, strlen(output));
		
		free(input);
		free(output);
	}
	
	close(socketF);
	return 1;
}
