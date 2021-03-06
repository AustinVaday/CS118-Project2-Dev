#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

// TCP Header Size total number of bits
// We need to know the size of a TCP header
// so that we can parse data sent over udp accordingly
#define HEADERSIZE 24
#define BUFSIZE 1024
#define PACKETSIZE 1024
#define WINDOWSIZE 5120
// Note: Below payload size does not account for UDP header
#define PAYLOADSIZE 1000 // Max packet length (1024) - TCP header size (24)
#define RETRANSMISSIONTIME 500
#define MAXSEQNUM 30720
#define BILLION 1000000000L

/*

    0                   1                   2                   3   
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    data_size                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                            TCP Header Format


*/

struct TCPHeader {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq_num;
  uint32_t ack_num;
  uint16_t offset_reserved_ctrl; // Merge data offset, reserved and control bits into one 16-bit val
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent_pointer;
  uint32_t data_size;
};

// Data structure stored in window array 
struct WindowPacket
{
  char *tcpObject;
  int tcpObjectLength;
  struct timespec transmissionTime;
  int valid;
  int acked;
  int seqNum;
};

// replace index at target with value
void replace(struct WindowPacket* window, int targetIndex, int valueIndex)
{
	int isValueIndexValid = window[valueIndex].valid;

	// Ensure to invalidate shifted index value
	// Set this immediately to help limit possible race conditions
	window[valueIndex].valid = 0; 

	window[targetIndex].tcpObject = window[valueIndex].tcpObject;
	window[targetIndex].tcpObjectLength = window[valueIndex].tcpObjectLength;
	window[targetIndex].transmissionTime = window[valueIndex].transmissionTime;
	window[targetIndex].valid = isValueIndexValid;
	window[targetIndex].acked = window[valueIndex].acked;
	window[targetIndex].seqNum = window[valueIndex].seqNum;

  // window[valueIndex].transmissionTime = time(NULL);
	window[valueIndex].acked = 0;

}

// Shifts TCP Window frames to right (i.e. left-most packets get dropped)
void shiftWindowRightN(struct WindowPacket* window, int windowSize, int n)
{
	for (int i = n; i < windowSize; i++)
	{
		replace(window, i - n, i);
	}
}

int windowIndexWithSeqNum(struct WindowPacket* window, int windowSize, int seq_num)
{
	for (int i = 0; i < windowSize; i++)
	{
		if (window[i].seqNum == seq_num)
		{
			return i;
		}
	}

	return -1;
}

void printWindow(struct WindowPacket *window, int windowSize)
{
	printf("Printing TCP WINDOW\n");
	printf("--------------------------\n");
	for (int i = 0; i < windowSize; i++)
	{
		printf("**************************\n");
		printf("tcpObject -> (not printing this)\n");
		printf("tcpObjectLength -> %d\n", window[i].tcpObjectLength);
		printf("transmissionTime -> %ld\n", window[i].transmissionTime);
		printf("valid -> %d\n", window[i].valid);
		printf("acked -> %d\n", window[i].acked);
		printf("seqNum -> %d\n", window[i].seqNum);
		printf("**************************\n");

	}
	printf("--------------------------\n");

}

void printTCPHeaderStruct(struct TCPHeader *header)
{
	printf("Printing TCP Header struct\n");
	printf("--------------------------\n");
	printf("header->src_port = %d\n", header->src_port);
	printf("header->dst_port = %d\n", header->dst_port);
	printf("header->seq_num = %d\n", header->seq_num);
	printf("header->ack_num = %d\n", header->ack_num);
    printf("header->offset_reserved_ctrl = %d\n", header->offset_reserved_ctrl);
    printf("header->window = %d\n", header->window);
    printf("header->checksum = %d\n", header->checksum);
    printf("header->urgent_pointer = %d\n", header->urgent_pointer);
    printf("header->data_size = %d\n", header->data_size);    
    printf("--------------------------\n");

}

char *constructTCPObject(char *tcpHeader, char *data)
{

  int tcpObjectLength = HEADERSIZE + strlen(data);
  char *tcpObject = (char *) malloc(tcpObjectLength);
  int i;

  for (i = 0; i < HEADERSIZE; i++) {
    tcpObject[i] = tcpHeader[i];
  }

  int objContinuedIndex = HEADERSIZE;
  for (i = 0; i < strlen(data); i++, objContinuedIndex++)
  {
    tcpObject[objContinuedIndex] = data[i]; 
  }

  // printf("The TCP object: each number represents a byte\n----tcpObject[0] to tcpObject[tcpObjectLength - 1]----\n");
  // for (i = 0; i < tcpObjectLength; i++) {
  //     printf("%x ", tcpObject[i] & 0xFF);  // mask out upper bits when C automatically converts char to int in vararg functions
  // }
  // printf("\n--------\n");
  return tcpObject;
}


char * serialize_int32(char *buffer, uint32_t value)
{
    /* we choose to store the most significant byte of the integer to the least
significant byte of the serialization buffer;
the policy does not matter as long as the same is used in client code to deserialize
    */
  /* Write big-endian int value into buffer; assumes 32-bit int and 8-bit char. */
  /* buffer[0] = value >> 24; */
  /* buffer[1] = value >> 16; */
  /* buffer[2] = value >> 8; */
  /* buffer[3] = value; */

    // little-endian serialization approach
    buffer[0] = value;
    buffer[1] = value >> 8;
    buffer[2] = value >> 16;
    buffer[3] = value >> 24;
    return buffer + 4;
}

