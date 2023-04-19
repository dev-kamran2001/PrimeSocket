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

#define MAX_TCP_PACKET_SIZE 65536
class TcpSocket;

#define ALLOCATION_MALLOC 1
#define ALLOCATION_PLATFORM 2

typedef struct
{
	char ipAddress[47];
	int clPort;
	int listenPort;
	SOCKET clientSock;
	void* dataPointers;
}CLIENT_CONNECTION_DATA;

typedef void(* NEW_CONNECTION_CALLBACK)(CLIENT_CONNECTION_DATA* client);
typedef void(* NEW_CONNECTION_MEMBER_CALLBACK)(CLIENT_CONNECTION_DATA* client); // additional data pointers are specified in CLIENT_CONNECTION_DATA structure (if present)
typedef void(* DATA_RECEIVED_CALLBACK)(TcpSocket* clientSocket, char* data, size_t dataSize);
typedef void(* DATA_RECEIVED_MEMBER_CALLBACK)(TcpSocket* clientSocket, char* data, size_t dataSize, void* classInstance);
typedef void(* CONNECTION_CLOSED_CALLBACK)(char* address, int port);
typedef void(* CONNECTION_CLOSED_MEMBER_CALLBACK)(char* address, int port, void* classInstance);

class TcpSocket
{
public:

	bool classValid;

	enum SOCKETOPT : DWORD // TODO: More options
	{
		NoDelay = 1,
		KeepAlive = 2,
		RecvTimeout = 3,
		SendTimeout = 4,
		IpDontFragment = 5
	};

	/* Create the socket */
	PRIMESOCKET_API TcpSocket();
	PRIMESOCKET_API TcpSocket(SOCKET client, int clientPort, DATA_RECEIVED_CALLBACK dataRecvCallback, CONNECTION_CLOSED_CALLBACK connClosedCallback, int readBufferSize = 65536);
	PRIMESOCKET_API TcpSocket(SOCKET client, int clientPort, DATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, CONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers, int readBufferSize = 65536);

	// Connect to specific host and become a client
	PRIMESOCKET_API bool Connect(char* addr, char* port, DATA_RECEIVED_CALLBACK dataRecvCallback, CONNECTION_CLOSED_CALLBACK connectionClosedCallback);
	PRIMESOCKET_API bool Connect(char* addr, char* port, DATA_RECEIVED_MEMBER_CALLBACK dataRecvCallback, CONNECTION_CLOSED_MEMBER_CALLBACK connectionClosedCallback, void* dataPointers);
	// Listen on the specific address and port for incoming connections and become a server
	PRIMESOCKET_API bool Listen(char* addr, char* port, NEW_CONNECTION_CALLBACK newConnCallback);
	PRIMESOCKET_API bool Listen(char* addr, char* port, NEW_CONNECTION_MEMBER_CALLBACK newConnCallback, void* dataPointers);

	// Enable/Disable/Modify a socket option 
	PRIMESOCKET_API bool setSocketOption(SOCKETOPT opt, DWORD value);
	PRIMESOCKET_API char* getAddress();
	PRIMESOCKET_API int getPort();
	PRIMESOCKET_API SOCKET getSocketDescriptor();

	// Set the read buffer size, only data equal or less than this value will be readed from the socket (65536 is the default value)
	PRIMESOCKET_API bool setReadBufferSize(int size);
	PRIMESOCKET_API bool isSocketClosed();

	PRIMESOCKET_API bool Write(void* data, size_t dataSize);
	PRIMESOCKET_API bool Write(SOCKET client, void* data, size_t dataSize);
	PRIMESOCKET_API char* Read(size_t len);
	PRIMESOCKET_API char* Read(SOCKET client, size_t len);

	// This function will terminate all the running Threads! avoid calling it inside your callbacks
	PRIMESOCKET_API void ForceShutdown();
	PRIMESOCKET_API void Close();

protected:
	void InitializeMembers();

private:
	typedef struct
	{
		TcpSocket* socket;
		char* buff;
		size_t len;
		void* dataPointers;
		int buffAllocType;
		int allocType;
	}DATA_RECEVIED_CALLBACK_DATA;
	typedef struct
	{
		TcpSocket* socket;
		char* ip;
		int port;
		void* dataPointers;
		int allocType;
	}CONNECTION_CLOSED_CALLBACK_DATA;

	NEW_CONNECTION_CALLBACK _newConCallback;
	DATA_RECEIVED_CALLBACK _dataReceivedCallback;
	CONNECTION_CLOSED_CALLBACK _connClosedCallback;

	NEW_CONNECTION_MEMBER_CALLBACK _newConMemberCallback;
	DATA_RECEIVED_MEMBER_CALLBACK _dataReceivedMemberCallback;
	CONNECTION_CLOSED_MEMBER_CALLBACK _connClosedMemberCallback;

	int callbackType;
	void* _dataPointers;

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
	bool _init;
	bool _socketClosed, _csCalled;
	SOCKET _sock;
	int _port;
	int _ai_family, _ai_socktype, _ai_protocol;
	int _readBufSize;
};