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
