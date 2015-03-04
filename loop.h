#ifndef __LOOP_H__
#define __LOOP_H__

enum {
	LOOP_OK  = 0,
	LOOP_ERR = -1
};

enum {
	LOOP_EOF    = 1 << 0,
	LOOP_READ   = 1 << 1,
	LOOP_WRITE  = 1 << 2,
	LOOP_ERROR  = 1 << 3,
};

enum {
	LOOP_FLAGS_REPEAT    = 1 << 0,
	LOOP_FLAGS_ABSOLUTE  = 1 << 1,
	LOOP_FLAGS_INVALID   = 1 << 2,
};

struct loop;

typedef void loop_proc_t(struct loop *loop, int id, int tag, int type, int flag, void *args); 

struct loop *
loop_open(int event_max, int timer_max);

void
loop_close(struct loop *loop);

int
loop_set_event(struct loop *loop, int fd, int tag, int type, int flag,
	loop_proc_t *proc, void *arg);

int
loop_del_event(struct loop *loop, int fd, int type);

int
loop_set_timer(struct loop *loop, int id, int tag, int type, int flag,
        long seconds, int nanoseconds, loop_proc_t *proc, void *arg);

int
loop_del_timer(struct loop *loop, int id, int tag, int type);

int
loop_start(struct loop *loop);

int
loop_stop(struct loop *loop);

#endif /* __LOOP_H__ */
