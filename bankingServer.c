#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <search.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "banking.h"
#include <errno.h>

//Function prototypes (May decide to move this to header file later if it gets too cluttered.
void* listenConnections(void* ptrListenSock);
void* clientSession(void* args);

//Global variables
int port;	//Port number program is listening on.
char** accountNames; //List of account names used when printing out diagnostic information.
int numAccounts;	//Used so that we know where to put the new account in accountNames upon creation.
sem_t* accountCreateLock;		//Semaphore on account creation.
int diagnosticActive;
int stopAndHammerTime;				//Boolean variable that will force all clients to close when shutdown command is received.
pthread_attr_t* createDetachAttr;	//Attribute for pthread creation that won't wait for joins for reasons.
struct itimerval timer; // timer for diagnostic output
pthread_mutex_t dummyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condVar = PTHREAD_COND_INITIALIZER;


//"Signal handler" for graceful termination and diagnostic output (Signal handling with multithreaded programs is weird):
void* signal_handler(void* args){
	int sigCaught;
	SigArgs* theArgs = (SigArgs*)args;
	while(31337){
		sigwait(theArgs->sigSet, &sigCaught);
		switch(sigCaught){
			case SIGINT:
				stopAndHammerTime = 1;
				close(*(theArgs->listenSockFD));
				pthread_cancel(*(theArgs->listenThread));
				pthread_exit(0);
			case SIGALRM:
				//Code for diagnostic output;
				//thread_timer_print();
				sem_wait(accountCreateLock);
				diagnosticActive = 1;

				printf("Diagnostic Output:\n");
				if(numAccounts == 0){
					printf("No accounts in database.\n");
				}
				int i;
				for(i = 0; i < numAccounts; i++) {
					char *  curr_name = accountNames[i];
					ENTRY keySearch, *accEntry;
					keySearch.key = curr_name;
					// get account from hash table
					accEntry = hsearch(keySearch, FIND);
					Account* curr_account = (Account*)(accEntry->data);
					printf("\"%s\"\t%f", curr_account -> name, curr_account -> balance);
					if((curr_account -> inService) != 0) {
						printf("\tIN SERVICE\n");
					} else{
						printf("\tNOT IN SERVICE\n");
					}
				}	

				diagnosticActive = 0;
				sem_post(accountCreateLock);
				pthread_cond_broadcast(&condVar);
				//break;
		}
		sigCaught = 0;
	}
}

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
	
	//Infinite loop to accept infinite number of incoming connections.
	while(stopAndHammerTime == 0){	//Loop will terminate when stopAndHammerTime is set to 1, starting termination sequence.
		int* newSockConnection = (int*)malloc(sizeof(int));
		*newSockConnection = accept(*listenSock, (struct sockaddr*)(&ipBinding), &sizeBinding);
		if(*newSockConnection < 0){
			write(STDERR,"Error: Failed to create new client socket.\n", 43);
			//printf("%s\n", strerror(errno));
			break;
		}
		printf("Accepted new client connection.\n");
		//Starting new thread for client session:
		pthread_t* newClientThread = (pthread_t*)malloc(sizeof(pthread_t));
		//Setting up a timeout on recv calls so that client won't be perma-blocking (needed for implementation of signal handling)
		struct timeval timeout;
		timeout.tv_sec = 4;
		int setSockOpts = setsockopt(*newSockConnection, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout,(socklen_t)(sizeof(timeout)));

		pthread_create(newClientThread, createDetachAttr, clientSession, (void*)newSockConnection);	//TODO: Change thread args for when i decide if anything else needs to be sent in there.
	}
	pthread_exit(0);
}

