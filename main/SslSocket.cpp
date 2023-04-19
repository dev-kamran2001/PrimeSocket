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

#define PRIMESOCKET_USE_SSL
#define USE_CRITICAL_HEAP
#include "PrimeSocket.h"

bool InitializeSSL()
{
	if (!SSL_library_init())
		return false;
	return true;
}

SslSocket::SslSocket()
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
	_sslInit = false;
	_init = false;
	_sslSocketClean = false;
	_readBufSize = 65536;
	_ai_family = AF_INET;
	_ai_socktype = SOCK_STREAM;
	_ai_protocol = IPPROTO_TCP;
	_port = 99999;
}

SslSocket::SslSocket(SSL* clSsl, int clientPort, SSLDATA_RECEIVED_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_CALLBACK connClosedCallback, int readBufferSize)
{
	if (*(int*)clSsl + 0 == 0 || !clientPort || !dataRecvCallback || !connClosedCallback)
		return;

	if (_readBufSize != readBufferSize)
	{
		if(readBufferSize >= 10 && readBufferSize <= 65536)
			_readBufSize = readBufferSize;
	}

	_isServer = false;
	_init = true;
	_socketClosed = false;

	ssl = clSsl;
	_port = clientPort;
	_dataReceivedCallback = dataRecvCallback;
	_connClosedCallback = connClosedCallback;

	callbackType = 0;
	_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);
}

SslSocket::SslSocket(SSL* clSsl, int clientPort, SSLDATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers, int readBufferSize)
{
	if (*(int*)clSsl + 0 == 0 || !clientPort || !dataRecvCallback || !connectionClosedCallback || !dataPointers)
		return;

	if (_readBufSize != readBufferSize)
	{
		if (readBufferSize >= 10 && readBufferSize <= 65536)
			_readBufSize = readBufferSize;
	}

	_isServer = false;
	_init = true;
	_socketClosed = false;

	ssl = clSsl;
	_port = clientPort;
	_dataReceivedMemberCallback = dataRecvCallback;
	_connClosedMemberCallback = connectionClosedCallback;

	callbackType = 1;
	_dataPointers = dataPointers;
	_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);
}

int SslSocket::setServerCertificate(char* CertFile, char* KeyFile)
{
	if (_init)
		return SSLSOCKET_INVALID_CALL;
	else
	{
		if (!_sslInit)
		{
			if (!InitializeServerSSL())
				return SSLSOCKET_SSL_INIT_ERROR;
			_sslInit = true;
		}
		else
		{
			return SSLSOCKET_INVALID_CALL;
		}
	}


	if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		return SSLSOCKET_INVALID_CERT;
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		return SSLSOCKET_INVALID_CERT;
	}
	/* verify private key */
	if (!SSL_CTX_check_private_key(ctx))
	{
		fprintf(stderr, "Private key does not match the public certificate\n");
		return SSLSOCKET_CERT_PRV_PUB_MISMATCH;
	}

	return SSLSOCKET_SUCCESS;
}

SSL_CERTIFICATE_DATA* SslSocket::getCertificateData(SSL* clSsl)
{
	X509* cert = SSL_get_peer_certificate(clSsl);
	if (cert != NULL)
	{
		SSL_CERTIFICATE_DATA* scd = (SSL_CERTIFICATE_DATA*)malloc(sizeof SSL_CERTIFICATE_DATA);
		scd->version = X509_get_version(cert);
		scd->subject = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		scd->issuer = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		return scd;
	}

	return 0;
}

