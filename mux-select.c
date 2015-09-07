#include "mux.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

struct mux {
	int maxfd;
	fd_set readfds;
	fd_set writefds;
};

int
mux_open(loop_t *loop, int event_max)
{
	if (event_max > FD_SETSIZE) {
		goto err;
	}

        loop->mux = malloc(sizeof(struct mux));
        if (loop->mux == NULL) {
                goto err;
        }

        return LOOP_OK;
err:
        free(loop->mux);

        return LOOP_ERROR;
}

void
mux_close(loop_t *loop)
{
	free(loop->mux);
}

int
mux_set_event(loop_t *loop, int fd, int tag, int type)
{
	if (type & LOOP_WRITE) {
		FD_SET(fd, &loop->mux->writefds);
	}

	if (type & LOOP_READ) {
		FD_SET(fd, &loop->mux->readfds);
	}

	if (fd > loop->mux->maxfd) {
		loop->mux->maxfd = fd;
	}

	return LOOP_OK;
}

int
mux_del_event(loop_t *loop, int fd, int type)
{
	if (type & LOOP_WRITE) {
		FD_CLR(fd, &loop->mux->writefds);
	}

	if (type & LOOP_READ) {
		FD_CLR(fd, &loop->mux->readfds);
	}

	if (loop->event[fd].type == 0) {
		memset(&loop->event[fd], 0, sizeof(struct event));
		if (fd >= loop->mux->maxfd) { 
			while (FD_ISSET(fd, &loop->mux->readfds)  == 0 &&
			       FD_ISSET(fd, &loop->mux->writefds) == 0)
			{
				fd--;
			}
			loop->mux->maxfd = fd;
		}
	}

	return LOOP_OK;
}

int
mux_polling(loop_t *loop, void *timer)
{
	int n;
	int fd;

        struct timeval *timeout;
        struct timeval  timeval;

	fd_set readfds;
	fd_set writefds;

	if (timer == NULL) {
		timeout = NULL;
	} else {
                timeval.tv_sec  = ((timer_t *)timer)->seconds;
                timeval.tv_usec = ((timer_t *)timer)->nanoseconds / NSEC_PER_USEC;

                timeout = &timeval;
	}
	memcpy(&readfds,  &loop->mux->readfds,  sizeof(readfds));
	memcpy(&writefds, &loop->mux->writefds, sizeof(writefds));

	n = select(loop->mux->maxfd + 1, &readfds, &writefds, NULL, timeout);
	if (n == -1) {
		return LOOP_ERR;
	}

	loop_timer_dispatch(loop);

	for (fd = 0; fd <= loop->mux->maxfd && n > 0; fd++) {
                int type;
                struct event *event;
                loop_proc_t  *proc;

                event = &loop->event[fd];

                if ((proc = event->proc) == NULL) {
                        continue;
                }

		type = 0;
		if (FD_ISSET(fd, &writefds)) {
			n--;
			type |= LOOP_WRITE;
		}

		if (FD_ISSET(fd, &readfds)) {
			n--;
			type |= LOOP_READ;
		}
		if (type == 0) {
			continue;
		}
		proc(loop, fd, event->tag, type, event->flag, event->args);
	}
	return LOOP_OK;
}
