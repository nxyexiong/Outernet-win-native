#pragma once
#include <Windows.h>
#include <stdint.h>

class Utils {
public:
	static void ipv4_to_str(uint32_t ipv4, char* outstr, bool is_bigend);
	static DWORD exec(LPCWSTR path, LPCWSTR params);
	static DWORD64 get_cur_time_ms();
};