int SslSocket::Connect(char* addr, char* port, SSLDATA_RECEIVED_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_CALLBACK connectionClosedCallback)
{
	if (!addr || !port || !dataRecvCallback || !connectionClosedCallback || _init)
		return SSLSOCKET_INVALID_CALL;

	if (!InitializeClientSSL())
		return SSLSOCKET_SSL_INIT_ERROR;

	int cResult = PerformConnect(addr, port);
	if (cResult != SSLSOCKET_SUCCESS)
		return cResult;

	_init = true;
	_sslInit = true;
	_isServer = false;
	_socketClosed = false;

	sscanf(port, "%d", &_port);
	_dataReceivedCallback = dataRecvCallback;
	_connClosedCallback = connectionClosedCallback;

	callbackType = 0;
	_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);

	return SSLSOCKET_SUCCESS;
}

int SslSocket::Connect(char* addr, char* port, SSLDATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers)
{
	if (!addr || !port || !dataRecvCallback || !connectionClosedCallback || !dataPointers || _init)
		return SSLSOCKET_INVALID_CALL;

	if (!InitializeClientSSL())
		return SSLSOCKET_SSL_INIT_ERROR;

	int cResult = PerformConnect(addr, port);
	if (cResult != SSLSOCKET_SUCCESS)
		return cResult;

	_init = true;
	_sslInit = true;
	_isServer = false;
	_socketClosed = false;

	sscanf(port, "%d", &_port);
	_dataReceivedMemberCallback = dataRecvCallback;
	_connClosedMemberCallback = connectionClosedCallback;

	callbackType = 1;
	_dataPointers = dataPointers;
	_hReadLoop = CreateThread(0, 0, ReadLoop_ThreadCall, this, 0, 0);

	return SSLSOCKET_SUCCESS;
}

int SslSocket::Listen(char* addr, char* port, SSLNEW_CONNECTION_CALLBACK newConnCallback)
{
	if (!addr || !port || !newConnCallback || _init)
		return SSLSOCKET_INVALID_CALL;

	if(!_sslInit)
		return SSLSOCKET_CERT_NOT_SET;

	int lResult = PerformListen(addr, port);
	if (lResult != SSLSOCKET_SUCCESS)
		return lResult;

	_isServer = true;
	_init = true;
	sscanf(port, "%d", &_port);

	_newConCallback = newConnCallback;
	callbackType = 0;
	_hAcceptLoop = CreateThread(0, 0, AcceptLoop_ThreadCall, this, 0, 0);

	return SSLSOCKET_SUCCESS;
}

int SslSocket::Listen(char* addr, char* port, SSLNEW_CONNECTION_MEMBER_CALLBACK newConnCallback, void* dataPointers)
{
	if (!addr || !port || !newConnCallback || !dataPointers || _init)
		return SSLSOCKET_INVALID_CALL;

	if (!_sslInit)
		return SSLSOCKET_CERT_NOT_SET;

	int lResult = PerformListen(addr, port);
	if (lResult != SSLSOCKET_SUCCESS)
		return lResult;

	_isServer = true;
	_init = true;
	sscanf(port, "%d", &_port);

	_dataPointers = dataPointers;
	_newConCallback = newConnCallback;
	callbackType = 1;
	_hAcceptLoop = CreateThread(0, 0, AcceptLoop_ThreadCall, this, 0, 0);

	return SSLSOCKET_SUCCESS;
}

char* SslSocket::getAddress()
{
	if (_socketClosed || _sslSocketClean)
		return 0;

	sockaddr_in* name = (sockaddr_in*)malloc(sizeof sockaddr_in);
	socklen_t namelen = sizeof(sockaddr_in);
	int err = getsockname(_sock, (struct sockaddr*)name, &namelen);
	if (err == SOCKET_ERROR)
		return 0;
	Heap::DbgHeapCheck(name, namelen);

	char* buffer = (char*)malloc(80);
	ZeroMemory(buffer, 80);
	const char* p = inet_ntop(AF_INET, &name->sin_addr, buffer, 80);
	Heap::DbgHeapCheck(buffer, 80);

	free(name);
	return buffer;
}

int SslSocket::getPort()
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

	if (_socketClosed || _sslSocketClean)
		return 0;
	return _port;
}

