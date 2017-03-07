/* 
 * server.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

// Helpful when retrieving commands from client
// with newlines and when we want to remove them
void replaceNewlineWithTerminator(char *buf) {
  if(buf[strlen(buf) - 1] == '\n')
  {
          buf[strlen(buf) - 1] = '\0';
  }
}

void printBufHex(char *buf) {
  int i;
  int x = strlen(buf);
  for (i = 0; i < x; i++)
  {
    if (i > 0) printf(":");
      printf("%02X", buf[i]);
  }
  printf("\n");
}


struct TCPHeader {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq_num;
  uint32_t ack_num;
  uint16_t offset_reserved_ctrl; // Merge data offset, reserved and control bits into one 16-bit val
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent_pointer;
  uint32_t options;
};

char * serialize_int32(char *buffer, uint32_t value)
{
  /* Write big-endian int value into buffer; assumes 32-bit int and 8-bit char. */
  buffer[0] = value >> 24;
  buffer[1] = value >> 16;
  buffer[2] = value >> 8;
  buffer[3] = value;
  return buffer + 4;
}

char * serialize_int16(char *buffer, int value)
{
  /* Write big-endian int value into buffer; assumes 32-bit int and 8-bit char. */
  buffer[0] = value >> 8;
  buffer[1] = value;
  return buffer + 2;
}

// unsigned char * serialize_char(unsigned char *buffer, char value)
// {
//   buffer[0] = value;
//   return buffer + 1;
// }

char * serialize_struct_data(char *buffer, struct TCPHeader *value)
{
  buffer = serialize_int16(buffer, value->src_port);
  buffer = serialize_int16(buffer, value->dst_port);
  buffer = serialize_int32(buffer, value->seq_num);
  buffer = serialize_int32(buffer, value->ack_num);
  buffer = serialize_int16(buffer, value->offset_reserved_ctrl);
  buffer = serialize_int16(buffer, value->window);
  buffer = serialize_int16(buffer, value->checksum);
  buffer = serialize_int16(buffer, value->urgent_pointer);
  buffer = serialize_int32(buffer, value->options);

  return buffer;
}


int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  FILE * file; /* file that is requested from client */
  struct TCPHeader header; /* struct that holds tcp header info */
  char headerBuf[BUFSIZE]; /* tcp header buffer */
  char *serializationPtr; /* location after serialization of struct */
  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    char responseBuf[BUFSIZE];

    // Attempt to open requested file in buf
    replaceNewlineWithTerminator(buf);
    file = fopen(buf, "r");
    
    // Check if file resides on system
    if (file)
    {
        sprintf(responseBuf, "Awesome! We found file '%s' on our system.\n", buf);
        fclose(file);
    }
    else 
    {
        sprintf(responseBuf, "Sorry, could not find file '%s' on our system.\n", buf);
    }
    
    printf("Server response: %s\n", responseBuf);

    /* 
     * sendto: echo the input back to the client 
     */
    n = sendto(sockfd, responseBuf, strlen(responseBuf), 0, 
         (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");

    header.src_port = 1;
    header.dst_port = 2;
    header.seq_num  = 3;
    header.ack_num  = 4;
    header.offset_reserved_ctrl = 5; // Merge data offset, reserved and control bits into one 16-bit val
    header.window = 6;
    header.checksum = 7;
    header.urgent_pointer = 8;
    header.options = 9;

    serializationPtr = serialize_struct_data(headerBuf, &header);
    serializationPtr[1] = '\0';

    printf("Length of headerBuf is: %d", strlen(headerBuf));
    printf("Attempting to send headerBuf with data: %s\n", headerBuf);
    printf("Location of headerBuf is: %p\n", headerBuf);
    printf("Location of serializationPtr is: %p\n", serializationPtr);

    n = sendto(sockfd, headerBuf, serializationPtr - headerBuf, 0, 
         (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");
  }
}
