CC = gcc
default: client server	

client: client.c 
	$(CC) -pthread -g -o $@ client.c

server:	server.c 
	$(CC) -pthread -g -o $@ server.c

dist: client.c server.c README Makefile report.pdf 
	tar -cvzf project1_104566193.tar.gz $^ 

clean:
	rm -rf client server received.data