SOCKET SslSocket::getSocketDescriptor()
{
	if (!_isServer || _socketClosed || _sslSocketClean)
		return 0;
	return _sock;
}

bool SslSocket::setReadBufferSize(int size)
{
	if (size > 65536 || size < 1)
		return false;

	_readBufSize = size;
	return true;
}

bool SslSocket::Write(void* data, size_t dataSize)
{
	if (_isServer || _socketClosed || _sslSocketClean)
		return false;

	int ret = 0;
	try
	{
		ret = SSL_write(ssl, data, dataSize);
		if (SSL_get_error(ssl, ret) == SSL_ERROR_WANT_WRITE)
		{
			int code = 0;
			while (code != SSL_ERROR_NONE)
			{
				ret = SSL_write(ssl, data, dataSize);
				code = SSL_get_error(ssl, ret);
			}
		}
		else
		{
			if (SSL_get_error(ssl, ret) == SSL_ERROR_NONE)
				return true;
		}
	}
	catch (std::exception& e)
	{
		printf("SslSocket Exception At Write(): %s\n", e.what());
	}

	return ret > 0 ? true : false;
}

void SslSocket::Cleanup()
{
	if (_sslSocketClean)
		return;
	CreateThread(0, 0, SslSocket_CleanupThread, this, 0, 0);
}

/* Private Members: Initialization And R/W Management */

bool SslSocket::InitializeServerSSL()
{
	method = (SSL_METHOD*)TLS_server_method();
	ctx = SSL_CTX_new(method);
	if (ctx == NULL)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	return true;
}

bool SslSocket::InitializeClientSSL()
{
	method = (SSL_METHOD*)TLS_client_method();
	ctx = SSL_CTX_new(method);
	if (ctx == NULL)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	return true;
}

int SslSocket::PerformConnect(char* addr, char* port)
{
	struct addrinfo* result = NULL, hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = _ai_family;
	hints.ai_socktype = _ai_socktype;
	hints.ai_protocol = _ai_protocol;

	iResult = getaddrinfo(addr, port, &hints, &result);
	if (iResult != 0) {
		return SSLSOCKET_WINSOCK_FAILURE;
	}

	_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (_sock == INVALID_SOCKET) {
		return SSLSOCKET_WINSOCK_FAILURE;
	}

	if (connect(_sock, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
		return SSLSOCKET_CONNECT_FAILED;

	ssl = SSL_new(ctx);
	SSL_set_fd(ssl, _sock);
	int ret = SSL_connect(ssl);
	if (ret <= 0)
	{
		ret = SSL_get_error(ssl, ret);
		return ret;
	}

	return SSLSOCKET_SUCCESS;
}

int SslSocket::PerformListen(char* addr, char* port)
{
	struct addrinfo* result = NULL, hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = _ai_family;
	hints.ai_socktype = _ai_socktype;
	hints.ai_protocol = _ai_protocol;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(addr, port, &hints, &result);
	if (iResult != 0) {
		return SSLSOCKET_WINSOCK_FAILURE;
	}

	_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (_sock == INVALID_SOCKET) {
		return SSLSOCKET_WINSOCK_FAILURE;
	}

	iResult = bind(_sock, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		return SSLSOCKET_BIND_FAILED;
	}
	freeaddrinfo(result);

	if (listen(_sock, SOMAXCONN) == SOCKET_ERROR) {
		return SSLSOCKET_LISTEN_FAILED;
	}

	return SSLSOCKET_SUCCESS;
}

DWORD SslSocket::SslSocketCleanup()
{
	if (_hAcceptLoop != INVALID_HANDLE_VALUE && _hAcceptLoop)
		TerminateThread(_hAcceptLoop, 0);
	if (_hReadLoop != INVALID_HANDLE_VALUE && _hReadLoop)
		TerminateThread(_hReadLoop, 0);

	if (ssl)
		SSL_free(ssl);
	SSL_CTX_free(ctx);
	if(_isServer)
		closesocket(_sock);

	_socketClosed = true;
	_sslSocketClean = true;

	return 0;
}

DWORD SslSocket::AcceptLoop()
{
	while (!_socketClosed)
	{
		sockaddr_in* clientAddr = (sockaddr_in*)malloc(sizeof sockaddr_in);
		ZeroMemory(clientAddr, sizeof sockaddr_in);
		int len = sizeof(sockaddr_in);
		SOCKET client = accept(_sock, (sockaddr*)clientAddr, &len);
		if (client != SOCKET_ERROR)
		{
			SSLCLIENT_CONNECTION_DATA* ccd = (SSLCLIENT_CONNECTION_DATA*)malloc(sizeof SSLCLIENT_CONNECTION_DATA);
			ZeroMemory(ccd, sizeof SSLCLIENT_CONNECTION_DATA);

			ccd->clSsl = SSL_new(ctx);
			SSL_set_fd(ccd->clSsl, client);
			if (SSL_accept(ccd->clSsl) <= 0) // TODO: Timeout for SSL_accept
			{
				SSL_free(ccd->clSsl);
				closesocket(client); // TODO: ssl error callback
			}
			else
			{
				ccd->socket = client;
				inet_ntop(clientAddr->sin_family, &clientAddr->sin_addr, ccd->ipAddress, 46);
				ccd->clPort = htons((u_short)clientAddr->sin_port);
				ccd->listenPort = _port;
				ccd->instance = _dataPointers ? _dataPointers : 0;

				Heap::DbgHeapCheck(ccd, sizeof SSLCLIENT_CONNECTION_DATA);
				if (callbackType == 0)
					CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_newConCallback, ccd, 0, 0);
				else
					CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_newConMemberCallback, ccd, 0, 0);
			}
		}
		Heap::DbgHeapCheck(clientAddr, sizeof sockaddr_in);
		free(clientAddr);
	}

	return 0;
}