//Thread function that's connected to a client.
void* clientSession(void* args){
	//TEMP: for now, args is just the file descriptor the socket is active on.
	int clientSock = *(int*)args;
	char recvBuffer[300];	//Buffer for reading data sent on socket
	int inSession = 0;		//Bool to determine if client is in a session
	Account* accountData;			//Pointer to the hash table entry where the info for the account being served is stored.
	//Infinite loop for received data. Will break when quit is received or shutdown sig was sent.
	while(stopAndHammerTime == 0){
		if(diagnosticActive == 1){
			pthread_cond_wait(&condVar, &dummyMutex);
		}
		int recvBytes = recv(clientSock, (void*)recvBuffer, 300, 0);	
		printf("%s\n", recvBuffer);
		if(strcmp(recvBuffer, "quit") == 0){
			char quitMes[] = "Quitting banking service. Have a nice day (≧ω≦)\n";
			send(clientSock, quitMes, 53, 0);
			memset(recvBuffer, 0, 300);	//Purging recvBuffer in preparation of receiving next command.
			break; 
		} else if(strcmp(recvBuffer, "end") == 0){
			if(inSession == 0){
				char errorMes[] = "Error: You are not currently in a service session.\n";
				send(clientSock, errorMes, 51, 0);
			} else{
				char endMes[] = "Ending current service session.\n";		//I may change this message to include account name being ended, but not a priority. Just looks nice is all
				send(clientSock, endMes, 32, 0);
				//Lock and unlock
				pthread_mutex_lock(accountData->serviceLock);
				accountData->inService = 0;
				pthread_mutex_unlock(accountData->serviceLock);
				inSession = 0;				
			}
		} else if(strcmp(recvBuffer, "query") == 0){
			if(inSession == 0){
				char errorMes[] = "Error: You are not currently in a service session.\n";
				send(clientSock, errorMes, 51, 0);					
			} else{
				char* returnBalance = malloc(sizeof(char)*100);
				memset(returnBalance, 0, 100);
				snprintf(returnBalance, 100, "You currently have: $%f\n", accountData->balance);
				send(clientSock, returnBalance, 100, 0);
				free(returnBalance);
			}
		} else if(strncmp(recvBuffer, "create ", 7) == 0){
			//Anti-segfault precaution:
			if(strlen(recvBuffer) < 8){
				continue;
			}
			if(inSession == 1){
				char errorMes[] = "Error: You are already in a service session. Please 'end' to create an account.\n";
				send(clientSock, errorMes, 82, 0);
				continue;
			}
			//Checking if new account name is already in hash table:
			char* paramName = strstr(recvBuffer, " ") + 1;
			ENTRY testKey, *testEntry;
			testKey.key = paramName;
			testEntry = hsearch(testKey, FIND);
			if(testEntry != NULL){
				char errorMes[] = "Error: Given account is already in database. Please try another name.\n";	
				send(clientSock, errorMes, 70, 0);		
			} else{		
				//Creating new account name on heap:
				char* newName = malloc(sizeof(char)*(strlen(paramName)+1));
				strcpy(newName, paramName);
				//Creating new mutex associated with account:
				pthread_mutex_t* newAccountMutex = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(newAccountMutex, NULL);
				//Creating new struct:
				Account* newAccountStruct = malloc(sizeof(Account));
				//Shoving everything into said struct:
				newAccountStruct->name = newName;
				newAccountStruct->balance = 0;
				newAccountStruct->inService = 0;
				newAccountStruct->serviceLock = newAccountMutex;
				//Loading new struct into hash table:
				ENTRY newEntry;
				newEntry.key = newName;
				newEntry.data = (void*)newAccountStruct;
				sem_wait(accountCreateLock);
				hsearch(newEntry, ENTER);
				accountNames[numAccounts] = newName;
				numAccounts++;
				sem_post(accountCreateLock);
				//Respond to client:
				char createMes[] = "Successfully created account.\n";
				send(clientSock, createMes, 30, 0);
			}
		} else if(strncmp(recvBuffer, "serve ", 6) == 0){
			if(inSession != 0){
				char errorMes[] = "Error: An account is already being serviced.\n";
				send(clientSock, errorMes, 46, 0);
				memset(recvBuffer, 0, 300);	//Purging recvBuffer in preparation of receiving next command.
			} else{
				//Anti-segfault precaution:
				if(strlen(recvBuffer) < 7){
					continue;
				}
				ENTRY searchAcc, *hashEntry;
				char* requestedName = strstr(recvBuffer, " ") + 1;
				searchAcc.key = requestedName;
				hashEntry = hsearch(searchAcc, FIND);
				if(hashEntry == NULL){
					char errorMes[] = "Error: Account requested is not in database. Please create account first.\n";
					send(clientSock, errorMes, 75, 0);
				} else{
					if( ((Account*)(hashEntry->data))->inService == 1 ){	//account already in session:
						char errorMes[] = "Error: Account requested is currently in service. Please try again later.\n";
						send(clientSock, errorMes, 74, 0);				
					} else{
						accountData = (Account*)(hashEntry->data);
						//Lock serviceLock to prevent race condition.
						pthread_mutex_lock(accountData->serviceLock);
						accountData->inService = 1;
						pthread_mutex_unlock(accountData->serviceLock);
						inSession = 1;	//indicating locally that client is in a session.
						char serveMes[] = "Now serving account.\n";		//Like above, may change to include account name to look nicer.
						send(clientSock, serveMes, 21, 0);
					}
				}
			}
		} else if(strncmp(recvBuffer, "deposit ", 8) == 0){
			if(inSession == 0){
				char errorMes[] = "Error: You are not currently in a service session.\n";
				send(clientSock, errorMes, 51, 0);	
			} else{
				//Anti-segfault precaution:
				if(strlen(recvBuffer) < 9){
					continue;
				}
				char* depositString = strstr(recvBuffer, " ") + 1;
				double depositVal = atof(depositString);
				accountData->balance += depositVal;
				char depositMes[] = "Money successfully deposited.\n";	//Again, may include money val to look nicer.
				send(clientSock, depositMes, 30, 0);
			}
		} else if(strncmp(recvBuffer, "withdraw ", 9) == 0){
			if(inSession == 0){
				char errorMes[] = "Error: You are not currently in a service session.\n";
				send(clientSock, errorMes, 51, 0);
			} else{
				//Anti-segfault precaution:
				if(strlen(recvBuffer) < 10){
					continue;
				}
				char* withdrawString = strstr(recvBuffer, " ") + 1;
				double withdrawVal = atof(withdrawString);
				if(accountData->balance - withdrawVal < 0){
					char youBrokeSon[] = "Insufficient funds to withdraw.\n";
					send(clientSock, youBrokeSon, 32, 0);
				} else{
					accountData->balance -= withdrawVal;
					char withdrawMes[] = "Money successfully withdrawn.\n";
					send(clientSock, withdrawMes, 30, 0);
				}
			}				
		} else if(recvBytes == 0){
			printf("Connection terminated.\n");
			if(inSession == 1){
				pthread_mutex_lock(accountData->serviceLock);
				accountData->inService = 0;
				pthread_mutex_unlock(accountData->serviceLock);
				inSession = 0;	
			}
			memset(recvBuffer, 0, 300);	//Purging recvBuffer in preparation of receiving next command.
			break;
		} else if(recvBytes == -1){
			memset(recvBuffer, 0, 300);	//Purging recvBuffer in preparation of receiving next command.
			continue;
		} else{
			//Really should never get here if client is sanitizing commands, but in case server/client messed up:
			char errorMsg[] = "Error: Received an invalid command (╯°□°）╯︵ ┻━┻\n";
			printf("%s\n", errorMsg);
			printf("%s\n", recvBuffer);
			send(clientSock, errorMsg, 68, 0);
		}
		memset(recvBuffer, 0, 300);	//Purging recvBuffer in preparation of receiving next command.		memset(recvBuffer, 0, 300);	//Purging recvBuffer in preparation of receiving next command.
	}
	//When the loop breaks, server will send message to client and close the socket.
	char shutdownMes[] = "Server shutting down. Terminating Connection.";
	send(clientSock, shutdownMes, 46, 0);
	close(clientSock);
}