char * serialize_int16(char *buffer, uint16_t value)
{
    /* Write big-endian int value into buffer; assumes 16-bit int and 8-bit char. */
    /* buffer[0] = value >> 8; */
    /* buffer[1] = value; */

    // little-endian serialization approach
    buffer[0] = value;
    buffer[1] = value >> 8;
    return buffer + 2;
}

char * serialize_struct_data(char *buffer, struct TCPHeader *value)
{
    char* buffer_incr = buffer;
    buffer_incr = serialize_int16(buffer_incr, value->src_port);
    buffer_incr = serialize_int16(buffer_incr, value->dst_port);
    buffer_incr = serialize_int32(buffer_incr, value->seq_num);
    buffer_incr = serialize_int32(buffer_incr, value->ack_num);
    buffer_incr = serialize_int16(buffer_incr, value->offset_reserved_ctrl);
    buffer_incr = serialize_int16(buffer_incr, value->window);
    buffer_incr = serialize_int16(buffer_incr, value->checksum);
    buffer_incr = serialize_int16(buffer_incr, value->urgent_pointer);
    buffer_incr = serialize_int32(buffer_incr, value->data_size);

    return buffer_incr;
}

char* deserialize_int32(char *buffer, uint32_t *value) {
    /* Big endian serialization approach
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
    */

    // little endian serialization approach
    uint32_t res = 0, temp = 0;
    temp = 0;
    temp = buffer[3];
    temp = (temp << 24) & 0xFF000000;  // BUG: pay attention to the bitwise AND
    res |= temp;

    temp = 0;
    temp = buffer[2];
    temp = (temp << 16) & 0x00FF0000;
    res |= temp;

    temp = 0;
    temp = buffer[1];
    temp = (temp << 8) & 0x0000FF00;
    res |= temp;

    temp = 0;
    temp = (buffer[0]) & 0x000000FF;
    res |= temp;

    *value = res;
    return buffer + 4;
}

char* deserialize_int16(char *buffer, uint16_t *value) {
    /* one_byte = buffer[0] << 8; */
    /* res |= one_byte; */

    /* one_byte = 0; */
    /* one_byte = buffer[1]; */
    /* res |= one_byte; */
    
    uint16_t res = 0, temp = 0;
    temp = buffer[1];
    // printf("temp = buffer[1] = %x\n", temp);
    temp = (temp << 8) & 0xFF00;
    // printf("temp << 8 = %x\n", temp);
    res |= temp;
    // printf("res = %x\n", res);

    temp = 0;
    temp = buffer[0] & 0x00FF;
    // BUG: should be "0000 0000 1101 0011", actually "1111 1111 1101 0011"
    // printf("temp = buffer[0] = %x\n", temp);
    res |= temp;
    // printf("res = %x\n", res);
    
    *value = res;
    return buffer + 2;
}

char* deserialize_struct_data(char *buffer, struct TCPHeader *value) {
    /* using the same policy in server code to deserialize the buffer and store
    values into the struct
    */
    char* buffer_incr = buffer;

    // printf("----deserializing src_port: ...\n");
    buffer_incr = deserialize_int16(buffer_incr, &value->src_port);
    // printf("----deserializing src_port done.\n");
    buffer_incr = deserialize_int16(buffer_incr, &value->dst_port);
    buffer_incr = deserialize_int32(buffer_incr, &value->seq_num);
    buffer_incr = deserialize_int32(buffer_incr, &value->ack_num);
    buffer_incr = deserialize_int16(buffer_incr, &value->offset_reserved_ctrl);
    buffer_incr = deserialize_int16(buffer_incr, &value->window);
    buffer_incr = deserialize_int16(buffer_incr, &value->checksum);
    buffer_incr = deserialize_int16(buffer_incr, &value->urgent_pointer);
    buffer_incr = deserialize_int32(buffer_incr, &value->data_size);

    return buffer_incr;
}


// Check 16th bit of offset_reserved_ctrl
int is_fin_bit_set(struct TCPHeader *data)
{
	uint16_t orc = data->offset_reserved_ctrl;
	uint16_t mask = 0x0001;
	return orc & mask;
}

// Check 15th bit of offset_reserved_ctrl
int is_syn_bit_set(struct TCPHeader *data)
{
	uint16_t orc = data->offset_reserved_ctrl;
	uint16_t mask = 0x0002;
	return orc & mask;
}

// Check 12th bit of offset_reserved_ctrl
int is_ack_bit_set(struct TCPHeader *data)
{
	uint16_t orc = data->offset_reserved_ctrl;
	uint16_t mask = 0x0010;
	return orc & mask;
}

// Set 16th bit of offset_reserved_ctrl
void set_fin_bit(struct TCPHeader *data)
{
	uint16_t orc = data->offset_reserved_ctrl;
	uint16_t mask = 0x0001;
	data->offset_reserved_ctrl = orc | mask;
}

// Set 15th bit of offset_reserved_ctrl
void set_syn_bit(struct TCPHeader *data)
{
	uint16_t orc = data->offset_reserved_ctrl;
	uint16_t mask = 0x0002;
	data->offset_reserved_ctrl = orc | mask;
}

// Set 12th bit of offset_reserved_ctrl
void set_ack_bit(struct TCPHeader *data)
{
	uint16_t orc = data->offset_reserved_ctrl;
	uint16_t mask = 0x0010;
	data->offset_reserved_ctrl = orc | mask;
}
