#include <winsock2.h>
#include <iphlpapi.h>
#include <algorithm>
#include <cctype>
#include "sys_helper.h"

#pragma comment(lib, "iphlpapi.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x)) 
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))


SysHelper::SysHelper() {
	network_inited = false;
	tun_if = Interface();
	cur_if = Interface();
}

std::list<Interface> SysHelper::get_interfaces(bool &found) {
	found = false;
	std::list<Interface> ret;

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	UINT i;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL) {
		return ret;
	}
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
		if (pAdapterInfo == NULL) {
			return ret;
		}
	}
	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			Interface iface;
			iface.guid = pAdapter->AdapterName;
			iface.index = pAdapter->Index;
			iface.addr = pAdapter->IpAddressList.IpAddress.String;
			iface.mask = pAdapter->IpAddressList.IpMask.String;
			iface.gateway = pAdapter->GatewayList.IpAddress.String;
			ret.push_back(iface);
			pAdapter = pAdapter->Next;
		}
	}
	if (pAdapterInfo) FREE(pAdapterInfo);
	found = true;
	return ret;
}

Interface SysHelper::get_tun_interface(bool &found) {
	found = false;
	bool is_ifs_found = false;
	auto list = SysHelper::get_interfaces(is_ifs_found);
	if (!is_ifs_found) return Interface();
	for (auto it : list) {
		std::string guid_lower = it.guid;
		std::transform(guid_lower.begin(), guid_lower.end(), guid_lower.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (guid_lower == "{c55970ca-5f6d-443d-8e50-2862939b5b0f}") {
			found = true;
			return it;
		}
	}
	return Interface();
}

Interface SysHelper::get_cur_interface(bool &found) {
	found = false;
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		return Interface();
	}
	sockaddr_in server_addr = { 0 };
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr.S_un.S_addr = inet_addr("8.8.8.8");
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	connect(sock, (const sockaddr*)&server_addr, sizeof(sockaddr_in));
	sockaddr local_addr = { 0 };
	int local_addr_len = sizeof(sockaddr);
	getsockname(sock, &local_addr, &local_addr_len);
	if (local_addr_len != sizeof(sockaddr_in)) return Interface();
	std::string addr = inet_ntoa(((sockaddr_in*)&local_addr)->sin_addr);

	bool ifs_found = false;
	auto list = SysHelper::get_interfaces(ifs_found);
	if (!ifs_found) return Interface();
	for (auto it : list) {
		if (it.addr == addr) {
			found = true;
			return it;
		}
	}
	return Interface();
}

int SysHelper::init_network(const char* server_addr, const char* ipv4_addr, const char* ipv4_gateway, const char* ipv4_network, const char* ipv4_netmask) {
	if (network_inited) return 1;

	this->server_addr = server_addr;
	this->ipv4_addr = ipv4_addr;
	this->ipv4_gateway = ipv4_gateway;
	this->ipv4_network = ipv4_network;
	this->ipv4_netmask = ipv4_netmask;

	bool found = false;
	tun_if = SysHelper::get_tun_interface(found);
	if (!found) return -1;
	cur_if = SysHelper::get_cur_interface(found);
	if (!found) return -1;

	std::wstringstream cmd;

	// set metric!=0
	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 set route 0.0.0.0/0 " << cur_if.index << " metric=100 store=active";
	Utils::exec(L"netsh", cmd.str().c_str());

	// setup tun
	cmd.str(L"");
	cmd.clear();
	cmd << "interface ip set address " << tun_if.index << " static " << ipv4_addr << " " << ipv4_netmask;
	Utils::exec(L"netsh", cmd.str().c_str());

	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 set interface " << tun_if.index << " forwarding=enable metric=0 mtu=1300";
	Utils::exec(L"netsh", cmd.str().c_str());

	// add server route
	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 add route " << server_addr << "/32 " << cur_if.index << " " << cur_if.gateway.c_str() << " metric=0";
	Utils::exec(L"netsh", cmd.str().c_str());

	// setup dns server
	cmd.str(L"");
	cmd.clear();
	cmd << "interface ip delete dns " << tun_if.index << " all";
	Utils::exec(L"netsh", cmd.str().c_str());

	cmd.str(L"");
	cmd.clear();
	cmd << "interface ip set dns " << tun_if.index << " static 1.1.1.1 validate=no";
	Utils::exec(L"netsh", cmd.str().c_str());

	cmd.str(L"");
	cmd.clear();
	cmd << "interface ip add dns " << tun_if.index << " 8.8.8.8 validate=no";
	Utils::exec(L"netsh", cmd.str().c_str());

	// delete wins
	cmd.str(L"");
	cmd.clear();
	cmd << "interface ip delete wins " << tun_if.index << " all";
	Utils::exec(L"netsh", cmd.str().c_str());

	network_inited = true;
	return 0;
}

int SysHelper::uninit_network() {
	if (!network_inited) return 1;

	std::wstringstream cmd;

	// recover interface metric
	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 set interface " << tun_if.index << " metric=256";
	Utils::exec(L"netsh", cmd.str().c_str());

	// delete server route
	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 delete route " << server_addr.c_str() << "/32 " << cur_if.index;
	Utils::exec(L"netsh", cmd.str().c_str());

	network_inited = false;
	return 0;
}

int SysHelper::add_route_white(const char* ip) {
	if (!network_inited) return -1;

	std::wstringstream cmd;

	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 add route " << ip << " " << tun_if.index << " " << ipv4_gateway.c_str() << " metric=0";
	Utils::exec(L"netsh", cmd.str().c_str());

	return 0;
}

int SysHelper::del_route_white(const char* ip) {
	if (!network_inited) return -1;

	std::wstringstream cmd;

	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 delete route " << ip << " " << tun_if.index << " " << ipv4_gateway.c_str();
	Utils::exec(L"netsh", cmd.str().c_str());

	return 0;
}

int SysHelper::add_route_black(const char* ip) {
	if (!network_inited) return -1;

	std::wstringstream cmd;

	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 add route " << ip << " " << cur_if.index << " " << cur_if.gateway.c_str() << " metric=0";
	Utils::exec(L"netsh", cmd.str().c_str());

	return 0;
}

int SysHelper::del_route_black(const char* ip) {
	if (!network_inited) return -1;

	std::wstringstream cmd;

	cmd.str(L"");
	cmd.clear();
	cmd << "interface ipv4 delete route " << ip << " " << cur_if.index << " " << cur_if.gateway.c_str();
	Utils::exec(L"netsh", cmd.str().c_str());

	return 0;
}

int SysHelper::fix_network() {
	std::wstringstream cmd;

	cmd.str(L"");
	cmd.clear();
	cmd << "winsock reset";
	Utils::exec(L"netsh", cmd.str().c_str());

	cmd.str(L"");
	cmd.clear();
	cmd << "-f";
	Utils::exec(L"route", cmd.str().c_str());

	return 0;
}

int SysHelper::restart_pc() {
	std::wstringstream cmd;

	cmd.str(L"");
	cmd.clear();
	cmd << "/r /t 0";
	Utils::exec(L"shutdown", cmd.str().c_str());

	return 0;
}
