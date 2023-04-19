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

#ifdef PRIMESOCKET_USE_SSL
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/rand.h>

// Statically linking the OpenSSL library because SslSocket is optional and declaration can be avoided by undefining PRIMESOCKET_USE_SSL
#pragma comment(lib, "libcrypto_static.lib")
#pragma comment(lib, "libssl_static.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "rpcrt4.lib")

class SslSocket;

#define SSLSOCKET_SUCCESS 1
#define SSLSOCKET_LISTEN_FAILED -1
#define SSLSOCKET_CONNECT_FAILED -2
#define SSLSOCKET_SSL_CONNECT_FAILED -3
#define SSLSOCKET_BIND_FAILED -4
#define SSLSOCKET_SSL_INIT_ERROR -5
#define SSLSOCKET_INVALID_CERT -6
#define SSLSOCKET_CERT_PRV_PUB_MISMATCH -7
#define SSLSOCKET_INVALID_CALL -8
#define SSLSOCKET_WINSOCK_FAILURE -9
#define SSLSOCKET_CERT_NOT_SET -10

typedef struct
{
	SSL* clSsl;
	SOCKET socket;
	char ipAddress[47];
	int clPort;
	int listenPort;
	void* instance;
}SSLCLIENT_CONNECTION_DATA;

typedef struct
{
	long version;
	char* subject;
	char* issuer;
}SSL_CERTIFICATE_DATA;

typedef void(*SSLNEW_CONNECTION_CALLBACK)(SSLCLIENT_CONNECTION_DATA* client);
typedef void(*SSLNEW_CONNECTION_MEMBER_CALLBACK)(SSLCLIENT_CONNECTION_DATA* client);
typedef void(*SSLDATA_RECEIVED_CALLBACK)(SslSocket* clientSocket, char* data, size_t dataSize);
typedef void(*SSLDATA_RECEIVED_MEMBER_CALLBACK)(SslSocket* clientSocket, char* data, size_t dataSize, void* classInstance);
typedef void(*SSLCONNECTION_CLOSED_CALLBACK)(char* address, int port);
typedef void(*SSLCONNECTION_CLOSED_MEMBER_CALLBACK)(char* address, int port, void* classInstance);

PRIMESOCKET_API bool InitializeSSL();

class SslSocket
{
public:

	/* Create the socket */
	PRIMESOCKET_API SslSocket();
	PRIMESOCKET_API SslSocket(SSL* clSsl, int clientPort, SSLDATA_RECEIVED_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_CALLBACK connClosedCallback, int readBufferSize = 65536);
	PRIMESOCKET_API SslSocket(SSL* clSsl, int clientPort, SSLDATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers, int readBufferSize = 65536);

	PRIMESOCKET_API int setServerCertificate(char* CertFile, char* KeyFile);
	PRIMESOCKET_API static SSL_CERTIFICATE_DATA* getCertificateData(SSL* clSsl);
	
	// Connect to specified host and become a client
	PRIMESOCKET_API int Connect(char* addr, char* port, SSLDATA_RECEIVED_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_CALLBACK connectionClosedCallback);
	PRIMESOCKET_API int Connect(char* addr, char* port, SSLDATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers);
	// Listen on the specified address and port for incoming connections and become a server
	PRIMESOCKET_API int Listen(char* addr, char* port, SSLNEW_CONNECTION_CALLBACK newConnCallback);
	PRIMESOCKET_API int Listen(char* addr, char* port, SSLNEW_CONNECTION_MEMBER_CALLBACK newConnCallback, void* dataPointers);

	PRIMESOCKET_API char* getAddress();
	PRIMESOCKET_API int getPort();
	PRIMESOCKET_API SOCKET getSocketDescriptor();
	
	// Set the read buffer size, only data equal or less than this value will be readed from the socket (65536 is the default value)
	PRIMESOCKET_API bool setReadBufferSize(int size = 65536);

	PRIMESOCKET_API bool Write(void* data, size_t dataSize);
	//PRIMESOCKET_API bool Write(SSL* clSsl, void* data, size_t dataSize);

	PRIMESOCKET_API void Cleanup();

private:
	typedef struct
	{
		SslSocket* socket;
		char* buff;
		size_t len;
		void* dataPointers;
	}DATA_RECEVIED_CALLBACK_DATA;
	typedef struct
	{
		SslSocket* socket;
		char* ip;
		int port;
		void* dataPointers;
	}CONNECTION_CLOSED_CALLBACK_DATA;


	SSL_METHOD* method;
	SSL_CTX* ctx;
	SSL* ssl;

	SSLNEW_CONNECTION_CALLBACK _newConCallback;
	SSLDATA_RECEIVED_CALLBACK _dataReceivedCallback;
	SSLCONNECTION_CLOSED_CALLBACK _connClosedCallback;

	SSLNEW_CONNECTION_MEMBER_CALLBACK _newConMemberCallback;
	SSLDATA_RECEIVED_MEMBER_CALLBACK _dataReceivedMemberCallback;
	SSLCONNECTION_CLOSED_MEMBER_CALLBACK _connClosedMemberCallback;

	int callbackType;
	void* _dataPointers;

	bool InitializeServerSSL();
	bool InitializeClientSSL();
	int PerformConnect(char* addr, char* port);
	int PerformListen(char* addr, char* port);

	static DWORD WINAPI SslSocket_CleanupThread(LPVOID param)
	{
		SslSocket* _instance = (SslSocket*)param;
		DWORD ret = _instance->SslSocketCleanup();
		return ret;
	}
	DWORD SslSocketCleanup();

	static DWORD WINAPI AcceptLoop_ThreadCall(LPVOID param)
	{
		SslSocket* _instance = (SslSocket*)param;
		DWORD ret = _instance->AcceptLoop();
		return ret;
	}
	DWORD AcceptLoop();

	static DWORD WINAPI ReadLoop_ThreadCall(LPVOID param)
	{
		SslSocket* _instance = (SslSocket*)param;
		DWORD ret = _instance->ReadLoop();
		_instance->Cleanup();
		return ret;
	}
	DWORD ReadLoop();

	static DWORD WINAPI CallbackDRCV_ThreadCall(LPVOID param)
	{
		DATA_RECEVIED_CALLBACK_DATA* drcd = (DATA_RECEVIED_CALLBACK_DATA*)param;
		if (drcd->dataPointers == 0)
			drcd->socket->_dataReceivedCallback(drcd->socket, drcd->buff, drcd->len);
		else
			drcd->socket->_dataReceivedMemberCallback(drcd->socket, drcd->buff, drcd->len, drcd->dataPointers);
		free(drcd->buff);
		free(drcd);
		return 0;
	}

	static DWORD WINAPI CallbackCCLSD_ThreadCall(LPVOID param)
	{
		CONNECTION_CLOSED_CALLBACK_DATA* ccd = (CONNECTION_CLOSED_CALLBACK_DATA*)param;
		if (ccd->dataPointers == 0)
			ccd->socket->_connClosedCallback(ccd->ip, ccd->port);
		else
			ccd->socket->_connClosedMemberCallback(ccd->ip, ccd->port, ccd->dataPointers);
		free(ccd->ip);
		free(ccd);
		return 0;
	}

	HANDLE _hAcceptLoop, _hReadLoop;
	bool _isServer;
	bool _init, _sslInit;
	bool _socketClosed, _sslSocketClean;
	SOCKET _sock;
	int _port;
	int _ai_family, _ai_socktype, _ai_protocol;
	int _readBufSize;

	bool _mnRead;
};
#endif