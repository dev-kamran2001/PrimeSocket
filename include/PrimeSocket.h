#pragma once

#ifdef PRIMESOCKET_USE_CPP
#include <string>
#include <cstring>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <exception>
#endif

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
#define PRIMESOCKET_VERSION_ALPHA_STATE TRUE
#define PRIMESOCKET_VERSION_BETA_STATE FALSE
#define PRIMESOCKET_VERSION_STRING "v0.1-Alpha"