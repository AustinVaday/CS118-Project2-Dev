/* 
 * client.c - A simple UDP client
 * usage: udpclient <host> <port>
 */

#include "tcp.h"
#include <signal.h>

// Globals
// Out file needs to be global so we can
// safely close it in case user attempts to kill
// program
FILE *outputFile;
struct WindowPacket window[5];

int prev_seq_num = -1;
int prev_data_size = -1;
/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

void ctrl_c_handler(int var)
{
    fclose(outputFile);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char headerBuf[HEADERSIZE];
    char *dataBuf;
    char *serializationPtr; /* location after serialization of struct */
    char *tcpObject;
    int syn = 0;
    int three_way_hs_ack = 0;
    int file_requested = 0;
    int seq_num = 0;
    int ack_num = 0;
    int file_requested_size = 0;
    int ack_skip = 0;
    int send_packet = 0;


    signal(SIGINT, ctrl_c_handler);

    /* open output file */
    outputFile = fopen("received.data", "a");
    if (outputFile == NULL)
    {
        error("Cannot open output file.\n");
    }

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

    struct TCPHeader tcp_header_send; /* struct that holds tcp header info */
    memset(&tcp_header_send, 0, sizeof(struct TCPHeader));

    struct TCPHeader tcp_header_receive;
    memset(&tcp_header_receive, 0, sizeof(struct TCPHeader));
    
    while(1) 
    {
        memset(&tcp_header_send, 0, sizeof(struct TCPHeader));
        memset(&tcp_header_receive, 0, sizeof(struct TCPHeader));

        // Send syn packet if we haven't already
        if (!syn)
        {
            send_packet = 1;
            seq_num = 0;
            ack_num = 0;

            tcp_header_send.src_port = portno;
            tcp_header_send.dst_port = portno;
            tcp_header_send.seq_num  = seq_num;
            tcp_header_send.ack_num  = ack_num;
            tcp_header_send.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
            tcp_header_send.window = WINDOWSIZE;
            tcp_header_send.checksum = 0;
            tcp_header_send.urgent_pointer = 0;
            tcp_header_send.data_size = 0;

            set_syn_bit(&tcp_header_send);
            printf("Sending packet SYN\n");
        }
        else if (!three_way_hs_ack)
        {
            send_packet = 1;
            seq_num = 1;
            ack_num = 1;

            tcp_header_send.src_port = portno;
            tcp_header_send.dst_port = portno;
            tcp_header_send.seq_num  = seq_num;
            tcp_header_send.ack_num  = ack_num;
            tcp_header_send.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
            tcp_header_send.window = WINDOWSIZE;
            tcp_header_send.checksum = 0;
            tcp_header_send.urgent_pointer = 0;
            tcp_header_send.data_size = 0;

            set_ack_bit(&tcp_header_send);

            printf("Sending packet 1 (three way handshake complete)\n");
            // printf("Hex val for ofc:%x\n", tcp_header_send.offset_reserved_ctrl);
            three_way_hs_ack = 1;
            ack_skip = 1;

            // Do not expect to receive more data after this, continue to next iteration
            // continue;
        }
        else if (!file_requested)
        {
            send_packet = 1;
            seq_num = 1;
            ack_num = ack_num;

            tcp_header_send.src_port = portno;
            tcp_header_send.dst_port = portno;
            tcp_header_send.seq_num  = seq_num;
            tcp_header_send.ack_num  = ack_num;
            tcp_header_send.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
            tcp_header_send.window = 0;
            tcp_header_send.checksum = 0;
            tcp_header_send.urgent_pointer = 0;

            /* get a message from the user */
            bzero(buf, BUFSIZE);
            printf("File to request from server: ");
            fgets(buf, BUFSIZE, stdin);

            file_requested = 1;
            file_requested_size = strlen(buf);
            
        }
        else
        {
            // tcp_header_send.src_port = portno;
            // tcp_header_send.dst_port = portno;
            // tcp_header_send.seq_num  = 0;
            // tcp_header_send.ack_num  = tcp_header_receive.seq_num + tcp_header_receive.data_size;;
            // tcp_header_send.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
            // tcp_header_send.window = 0;
            // tcp_header_send.checksum = 0;
            // tcp_header_send.urgent_pointer = 0;
            // tcp_header_send.data_size = tcp_header_receive.data_size;
            // set_ack_bit(&tcp_header_send);

            // printf("Sending packet %d\n", tcp_header_send.ack_num);
        }

        // Serialize struct into character string
        //printTCPHeaderStruct(&tcp_header_send);

        if (send_packet)
        {
            send_packet = 0;
            serializationPtr = serialize_struct_data(headerBuf, &tcp_header_send);

            // Construct TCP object that consists of TCP header (headerBuf) + data (responseBuf)
            // Allocate enough space for header + response
            int tcpObjectLength = HEADERSIZE + strlen(buf);
            tcpObject = constructTCPObject(headerBuf, buf);

            /* send the message to the server */
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, tcpObject, tcpObjectLength, 0, &serveraddr, serverlen);
            if (n < 0) 
              error("ERROR in sendto");
        }

        // After threeway handshake is complete, ask user for file to send
        if (ack_skip == 1)
        {
            ack_skip = 0;
            continue;
        }

        /* print the server's reply */
        n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
        if (n < 0) 
          error("ERROR in recvfrom");
        /* printf("Server response: %s", buf); */

        // Store header byte stream into header buff
        int i;
        for (i = 0; i < HEADERSIZE; i++) {
           headerBuf[i] = buf[i];
        }
        
        // Point data buf to location where data begins
        dataBuf = buf + HEADERSIZE;

    	deserialize_struct_data(headerBuf, &tcp_header_receive);
    	

	//	printf("prev = %d\n", prev_seq_num);
	if ((prev_seq_num == -1) || (prev_seq_num == 0) || (tcp_header_receive.seq_num == (prev_seq_num + prev_data_size + 24))) {
	  // printTCPHeaderStruct(&tcp_header_receive);

	  printf("Receiving packet %d\n", tcp_header_receive.seq_num);
	  
	  prev_seq_num = tcp_header_receive.seq_num;
	  prev_data_size = tcp_header_receive.data_size;
	  
	  if (!syn && is_syn_bit_set(&tcp_header_receive) && is_ack_bit_set(&tcp_header_receive))
	    {
	      // printf("*** Server SYN ACK response! ***\n");
	      syn = 1;

	      // Need to send 1 more ack to finalize three way handshake.
	      three_way_hs_ack = 0;
	    }
	  // Write data from data buffer to local file
	  else if (tcp_header_receive.data_size != 0)
	    {
	      fwrite(dataBuf, 1, tcp_header_receive.data_size, outputFile);

	      tcp_header_send.src_port = portno;
	      tcp_header_send.dst_port = portno;
	      tcp_header_send.seq_num  = 0;
	      tcp_header_send.ack_num  = tcp_header_receive.seq_num + tcp_header_receive.data_size + 24;
	      tcp_header_send.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
	      tcp_header_send.window = 0;
	      tcp_header_send.checksum = 0;
	      tcp_header_send.urgent_pointer = 0;
	      tcp_header_send.data_size = tcp_header_receive.data_size;
	      set_ack_bit(&tcp_header_send);

	      printf("Sending packet %d\n", tcp_header_send.ack_num);

	      serializationPtr = serialize_struct_data(headerBuf, &tcp_header_send);

	      // Construct TCP object that consists of TCP header (headerBuf) + data (responseBuf)
	      // Allocate enough space for header + response
	      int tcpObjectLength = HEADERSIZE + strlen(buf);
	      tcpObject = constructTCPObject(headerBuf, buf);

	      /* send the message to the server */
	      serverlen = sizeof(serveraddr);
	      n = sendto(sockfd, tcpObject, tcpObjectLength, 0, &serveraddr, serverlen);
	      if (n < 0) 
		error("ERROR in sendto");

	      free(tcpObject);
	}
        }
        
        // If ack corresponds to our file request, do something
        if (tcp_header_receive.ack_num == file_requested_size)
        {
            // printf("Received ACK 9 from server lol\n");

        }

        // seq_num = ack_num;
        // ack_num = ack_num + tcp_header_receive.data_size + HEADERSIZE;
 
        memset(buf, 0, sizeof(buf));
        memset(dataBuf, 0, sizeof(dataBuf));

        }

        fclose(outputFile);
    return 0;
}
