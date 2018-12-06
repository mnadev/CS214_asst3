//Definitions for ease of access

#define STDIN 0
#define STDOUT 1
#define STDERR 2

//Struct definitions:

typedef
struct _Account{
	char* name;
	double balance;
	int inService;	//0 for False, 1 for True
	pthread_mutex_t* serviceLock;	//A mutex to prevent multiple threads from trying to change the inService field at once. 
} Account;

typedef
struct _sigArgs{
	int* listenSockFD;
	sigset_t* sigSet;
} SigArgs;

//Function Prototypes:

