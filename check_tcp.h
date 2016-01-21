#ifndef _CHECK_TCP_H_
#define _CHECK_TCP_H_

#include <event2/event.h>

#ifdef __WINDOWS__
#undef evutil_socket_t
#define evutil_socket_t int
#endif

enum connect_result {
	connect_error,
	connect_in_progress,
	connect_timeout,
	connect_success
};

struct _service;
typedef struct _checker {
    struct event_base * base;
    timeval endtime;
    int retry_it;
    struct sockaddr_storage * co;
    int status;  //0 none 1 up 2 down
	struct _service * service;
}checker_t;

void tcp_connect_thread(evutil_socket_t fd, short events, void* arg);

#endif
