CC = gcc
default: client server	

client: client.c
	$(CC) -g -o $@ client.c

server:	server.c
	$(CC) -g -o $@ server.c

dist: client.c server.c README Makefile report.pdf 
	tar -cvzf project1_104566193.tar.gz $^ 

clean: client server 
	rm -rf $^
