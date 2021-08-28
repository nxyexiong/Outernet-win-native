#pragma once
#include <sstream>
#include <string>
#include <list>
#include "utils.h"


struct Interface {
	std::string guid;
	int index;
	std::string addr;
	std::string mask;
	std::string gateway;
};

class SysHelper {
private:
	bool network_inited;
	Interface tun_if;
	Interface cur_if;
	std::string server_addr;
	std::string ipv4_addr;
	std::string ipv4_gateway;
	std::string ipv4_network;
	std::string ipv4_netmask;

	SysHelper();
public:
	static SysHelper& get_instance()
	{
		static SysHelper instance;
		return instance;
	}
	static std::list<Interface> get_interfaces(bool &found);
	static Interface get_tun_interface(bool &found);
	static Interface get_cur_interface(bool &found);
	int init_network(const char* server_addr, const char* ipv4_addr, const char* ipv4_gateway, const char* ipv4_network, const char* ipv4_netmask);
	int uninit_network();
	int add_route_white(const char* ip); // 1.2.3.4/32
	int del_route_white(const char* ip);
	int add_route_black(const char* ip);
	int del_route_black(const char* ip);
	static int fix_network();
	static int restart_pc();
};