#pragma once
#include <stdint.h>
#include <thread>

class Tun;
class Crypto;
class Buffer;

class Client {
private:
	std::thread loop_thread;
	bool inited;
	bool running;
	uint8_t identification[32];
	Crypto* crypto;
	Tun* tun;
	int sock;
	struct sockaddr_in* server_addr;
	bool handshaked;
	uint64_t handshake_sent_time_ms;
	int handshake_retry_cnt;
	char tun_ip[32];
	char dst_ip[32];

private:
	Client();
	~Client();
	int handle_recv();
	int handle_read();
	void handle_handshake();
	void wrap_data(Buffer* buf);
	void unwrap_data(Buffer* buf);
public:
	static Client& get_instance()
	{
		static Client instance;
		return instance;
	}
	int init(const char* host, int port, const char* username, const char* secret, Tun* tun);
	void uninit();
	int run();
	void stop();
	void loop();
	bool is_inited() { return inited; }
	bool is_running() { return running; }
	bool is_handshaked() { return handshaked; }
	std::string get_tun_ip() { return tun_ip; }
	std::string get_dst_ip() { return dst_ip; }
};