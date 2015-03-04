#ifndef __MUX_H__
#define __MUX_H__

struct mux;
struct loop;
struct timer;

int
mux_open(struct loop *loop, int event_max);

void
mux_close(struct loop *loop);

int
mux_set_event(struct loop *loop, int fd, int tag, int type);

int
mux_del_event(struct loop *loop, int fd, int type);

int
mux_polling(struct loop *loop, struct timer *timer);

#endif /* __MUX_H__ */
