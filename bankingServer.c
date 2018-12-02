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
void* listenConnections(void* ptrListenSock){
	//Socket creation
	int* listenSock = (int*)ptrListenSock;
	(*listenSock) = socket(AF_INET, SOCK_STREAM, 0);
	//Making sure socket is actually created. (Error handling)
	if(*listenSock <= 0){
		write(STDERR, "Error: Failed to create listening socket.\n", 42);
		exit(-1);
	}
	//Setting socket options (Specifically making sure linger is set to 0 to avoid annoyances later when testing)
	int setSockOpts = setsockopt(*listenSock, SOL_SOCKET, SO_LINGER, &(int*){0}, sizeof(int*));
	if(setSockOpts != 0){	//Same as above
		write(STDERR, "Error: Failed to set listening socket options.\n", 47);
		exit(-1);
	}
	//Binding port/IP to socket
	struct sockaddr_in ipBinding;
	int sizeBinding = sizeof(ipBinding);
	ipBinding.sin_family = AF_INET;
	ipBinding.sin_addr.s_addr = INADDR_ANY;
	ipBinding.sin_port = htons(port);
	if(bind(*listenSock, (struct sockaddr*)&ipBinding, sizeBinding)<0){
		write(STDERR, "Error: Failed to bind listening socket.\n", 40);
	}
	//Making the listening socket actually listen for connections:
	if(listen(*listenSock, 10) < 0){
		write(STDERR, "Error: listenSock failed to listen.\n", 36);
		exit(-1);
	}
	
	//Array of all spawned threads
	pthread_t** childrenThreadHandles = (pthread_t**)malloc(sizeof(pthread_t*)*256);
	int numThreadsSpawned = 0;
	int sizeOfThreadArray = 256;

	//Infinite loop to accept infinite number of incoming connections.
	while(ptrListenSock != NULL){	//Loop will terminate when ptrListenSock is freed in main, will start closing all spawned sockets.
		int* newSockConnection = (int*)malloc(sizeof(int));
		if((*newSockConnection = accept(*listenSock, (struct sockaddr *)(&ipBinding), (socklen_t*)(&sizeBinding)))<0){
			write(STDERR,"Error: Failed to create new client socket.\n", 43);
		}
		pthread_t* newClientThread = (pthread_t*)malloc(sizeof(pthread_t));
		//TODO: Create new thread, determine what to put in args (probably socket FD num and mutexes?), add threadHandle to childrenThreadHandles
		//TODO: Determine when to close sockets (in new thread or during termination sequence.
				
	}
	
	//TODO:Closing all spawned sockets and blah:


	pthread_exit(0);	//Placeholder. May need to use later.
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
	int port = atoi(argv[1]);		//Getting port number from argument
	if(port < 8000 || port > 66000){
		write(STDERR, "Fatal Error: Invalid port number.\n", 34);
	}
	//Creating thread to listen on incoming connections:
	int* listenSocket = (int*)malloc(sizeof(int));	//Creating a space on the heap for listenSocket here. Will help in graceful termination.
	pthread_t* listenThread = (pthread_t*)malloc(sizeof(pthread_t));
	pthread_create(listenThread, NULL, listenConnections, (void*)listenSocket);

	//The code below will presumably be the timer code and signal handling stuff.

}
