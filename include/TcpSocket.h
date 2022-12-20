#pragma once

/* TcpSocket
* Establish a TCP connection or server
* Callbacks for class static members are supported
* 
* Callbacks are not created in a separated thread, so don't do any long blocking operations in your callbacks!
*/

#define MAX_TCP_PACKET_SIZE 65536
class TcpSocket;

typedef struct
{
	char ipAddress[47];
	int port;
	SOCKET clientSock;
	void* instance;
}CLIENT_CONNECTION_DATA;

typedef void(__stdcall* NEW_CONNECTION_CALLBACK)(CLIENT_CONNECTION_DATA* client);
typedef void(__stdcall* NEW_CONNECTION_MEMBER_CALLBACK)(CLIENT_CONNECTION_DATA* client); // Class instance are specified in CLIENT_CONNECTION_DATA structure (if present)
typedef void(__stdcall* DATA_RECEIVED_CALLBACK)(TcpSocket* clientSocket, char* data, size_t dataSize);
typedef void(__stdcall* DATA_RECEIVED_MEMBER_CALLBACK)(TcpSocket* clientSocket, char* data, size_t dataSize, void* classInstance);
typedef void(__stdcall* CONNECTION_CLOSED_CALLBACK)(char* address, int port);
typedef void(__stdcall* CONNECTION_CLOSED_MEMBER_CALLBACK)(char* address, int port, void* classInstance);

class TcpSocket
{
public:

	enum SOCKETOPT : DWORD // TODO: More options
	{
		NoDelay = 1,
		KeepAlive = 2,
		RecvTimeout = 3,
		SendTimeout = 4
	};

	/* Create the socket */
	PRIMESOCKET_API TcpSocket();
	/* Create the socket as a client and manage the client socket */
	PRIMESOCKET_API TcpSocket(SOCKET client, int clientPort, DATA_RECEIVED_CALLBACK dataRecvCallback, CONNECTION_CLOSED_CALLBACK connClosedCallback);
	PRIMESOCKET_API TcpSocket(SOCKET client, int clientPort, DATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, CONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* classInst);

	// Connect to specified host and become a client
	PRIMESOCKET_API bool Connect(char* addr, char* port, DATA_RECEIVED_CALLBACK dataRecvCallback, CONNECTION_CLOSED_CALLBACK connectionClosedCallback);
	PRIMESOCKET_API bool Connect(char* addr, char* port, DATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, CONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* classInst);
	// Listen on the specified address and port for incoming connections and become a server
	PRIMESOCKET_API bool Listen(char* addr, char* port, NEW_CONNECTION_CALLBACK newConnCallback);
	PRIMESOCKET_API bool Listen(char* addr, char* port, NEW_CONNECTION_MEMBER_CALLBACK newConnCallback, void* classInst);

	// Enable/Disable/Modify a socket option 
	PRIMESOCKET_API bool setSocketOption(SOCKETOPT opt, DWORD value);
	PRIMESOCKET_API char* getAddress();
	PRIMESOCKET_API int getPort();
	PRIMESOCKET_API SOCKET getSocketDescriptor();

	PRIMESOCKET_API bool Write(void* data, size_t dataSize);
	PRIMESOCKET_API bool Write(SOCKET client, void* data, size_t dataSize);
	PRIMESOCKET_API char* Read(size_t len);
	PRIMESOCKET_API char* Read(SOCKET client, size_t len);

	PRIMESOCKET_API void Close();

private:
	NEW_CONNECTION_CALLBACK _newConCallback;
	DATA_RECEIVED_CALLBACK _dataReceivedCallback;
	CONNECTION_CLOSED_CALLBACK _connClosedCallback;

	NEW_CONNECTION_MEMBER_CALLBACK _newConMemberCallback;
	DATA_RECEIVED_MEMBER_CALLBACK _dataReceivedMemberCallback;
	CONNECTION_CLOSED_MEMBER_CALLBACK _connClosedMemberCallback;

	int callbackType;
	void* classInstance;

	static DWORD WINAPI AcceptLoop_ThreadCall(LPVOID param)
	{
		TcpSocket* _instance = (TcpSocket*)param;
		return _instance->AcceptLoop();
	}
	DWORD AcceptLoop();

	static DWORD WINAPI ReadLoop_ThreadCall(LPVOID param)
	{
		TcpSocket* _instance = (TcpSocket*)param;
		return _instance->ReadLoop();
	}
	DWORD ReadLoop();

	HANDLE _hAcceptLoop, _hReadLoop;
	bool _isServer;
	bool _init;
	bool _socketClosed;
	SOCKET _sock;
	int _clPort;
	int _ai_family, _ai_socktype, _ai_protocol;

	bool _mnRead;
};