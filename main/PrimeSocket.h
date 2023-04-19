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

#ifdef LIBRARY_EXPORTS
#define PRIMESOCKET_API __declspec(dllexport)
#else
#define PRIMESOCKET_API __declspec(dllimport)
#endif

#ifdef _WIN32
#include "incwin_sock.h"
#else
#include "inclinux_sock.h"
#endif
#ifdef USE_CRITICAL_HEAP
#include "Heap.h"
#endif
#include "TcpSocket.h"
#include "UdpSocket.h"
#include "RawSocket.h"
#ifdef PRIMESOCKET_USE_SSL // SslSocket is optional, requires OpenSSL library
#include "SslSocket.h"
#endif

#ifdef _WIN32
PRIMESOCKET_API bool InitializeWSA();
#endif

#define PRIMESOCKET_VERSION_MINOR 0
#define PRIMESOCKET_VERSION_MAJOR 1
#define PRIMESOCKET_VERSION_BETA_STATE TRUE
#define PRIMESOCKET_VERSION_STRING "v0.1-Beta"