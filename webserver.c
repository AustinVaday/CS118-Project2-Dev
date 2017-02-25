#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#define BUF_SIZE 1024

int PORT_NUM = 6037;

int newsockfd;
char *recv_buf;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

char* get_date_string(time_t t) {
    int len = 512;  // size of a large enough C string buffer    
    struct tm time_s = *localtime(&t);

    char *date_str = (char*) malloc(len);
    bzero(date_str, len);
    
    char *weekday_str = (char*) malloc(8 * sizeof(char));
    switch (time_s.tm_wday) {
    case 0:
	strcpy(weekday_str, "Sun");
	break;
    case 1:
	strcpy(weekday_str, "Mon");
	break;
    case 2:
	strcpy(weekday_str, "Tue");
	break;
    case 3:
	strcpy(weekday_str, "Wed");
	break;
    case 4:
	strcpy(weekday_str, "Thu");
	break;
    case 5:
	strcpy(weekday_str, "Fri");
	break;
    case 6:
	strcpy(weekday_str, "Sat");
	break;
    }

    char *month_str = (char*) malloc(8 * sizeof(char));
    switch (time_s.tm_mon) {
    case 0:
	strcpy(month_str, "Jan");
	break;
    case 1:
	strcpy(month_str, "Feb");
	break;
    case 2:
	strcpy(month_str, "Mar");
	break;
    case 3:
	strcpy(month_str, "Apr");
	break;
    case 4:
	strcpy(month_str, "May");
	break;
    case 5:
	strcpy(month_str, "Jun");
	break;
    case 6:
	strcpy(month_str, "Jul");
	break;
    case 7:
	strcpy(month_str, "Aug");
	break;
    case 8:
	strcpy(month_str, "Sep");
	break;
    case 9:
	strcpy(month_str, "Oct");
	break;
    case 10:
	strcpy(month_str, "Nov");
	break;
    case 11:
	strcpy(month_str, "Dec");
	break;
    }

    sprintf(date_str, "%s, %02d %s %d %02d:%02d:%02d GMT",
	    weekday_str, time_s.tm_mday, month_str, (time_s.tm_year + 1900),
	    time_s.tm_hour, time_s.tm_min, time_s.tm_sec);

    return date_str;    
}

void send_http_response(char *file_path) {
    /*
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
	error("Error opening requested HTML object");
    }
    */

    int n, i, len = 16;
    // check requested object type: HTML or JPEG or GIF
    int is_html = 0, is_jpeg = 0, is_gif = 0;
    char *filename_ext5 = (char*) malloc(len * sizeof(char));  // filename_extension
    memset(filename_ext5, 0, len);
    char *filename_ext4 = (char*) malloc(len * sizeof(char));
    memset(filename_ext4, 0, len);
    
    strcpy(filename_ext5, (file_path + strlen(file_path) - 5));
    strcpy(filename_ext4, (file_path + strlen(file_path) - 4));
    fprintf(stderr, "Filename extension of the requested file: %s OR %s.\n", filename_ext5, filename_ext4);
    
    if ( strcmp(filename_ext5, ".html") == 0 ) {
	is_html = 1;
    } else if ( strcmp(filename_ext5, ".jpeg") == 0 || strcmp(filename_ext4, ".jpg") == 0 ) {
	is_jpeg = 1;
    } else if ( strcmp(filename_ext4, ".gif") == 0 ) {
	is_gif = 1;
    } else {
	fprintf(stderr, "Error: the requested file type is undefined.\n");
	return;
    }

    // read the requested HTML object file into buffer
    FILE *f;
    if (is_html) {
	f = fopen(file_path, "r");
    } else {
	f = fopen(file_path, "rb");  // read file with BINARY flag
    }
    if (f == NULL) {
	perror("Error opening requested HTML object");
	return;
    }

    int file_size = 0;
    struct stat file_attr;
    if (stat(file_path, &file_attr) == 0) {
	file_size = file_attr.st_size;
    }
    
    char *object_data = (char*) malloc(file_size + 1);
    memset(object_data, 0, file_size + 1);
    fread(object_data, sizeof(char), file_size, f);

    /* legacy approach to read in a file, which is typically used for growing files with no fixed size
    int str_size = BUF_SIZE, file_size = 0;
    char *object_data = (char*) malloc(str_size);
    bzero(object_data, str_size);
    char *file_buf = (char*) malloc(BUF_SIZE);
    memset(file_buf, 0, BUF_SIZE);

    int n;
    while ( (n = read(fd, file_buf, BUF_SIZE - 1)) != 0) {
	fprintf(stderr, "Read %d bytes from requested HTML file this round.\n", n);

	// expand the HTML object buffer to read later part of the file
	strcat(object_data, file_buf);
	file_size += n;
	str_size += (BUF_SIZE - 1);
	object_data = (char*) realloc(object_data, str_size);

	memset(file_buf, 0, BUF_SIZE);
    }
    object_data[file_size] = 0;
    */    
    
    // generate the HTTP response message, including the header and data
    char *response_str = (char*) malloc(BUF_SIZE + file_size);
    char *header_line = (char*) malloc(BUF_SIZE);
    bzero(response_str, BUF_SIZE + file_size);
    bzero(header_line, BUF_SIZE);

    sprintf(header_line, "HTTP/1.1 200 OK\r\n");
    strcpy(response_str, header_line);

    time_t current_time = time(NULL);
    sprintf(header_line, "Date: %s\r\n", get_date_string(current_time));
    strcat(response_str, header_line);

    sprintf(header_line, "Server: C/1.0.0 (Ubuntu)\r\n");
    strcat(response_str, header_line);

    time_t file_mtime = file_attr.st_mtime;
    sprintf(header_line, "Last-Modified: %s\r\n", get_date_string(file_mtime));
    strcat(response_str, header_line);

    sprintf(header_line, "Content-Length: %d\r\n", file_size);
    strcat(response_str, header_line);

    if (is_html) {
	sprintf(header_line, "Content-Type: text/html\r\n\r\n");
    } else if (is_jpeg) {
	sprintf(header_line, "Content-Type: image/jpeg\r\n\r\n");
    } else if (is_gif) {
	sprintf(header_line, "Content-Type: image/gif\r\n\r\n");
    }
    strcat(response_str, header_line);

    // attach data to the HTTP response message; note that no string-related functions can be used for binary data
    //strcat(response_str, object_data);
    int header_len = strlen(response_str);
    int response_len = header_len + file_size;
    for (i = header_len; i < response_len; i++) {
	response_str[i] = object_data[i - header_len];
    }
    
    fprintf(stderr, "--------Beginning of HTTP Response--------\n");
    fprintf(stderr, "%s\n", response_str);
    fprintf(stderr, "--------End of HTTP Response--------------\n");
    
    if ( write(newsockfd, response_str, response_len) < 0 ) {
	fprintf(stderr, "Error writing HTTP response to socket.\n");
	exit(1);
    }

    fprintf(stderr, "HTTP Response sent.\n");
    return;
}

