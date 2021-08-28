#pragma once
#include <stdint.h>

#define CMD_UNKNOWN 0x00
#define CMD_CLIENT_HANDSHAKE 0x01
#define CMD_SERVER_HANDSHAKE 0x02
#define CMD_CLIENT_DATA 0x03
#define CMD_SERVER_DATA 0x04

class Buffer;

class Protocol {
private:
	bool complete;
public:
	uint8_t cmd;
	uint8_t identification[32];
	uint32_t tun_ip;
	uint32_t dst_ip;

	Protocol();
	int parse_header(Buffer* data);
	bool is_header_complete();
	void get_header_bytes(Buffer* buf);
};