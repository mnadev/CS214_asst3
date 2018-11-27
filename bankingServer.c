#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include "banking.h"

int port;	//Global indicating port number program is listening on.

//Thread function to accept any incoming connections
void* listenConnections(){
}

//Thread function that's connected to a client.
void* clientSession(void* args){
}

int main(int argc, char** argv){
	//Verifying server program arguments:
	if(argc != 2){
		write(STDERR, "Fatal Error: Invalid number of arguments.\n", 42);
		return -1;
	}
	int port = atoi(argv[0]);		//Getting port number from argument
	if(port < 8000 || port > 66000){
		write(STDERR, "Fatal Error: Invalid port number.\n", 34);
		return -1;
	}

	

}