int main(int argc, char *argv[])
{
    int sockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);	
    if (sockfd < 0) 
        error("Error opening socket");
    memset((char*) &serv_addr, 0, sizeof(serv_addr));
    portno = PORT_NUM;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
     
    if ( bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0 ) 
	error("Error on binding");

    listen(sockfd, 5);	 // allowing 5 simultaneous connections at most
    fprintf(stderr, "Socket listening at localhost:%d\n", PORT_NUM);
     
    // accept connections from browsers
    newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
         
    if (newsockfd < 0) 
	error("ERROR on accept");
    
    int n;
    recv_buf = (char*) malloc(BUF_SIZE);
    bzero(recv_buf, BUF_SIZE);
    
    // read client's HTTP requests continuously
    while ( (n = read(newsockfd, recv_buf, BUF_SIZE - 1)) != 0 ) {
	if (n < 0)
	    error("ERROR reading from socket");
	printf("--------Beginning of HTTP Request--------\n");
	printf("%s\n", recv_buf);
	printf("--------End of HTTP Request--------------\n");

	// get the requested object's file path and send back HTTP response
	int i, len = 128;
	char *file_path = (char*) malloc(len * sizeof(char));
	memset(file_path, 0, len);
	char temp_char;
	for (i = 5; (temp_char = recv_buf[i]) != ' '; i++) {
	    file_path[strlen(file_path)] = temp_char;
	}
	fprintf(stderr, "Requested object: %s\n", file_path);
	
	send_http_response(file_path);
    }

    close(newsockfd);
    close(sockfd);
         
    return 0; 
}

    /* sending HTTP response of 404 Not Found, if a non-existent file is requested
    if (fd < 0) {
	fprintf(stderr, "Error opening requested file; sending error HTTP response.\n");

	char *response_str = (char*) malloc(BUF_SIZE);
	char *header_line = (char*) malloc(BUF_SIZE);
	bzero(response_str, BUF_SIZE);
	bzero(header_line, BUF_SIZE);

	sprintf(response_str, "HTTP/1.1 404 Not Found\n");

	time_t current_time = time(NULL);
	sprintf(header_line, "Date: %s\n", get_date_string(current_time));
	strcat(response_str, header_line);

	sprintf(header_line, "Server: C/1.0.0 (Ubuntu)\n");
	strcat(response_str, header_line);
	
	fprintf(stderr, "--------Beginning of HTTP Response--------\n");
	fprintf(stderr, "%s\n", response_str);
	fprintf(stderr, "--------End of HTTP Response--------------\n");
	
	if ( write(newsockfd, response_str, strlen(response_str)) < 0 ) {
	    fprintf(stderr, "Error writing HTTP response to socket.\n");
	    exit(1);
	}

    }
    */
