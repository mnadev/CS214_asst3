#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#define STDOUT 0
#define STDIN 1
#define STDERR 2


int main(int argc, char** argv) {
	if(argc != 3) {
		write(STDERR, "Illegal number of arguments.",28);
		return -1;
	}
	
}
