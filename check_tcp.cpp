#include "stdafx.h"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "check_tcp.h"

#ifdef __WINDOWS__
#define EINPROGRESS WSAEINPROGRESS
#define fcntl(...) 0
#endif

#include "parser.h"

// time_util
#define TIMER_HZ		1000000
#define timer_long(T) ((T).tv_sec * TIMER_HZ + (T).tv_usec)
#define timer_ltot(t, l) \
    do { \
        (t)->tv_sec = (l) / TIMER_HZ; \
        (t)->tv_usec = (l) % TIMER_HZ; \
    } while (0)

const char *sockaddr_string(const struct sockaddr_storage *addr, char *dst, size_t len) {
    int port;
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)addr;
        port = ntohs(s->sin_port);
        evutil_inet_ntop(AF_INET, &s->sin_addr, dst, len);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)addr;
        port = ntohs(s->sin6_port);
        evutil_inet_ntop(AF_INET6, &s->sin6_addr, dst, len);
    }

    strcat(dst, ":");

    char portstr[6];
    strcat(dst, itoa(port, portstr, 10));
    return dst;
}

void thread_add_event(struct event_base * base, event_callback_fn callback, checker_t *checker, evutil_socket_t fd, short events, long timeout, const char* fname)
{
    timeval delay;
    timer_ltot(&delay, timeout *TIMER_HZ);

    timeval now;
    evutil_gettimeofday(&now, NULL);
    evutil_timeradd(&now, &delay, &checker->endtime);

    if (event_base_once(base, fd, events, callback, checker, &delay) < 0) {
        syslog(LOG_ERR, "%s failed", fname);
    }
}

#define thread_add_write(b,c,a,f,t) thread_add_event(b,c,a,f,EV_WRITE,t,"thread_add_write")
#define thread_add_timer(b,c,a,t) thread_add_event(b,c,a,-1,0,t,"thread_add_timer")

#define svr_checker_up(chk) (checker->status == 1)
void update_svr_checker_state(checker_t * checker, int status);

// up my

enum connect_result
tcp_bind_connect(int fd, struct sockaddr_storage *addr)
{
	struct linger li = { 0, 0 };
	socklen_t addrlen;
	int ret;
	int val;

	/* free the tcp port after closing the socket descriptor */
	li.l_onoff = 1;
	li.l_linger = 0;
	setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof (struct linger));

	/* Make socket non-block. */
	val = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, val | O_NONBLOCK);

	/* Set remote IP and connect */
	addrlen = sizeof(*addr);
	ret = connect(fd, (struct sockaddr *) addr, addrlen);

	/* Immediate success */
	if (ret == 0) {
		fcntl(fd, F_SETFL, val);
		return connect_success;
	}

	/* If connect is in progress then return 1 else it's real error. */
	if (ret < 0) {
		if (errno != EINPROGRESS)
			return connect_error;
	}

	/* restore previous fd args */
	fcntl(fd, F_SETFL, val);
	return connect_in_progress;
}

enum connect_result
tcp_socket_state(evutil_socket_t fd, short events, checker_t* checker, event_callback_fn func)
{
	int status;
	socklen_t addrlen;
	int ret = 0;
	timeval timer_min;

	/* Handle connection timeout */
	if (events & EV_TIMEOUT) {
		close(fd);
		return connect_timeout;
	}

	/* Check file descriptor */
	addrlen = sizeof(status);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &status, &addrlen) < 0)
		ret = errno;

	/* Connection failed !!! */
	if (ret) {
		close(fd);
		return connect_error;
	}

	/* If status = 0, TCP connection to remote host is established.
	 * Otherwise register checker thread to handle connection in progress,
	 * and other error code until connection is established.
	 * Recompute the write timeout (or pending connection).
	 */
	if (status == EINPROGRESS) {
        timeval now;
        evutil_gettimeofday(&now, NULL);
        evutil_timersub(&checker->endtime, &now, &timer_min);
        if (timer_min.tv_sec < 0) evutil_timerclear(&timer_min);

		thread_add_write(checker->base, func, checker, fd, timer_long(timer_min));
		return connect_in_progress;
	} else if (status != 0) {
		close(fd);
		return connect_error;
	}

	return connect_success;
}

