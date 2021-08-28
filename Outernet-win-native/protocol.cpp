#include <string.h>
#include "protocol.h"
#include "buffer.h"

Protocol::Protocol() {
	complete = false;
	cmd = CMD_UNKNOWN;
	memset(identification, 0, sizeof(identification));
	tun_ip = 0;
	dst_ip = 0;
}

int Protocol::parse_header(Buffer* data) {
	int ret = 0;
	if (data->get_len() < 1) return ret;
	cmd = data->get_buf()[0];
	ret++;
	if (cmd == CMD_CLIENT_HANDSHAKE) {
		if (data->get_len() < 1 + 32) return ret;
		memcpy(identification, data->get_buf() + 1, 32);
		ret += 32;
		complete = true;
	}
	else if (cmd == CMD_SERVER_HANDSHAKE) {
		if (data->get_len() < 1 + 8) return ret;
		memcpy((char*)&tun_ip, data->get_buf() + 1, 4);
		memcpy((char*)&dst_ip, data->get_buf() + 1 + 4, 4);
		ret += 8;
		complete = true;
	}
	else if (cmd == CMD_CLIENT_DATA) {
		if (data->get_len() < 1 + 32) return ret;
		memcpy(identification, data->get_buf() + 1, 32);
		ret += 32;
		complete = true;
	}
	else if (cmd == CMD_SERVER_DATA) {
		complete = true;
	}
	return ret;
}

bool Protocol::is_header_complete() {
	return complete;
}

void Protocol::get_header_bytes(Buffer* buf) {
	buf->clear();
	uint8_t cmd[1] = { 0 };
	cmd[0] = this->cmd;
	buf->insert_back(cmd, 1);
	if (this->cmd == CMD_CLIENT_HANDSHAKE) {
		buf->insert_back(identification, 32);
	}
	else if (this->cmd == CMD_SERVER_HANDSHAKE) {
		buf->insert_back((uint8_t*)&tun_ip, 4);
		buf->insert_back((uint8_t*)&dst_ip, 4);
	}
	else if (this->cmd == CMD_CLIENT_DATA) {
		buf->insert_back(identification, 32);
	}
	else if (this->cmd == CMD_SERVER_DATA) {
		// pass
	}
}