DWORD SslSocket::ReadLoop()
{
	while (!_socketClosed)
	{
		char* buffer = (char*)malloc(65536);
		ZeroMemory(buffer, 65536);
		if (!buffer)
		{
			perror("Heap allocation failed!\n");
			continue;
		}
		int len = SSL_read(ssl, buffer, 65536);
		if (len > 0)
		{
			DATA_RECEVIED_CALLBACK_DATA* drcd = (DATA_RECEVIED_CALLBACK_DATA*)malloc(sizeof DATA_RECEVIED_CALLBACK_DATA);
			drcd->socket = this;
			drcd->buff = buffer;
			drcd->len = len;
			drcd->dataPointers = 0;
			if (callbackType != 0)
				drcd->dataPointers = _dataPointers;
			Heap::DbgHeapCheck(drcd, sizeof DATA_RECEVIED_CALLBACK_DATA);
			CreateThread(0, 0, CallbackDRCV_ThreadCall, drcd, 0, 0);
		}
		else
		{
			int error = SSL_get_error(ssl, len);
			if (error == SSL_ERROR_ZERO_RETURN || error == SSL_ERROR_SYSCALL)
			{
				CONNECTION_CLOSED_CALLBACK_DATA* ccd = (CONNECTION_CLOSED_CALLBACK_DATA*)malloc(sizeof CONNECTION_CLOSED_CALLBACK_DATA);
				ccd->socket = this;
				ccd->ip = getAddress();
				ccd->port = getPort();
				ccd->dataPointers = 0;
				if(callbackType != 0)
					ccd->dataPointers = _dataPointers;
				Heap::DbgHeapCheck(ccd, sizeof CONNECTION_CLOSED_CALLBACK_DATA);
				CreateThread(0, 0, CallbackCCLSD_ThreadCall, ccd, 0, 0);

				_socketClosed = true;
			}
		}
	}

	return 0;
}