/* 
 * server.c - A simple UDP echo server 
 * usage: udpserver <port>
 */
#include "tcp.h"

struct WindowPacket window[5];
// int windowMinPacketByte = 0;
int done = 0;
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


void retransmit(int index)
{
  printf("Retransmit packet %d in window...\n", index);
}

// Function to help us check the array of timers for each packet
void *threadFunction(void * args)
{
  // Continuously check if a packet needs to be retransmitted
  time_t currentTime;
  
  printf("Thread created.\n");
  while (!done)
  {
    for (int i = 0; i < WINDOWSIZE / PACKETSIZE; i++)
    {
      currentTime = time(NULL);

      if (!window[i].valid || window[i].tcpObject == NULL)
      {
        // If no packet, just skip and go to next iteration
        continue;
      }
      // Retransmit if > 500MS
      if ((currentTime - window[i].transmissionTime) <= 0.5)
      {
          // retransmit(i);
      }
    }

  }

  return NULL;
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
  pthread_t threadId;
  int numPacketsToSend;

  // Allocate proper memory
  // window = (struct WindowPacket *) malloc(WINDOWSIZE / PACKETSIZE); // 5 elements

  // if (window == NULL)
  // {
  //   error("Could not allocate memory for window ptr\n");
  // }

  // Set all window values to invalid
  for (int i = 0; i < WINDOWSIZE / PACKETSIZE; i++)
  {
    window[i].valid = 0;
    window[i].acked = 0;
  }

  memset(headerBuf, 0, HEADERSIZE);

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

  // Create thread to help us detect necessary retransmissions based on timers
  pthread_create(&threadId, NULL, &threadFunction, NULL);

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
      header.data_size = 0;

      set_ack_bit(&header);
      set_syn_bit(&header);

      numPacketsToSend = 1;
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

          // Check how many packets we want to send so we can organize reads 
          // accordingly
          numPacketsToSend = (WINDOWSIZE / PAYLOADSIZE);


          // Read payload bytes into buffer
          memset(responseBuf, 0, PAYLOADSIZE);
          responseBufSize = fread(responseBuf, 1, PAYLOADSIZE, file);

          // printf("Response buf size is %d: ", responseBufSize);

          // for (int i = 0; i < responseBufSize; i++)
          // {
          //   printf("%c", responseBuf[i]);
          // }

          // File has finished sending.. do something other than exit
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

      // Check if this is the response after client requests file
      if (header_rec.seq_num == 1 && header_rec.ack_num == 1)
      {
          // Ack number is previous + length of client's request
          ack_num = 1 + strlen(dataBuf); 
          printf("Ack num is now: %d\n", ack_num);
      }
      else 
      {
        // seq_num = header_rec.seq_numV;
        // ack_num = ack_num + header_rec.ack_num + HEADERSIZE;
      }

      header.src_port = portno;
      header.dst_port = portno;
      header.seq_num  = seq_num;
      header.ack_num  = ack_num;
      header.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
      header.window = 0;
      header.checksum = 0;
      header.urgent_pointer = 0;
      header.data_size = responseBufSize;

      printf("Sending packet %d %d\n", seq_num, WINDOWSIZE);

    }


    // Compute number of packets to send out 
    if (!file || is_syn_bit_set(&header_rec) || is_ack_bit_set(&header_rec))
    {
      numPacketsToSend = 1;
    }
    else 
    {
      // Move window sizes as necessary (i.e. if packet gets acked move window 
      // to right by 1)
      // If initial phase, 
      int availableSlots;
      for (availableSlots = 0; availableSlots < WINDOWSIZE / PACKETSIZE; availableSlots++)
      {
        // If first packet is acked already, move to right by 1
        if (window[0].acked)
        {
          shiftWindowRightN(window, WINDOWSIZE, 1);
        }
        else {
          break;
        }
      }

      numPacketsToSend = availableSlots;
    }

    for (int i = 0; i < numPacketsToSend; i++)
    {
      serializationPtr = serialize_struct_data(headerBuf, &header);

      // Construct TCP object that consists of TCP header (headerBuf) + data (responseBuf)
      // Allocate enough space for header + response
      int tcpObjectLength = HEADERSIZE + responseBufSize;
      tcpObject = constructTCPObject(headerBuf, responseBuf);

      window[i].tcpObject = tcpObject;
      window[i].tcpObjectLength = tcpObjectLength;
      window[i].valid = 1;
      window[i].transmissionTime = time(NULL);

      n = sendto(sockfd, tcpObject, tcpObjectLength, 0, 
           (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0) 
        error("ERROR in sendto");

      // // Free pointers created via constructTCPObject
      // free(tcpObject);
    }

  }

  // free(window);
  return 0;
}
