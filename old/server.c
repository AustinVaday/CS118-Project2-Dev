/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <unistd.h>
#include <arpa/inet.h>

#define BUFF_LEN 512

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     int numPackets = 5;
     unsigned int slen=sizeof(cli_addr);
     char buffer[BUFF_LEN];

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);	//create socket
     if (sockfd < 0) 
        error("ERROR opening socket");
     memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
     //fill in address info
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     printf("Before bind\n");
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     printf("After bind\n");
     for (int i = 0; i < numPackets; i++)
     {
        printf("Before recv\n");
        if (recvfrom(sockfd, buffer, BUFF_LEN, 0, (struct sockaddr *)&cli_addr, &slen) < 0)
        {
            error("ERROR ON RECV");
        }

        printf("Received packet from %s:%d\nData: %s\n\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer); 
     }
//     listen(sockfd,5);	//5 simultaneous connection at most
//     
//     //accept connections
//     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
//         
//     if (newsockfd < 0) 
//       error("ERROR on accept");
         
//     int n;
//   	 char buffer[256];
//   			 
//   	 memset(buffer, 0, 256);	//reset memory
//      
// 		 //read client's message
//   	 n = read(newsockfd,buffer,255);
//   	 if (n < 0) error("ERROR reading from socket");
//   	 printf("Here is the message: %s\n",buffer);
//   	 
//   	 //reply to client
//   	 n = write(newsockfd, "Gotcha. Searching for ",27);
//   	 n = write(newsockfd, buffer, strlen(buffer));
//   	 if (n < 0) error("ERROR writing to socket");
//         
//     
//     close(newsockfd);//close connection 
     close(sockfd);
         
     return 0; 
}

