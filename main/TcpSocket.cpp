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

#define LIBRARY_EXPORTS
#include "PrimeSocket.h"

char* Safe_Alloc(int size, int* allocType = 0)
{
	char* buffer = 0;
	buffer = (char*)malloc(size);
	if (!buffer)
	{
#ifdef WIN32 || __AMD64__
		buffer = (char*)VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
#else
		return 0;
#endif
	}

	return buffer;
}

TcpSocket::TcpSocket()
{
	classValid = true;
	_init = false;
	_socketClosed = false;
	_csCalled = false;
	_ai_family = AF_INET;
	_ai_socktype = SOCK_STREAM;
	_ai_protocol = IPPROTO_TCP;
	_readBufSize = 65536;

	_hAcceptLoop = INVALID_HANDLE_VALUE;
	_hReadLoop = INVALID_HANDLE_VALUE;
}

TcpSocket::TcpSocket(SOCKET client, int clientPort, DATA_RECEIVED_CALLBACK dataRecvCallback, CONNECTION_CLOSED_CALLBACK connClosedCallback, int readBufferSize)
{
	if (client == 0 ||
		dataRecvCallback == 0)
		return;

	_isServer = false;
	_init = true;
	_socketClosed = false;
	_ai_family = AF_INET;
	_ai_socktype = SOCK_STREAM;
	_ai_protocol = IPPROTO_TCP;
	_readBufSize = readBufferSize;

	_sock = client;
	_port = clientPort;
	_dataReceivedCallback = dataRecvCallback;
	_connClosedCallback = connClosedCallback;

	callbackType = 0;
	_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);
}

TcpSocket::TcpSocket(SOCKET client, int clientPort, DATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, CONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers, int readBufferSize)
{
	if (client == 0 ||
		dataRecvCallback == 0)
		return;

	_isServer = false;
	_init = true;
	_socketClosed = false;
	_ai_family = AF_INET;
	_ai_socktype = SOCK_STREAM;
	_ai_protocol = IPPROTO_TCP;
	_readBufSize = readBufferSize;

	_sock = client;
	_port = clientPort;
	_dataReceivedMemberCallback = dataRecvCallback;
	_connClosedMemberCallback = connectionClosedCallback;

	callbackType = 1;
	_dataPointers = dataPointers;
	_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);
}

