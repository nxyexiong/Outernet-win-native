#include <sstream>
#include "tun.h"

static WINTUN_CREATE_ADAPTER_FUNC WintunCreateAdapter;
static WINTUN_DELETE_ADAPTER_FUNC WintunDeleteAdapter;
static WINTUN_DELETE_POOL_DRIVER_FUNC WintunDeletePoolDriver;
static WINTUN_ENUM_ADAPTERS_FUNC WintunEnumAdapters;
static WINTUN_FREE_ADAPTER_FUNC WintunFreeAdapter;
static WINTUN_OPEN_ADAPTER_FUNC WintunOpenAdapter;
static WINTUN_GET_ADAPTER_LUID_FUNC WintunGetAdapterLUID;
static WINTUN_GET_ADAPTER_NAME_FUNC WintunGetAdapterName;
static WINTUN_SET_ADAPTER_NAME_FUNC WintunSetAdapterName;
static WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC WintunGetRunningDriverVersion;
static WINTUN_SET_LOGGER_FUNC WintunSetLogger;
static WINTUN_START_SESSION_FUNC WintunStartSession;
static WINTUN_END_SESSION_FUNC WintunEndSession;
static WINTUN_GET_READ_WAIT_EVENT_FUNC WintunGetReadWaitEvent;
static WINTUN_RECEIVE_PACKET_FUNC WintunReceivePacket;
static WINTUN_RELEASE_RECEIVE_PACKET_FUNC WintunReleaseReceivePacket;
static WINTUN_ALLOCATE_SEND_PACKET_FUNC WintunAllocateSendPacket;
static WINTUN_SEND_PACKET_FUNC WintunSendPacket;


static HMODULE
InitializeWintun(void)
{
	HMODULE Wintun =
		LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!Wintun)
		return NULL;
#define X(Name, Type) ((Name = (Type)GetProcAddress(Wintun, #Name)) == NULL)
	if (X(WintunCreateAdapter, WINTUN_CREATE_ADAPTER_FUNC) || X(WintunDeleteAdapter, WINTUN_DELETE_ADAPTER_FUNC) ||
		X(WintunDeletePoolDriver, WINTUN_DELETE_POOL_DRIVER_FUNC) || X(WintunEnumAdapters, WINTUN_ENUM_ADAPTERS_FUNC) ||
		X(WintunFreeAdapter, WINTUN_FREE_ADAPTER_FUNC) || X(WintunOpenAdapter, WINTUN_OPEN_ADAPTER_FUNC) ||
		X(WintunGetAdapterLUID, WINTUN_GET_ADAPTER_LUID_FUNC) ||
		X(WintunGetAdapterName, WINTUN_GET_ADAPTER_NAME_FUNC) ||
		X(WintunSetAdapterName, WINTUN_SET_ADAPTER_NAME_FUNC) ||
		X(WintunGetRunningDriverVersion, WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC) ||
		X(WintunSetLogger, WINTUN_SET_LOGGER_FUNC) || X(WintunStartSession, WINTUN_START_SESSION_FUNC) ||
		X(WintunEndSession, WINTUN_END_SESSION_FUNC) || X(WintunGetReadWaitEvent, WINTUN_GET_READ_WAIT_EVENT_FUNC) ||
		X(WintunReceivePacket, WINTUN_RECEIVE_PACKET_FUNC) ||
		X(WintunReleaseReceivePacket, WINTUN_RELEASE_RECEIVE_PACKET_FUNC) ||
		X(WintunAllocateSendPacket, WINTUN_ALLOCATE_SEND_PACKET_FUNC) || X(WintunSendPacket, WINTUN_SEND_PACKET_FUNC))
#undef X
	{
		DWORD LastError = GetLastError();
		FreeLibrary(Wintun);
		SetLastError(LastError);
		return NULL;
	}
	return Wintun;
}

Tun::Tun() {
	wintun = NULL;
	adapter = NULL;
	session = NULL;
}

int Tun::init() {
	// load library
	wintun = InitializeWintun();
	if (!wintun) {
		int err = GetLastError();
		return err;
	}
	// open or create adapter
	adapter = WintunOpenAdapter(L"Outernet", L"Outernet");
	if (!adapter) {
		// c55970ca-5f6d-443d-8e50-2862939b5b0f
		GUID guid = { 0xc55970ca, 0x5f6d, 0x443d,{ 0x8e, 0x50, 0x28, 0x62, 0x93, 0x9b, 0x5b, 0x0f } };
		adapter = WintunCreateAdapter(L"Outernet", L"Outernet", &guid, NULL);
		if (!adapter) {
			int err = GetLastError();
			return err;
		}
	}
	// init session
	session = NULL;
	
	return 0;
}

void Tun::uninit() {
	// close session
	if (session) {
		close_session();
		session = NULL;
	}
	// free adapter
	if (adapter) {
		WintunFreeAdapter(adapter);
		adapter = NULL;
	}
	// free library
	if (wintun) {
		FreeLibrary(wintun);
		wintun = NULL;
	}
}

int Tun::start_session() {
	session = WintunStartSession(adapter, 0x400000);
	if (!session) {
		int err = GetLastError();
		session = NULL;
		return err;
	}
	return 0;
}

void Tun::close_session() {
	if (session) {
		WintunEndSession(session);
		session = NULL;
	}
}

int Tun::write_packet(Buffer* buf) {
	return write_packet(buf->get_buf(), buf->get_len());
}

int Tun::write_packet(uint8_t* buf, int size) {
	if (!session) return -1;
	BYTE *Packet = WintunAllocateSendPacket(session, size);
	if (Packet) {
		memcpy(Packet, buf, size);
		WintunSendPacket(session, Packet);
	}
	else {
		DWORD LastError = GetLastError();
		switch (LastError)
		{
		case ERROR_BUFFER_OVERFLOW:
			return 1;
		default:
			return -1;
		}
	}
	return 0;
}

int Tun::read_packet(Buffer* buf) {
	if (!session) return -1;
	DWORD PacketSize;
	BYTE *Packet = WintunReceivePacket(session, &PacketSize);
	if (Packet) {
		buf->insert_back(Packet, PacketSize);
		WintunReleaseReceivePacket(session, Packet);
	}
	else {
		DWORD LastError = GetLastError();
		switch (LastError)
		{
		case ERROR_NO_MORE_ITEMS:
			return 1;
		default:
			return -1;
		}
	}
	return 0;
}