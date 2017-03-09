/* 
 * client.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "tcp.h"

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

char* deserialize_int32(char *buffer, uint32_t *value) {
    uint32_t res = 0;
    uint32_t one_byte = 0;
    one_byte = buffer[0] << 24;
    res |= one_byte;

    one_byte = 0;
    one_byte = buffer[1] << 16;
    res |= one_byte;

    one_byte = 0;
    one_byte = buffer[2] << 8;
    res |= one_byte;

    one_byte = 0;
    one_byte = buffer[3];
    res |= one_byte;

    *value = res;
    return buffer + 4;
}

char* deserialize_int16(char *buffer, uint16_t *value) {
    uint16_t res = 0;
    uint16_t one_byte = 0;
    one_byte = buffer[0] << 8;
    res |= one_byte;

    one_byte = 0;
    one_byte = buffer[1];
    res |= one_byte;

    *value = res;
    return buffer + 2;
}

char* deserialize_struct_data(char *buffer, struct TCPHeader *value) {
    /* using the same policy in server code to deserialize the buffer and store
    values into the struct
    */
    char* buffer_incr = buffer;
    
    buffer_incr = deserialize_int16(buffer_incr, &value->src_port);
    buffer_incr = deserialize_int16(buffer_incr, &value->dst_port);
    buffer_incr = deserialize_int32(buffer_incr, &value->seq_num);
    buffer_incr = deserialize_int32(buffer_incr, &value->ack_num);
    buffer_incr = deserialize_int16(buffer_incr, &value->offset_reserved_ctrl);
    buffer_incr = deserialize_int16(buffer_incr, &value->window);
    buffer_incr = deserialize_int16(buffer_incr, &value->checksum);
    buffer_incr = deserialize_int16(buffer_incr, &value->urgent_pointer);
    buffer_incr = deserialize_int32(buffer_incr, &value->options);

    return buffer_incr;
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    struct TCPHeader tcp_header;
    memset(&tcp_header, 0, sizeof(struct TCPHeader));
    
    while(1) 
    {
        /* get a message from the user */
        bzero(buf, BUFSIZE);
        printf("File to request from server: ");
        fgets(buf, BUFSIZE, stdin);
    
        /* send the message to the server */
        serverlen = sizeof(serveraddr);
        n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sendto");
        
        /* print the server's reply */
        n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
        if (n < 0) 
          error("ERROR in recvfrom");
        /* printf("Server response: %s", buf); */
	deserialize_struct_data(buf, &tcp_header);
	
	printf("tcp_header.src_port = %d\n", tcp_header.src_port);
	printf("tcp_header.dst_port = %d\n", tcp_header.dst_port);
	printf("tcp_header.seq_num = %d\n", tcp_header.seq_num);
    }
    return 0;
}
