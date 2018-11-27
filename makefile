make all:
	gcc bankingServer.c 9999
	gcc bankingClient.c cp.cs.rutgers.edu 9999
make debug:
	gcc bankingServer.c -g
	gcc bankingClient.c -g
make clean:
