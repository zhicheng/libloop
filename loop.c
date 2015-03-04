#include "loop.h"

#include "heapq.h"
#include "hashtable.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define USEC_PER_SEC	1000000
#define NSEC_PER_SEC	1000000000
#define NSEC_PER_USEC	1000

#include "mux.h"

struct event {
        int id;
        int tag;
        int type;
        int flag;

        void *args;
        loop_proc_t *proc;
};

struct timer {
        struct event event;

        long seconds;
        int  nanoseconds;
};

struct loop {
	int stop;

        int event_len;
        int event_max;
        struct event *event;

        int timer_len;
        int timer_max;
        struct timer *timer;

        void **timer_table;

        struct mux *mux;
};

static int
timer_hash(const void *a)
{
	const struct timer *timer = a;
	return (timer->event.id + timer->event.tag + timer->event.type);
}

static int
timer_compare(const void *a, const void *b)
{
	const struct timer *timera = a;
	const struct timer *timerb = b;

	if (timera->event.id   == timerb->event.id  &&
	    timera->event.tag  == timerb->event.tag &&
	    timera->event.type == timerb->event.type)
	{
		return 0;
	}

	if (timera->seconds == timerb->seconds) {
		if (timera->nanoseconds > timerb->nanoseconds) {
			return 1;
		}
		if (timera->nanoseconds < timerb->nanoseconds) {
			return -1;
		}
		return 1;
	}
	if (timera->seconds > timerb->seconds) {
		return 1;
	}
	if (timera->seconds < timerb->seconds) {
		return -1;
	}

	return 1;
}

int
timer_swap(void *args, void *a, void *b)
{
	struct timer timer;

	struct timer *timera = a;
	struct timer *timerb = b;

	struct loop *loop = args;

	timer = *timera;
	*timera = *timerb;
	*timerb = timer;

	hashtable_set(loop->timer_table, loop->timer_max * 2, timera,
		timer_hash, timer_compare);

	hashtable_set(loop->timer_table, loop->timer_max * 2, timerb,
		timer_hash, timer_compare);

	return 0;
}

int
loop_push_timer(struct loop *loop, struct timer *timer)
{
	if (loop->timer_len == loop->timer_max) {
		return LOOP_ERR;
	}

	loop->timer_len += 1;
	heapq_push(loop->timer, loop->timer_len, sizeof(*timer), timer,
	           loop, timer_swap, timer_compare);

	hashtable_set(loop->timer_table, loop->timer_max * 2, timer,
		timer_hash, timer_compare);

        return LOOP_OK;
}

int
loop_pop_timer(struct loop *loop, struct timer *timer)
{
	while (loop->timer_len) {
		heapq_pop(loop->timer, loop->timer_len--, sizeof(*timer), timer,
		           loop, timer_swap, timer_compare);
		if (!(timer->event.flag & LOOP_FLAGS_INVALID)) {
			return LOOP_OK;
		}
	}

        return LOOP_ERR;
}

int
loop_set_timer(struct loop *loop, int id, int tag, int type, int flag,
	long sec, int nsec, loop_proc_t *proc, void *args)
{
	if (loop->timer_len == loop->timer_max) {
		return LOOP_ERR;
	}

	struct timeval now;
	struct timer *timer;

	gettimeofday(&now, NULL);

	timer = &loop->timer[loop->timer_len];
	timer->event.id   = id;
	timer->event.tag  = tag;
	timer->event.type = type;
	timer->event.flag = flag;
	timer->event.proc = proc;
	timer->event.args = args;
	timer->seconds     = now.tv_sec + sec;
	timer->nanoseconds = now.tv_usec * NSEC_PER_USEC + nsec;

	if (timer->nanoseconds >= NSEC_PER_SEC) {
		timer->nanoseconds -= NSEC_PER_SEC;
		timer->seconds++;
	}

	return loop_push_timer(loop, timer);
}

int
loop_del_timer(struct loop *loop, int id, int tag, int type)
{
	struct timer timer;
	struct timer *p;

	timer.event.id   = id;
	timer.event.tag  = tag;
	timer.event.type = type;

	p = hashtable_get(loop->timer_table, loop->timer_max * 2, &timer,
		timer_hash, timer_compare);

	if (p == NULL) {
		return LOOP_ERR;
	}

	p->event.flag |= LOOP_FLAGS_INVALID;

	return LOOP_OK;
}

int
loop_timer_nearest(struct loop *loop, struct timer *timer)
{
	if (loop->timer_len > 0) {
		*timer = loop->timer[0];

		return LOOP_OK;
	}

	return LOOP_ERR;
}

