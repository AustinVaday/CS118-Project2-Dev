CC = gcc
default: client server	

client: client.c 
	$(CC) -pthread -g -o $@ client.c

server:	server.c 
	$(CC) -pthread -g -o $@ server.c

dist: client.c server.c tcp.h README Makefile report.pdf 
	tar -cvzf project2_104566193_604593537.tar.gz $^ 

clean:
	rm -rf client server received.data
