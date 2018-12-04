#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include "banking.h"
#include <errno.h>

//Function prototypes (May decide to move this to header file later if it gets too cluttered.
void* listenConnections(void* ptrListenSock);
void* clientSession(void* args);

int port;	//Global indicating port number program is listening on.

//Thread function to accept any incoming connections
void* listenConnections(void* ptrListenSock){
	//Socket creation
	int* listenSock = (int*)ptrListenSock;
	memset(listenSock, 0, sizeof(int));
	(*listenSock) = socket(AF_INET, SOCK_STREAM, 0);
	//Making sure socket is actually created. (Error handling)
	if(*listenSock <= 0){
		write(STDERR, "Error: Failed to create listening socket.\n", 42);
		exit(-1);
	}
	//Setting socket options (Specifically making sure linger is set to 0 to avoid annoyances later when testing)
	struct linger lingerOpts = {1,0};
	int setSockOpts = setsockopt(*listenSock, SOL_SOCKET, SO_LINGER, (void*)&lingerOpts,(socklen_t)(sizeof(lingerOpts)));
	if(setSockOpts != 0){	//Same as above
		write(STDERR, "Error: Failed to set listening socket options.\n", 47);
		exit(-1);
	}
	//Binding port/IP to socket
	struct sockaddr_in ipBinding;
	memset(&ipBinding, 0, sizeof(ipBinding));
	socklen_t sizeBinding = sizeof(ipBinding);
	ipBinding.sin_family = AF_INET;
	ipBinding.sin_addr.s_addr = INADDR_ANY;
	ipBinding.sin_port = htons(port);
	if(bind(*listenSock, (struct sockaddr*)(&ipBinding), sizeBinding)<0){
		write(STDERR, "Error: Failed to bind listening socket.\n", 40);
		exit(-1);
	}
	//Making the listening socket actually listen for connections:
	if(listen(*listenSock, 10) < 0){
		write(STDERR, "Error: listenSock failed to listen.\n", 36);
		exit(-1);
	}
	
	//Array of all spawned threads
	pthread_t** childrenThreadHandles = (pthread_t**)malloc(sizeof(pthread_t*)*256);
	memset(childrenThreadHandles, 0, sizeof(pthread_t*)*256);	//Setting all bytes to 0 for joining later on.
	int numThreadsSpawned = 0;
	int sizeOfThreadArray = 256;
	//Infinite loop to accept infinite number of incoming connections.
	while(ptrListenSock != NULL){	//Loop will terminate when ptrListenSock is freed in main, will start closing all spawned sockets.
		int* newSockConnection = (int*)malloc(sizeof(int));
		printf("Did it get here\n");
		*newSockConnection = accept(*listenSock, (struct sockaddr*)(&ipBinding), &sizeBinding);
		if(*newSockConnection < 0){
			write(STDERR,"Error: Failed to create new client socket.\n", 43);
		}
		
		//Starting new thread for client session:
		pthread_t* newClientThread = (pthread_t*)malloc(sizeof(pthread_t));
		//TODO: Create new thread, determine what to put in args (probably socket FD num and mutexes?), add threadHandle to childrenThreadHandles
		//SELF NOTE: Use a monitor when a thread wants access to some value in the master database in preparation for adding the diagnostic output (Lec 19-20)
		if(numThreadsSpawned >= sizeOfThreadArray){
			//If for whatever reason there exceeds 256 client connections, we're doing this memcpy nonsense instead of realloc to ensure that all non-used array entries are still "0"
			pthread_t** testRealloc = malloc(sizeof(pthread_t*)*2*sizeOfThreadArray);
			memset(testRealloc, 0, sizeof(pthread_t*)*2*sizeOfThreadArray);
			memcpy(testRealloc, childrenThreadHandles, sizeof(pthread_t*)*sizeOfThreadArray);
			free(childrenThreadHandles);
			childrenThreadHandles = testRealloc;
		}
		childrenThreadHandles[numThreadsSpawned] = newClientThread;
		newClientThread++;
		pthread_create(newClientThread, NULL, clientSession, (void*)newSockConnection);	//TODO: Change thread args for when i decide if anything else needs to be sent in there.
	}
	
	//TODO:Closing all spawned sockets and blah:
	//Self-Note: This is the plan:
	//	Each thread will close it's own socket file descriptor
	//	On server termination (which i still need to figure out how to do), all client threads will return the socket# they are using, and those will be closed externally.


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
	port = atoi(argv[1]);		//Getting port number from argument
	if(port < 8000 || port > 66000){
		write(STDERR, "Fatal Error: Invalid port number.\n", 34);
	}
	//Creating thread to listen on incoming connections:
	int* listenSocket = (int*)malloc(sizeof(int));	//Creating a space on the heap for listenSocket here. Will help in graceful termination.
	pthread_t* listenThread = (pthread_t*)malloc(sizeof(pthread_t));
	pthread_create(listenThread, NULL, listenConnections, (void*)listenSocket);
	
	//The code below will presumably be the timer code and signal handling stuff.
	sleep(1000);
}
