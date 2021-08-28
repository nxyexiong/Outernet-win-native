#include <winsock2.h>
#include <ws2tcpip.h>
#include "client.h"
#include "tun.h"
#include "crypto.h"
#include "protocol.h"
#include "buffer.h"
#include "utils.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define BUF_SIZE 2048
#define CONNECT_RETRY_TIMES 5


void loop_cb(Client* client) {
	client->loop();
}

Client::Client() {
	inited = false;
	running = false;
	crypto = nullptr;
	tun = nullptr;
	sock = -1;
	server_addr = nullptr;

	handshaked = false;
	handshake_sent_time_ms = 0;
	handshake_retry_cnt = 0;
}

Client::~Client() {
	uninit();
}

int Client::init(const char* host, int port, const char* username, const char* secret, Tun* tun) {
	if (inited) return 1;

	this->tun = tun;

	crypto = new Crypto;
	crypto->init(secret);
	crypto->sha256(identification, (uint8_t*)username, strlen(username));

	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		uninit();
		return -1;
	}

	server_addr = new sockaddr_in;
	memset(server_addr, 0, sizeof(server_addr));
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(port);
	server_addr->sin_addr.S_un.S_addr = inet_addr(host);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock <= 0) {
		uninit();
		return -1;
	}
	if (bind(sock, (sockaddr *)&addr, sizeof(addr)) != 0) {
		uninit();
		return -1;
	}

	ULONG mode = 1;
	if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR) {
		uninit();
		return -1;
	}

	inited = true;
	return 0;
}

void Client::uninit() {
	if (!inited) return;

	stop();

	tun = nullptr;

	if (crypto) {
		delete crypto;
		crypto = nullptr;
	}

	if (server_addr) {
		delete server_addr;
		server_addr = nullptr;
	}

	if (sock != -1) {
		closesocket(sock);
		sock = -1;
	}

	inited = false;
}

int Client::run() {
	if (running) return 1;
	
	running = true;
	loop_thread = std::thread(loop_cb, this);
	
	return 0;
}

void Client::stop() {
	if (!running) return;

	handshaked = false;
	running = false;
	loop_thread.join();
}

void Client::loop() {
	while (running) {
		if (!handshaked) handle_handshake();
		bool has_data = false;
		if (handle_read() == 0) has_data = true;
		if (handle_recv() == 0) has_data = true;

		if (!has_data) Sleep(1);
	}
}

int Client::handle_read() {
	Buffer buf;
	buf.init();
	buf.alloc(BUF_SIZE);

	Protocol protocol;
	protocol.cmd = CMD_CLIENT_DATA;
	memcpy(protocol.identification, identification, 32);
	protocol.get_header_bytes(&buf);

	if (!tun) {
		buf.uninit();
		return -1;
	}
	int ret = tun->read_packet(&buf);

	if (ret == 0) {
		wrap_data(&buf);
		sendto(sock, (const char*)buf.get_buf(), buf.get_len(), 0, (sockaddr*)server_addr, sizeof(sockaddr_in));
	}

	buf.uninit();
	return ret;
}

int Client::handle_recv() {
	sockaddr addr = { 0 };
	socklen_t addrlen = sizeof(addr);
	Buffer buf;
	buf.init();
	buf.alloc(BUF_SIZE);
	int r = recvfrom(sock, (char*)buf.get_buf(), BUF_SIZE, 0, &addr, &addrlen);
	if (r <= 0) {
		buf.uninit();
		return 1;
	}
	buf.set_len(r);
	unwrap_data(&buf);

	if (!handshaked) {
		Protocol protocol;
		int parsed = protocol.parse_header(&buf);
		if (!protocol.is_header_complete() || protocol.cmd != CMD_SERVER_HANDSHAKE) {
			buf.uninit();
			return 0;
		}
		Utils::ipv4_to_str(protocol.tun_ip, tun_ip, true);
		Utils::ipv4_to_str(protocol.dst_ip, dst_ip, true);

		handshaked = true;
		buf.uninit();
		return 0;
	}
	else {
		Protocol protocol;
		int parsed = protocol.parse_header(&buf);
		if (!protocol.is_header_complete() || protocol.cmd != CMD_SERVER_DATA) {
			buf.uninit();
			return 0;
		}
		if (!tun) {
			buf.uninit();
			return -1;
		}
		int ret = tun->write_packet(buf.get_buf() + parsed, buf.get_len() - parsed);
		buf.uninit();
		return ret;
	}
}

void Client::handle_handshake() {
	uint64_t now = Utils::get_cur_time_ms();
	if (now - handshake_sent_time_ms < 1000) return;
	handshake_sent_time_ms = now;

	if (handshake_retry_cnt >= CONNECT_RETRY_TIMES) {
		running = false;
		return;
	}
	handshake_retry_cnt++;

	// start handshake
	Buffer buf;
	buf.init();
	Protocol protocol;
	protocol.cmd = CMD_CLIENT_HANDSHAKE;
	memcpy(protocol.identification, identification, 32);
	protocol.get_header_bytes(&buf);

	wrap_data(&buf);
	sendto(sock, (const char*)buf.get_buf(), buf.get_len(), 0, (sockaddr*)server_addr, sizeof(sockaddr_in));

	buf.uninit();
}

void Client::wrap_data(Buffer* buf) {
	Buffer tmp;
	tmp.init();
	crypto->encrypt(&tmp, buf);
	buf->copy(&tmp);
	tmp.uninit();
}

void Client::unwrap_data(Buffer* buf) {
	Buffer tmp;
	tmp.init();
	crypto->decrypt(&tmp, buf);
	buf->copy(&tmp);
	tmp.uninit();
}