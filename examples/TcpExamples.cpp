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

#include <PrimeSocket.h>

void TcpServer_DataReceived(TcpSocket* clientSocket, char* data, size_t dataSize, void* pointer)
{
	std::cout << "Received: " << data << std::endl;
	// Echo back the data to client
	clientSocket->Write(data, dataSize);
}

void TcpServer_ConnectionClosed(char* address, int port, void* pointer)
{
	bool* serverRunning = (bool*)pointer;
	
	std::cout << "Client Disconnected, Server Now Will Exit.\n";
	*serverRunning = false;
}

void TcpServer_NewConnection(CLIENT_CONNECTION_DATA* client)
{
	std::cout << "Client [" << client->ipAddress << ":" << client->clPort << "] Connected!\n";

	// This will create a Data Read Loop for this socket and the defined functions will be called whenever data received or connection closed
	TcpSocket* clientHandler = new TcpSocket(client->clientSock, client->clPort, TcpServer_DataReceived, TcpServer_ConnectionClosed, client->dataPointers);
	// Enable Keep-Alive packets so we will be notified when the client disconnects as soon as possible (not necessary on Local Host connections)
	clientHandler->setSocketOption(TcpSocket::SOCKETOPT::KeepAlive, 1);
}

bool ConnectToGoogle()
{
	const char* request = "GET / HTTP/1.1\r\n"
		"Host: www.google.com\r\n"
		"Connection: Keep-Alive\r\n"
		"Cache-Control: no-cache\r\n"
		"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 OPR/93.0.0.0\r\n"
		"\r\n";

	TcpSocket* clientSocket = new TcpSocket();
	clientSocket->Connect((char*)"216.239.38.120", (char*)"80", 0, 0);
	clientSocket->Write((void*)request, strlen(request));

	char* response = clientSocket->Read(512);
	clientSocket->Close();
	if (strstr(response, "HTTP/1.1 200 OK") || strstr(response, "HTTP/1.1 302 Found"))
	{
		free(response);
		return true;
	}
	else
	{
		free(response);
		return false;
	}
}

int RunTcpClient()
{
	std::string ipAddr;
	std::cout << "Enter Server IPv4 Address: ";
	std::getline(std::cin, ipAddr);

	TcpSocket* clientSocket = new TcpSocket();
	bool result = clientSocket->Connect((char*)ipAddr.c_str(), (char*)"5050", 0, 0);
	if (!result)
	{
		std::cout << "Failed to connect!\n";
		return 1;
	}

	std::cout << "\nConnection Successful! Enter \"exit\" or \"close\" To Exit Socket I/O Mode\n\n";
	while (true)
	{
		std::cout << "$ ";
		std::string data;
		std::getline(std::cin, data);
		std::cout << std::endl;

		if (data._Equal("exit") || data._Equal("close"))
			break;

		clientSocket->Write((void*)data.c_str(), data.size());
		char* response = clientSocket->Read(data.size());
		std::cout << "Received: " << response << "\n\n";
	}

	return 0;
}

int RunTcpServer()
{
	bool serverRunning = true;
	TcpSocket* serverSocket = new TcpSocket();
	bool result = serverSocket->Listen((char*)"127.0.0.1", (char*)"5050", TcpServer_NewConnection, &serverRunning);
	if (!result)
	{
		std::cout << "Failed to listen on port 5050!\n";
		return 1;
	}

	std::cout << "Server is now running on 127.0.0.1:5050\n";
	std::cout << "Note: The server will exit when client disconnect\n";

	while (serverRunning)
		Sleep(90);

	return 0;
}