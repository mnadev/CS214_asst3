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

	/*struct hostent *h;	
	h = gethostbyname(machineName);
	if(h == NULL) {
		write(STDERR, "Could not resolve hostname\n", 27);
		return -1;
	}*/

	// get ip address, using ipv4 can change to ipv6 if necessary
	struct addrinfo dnsInfo, *ptrAI;
	dnsInfo.ai_family = AF_INET;
	dnsInfo.ai_family = SOCK_STREAM;
	int try_addr = getaddrinfo(machineName, NULL, &dnsInfo, &ptrAI);
	//char* ipv = inet_ntoa(*((struct in_addr*) h->h_addr)); ;
	
	ptrAI -> ai_addr ->sin_port = htons(portNo)
	
	//struct* sockaddr addr = ptrAI -> ai_addr;	
	
	struct sockaddr_in addr = (struct sockaddr_in) ptrAI -> ai_addr;
	
	addr.sin_family = AF_INET; 
	//addr.sin_addr.s_addr = inet_addr(ipv); 
	addr.sin_port = htons(portNo);
	
	// create sockets and connect
	int socketF = socket(AF_INET, SOCK_STREAM, 0);
	if(socketF < 0 ) {
		write(STDERR, "Failed at creating socket, exiting now.\n", 41);
		return -1;
	}
	
	int try_bind = bind(socketF, (struct sockaddr *)&addr, sizeof(addr));
	if(try_bind < 0 ) {
		write(STDERR, "Failed at binding, exiting now.\n", 34);
		return -1;
	}
	int try_conn = connect(socketF, ptrAI -> ai_addr, ptrAI -> ai_addrlen);;
	if(try_conn < 0 ) {
		write(STDERR, "Failed at connecting, exiting now.\n", 37);
		return -1;
	}

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
				write(STDOUT, "Illegal Command\n", 16);
			}
		} while(parsedInput == NULL);
		
		if(strcmp(parsedInput, "quit") == 0) {
			write(socketF,"quit\0", 5);
			break;
		}
		
		int noAttempts = 0;
		// write to server
		while(send(socketF,input, strlen(input), 0) == -1) {
			noAttempts++;
			if(noAttempts > 10) {
				write(STDERR, "Failed to send data.\n",23);
				return -1;
			}
		}
		

		noAttempts = 0;
		// get input from server
		char * output = (char*) malloc(sizeof(char)*100);
		while(recv(socketF, output, 100, 0) == -1) {
			noAttempts++;
			if(noAttempts > 10) {
				write(STDERR, "Failed to recieve data.\n", 26);
				return -1;
			}
		}
		
		// print output
		write(STDOUT, output, strlen(output));
		
		free(input);
		free(output);
	}
	
	close(socketF);
	return 1;
}
