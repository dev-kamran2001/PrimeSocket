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

#include "PrimeSocket.h"

char* RawSocket::GetActiveInterfaceAddress()
{
	const char* google_server = "216.239.38.120";
	int port = 80;

	struct sockaddr_in serv;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == SOCKET_ERROR)
		return 0;

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(google_server);
	serv.sin_port = htons(port);

	int err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));
	if (err == SOCKET_ERROR)
		return 0;

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr*)&name, &namelen);
	if (err == SOCKET_ERROR)
		return 0;

	char *buffer = (char*)malloc(80);
	ZeroMemory(buffer, 80);
	const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);
	
	closesocket(sock);
	return buffer;
}

RawSocket::RawSocket()
{
	_autoIphGen = true;
	_protoNum = 200;
	_autoChecksum = false;
	_protoId = 0;

	_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	int optval = TRUE;
	setsockopt(_sock, IPPROTO_IP, IP_HDRINCL, (char*)&optval, sizeof(optval));
}

void RawSocket::setAutoIpHeaderGenEnabled(bool enabled)
{
	_autoIphGen = enabled;
}

void RawSocket::setProtocolNumber(int number)
{
	_protoNum = number;
}

void RawSocket::setAutoChecksumEnabled(bool enabled)
{
	_autoChecksum = enabled;
}

void RawSocket::setPacketProtocolId(int id)
{
	_protoId = id;
}

bool RawSocket::Write(char* data, size_t size, char* destAddress)
{
	char* buf = (char*)malloc(65536);
	SOCKADDR_IN dest;
	hostent* server;
	ZeroMemory(buf, 65536);

	if ((server = gethostbyname(destAddress)) == 0)
	{
		free(buf);
		return false;
	}
	dest.sin_family = AF_INET;
	//dest.sin_port = htons(555); //your destination port
	memcpy(&dest.sin_addr.s_addr, server->h_addr, server->h_length);

	IPV4_HDR *v4hdr = (IPV4_HDR*)buf;
	v4hdr->ip_version = 4; // IP version 4
	v4hdr->ip_header_len = 5; // 5 = 20 bytes
	v4hdr->ip_tos = 0; // Type of service which is 0 here
	//v4hdr->ip_total_length = htons(sizeof(IPV4_HDR) + sizeof(TCP_HDR) + payload);
	v4hdr->ip_total_length = htons(sizeof(IPV4_HDR) + size);
	v4hdr->ip_id = (_protoId == 0 ? 0 : htons(_protoId)); // Identification
	v4hdr->ip_frag_offset = 0; // Fragmentation offset1 (may set by ISP)
	v4hdr->ip_frag_offset1 = 0; // Fragmentation offset2 (may set by ISP)
	v4hdr->ip_reserved_zero = 0; // Fragment Option "Reserved"
	v4hdr->ip_dont_fragment = 1; // Fragment Option "Don't Fragment" : true=1 & false=0
	v4hdr->ip_more_fragment = 0; // Fragment Option "More Fragments" (may set by ISP)
	v4hdr->ip_ttl = 64; // Time-to-Live
	v4hdr->ip_protocol = _protoNum; // Protocol
	v4hdr->ip_srcaddr = inet_addr(GetActiveInterfaceAddress());
	v4hdr->ip_destaddr = inet_addr(inet_ntoa(dest.sin_addr));
	v4hdr->ip_checksum = 0; // Header Checksum

	char* packetData = &buf[sizeof(IPV4_HDR)];
	memcpy(packetData, data, size);

	if (sendto(_sock, buf, sizeof(IPV4_HDR) + size, 0, (SOCKADDR*)&dest, sizeof(dest)) == SOCKET_ERROR)
	{
		free(buf);
		return false;
	}

	free(buf);
	return true;
}