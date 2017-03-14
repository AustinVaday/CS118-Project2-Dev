/* 
 * client.c - A simple UDP client
 * usage: udpclient <host> <port>
 */

#include "tcp.h"

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
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
        /* get a message from the user */
        bzero(buf, BUFSIZE);
        printf("File to request from server: ");
        fgets(buf, BUFSIZE, stdin);
    
        // Construct TCP header
        tcp_header_send.src_port = portno;
        tcp_header_send.dst_port = portno;
        tcp_header_send.seq_num  = 3;
        tcp_header_send.ack_num  = 4;
        tcp_header_send.offset_reserved_ctrl = 0; // Merge data offset, reserved and control bits into one 16-bit val
        tcp_header_send.window = 6;
        tcp_header_send.checksum = 7;
        tcp_header_send.urgent_pointer = 8;
        tcp_header_send.options = 9;

        // Serialize struct into character string
        printTCPHeaderStruct(&tcp_header_send);

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

        printf("The serialized buffer: each number represents a byte\n----headerBuf[0] to headerBuf[HEADERSIZE - 1]----\n");
        for (i = 0; i < HEADERSIZE; i++) {
           printf("%x ", headerBuf[i]);
        }
        printf("\n--------\n");

        printf("The data sent was: %s", dataBuf);

    	deserialize_struct_data(headerBuf, &tcp_header_receive);
    	
        printTCPHeaderStruct(&tcp_header_receive);


        }
    return 0;
}