int main(int argc, char** argv){
	//Verifying server program arguments:
	if(argc != 2){
		write(STDERR, "Fatal Error: Invalid number of arguments.\n", 42);
		return -1;
	}
	port = atoi(argv[1]);		//Getting port number from argument
	if(port < 1000 || port > 66000){
		write(STDERR, "Fatal Error: Invalid port number.\n", 34);
		return -1;
	}
	
	stopAndHammerTime = 0;
	//Initializing Hash Table to store account information:
	//NOTE: With the POSIX hash table, it doesn't support hash table resizing, so i'm setting the table size to be extremely large here and hoping that we don't exceed that.
	//Check 'man hcreate' for usage.
	hcreate(8192);
	accountNames = (char**)malloc(sizeof(char*)*8192);	//initializing array that holds all account names.
	memset(accountNames, 0, sizeof(char*)*8192);		//setting all array values to 0 so that we know where the account names stop. (Anti-segfault measure)
	numAccounts = 0;
	accountCreateLock = (sem_t*)malloc(sizeof(sem_t));	//creating mutex for account creation.
	sem_init(accountCreateLock, 0, 1);
	
	// setting timer to 15 seconds
	timer.it_interval.tv_sec = 15;	//interval to 15 sec allows for auto-restart.
   	timer.it_interval.tv_usec = 0;
   	timer.it_value.tv_sec = 15;
   	timer.it_value.tv_usec = 0;
	
	//Creating a thread for signal handling (all signals will be redirected to that thread. (An hour of googling later, this is the result I came up with)
	sigset_t* signalsToCatch = malloc(sizeof(sigset_t));		//creating the set of signals to be caught (sigint and sigalarm)
	sigemptyset(signalsToCatch);
	sigaddset(signalsToCatch, SIGINT);
	sigaddset(signalsToCatch, SIGALRM);

	createDetachAttr = malloc(sizeof(pthread_attr_t));
	pthread_attr_init(createDetachAttr);
	pthread_attr_setdetachstate(createDetachAttr, PTHREAD_CREATE_DETACHED);
	pthread_sigmask(SIG_BLOCK, signalsToCatch, 0);				//Causes all spawned threads to ignore signals sent.

	//Creating thread to listen on incoming connections:
	int* listenSocket = (int*)malloc(sizeof(int));	//Creating a space on the heap for listenSocket here. Will help in graceful termination.
	pthread_t* listenThread = (pthread_t*)malloc(sizeof(pthread_t));
	pthread_create(listenThread, NULL, listenConnections, (void*)listenSocket);

	//Starting the sig handler.
	pthread_t* sigHandler = malloc(sizeof(pthread_t));
	SigArgs* args = malloc(sizeof(SigArgs));
	args->listenSockFD = listenSocket;
	args->sigSet = signalsToCatch;
	args->listenThread = listenThread;
	pthread_create(sigHandler, createDetachAttr, signal_handler, (void*)args);
	
	printf("Banking server has started up.\n");

	//Starting timer:
	setitimer(ITIMER_REAL, &timer, 0);
	diagnosticActive = 0;

	pthread_join(*listenThread, NULL);
	printf("\nServer shutdown in progress....\n");
	sleep(5);
	printf("Server shutdown complete\n");
}
