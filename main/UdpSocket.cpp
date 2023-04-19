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

UdpSocket::UdpSocket()
{
    _sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    _datagramReceivedCallback = 0;
    _datagramReceivedMemberCallback = 0;
    _hReadLoop = INVALID_HANDLE_VALUE;
    _hMemCallback = INVALID_HANDLE_VALUE;
}

bool UdpSocket::Bind(char* addr, char* port, DATAGRAM_RECEIVED_CALLBACK datagramReceivedCallback)
{
    struct addrinfo* result = NULL, hints;
    int iResult;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(addr, port, &hints, &result);
    if (iResult != 0) {
        return false;
    }

    iResult = bind(_sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        return false;
    }
    freeaddrinfo(result); // No longer needed

    _bound = true;
    _datagramReceivedCallback = datagramReceivedCallback;
    callbackType = 0;
    _hReadLoop = CreateThread(0, 0, DatagramReadLoop_ThreadCall, this, 0, 0);

    return true;
}

bool UdpSocket::Bind(char* addr, char* port, DATAGRAM_RECEIVED_P_CALLBACK datagramReceivedCallback, void* dataPointers)
{
    struct addrinfo* result = NULL, hints;
    int iResult;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(addr, port, &hints, &result);
    if (iResult != 0) {
        return false;
    }

    iResult = bind(_sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        return false;
    }
    freeaddrinfo(result); // No longer needed

    _bound = true;
    _datagramReceivedMemberCallback = datagramReceivedCallback;
    _dataPointers = dataPointers;
    callbackType = 1;
    _hReadLoop = CreateThread(0, 0, DatagramReadLoop_ThreadCall, this, 0, 0);

    return true;
}

bool UdpSocket::setReceiverCallback(DATAGRAM_RECEIVED_CALLBACK datagramReceivedCallback)
{
    if (!_sock || _sock == INVALID_SOCKET || _sock == SOCKET_ERROR)
        return false;

    _datagramReceivedCallback = datagramReceivedCallback;
    callbackType = 0;
    return true;
}

bool UdpSocket::setReceiverCallback(DATAGRAM_RECEIVED_P_CALLBACK datagramReceivedCallback, void* dataPointers)
{
    if (!_sock || _sock == INVALID_SOCKET || _sock == SOCKET_ERROR)
        return false;

    _datagramReceivedMemberCallback = datagramReceivedCallback;
    _dataPointers = dataPointers;
    callbackType = 1;
    return true;
}

bool UdpSocket::setSocketOption(SOCKETOPT opt, DWORD value)
{
    if (opt == 0 || opt > 5)
        return false;

    int result = 0;
    switch (opt)
    {
    case ReuseAddress:
        if (value != TRUE && value != FALSE)
            return false;

        result = setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(DWORD));
        if (result != SOCKET_ERROR)
            return true;
        else
            return false;

        break;
    case ChecksumEnabled:
        if (value != TRUE && value != FALSE)
            return false;

        if (value)
        {
            result = setsockopt(_sock, IPPROTO_UDP, UDP_CHECKSUM_COVERAGE, (char*)&value, sizeof(DWORD));
            if (result != SOCKET_ERROR)
                return true;
            else
                return false;
        }
        else
        {
            result = setsockopt(_sock, IPPROTO_UDP, UDP_CHECKSUM_COVERAGE, (char*)&value, sizeof(DWORD));
            if (result != SOCKET_ERROR)
                return true;
            else
                return false;

            int val = TRUE;
            result = setsockopt(_sock, IPPROTO_UDP, UDP_NOCHECKSUM, (char*)&val, sizeof(DWORD));
            if (result != SOCKET_ERROR)
                return true;
            else
                return false;
        }

        break;
    case SendMsgSize:
#ifdef _WIN32
        result = WSASetUdpSendMessageSize(_sock, value);
#else
        result = setsockopt(_sock, IPPROTO_UDP, UDP_SEND_MSG_SIZE, (char*)&value, sizeof(unsigned long));
#endif

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
    }

    return false;
}

