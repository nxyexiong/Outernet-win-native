#include <fstream>
#include "json.hpp"
#include "control.h"
#include "sys_helper.h"

#define NETWORK_IP "10.0.0.0"
#define NETWORK_MASK "255.255.255.0"
#define IP_ANY "0.0.0.0/0"
#define CONFIGS_FILE_NAME "configs.json"


void loop(Control* control) {
	control->handle_loop();
}

Control::Control(const std::function<void(ControlState)>& state_change_cb) {
	on_state_changed = state_change_cb;
	background_thread = nullptr;
	running = false;
}

Control::~Control() {
	if (running) {
		stop();
	}
}

void Control::start(const Configs& configs) {
	if (!running) {
		this->configs = configs;
		running = true;
		background_thread = new std::thread(loop, this);
	}
}

void Control::stop() {
	if (running) {
		running = false;
		if (background_thread && background_thread->joinable()) {
			background_thread->join();
			delete background_thread;
			background_thread = nullptr;
		}
	}
}

void Control::handle_loop() {
	Tun tun;

	// start sequence
	int state = CONTROL_STATE_DISCONNECTED;
	while (running) {
		if (state == CONTROL_STATE_DISCONNECTED) {
			// connecting
			state = CONTROL_STATE_CONNECTING;
			ControlState control_state;
			control_state.type = state;
			control_state.msg = "";
			on_state_changed(control_state);

			Client::get_instance().init(configs.server_ip.c_str(), configs.server_port, configs.username.c_str(), configs.secret.c_str(), &tun);
			Client::get_instance().run();
		}
		else if (state == CONTROL_STATE_CONNECTING) {
			if (!Client::get_instance().is_running()) {
				// timeout
				state = CONTROL_STATE_DISCONNECTED;
				ControlState control_state;
				control_state.type = state;
				control_state.msg = "";
				on_state_changed(control_state);
				running = false;
			}
			else if (Client::get_instance().is_handshaked()) {
				// setting up network
				state = CONTROL_STATE_SETTING_UP;
				ControlState control_state;
				control_state.type = state;
				control_state.msg = "";
				on_state_changed(control_state);

				auto tun_ip = Client::get_instance().get_tun_ip();
				auto dst_ip = Client::get_instance().get_dst_ip();
				SysHelper::get_instance().init_network(configs.server_ip.c_str(), dst_ip.c_str(), tun_ip.c_str(), NETWORK_IP, NETWORK_MASK);
				int rst = tun.init();
				if (rst != 0) {
					state = CONTROL_STATE_ERROR;
					ControlState control_state;
					control_state.type = state;
					std::stringstream msg_stream;
					msg_stream << "wintun init failed: " << rst;
					control_state.msg = msg_stream.str();
					on_state_changed(control_state);
					running = false;
				}
				rst = tun.start_session();
				if (rst != 0) {
					state = CONTROL_STATE_ERROR;
					ControlState control_state;
					control_state.type = state;
					std::stringstream msg_stream;
					msg_stream <<"session start failed: " << rst;
					control_state.msg = msg_stream.str();
					on_state_changed(control_state);
					running = false;
				}
			}
		}
		else if (state == CONTROL_STATE_SETTING_UP) {
			// adding route
			state = CONTROL_STATE_ADDING_ROUTE;
			ControlState control_state;
			control_state.type = state;
			control_state.msg = "";
			on_state_changed(control_state);

			SysHelper::get_instance().add_route_white(IP_ANY);
		}
		else if (state == CONTROL_STATE_ADDING_ROUTE) {
			// connected
			state = CONTROL_STATE_CONNECTED;
			ControlState control_state;
			control_state.type = state;
			control_state.msg = "";
			on_state_changed(control_state);
		}
		Sleep(1);
	}

	// stop sequence
	state = CONTROL_STATE_DISCONNECTING;
	bool stopped = false;
	while (!stopped) {
		if (state == CONTROL_STATE_DISCONNECTING) {
			// deleting route
			state = CONTROL_STATE_DELETING_ROUTE;
			ControlState control_state;
			control_state.type = state;
			control_state.msg = "";
			on_state_changed(control_state);

			SysHelper::get_instance().del_route_white(IP_ANY);
		}
		else if (state == CONTROL_STATE_DELETING_ROUTE) {
			// tearing down
			state = CONTROL_STATE_TEARING_DOWN;
			ControlState control_state;
			control_state.type = state;
			control_state.msg = "";
			on_state_changed(control_state);

			SysHelper::get_instance().uninit_network();
			tun.close_session();
			tun.uninit();
			Client::get_instance().stop();
			Client::get_instance().uninit();
		}
		else if (state == CONTROL_STATE_TEARING_DOWN) {
			// disconnected
			state = CONTROL_STATE_DISCONNECTED;
			ControlState control_state;
			control_state.type = state;
			control_state.msg = "";
			on_state_changed(control_state);

			stopped = true;
		}
		Sleep(1);
	}
}

void Control::save_configs(const Configs& configs) {
	nlohmann::json configs_json;
	configs_json["server_ip"] = configs.server_ip;
	configs_json["server_port"] = configs.server_port;
	configs_json["username"] = configs.username;
	configs_json["secret"] = configs.secret;
	std::string dump = configs_json.dump();
	std::ofstream ofile;
	ofile.open(CONFIGS_FILE_NAME, std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
	if (ofile) {
		ofile << dump;
		ofile.close();
	}
}

Configs Control::load_configs() const {
	Configs configs;
	std::ifstream ifile;
	ifile.open(CONFIGS_FILE_NAME, std::ios_base::in);
	if (ifile) {
		std::stringstream buffer;
		buffer << ifile.rdbuf();
		std::string dump = buffer.str();
		auto configs_json = nlohmann::json::parse(dump);
		configs.server_ip = configs_json["server_ip"];
		configs.server_port = configs_json["server_port"];
		configs.username = configs_json["username"];
		configs.secret = configs_json["secret"];
		ifile.close();
	}
	return configs;
}
