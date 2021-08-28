#include <string>
#include <sstream>
#include <time.h>
#include "utils.h"
#include "buffer.h"


int gettimeofday(struct timeval *tp, void *tzp) {
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}

void Utils::ipv4_to_str(uint32_t ipv4, char* outstr, bool is_bigend) {
	std::string ipstr;
	ipstr.assign(50, '\0');
	char addr_str[257];
	if (is_bigend)
		sprintf(&ipstr[0], "%d.%d.%d.%d", ipv4 << 24 >> 24, ipv4 << 16 >> 24, ipv4 << 8 >> 24, ipv4 >> 24);
	else
		sprintf(&ipstr[0], "%d.%d.%d.%d", ipv4 >> 24, ipv4 << 8 >> 24, ipv4 << 16 >> 24, ipv4 << 24 >> 24);
	strcpy(outstr, ipstr.c_str());
}

DWORD Utils::exec(LPCWSTR path, LPCWSTR params) {
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = path;
	ShExecInfo.lpParameters = params;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
	DWORD exitCode;
	if (GetExitCodeProcess(ShExecInfo.hProcess, &exitCode))
		return exitCode;
	else
		return -1;
}

DWORD64 Utils::get_cur_time_ms() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}