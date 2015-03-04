#include "loop.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/event.h>

struct mux {
	int fd;
};

int
mux_open(struct loop *loop, int event_max)
{
	loop->mux = malloc(sizeof(struct mux));
	if (loop->mux == NULL) {
		goto err;
	}

	loop->mux->fd = kqueue();
	if (loop->mux->fd == -1) {
		goto err;
	}

	return LOOP_OK;
err:
	free(loop->mux);

	return LOOP_ERROR;
}

void
mux_close(struct loop *loop)
{
	free(loop->mux);
}

int
mux_set_event(struct loop *loop, int fd, int tag, int type)
{
	struct kevent event;

	if (type & LOOP_WRITE) {
		EV_SET(&event, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
	}

	if (type & LOOP_READ) {
		EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	}

	if (kevent(loop->mux->fd, &event, 1, NULL, 0, NULL) == -1) {
		return LOOP_ERR;
	}

	return LOOP_OK;
}

int
mux_del_event(struct loop *loop, int fd, int type)
{
	struct kevent event;

	if (type & LOOP_WRITE) {
                EV_SET(&event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	}

	if (type & LOOP_READ) {
                EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	}

	if (kevent(loop->mux->fd, &event, 1, NULL, 0, NULL) == -1) {
		return LOOP_ERR;
	}

	return LOOP_OK;
}

#define EVENTS_NR	128

int
mux_polling(struct loop *loop, struct timer *timer)
{
	int i;
	int n;
	int fd;

	struct timespec *timeout;
	struct timespec  timespec;

	struct kevent events[EVENTS_NR];

	if (timer == NULL) {
		timeout = NULL;
	} else {
		timespec.tv_sec  = timer->seconds;
		timespec.tv_nsec = timer->nanoseconds;

		timeout = &timespec;
	}

	n = kevent(loop->mux->fd, NULL, 0, events, EVENTS_NR, timeout);
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

		fd    = events[i].ident;
		event = &loop->event[fd];

		if ((proc = event->proc) == NULL) {
			continue;
		}

		type = 0;
		if (events[i].flags & EV_EOF) {
			type = LOOP_EOF;
		} else {
			switch (events[i].filter) {
			case EVFILT_READ:
				type = LOOP_READ;
				break;
			case EVFILT_WRITE:
				type = LOOP_WRITE;
				break;
			default:
				continue;
			}
		}

		proc(loop, fd, event->tag, type, event->flag, event->args);
	}
	return LOOP_OK;
}
