#pragma once
#include "tun.h"
#include "client.h"
#include <thread>
#include <string>
#include <functional>

#define CONTROL_STATE_DISCONNECTED     0
#define CONTROL_STATE_CONNECTED        1
#define CONTROL_STATE_CONNECTING       2
#define CONTROL_STATE_SETTING_UP       3
#define CONTROL_STATE_ADDING_ROUTE     4
#define CONTROL_STATE_DISCONNECTING    5
#define CONTROL_STATE_DELETING_ROUTE   6
#define CONTROL_STATE_TEARING_DOWN     7
#define CONTROL_STATE_ERROR            8

struct ControlState {
	int type;
	std::string msg;
	ControlState() {
		type = CONTROL_STATE_DISCONNECTED;
		msg = "";
	}
};

struct Configs {
	std::string server_ip;
	int server_port;
	std::string username;
	std::string secret;
	Configs() {
		server_ip = "";
		server_port = 0;
		username = "";
		secret = "";
	}
};

class Control {
private:
	std::function<void(ControlState)> on_state_changed;
	std::thread* background_thread;

	bool running;

	Configs configs;

public:
	// callback will return from background thread
	Control(const std::function<void(ControlState)>& state_change_cb);
	~Control();
	void start(const Configs& configs);
	void stop();
	void handle_loop();

	void save_configs(const Configs& configs);
	Configs load_configs() const;
};