int
loop_timer_dispatch(struct loop *loop)
{
	int i;
	struct timer timer;
	struct timeval now;

	for (i = 0;;i++) {
		if (loop_pop_timer(loop, &timer) != LOOP_OK) {
			break;
		}

		gettimeofday(&now, NULL);
		if (now.tv_sec > timer.seconds ||
		    (now.tv_sec == timer.seconds &&
		     now.tv_usec >= timer.nanoseconds / NSEC_PER_USEC))
		{
			timer.event.proc(loop,
					 timer.event.id,
					 timer.event.tag,
					 timer.event.type,
					 timer.event.flag,
					 timer.event.args);
		} else {
			loop_push_timer(loop, &timer);

			break;
		}
	}

	return i;
}

struct loop *
loop_open(int event_max, int timer_max)
{
        struct loop *loop = NULL;

        if (event_max < 0 || timer_max < 0) {
		return NULL;
        }

        loop = malloc(sizeof(struct loop));
        if (loop == NULL) {
		return NULL;
	}

        memset(loop, 0, sizeof(struct loop));

        loop->event = calloc(event_max, sizeof(struct event));
        loop->event_max = event_max;
        if (loop->event == NULL) {
                goto err;
	}

        loop->timer = calloc(timer_max, sizeof(struct timer));
        loop->timer_max = timer_max;
        if (loop->timer == NULL) {
                goto err;
	}

        loop->timer_table = calloc(timer_max * 2, sizeof(void *));
        if (loop->timer_table == NULL) {
                goto err;
	}

	if (mux_open(loop, event_max) != LOOP_OK) {
                goto err;
	}

        return loop;
err:
        free(loop->event);
        free(loop->timer);
        free(loop->timer_table);
	mux_close(loop);
        free(loop);

        return NULL;
}

void
loop_close(struct loop *loop)
{
        free(loop->event);
        free(loop->timer);
        free(loop->timer_table);
	mux_close(loop);
        free(loop);
}

int
loop_set_event(struct loop *loop, int fd, int tag, int type, int flag,
        loop_proc_t *proc, void *args)
{
        if (fd >= loop->event_max) {
                return LOOP_ERR;
        }

        if ((type & LOOP_WRITE) == 0 && (type & LOOP_READ) == 0) {
                return LOOP_ERR;
        }

        loop->event[fd].id   = fd;
        loop->event[fd].tag  = tag;
        loop->event[fd].flag = flag;
        loop->event[fd].args = args;
        loop->event[fd].proc = proc;

	if (mux_set_event(loop, fd, tag, type) != LOOP_OK) {
		return LOOP_ERR;
	}
        loop->event[fd].type |= type;

	return LOOP_OK;
}

int
loop_del_event(struct loop *loop, int fd, int type)
{
        if ((type & LOOP_WRITE) == 0 && (type & LOOP_READ) == 0) {
                return LOOP_ERR;
        }

	if (mux_del_event(loop, fd, type) != LOOP_OK) {
		return LOOP_ERR;
	}

	loop->event[fd].type &= ~type;

	return LOOP_OK;
}

int
loop_start(struct loop *loop)
{
	struct timer *timeout;
        struct timer timer;
        struct timeval now;

	loop->stop = 0;
        for (;!loop->stop;) {
		timeout = NULL;
                if (loop_timer_nearest(loop, &timer) == LOOP_OK) {
                        gettimeofday(&now, NULL);

			if (now.tv_sec > timer.seconds ||
			    (now.tv_sec == timer.seconds &&
			     now.tv_usec >= timer.nanoseconds / NSEC_PER_USEC))
			{
                		loop_timer_dispatch(loop);

				continue;
			}
			timer.nanoseconds -= now.tv_usec * NSEC_PER_USEC;
			if (timer.nanoseconds < 0) {
				timer.nanoseconds += NSEC_PER_SEC;
				timer.seconds--;
			}
			if (timer.seconds > now.tv_sec) {
				timer.seconds -= now.tv_sec;
			} else {
				timer.seconds  = 0;
			}
			timeout = &timer;
                }
		if (mux_polling(loop, timeout) != LOOP_OK) {
			printf("err\n");
			return LOOP_ERR;
		}
        }

	/* loop stoped */
        return LOOP_OK;
}

int
loop_stop(struct loop *loop)
{
	loop->stop = 1;

	return LOOP_OK;
}

#if defined(USE_KQUEUE)
#include "mux-kqueue.c"
#elif defined(USE_EPOLL)
#include "mux-epoll.c"
#else
#include "mux-select.c"
#endif
