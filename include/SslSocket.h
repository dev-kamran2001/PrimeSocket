#pragma once

/* SslSocket
* Establish SSL/TLS connection or server with similar interface to TcpSocket
* You need to provide your own certificates before the server can accept clients!
*/

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

// Statically linking the OpenSSL library because SslSocket is optional and declaration can be avoided by undefining PRIMESOCKET_USE_SSL!
#pragma comment(lib, "libcrypto_static.lib")
#pragma comment(lib, "libssl_static.lib")
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

typedef struct
{
	SSL* clSsl;
	char ipAddress[47];
	int port;
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
	PRIMESOCKET_API SslSocket(SSL* clSsl, int clientPort, SSLDATA_RECEIVED_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_CALLBACK connClosedCallback);
	PRIMESOCKET_API SslSocket(SSL* clSsl, int clientPort, SSLDATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* classInst);

	PRIMESOCKET_API int setServerCertificate(char* CertFile, char* KeyFile);
	PRIMESOCKET_API static SSL_CERTIFICATE_DATA* getCertificateData(SSL* clSsl);
	
	// Connect to specified host and become a client
	PRIMESOCKET_API int Connect(char* addr, char* port, SSLDATA_RECEIVED_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_CALLBACK connectionClosedCallback);
	PRIMESOCKET_API int Connect(char* addr, char* port, SSLDATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, SSLCONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* classInst);
	// Listen on the specified address and port for incoming connections and become a server
	PRIMESOCKET_API int Listen(char* addr, char* port, SSLNEW_CONNECTION_CALLBACK newConnCallback);
	PRIMESOCKET_API int Listen(char* addr, char* port, SSLNEW_CONNECTION_MEMBER_CALLBACK newConnCallback, void* classInst);

	PRIMESOCKET_API char* getAddress();
	PRIMESOCKET_API int getPort();
	PRIMESOCKET_API SOCKET getSocketDescriptor();

	PRIMESOCKET_API bool Write(void* data, size_t dataSize);
	PRIMESOCKET_API bool Write(SSL* clSsl, void* data, size_t dataSize);

	PRIMESOCKET_API void Cleanup();

private:
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
	void* classInstance;

	bool InitializeServerSSL();
	bool InitializeClientSSL();
	int PerformConnect(char* addr, char* port);
	int PerformListen(char* addr, char* port);

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

	HANDLE _hAcceptLoop, _hReadLoop;
	bool _isServer;
	bool _init, _sslInit;
	bool _socketClosed;
	SOCKET _sock;
	int _clPort;
	int _ai_family, _ai_socktype, _ai_protocol;

	bool _mnRead;
};
#endif