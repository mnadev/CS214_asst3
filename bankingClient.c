#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<pthread.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
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
		if( i == 0) {
			if(!(strcmp(tok,"create") == 0 || strcmp(tok, "deposit") == 0 || strcmp(tok, "withdraw") == 0 || strcmp(tok, "serve") == 0)){
				if(strcmp(tok, "query") != 0 && strcmp(tok,"end") != 0 && strcmp(tok, "quit") != 0) {
					return NULL;
				}
			}

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
		
		}

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


}
