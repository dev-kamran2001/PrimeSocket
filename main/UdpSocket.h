/*
 * MIT License
 * Copyright (c) 2023 Kamran
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

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
typedef void(__stdcall* DATAGRAM_RECEIVED_P_CALLBACK)(UDP_DATAGRAM* datagram, void* dataPointers);

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
	PRIMESOCKET_API bool Bind(char* addr, char* port, DATAGRAM_RECEIVED_P_CALLBACK datagramReceivedCallback, void* dataPointers);
	PRIMESOCKET_API bool setReceiverCallback(DATAGRAM_RECEIVED_CALLBACK datagramReceivedCallback);
	PRIMESOCKET_API bool setReceiverCallback(DATAGRAM_RECEIVED_P_CALLBACK datagramReceivedCallback, void* dataPointers);

	// Enable/Disable/Modify a socket option 
	PRIMESOCKET_API bool setSocketOption(SOCKETOPT opt, DWORD value);

	PRIMESOCKET_API bool Write(char* addr, int port, char* datagram, int datagram_len = 0L);
	PRIMESOCKET_API bool Write(UDP_DATAGRAM* datagram);
	PRIMESOCKET_API UDP_DATAGRAM* Read(size_t len = 0L);

	PRIMESOCKET_API void Close();

private:
	typedef struct
	{
		UdpSocket* _instance;
		UDP_DATAGRAM* datagram;
	}MEMBER_CALLBACK_CALLINFO;

	DATAGRAM_RECEIVED_CALLBACK _datagramReceivedCallback;
	DATAGRAM_RECEIVED_P_CALLBACK _datagramReceivedMemberCallback;

	void* _dataPointers;
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
		_mcci->_instance->_datagramReceivedMemberCallback(_mcci->datagram, _mcci->_instance->_dataPointers);
		free(_mcci);
		return 0;
	}

	bool _bound;
	HANDLE _hReadLoop, _hMemCallback;
	SOCKET _sock;
};