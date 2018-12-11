all:
	gcc bankingServer.c -pthread -o bankingServer
	gcc bankingClient.c -pthread -o bankingClient
debug:
	gcc bankingServer.c -g -pthread -o debugServer
	gcc bankingClient.c -g -o debugClient
clean:
	rm bankingClient
	rm bankingServer
