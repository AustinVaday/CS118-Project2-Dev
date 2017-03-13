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
   |                    Options                    |    Padding    |
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
  uint32_t options;
};

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