bool TcpSocket::Connect(char* addr, char* port, DATA_RECEIVED_CALLBACK dataRecvCallback, CONNECTION_CLOSED_CALLBACK connectionClosedCallback)
{
	if (addr == 0 ||
		port == 0 ||
		_init)
		return false;

	struct addrinfo* result = NULL, hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = _ai_family;
	hints.ai_socktype = _ai_socktype;
	hints.ai_protocol = _ai_protocol;

	// Resolve the server address and port
	iResult = getaddrinfo(addr, port, &hints, &result);
	if (iResult != 0) {
		return false;
	}

	_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (_sock == INVALID_SOCKET) {
		return false;
	}

	if (connect(_sock, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
		return false;

	_isServer = false;
	_init = true;
	sscanf(port, "%d", &_port);
	if(dataRecvCallback != NULL)
	{
		_dataReceivedCallback = dataRecvCallback;
		_connClosedCallback = connectionClosedCallback;
		_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);
	}

	return true;
}

bool TcpSocket::Connect(char* addr, char* port, DATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, CONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers)
{
	if (addr == 0 ||
		port == 0 ||
		_init)
		return false;

	struct addrinfo* result = NULL, hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = _ai_family;
	hints.ai_socktype = _ai_socktype;
	hints.ai_protocol = _ai_protocol;

	// Resolve the server address and port
	iResult = getaddrinfo(addr, port, &hints, &result);
	if (iResult != 0) {
		return false;
	}

	_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (_sock == INVALID_SOCKET) {
		return false;
	}

	if (connect(_sock, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
		return false;

	_isServer = false;
	_init = true;

	callbackType = 1;
	_dataPointers = dataPointers;
	sscanf(port, "%d", &_port);
	if(dataRecvCallback != NULL)
	{
		_dataReceivedMemberCallback = dataRecvCallback;
		_connClosedMemberCallback = connectionClosedCallback;
		_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);
	}

	return true;
}

bool TcpSocket::Listen(char* addr, char* port, NEW_CONNECTION_CALLBACK newConnCallback)
{
	if (addr == 0 ||
		port == 0 ||
		newConnCallback == 0 ||
		_init)
		return false;

    struct addrinfo* result = NULL, hints;
    int iResult;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = _ai_family;
    hints.ai_socktype = _ai_socktype;
    hints.ai_protocol = _ai_protocol;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(addr, port, &hints, &result);
    if (iResult != 0) {
        return false;
    }

	_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (_sock == INVALID_SOCKET) {
		return false;
    }

    iResult = bind(_sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
		return false;
    }
    freeaddrinfo(result); // No longer needed

    if (listen(_sock, SOMAXCONN) == SOCKET_ERROR) {
		return false;
    }

	_isServer = true;
	_init = true;
	_newConCallback = newConnCallback;
	sscanf(port, "%d", &_port);
	_hAcceptLoop = CreateThread(0, 0, AcceptLoop_ThreadCall, this, 0, 0);
}

bool TcpSocket::Listen(char* addr, char* port, NEW_CONNECTION_MEMBER_CALLBACK newConnCallback, void* dataPointers)
{
	if (addr == 0 ||
		port == 0 ||
		newConnCallback == 0 ||
		_init)
		return false;

	struct addrinfo* result = NULL, hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = _ai_family;
	hints.ai_socktype = _ai_socktype;
	hints.ai_protocol = _ai_protocol;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(addr, port, &hints, &result);
	if (iResult != 0) {
		return false;
	}

	_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (_sock == INVALID_SOCKET) {
		return false;
	}

	iResult = bind(_sock, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		return false;
	}
	freeaddrinfo(result); // No longer needed

	if (listen(_sock, SOMAXCONN) == SOCKET_ERROR) {
		return false;
	}

	_isServer = true;
	_init = true;
	_newConMemberCallback = newConnCallback;

	callbackType = 1;
	_dataPointers = dataPointers;
	sscanf(port, "%d", &_port);
	_hAcceptLoop = CreateThread(0, 0, AcceptLoop_ThreadCall, this, 0, 0);
}

bool TcpSocket::setSocketOption(SOCKETOPT opt, DWORD value)
{
	if (opt == 0 || opt > 4)
		return false;

	int result = 0;
	switch (opt)
	{
	case NoDelay:
		if (value != TRUE && value != FALSE)
			return false;

		result = setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(DWORD));
		if (result != SOCKET_ERROR)
			return true;
		else
			return false;

		break;
	case KeepAlive:
		if (value != TRUE && value != FALSE)
			return false;

		result = setsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&value, sizeof(DWORD));
		if (result != SOCKET_ERROR)
			return true;
		else
			return false;

		break;
	case RecvTimeout:
		result = setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&value, sizeof(DWORD));
		if (result != SOCKET_ERROR)
			return true;
		else
			return false;

		break;
	case SendTimeout:
		result = setsockopt(_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&value, sizeof(DWORD));
		if (result != SOCKET_ERROR)
			return true;
		else
			return false;

		break;
	case IpDontFragment:
		result = setsockopt(_sock, IPPROTO_IP, IP_DONTFRAGMENT, (char*)&value, sizeof(DWORD));
		if (result != SOCKET_ERROR)
			return true;
		else
			return false;

		break;
	}

	return false;
}

char* TcpSocket::getAddress()
{
	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	int err = getsockname(_sock, (struct sockaddr*)&name, &namelen);
	if (err == SOCKET_ERROR)
		return 0;

	char* buffer = (char*)malloc(80);
	ZeroMemory(buffer, 80);
	const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);

	return buffer;
}

int TcpSocket::getPort()
{
	/*if (_isServer)
	{
		struct sockaddr_in name;
		socklen_t namelen = sizeof(name);
		int err = getsockname(_sock, (struct sockaddr*)&name, &namelen);
		if (err == SOCKET_ERROR)
			return 0;

		return ntohs(name.sin_port);
	}
	else
		return _port;*/

	return _port;
}

SOCKET TcpSocket::getSocketDescriptor()
{
	return _sock;
}

bool TcpSocket::setReadBufferSize(int size)
{
	if (size < 1)
		return false;

	_readBufSize = size;
	return true;
}

bool TcpSocket::isSocketClosed()
{
	return _csCalled;
}

bool TcpSocket::Write(void* data, size_t dataSize)
{
	if (_isServer || data == NULL || dataSize <= 0)
		return false;

	return send(_sock, (const char*)data, dataSize, 0) == SOCKET_ERROR ? false : true;
}

bool TcpSocket::Write(SOCKET client, void* data, size_t dataSize)
{
	if ((client == NULL || client == SOCKET_ERROR) || data == NULL || dataSize <= 0)
		return false;

	return send(client, (const char*)data, dataSize, 0) == SOCKET_ERROR ? false : true;
}

