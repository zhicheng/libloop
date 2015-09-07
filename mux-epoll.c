#include "loop.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>

struct mux {
	int fd;
};

int
mux_open(loop_t *loop, int event_max)
{
	loop->mux = malloc(sizeof(struct mux));
	if (loop->mux == NULL) {
		goto err;
	}

	loop->mux->fd = epoll_create(event_max);
	if (loop->mux->fd == -1) {
		goto err;
	}

	return LOOP_OK;
err:
	free(loop->mux);

	return LOOP_ERR;
}

void
mux_close(loop_t *loop)
{
	free(loop->mux);
}

int
mux_set_event(loop_t *loop, int fd, int tag, int type)
{
	uint32_t events;
	struct epoll_event event;

	events = 0;
	memset(&event, 0, sizeof(event));
	if (type & LOOP_WRITE) {
		events |= EPOLLOUT;
	}

	if (type & LOOP_READ) {
		events |= EPOLLIN;
	}

	event.events  = events;
	event.data.fd = fd;
	if (loop->event[fd].type == 0) {
		if (epoll_ctl(loop->mux->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
			return LOOP_ERR;
		}
	} else {
		if (epoll_ctl(loop->mux->fd, EPOLL_CTL_MOD, fd, &event) == -1) {
			return LOOP_ERR;
		}
	}

	return LOOP_OK;
}

int
mux_del_event(loop_t *loop, int fd, int type)
{
	uint32_t events;
	struct epoll_event event;

	if ((type & LOOP_WRITE) && (type & LOOP_READ)) {
		if (epoll_ctl(loop->mux->fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
			return LOOP_ERR;
		}

		return LOOP_OK;
	}

	events = 0;
	if (type & LOOP_WRITE) {
		events |= EPOLLOUT;
	}

	if (type & LOOP_READ) {
		events |= EPOLLIN;
	}

	event.events  = events;
	event.data.fd = fd;
	if (epoll_ctl(loop->mux->fd, EPOLL_CTL_MOD, fd, &event) == -1) {
		return LOOP_ERR;
	}

	return LOOP_OK;
}

#define EVENTS_NR	64

int
mux_polling(loop_t *loop, void *timer)
{
	int i;
	int n;
	int fd;
	int timeout;

	struct epoll_event events[EVENTS_NR];

	if (timer == NULL) {
		timeout = -1;
	} else {
		timeout  = ((timer_t *)timer)->seconds * USEC_PER_SEC;
		timeout += ((timer_t *)timer)->seconds / NSEC_PER_USEC;
	}

	n = epoll_wait(loop->mux->fd, events, EVENTS_NR, timeout);
	if (n == -1) {
                if (errno == EINTR) {
                        n = 0;
                } else {
                        return LOOP_ERR;
                }
	}
	loop_timer_dispatch(loop);

	for (i = 0; i < n; i++) {
		int type;
		struct event *event;
		loop_proc_t  *proc;

		fd    = events[i].data.fd;
		event = &loop->event[fd];

		if ((proc = event->proc) == NULL) {
			continue;
		}

		type = 0;
		if (events[i].events & EPOLLERR) {
			type = LOOP_ERROR;
		} else {
			if (events[i].events & EPOLLIN) {
				type |= LOOP_READ;
			}
			if (events[i].events & EPOLLOUT) {
				type |= LOOP_WRITE;
			}
		}

		proc(loop, fd, event->tag, type, event->flag, event->args);
	}
	return LOOP_OK;
}
