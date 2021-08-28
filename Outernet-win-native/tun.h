#pragma once
#include <wintun.h>
#include "buffer.h"

class Tun {
public:
	Tun();
	int init();
	void uninit();
	int start_session();
	void close_session();
	int write_packet(Buffer* buf);
	int write_packet(uint8_t* buf, int size);
	int read_packet(Buffer* buf);
private:
	HMODULE wintun;
	WINTUN_ADAPTER_HANDLE adapter;
	WINTUN_SESSION_HANDLE session;
};