char* TcpSocket::Read(size_t len)
{
	if (_isServer)
		return 0;

	char* buf = (char*)malloc(len);
	ZeroMemory(buf, len);

	int result = recv(_sock, buf, len, 0);
	if (result == 0)
		return (char*)result;
	if (result == SOCKET_ERROR)
		return (char*)-1;

	buf[result] = '\0';
	return buf;
}

char* TcpSocket::Read(SOCKET client, size_t len)
{
	if (client == NULL || client == SOCKET_ERROR)
		return 0;

	char* buf = (char*)malloc(len);
	ZeroMemory(buf, len);

	int result = recv(client, buf, len, 0);
	if (result == 0)
		return (char*)result;
	if (result == SOCKET_ERROR)
		return (char*)-1;

	buf[result] = '\0';
	return buf;
}

DWORD TcpSocket::AcceptLoop()
{
	while (!_socketClosed)
	{
		sockaddr_in* clientAddr = (sockaddr_in*)malloc(sizeof sockaddr_in);
		ZeroMemory(clientAddr, sizeof sockaddr_in);
		int len = sizeof(sockaddr_in);
		SOCKET client = accept(_sock, (sockaddr*)clientAddr, &len);
		if (client != SOCKET_ERROR)
		{
			CLIENT_CONNECTION_DATA* ccd = (CLIENT_CONNECTION_DATA*)malloc(sizeof CLIENT_CONNECTION_DATA);
			ZeroMemory(ccd, sizeof CLIENT_CONNECTION_DATA);
			
			inet_ntop(clientAddr->sin_family, &clientAddr->sin_addr, ccd->ipAddress, 47);
			ccd->clPort = htons((u_short)clientAddr->sin_port);
			ccd->listenPort = _port;
			ccd->clientSock = client;
			ccd->dataPointers = _dataPointers ? _dataPointers : 0;

			if (callbackType == 0)
				CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_newConCallback, ccd, 0, 0);
			else
				CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_newConMemberCallback, ccd, 0, 0);
		}
		else
		{
			free(clientAddr);
			break;
		}
		free(clientAddr);
	}

	return 0;
}

DWORD TcpSocket::ReadLoop()
{
	while (!_socketClosed)
	{
		char* buf = (char*)calloc(1, _readBufSize);
		int len = recv(_sock, buf, _readBufSize, 0);
		if (len > 0)
		{
			DATA_RECEVIED_CALLBACK_DATA* drcd = (DATA_RECEVIED_CALLBACK_DATA*)malloc(sizeof DATA_RECEVIED_CALLBACK_DATA);
			drcd->socket = this;
			drcd->buff = buf;
			drcd->len = len;
			drcd->dataPointers = 0;
			if (callbackType != 0)
				drcd->dataPointers = _dataPointers;
			CreateThread(0, 0, CallbackDRCV_ThreadCall, drcd, 0, 0);
		}
		else
		{
			if (len <= 0 /*&& (WSAGetLastError() == WSAENOTSOCK || WSAGetLastError() == WSAECONNRESET)*/)
			{
				CONNECTION_CLOSED_CALLBACK_DATA* ccd = (CONNECTION_CLOSED_CALLBACK_DATA*)malloc(sizeof CONNECTION_CLOSED_CALLBACK_DATA);
				ccd->socket = this;
				ccd->ip = getAddress();
				ccd->port = getPort();
				ccd->dataPointers = 0;
				if (callbackType != 0)
					ccd->dataPointers = _dataPointers;
				CreateThread(0, 0, CallbackCCLSD_ThreadCall, ccd, 0, 0);

				_socketClosed = true;
			}
		}
	}

	return 0;
}

void TcpSocket::ForceShutdown()
{
	__try
	{
		if (_hAcceptLoop && _hAcceptLoop != INVALID_HANDLE_VALUE)
			TerminateThread(_hAcceptLoop, 0);
		if (_hReadLoop && _hReadLoop != INVALID_HANDLE_VALUE)
			TerminateThread(_hReadLoop, 0);

		if (!_csCalled)
		{
			closesocket(_sock);
			_csCalled = true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

	}
}

void TcpSocket::Close()
{
	__try
	{/*if (_hAcceptLoop)
		TerminateThread(_hAcceptLoop, 0);
	if (_hReadLoop)
		TerminateThread(_hReadLoop, 0);

	if (callbackType == 0)
		_connClosedCallback(getAddress(), getPort());
	else
		_connClosedMemberCallback(getAddress(), getPort(), classInstance);*/

		if (!_csCalled)
		{
			closesocket(_sock);
			_csCalled = true;
		}
		else
			return;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

	}
}