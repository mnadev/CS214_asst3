make all:
	gcc bankingServer.c -pthread -o bankingServer
	gcc bankingClient.c -pthread -o bankingClient
make debug:
	gcc bankingServer.c -g -pthread -o debugServer
	gcc bankingClient.c -g -o debugClient
make clean:
	rm bankingClient
	rm bankingServer