bool UdpSocket::Write(char* addr, int port, char* datagram, int datagram_len)
{
    sockaddr_in *so_addr = (sockaddr_in*)malloc(sizeof sockaddr_in);
    ZeroMemory(so_addr, sizeof sockaddr_in);
    int slen = sizeof(sockaddr_in);

    so_addr->sin_family = AF_INET;
    so_addr->sin_port = htons(port);
    so_addr->sin_addr.S_un.S_addr = inet_addr(addr);

    if (sendto(_sock, datagram, datagram_len, 0, (const sockaddr*)so_addr, slen) == SOCKET_ERROR)
    {
        free(so_addr);
        return false;
    }

    free(so_addr);

    // If not bound, create the read loop after first call to Write
    // This is because we are not able to Read first before Writing when the socket is not bound
    if (!_bound && (!_hReadLoop || _hReadLoop == INVALID_HANDLE_VALUE)
        && (_datagramReceivedCallback || _datagramReceivedMemberCallback))
    {
        _hReadLoop = CreateThread(0, 0, DatagramReadLoop_ThreadCall, this, 0, 0);
    }

    return true;
}

bool UdpSocket::Write(UDP_DATAGRAM *datagram)
{
    sockaddr_in* so_addr = (sockaddr_in*)malloc(sizeof sockaddr_in);
    int slen = sizeof(sockaddr_in);

    memset((char*)&so_addr, 0, sizeof(so_addr));
    so_addr->sin_family = AF_INET;
    so_addr->sin_port = htons(datagram->peer.port);
    so_addr->sin_addr.S_un.S_addr = inet_addr(datagram->peer.addr);

    if (sendto(_sock, datagram->data, datagram->len, 0, (const sockaddr*)so_addr, slen) == SOCKET_ERROR)
    {
        free(so_addr);
        return false;
    }

    free(so_addr);

    // If not bound, create the read loop after first call to Write
    // This is because we are not able to Read first before Writing when the socket is not bound
    if (!_bound && (!_hReadLoop || _hReadLoop == INVALID_HANDLE_VALUE)
        && (_datagramReceivedCallback || _datagramReceivedMemberCallback))
    {
        _hReadLoop = CreateThread(0, 0, DatagramReadLoop_ThreadCall, this, 0, 0);
    }

    return true;
}

UDP_DATAGRAM* UdpSocket::Read(size_t len)
{
    int len_read = (len == 0 ? 65536 : len);
    int iResult = 0;
    struct sockaddr_in si_other;
    int slen = sizeof(sockaddr_in);

    char* buf = (char*)malloc(65536);
    iResult = recvfrom(_sock, buf, 65536, 0, (struct sockaddr*)&si_other, &slen);
    if (iResult > 0)
    {
        UDP_DATAGRAM* datagram = (UDP_DATAGRAM*)malloc(sizeof UDP_DATAGRAM);
        ZeroMemory(datagram, sizeof UDP_DATAGRAM);
        datagram->peer.addr = inet_ntoa(si_other.sin_addr);
        datagram->peer.port = ntohs(si_other.sin_port);

        memcpy(datagram->data, buf, iResult);
        datagram->len = iResult;

        free(buf);
        return datagram;
    }

    free(buf);
    return 0;
}

DWORD UdpSocket::DatagramReadLoop()
{
    int iResult = 0;
    struct sockaddr_in si_other;
    int slen = sizeof(sockaddr_in);

    char* buf = (char*)malloc(65536);
    while(true)
    {
        iResult = recvfrom(_sock, buf, 65536, 0, (struct sockaddr*)&si_other, &slen);
        if (iResult > 0)
        {
            UDP_DATAGRAM* datagram = (UDP_DATAGRAM*)malloc(sizeof UDP_DATAGRAM);
            ZeroMemory(datagram, sizeof UDP_DATAGRAM);
            datagram->peer.addr = inet_ntoa(si_other.sin_addr);
            datagram->peer.port = ntohs(si_other.sin_port);

            memcpy(datagram->data, buf, iResult);
            datagram->len = iResult;
            if(callbackType == 0)
            {
                CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_datagramReceivedCallback, datagram, 0, 0);
            }
            else
            {
                MEMBER_CALLBACK_CALLINFO* _mcci = (MEMBER_CALLBACK_CALLINFO*)malloc(sizeof MEMBER_CALLBACK_CALLINFO);
                _mcci->datagram = datagram;
                _mcci->_instance = this;
                _hMemCallback = CreateThread(0, 0, MemberCallback_StaticCall, _mcci, 0, 0); 
            }
        }
        ZeroMemory(buf, 65536);
    }
    return 0;
}

void UdpSocket::Close()
{
    if (_hReadLoop)
        TerminateThread(_hReadLoop, 0);
    closesocket(_sock);
}
