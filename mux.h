#ifndef __MUX_H__
#define __MUX_H__

#include "loop.h"

int
mux_open(loop_t *loop, int event_max);

void
mux_close(loop_t *loop);

int
mux_set_event(loop_t *loop, int fd, int tag, int type);

int
mux_del_event(loop_t *loop, int fd, int type);

int
mux_polling(loop_t *loop, void *timer);

#endif /* __MUX_H__ */
