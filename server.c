/* 
 * server.c - A simple UDP echo server 
 * usage: udpserver <port>
 */
#include "tcp.h"


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

// void readFileIntoBuffer(char *fileBuffer, FILE *file, int maxSize, int offset)
// {
//   fread(fileBuffer, PAYLOADSIZE, 1, file);
// }

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
  struct TCPHeader header_rec;
  char headerBuf[HEADERSIZE]; /* tcp header buffer */
  char *serializationPtr; /* location after serialization of struct */
  char *tcpObject;
  char *dataBuf;
  // char fileBuffer[PAYLOADSIZE];
  char responseBuf[PAYLOADSIZE];
  int responseBufSize = 0;
  int syn = 0;
  int seq_num = 0;
  int ack_num = 0;

  memset(headerBuf, 0, HEADERSIZE);

//   file = fopen("server.c", "rb");
  
//   while (fread(fileBuffer, PAYLOADSIZE, 1, file) > 0)
//   {
//     for (int i = 0; i < PAYLOADSIZE; i++)
//     {
//       printf("%c", fileBuffer[i]);
//     }
//     printf("BRUH**************************\n");
//     memset(fileBuffer, 0, PAYLOADSIZE);
//   }

// for (int i = 0; i < PAYLOADSIZE; i++)
//     {
//       printf("%c", fileBuffer[i]);
//     }
//     printf("BRUH**************************\n");

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
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, (socklen_t*) &clientlen);

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
    // printf("server received datagram from %s (%s)\n", 
	   // hostp->h_name, hostaddrp);
    // printf("server received %d/%d bytes: %s\n", (int) strlen(buf), n, buf);
    
    // Parse tcp object buf into corresponding header and data buffers
    // Store header byte stream into header buff
    int i;
    for (i = 0; i < HEADERSIZE; i++) {
       headerBuf[i] = buf[i];
    }
    
    // Deserialize data 
    deserialize_struct_data(headerBuf, &header_rec);

    // Point data buf to location where data begins
    dataBuf = buf + HEADERSIZE;

    printf("Receiving packet %d\n", header_rec.seq_num);

    if (/*!syn &&*/ is_syn_bit_set(&header_rec))
    {
      header.src_port = portno;
      header.dst_port = portno;
      header.seq_num  = seq_num;
      header.ack_num  = ack_num++;
      header.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
      header.window = WINDOWSIZE;
      header.checksum = 0;
      header.urgent_pointer = 0;
      header.options = 0;

      set_ack_bit(&header);
      set_syn_bit(&header);

      printf("Sending packet %d %d SYN\n", seq_num, WINDOWSIZE);
    }
    // If client sends ack (after syn)
    else if (is_ack_bit_set(&header_rec))
    {
      // Just skip to next iteration, do not need to send anything
      // bzero((struct TCPHeader *) &header_rec, sizeof(struct TCPHeader));
      continue;
    }
    else {

      if (!file)
      {
        // Attempt to open requested file in buf
        replaceNewlineWithTerminator(dataBuf);
        file = fopen(dataBuf, "rb");
      }

      // Check if file resides on system
      if (file)
      {

          // Read payload bytes into buffer
          memset(responseBuf, 0, PAYLOADSIZE);
          responseBufSize = fread(responseBuf, 1, PAYLOADSIZE, file);

          printf("Response buf size is %d: ", responseBufSize);

          for (int i = 0; i < responseBufSize; i++)
          {
            printf("%c", responseBuf[i]);
          }

          if (responseBufSize == 0)
          {
            exit(0);
          }
          // exit(0);

          // fclose(file);
      }
      else 
      {
          sprintf(responseBuf, "Sorry, could not find file '%s' on our system.\n", dataBuf);
          responseBufSize = strlen(responseBuf);
      }
      
      // printf("Server response: %s\n", responseBuf);

      header.src_port = portno;
      header.dst_port = 2;
      header.seq_num  = 3;
      header.ack_num  = 4;
      header.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
      header.window = 6;
      header.checksum = 7;
      header.urgent_pointer = 8;
      header.options = 9;

      printf("Sending packet ACK for file transfer %d %d\n", seq_num, WINDOWSIZE);

    }

    

    /* 
     * sendto: echo the input back to the client 
     */
    /* n = sendto(sockfd, responseBuf, strlen(responseBuf), 0,  */
    /*      (struct sockaddr *) &clientaddr, clientlen); */
    /* if (n < 0)  */
    /*   error("ERROR in sendto"); */


    // printf("Testing bit sets. Initial is: %x\n", header.offset_reserved_ctrl);
    // set_fin_bit(&header);
    // printf("After setting fin bit: %x\n", header.offset_reserved_ctrl);
    // set_syn_bit(&header);
    // printf("After setting syn bit: %x\n", header.offset_reserved_ctrl);
    // set_ack_bit(&header);
    // printf("After setting syn bit: %x\n", header.offset_reserved_ctrl);

    //printTCPHeaderStruct(&header);

    serializationPtr = serialize_struct_data(headerBuf, &header);

    // Construct TCP object that consists of TCP header (headerBuf) + data (responseBuf)
    // Allocate enough space for header + response
    int tcpObjectLength = HEADERSIZE + responseBufSize;
    tcpObject = constructTCPObject(headerBuf, responseBuf);


    // n = sendto(sockfd, headerBuf, serializationPtr - headerBuf, 0, 
    //      (struct sockaddr *) &clientaddr, clientlen);
    n = sendto(sockfd, tcpObject, tcpObjectLength, 0, 
         (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");

    // Free pointers created via constructTCPObject
    free(tcpObject);
  }
}
