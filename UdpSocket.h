#pragma once

/* UdpSocket
* Create UDP socket for data Read/Write
* 
* Callbacks are created in a separated thread in UdpSocket and they are thread-safe
*/

typedef struct
{
	char* addr;
	int port;
}UDP_PEER, UDP_CLIENT;

typedef struct
{
	char data[65536];
	size_t len;
	UDP_PEER peer;
}UDP_DATAGRAM;

typedef void(__stdcall* DATAGRAM_RECEIVED_CALLBACK)(UDP_DATAGRAM* datagram);
typedef void(__stdcall* DATAGRAM_RECEIVED_MEMBER_CALLBACK)(UDP_DATAGRAM* datagram, void* classInstance);

class UdpSocket
{
public:
	enum SOCKETOPT : DWORD
	{
		ReuseAddress = 1,
		ChecksumEnabled = 2,
		SendMsgSize = 3,
		RecvTimeout = 4,
		SendTimeout = 5
	};

	PRIMESOCKET_API UdpSocket();

	PRIMESOCKET_API bool Bind(char* addr, char* port, DATAGRAM_RECEIVED_CALLBACK datagramReceivedCallback);
	PRIMESOCKET_API bool Bind(char* addr, char* port, DATAGRAM_RECEIVED_MEMBER_CALLBACK datagramReceivedCallback, void* classRef);
	PRIMESOCKET_API bool setReceiverCallback(DATAGRAM_RECEIVED_CALLBACK datagramReceivedCallback);
	PRIMESOCKET_API bool setReceiverCallback(DATAGRAM_RECEIVED_MEMBER_CALLBACK datagramReceivedCallback, void* classRef);

	// Enable/Disable/Modify a socket option 
	PRIMESOCKET_API bool setSocketOption(SOCKETOPT opt, DWORD value);

	PRIMESOCKET_API bool Write(char* addr, int port, char* datagram, int datagram_len = 0L);
	PRIMESOCKET_API bool Write(UDP_DATAGRAM* datagram);
	PRIMESOCKET_API UDP_DATAGRAM* Read(size_t len = 0L); // 0 means read everything

	PRIMESOCKET_API void Close();

private:
	typedef struct
	{
		UdpSocket* _instance;
		UDP_DATAGRAM* datagram;
	}MEMBER_CALLBACK_CALLINFO;

	DATAGRAM_RECEIVED_CALLBACK _datagramReceivedCallback;
	DATAGRAM_RECEIVED_MEMBER_CALLBACK _datagramReceivedMemberCallback;

	void* classInstance;
	int callbackType;

	static DWORD WINAPI DatagramReadLoop_ThreadCall(LPVOID param)
	{
		UdpSocket* _instance = (UdpSocket*)param;
		return _instance->DatagramReadLoop();
	}
	DWORD DatagramReadLoop();

	static DWORD WINAPI MemberCallback_StaticCall(LPVOID param)
	{
		MEMBER_CALLBACK_CALLINFO* _mcci = (MEMBER_CALLBACK_CALLINFO*)param;
		_mcci->_instance->_datagramReceivedMemberCallback(_mcci->datagram, _mcci->_instance->classInstance);
		free(_mcci);
		return 0;
	}

	bool _bound;
	HANDLE _hReadLoop, _hMemCallback;
	SOCKET _sock;
};