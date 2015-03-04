#ifndef __HEAPQ_H__
#define __HEAPQ_H__

#include <stddef.h>

void
heapq_push(void *base, size_t nel, size_t width, void *item, void *args,
	int (*swap)(void *, void *, void *),
        int (*compare)(const void *, const void *));

void
heapq_pop(void *base, size_t nel, size_t width, void *item, void *args,
	int (*swap)(void *, void *, void *),
        int (*compare)(const void *, const void *));

void
heapq_heapify(void *base, size_t nel, size_t width, void *args,
	int (*swap)(void *, void *, void *),
        int (*compare)(const void *, const void *));

#endif /* __HEAPQ_H__ */
