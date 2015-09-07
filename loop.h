#ifndef __LOOP_H__
#define __LOOP_H__

enum {
	LOOP_OK  =  0,
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

typedef struct loop loop_t;

typedef void loop_proc_t(loop_t *loop, int id, int tag, int type, int flag, void *args); 

loop_t *
loop_open(int event_max, int timer_max);

void
loop_close(loop_t *loop);

int
loop_set_event(loop_t *loop, int fd, int tag, int type, int flag,
	loop_proc_t *proc, void *args);

int
loop_del_event(loop_t *loop, int fd, int type);

int
loop_set_timer(loop_t *loop, int id, int tag, int type, int flag,
        long seconds, int nanoseconds, loop_proc_t *proc, void *args);

int
loop_del_timer(loop_t *loop, int id, int tag, int type);

int
loop_start(loop_t *loop);

int
loop_stop(loop_t *loop);

#endif /* __LOOP_H__ */