int
tcp_connection_state(int fd, enum connect_result status, checker_t* checker,
		     event_callback_fn func, long timeout)
{
	switch (status) {
	case connect_success:
		thread_add_write(checker->base, func, checker, fd, timeout);
		return 0;

		/* Checking non-blocking connect, we wait until socket is writable */
	case connect_in_progress:
		thread_add_write(checker->base, func, checker, fd, timeout);
		return 0;

	default:
		return 1;
	}
}

// up layer4

void
tcp_eplilog(checker_t* checker, int is_success)
{
	long delay;

	if (is_success || checker->retry_it > g_config.n_retry - 1) {
		delay = g_config.delay_loop;
		checker->retry_it = 0;
        char ipstr[INET6_ADDRSTRLEN + 8];

		if (is_success && !svr_checker_up(checker)) {
			syslog(LOG_INFO, "TCP connection to %s success."
					, sockaddr_string(checker->co, ipstr, sizeof(ipstr)));
			update_svr_checker_state(checker, 1);
		} else if (! is_success
			   && svr_checker_up(checker)) {
			if (g_config.n_retry)
				syslog(LOG_INFO
				    , "Check on service %s failed after %d retry."
				    , sockaddr_string(checker->co, ipstr, sizeof(ipstr))
				    , g_config.n_retry);
			update_svr_checker_state(checker, 2);
		}
	} else {
		delay = g_config.delay_before_retry;
		++checker->retry_it;
	}

	/* Register next timer checker */
	thread_add_timer(checker->base, tcp_connect_thread, checker, delay);
}

void
tcp_check_thread(evutil_socket_t fd, short events, void* arg)
{
	checker_t *checker = (checker_t *)arg;
	int status;
    char ipstr[INET6_ADDRSTRLEN + 8];

	status = tcp_socket_state(fd, events, checker, tcp_check_thread);

	/* If status = connect_in_progress, next thread is already registered.
	 * If it is connect_success, the fd is still open.
	 * Otherwise we have a real connection error or connection timeout.
	 */
	switch(status) {
	case connect_in_progress:
		break;
	case connect_success:
		close(fd);
		tcp_eplilog(checker, 1);
		break;
	case connect_timeout:
		if (svr_checker_up(checker))
			syslog(LOG_INFO, "TCP connection to %s timeout."
					, sockaddr_string(checker->co, ipstr, sizeof(ipstr)));
		tcp_eplilog(checker, 0);
		break;
	default:
		if (svr_checker_up(checker))
			syslog(LOG_INFO, "TCP connection to %s failed."
					, sockaddr_string(checker->co, ipstr, sizeof(ipstr)));
		tcp_eplilog(checker, 0);
	}

	return;
}

void
tcp_connect_thread(evutil_socket_t, short, void* arg)
{
	checker_t *checker = (checker_t *)arg;
	struct sockaddr_storage *co = checker->co;
	int fd;
	enum connect_result status;

	if ((fd = socket(co->ss_family, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		syslog(LOG_INFO, "TCP connect fail to create socket. Rescheduling.");
		thread_add_timer(checker->base, tcp_connect_thread, checker,
				g_config.delay_loop);

		return;
	}

	status = tcp_bind_connect(fd, co);

	/* handle tcp connection status & register check worker thread */
	if(tcp_connection_state(fd, status, checker, tcp_check_thread,
			g_config.connection_to)) {
		close(fd);
		syslog(LOG_INFO, "TCP socket bind failed. Rescheduling.");
		thread_add_timer(checker->base, tcp_connect_thread, checker,
				g_config.delay_loop);
	}

	return;
}
