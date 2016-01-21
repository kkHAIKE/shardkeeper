#ifndef _STDAFX_H_
#define _STDAFX_H_

// 避免socks的解析问题
#ifdef __WINDOWS__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define syslog(...) 0
#define openlog(...) 0

#else

#include <sys/types.h>
#include <sys/socket.h>

#include <syslog.h>

#endif

#include <exception>
#include <string>

#define SIMPLE_EXCEPTION(name) \
class name: std::exception { \
protected: \
    std::string _msg; \
public: \
    name(const char* msg):_msg(msg){} \
    const char* what() const noexcept { return _msg.c_str();} \
};

#endif
