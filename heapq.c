#include "heapq.h"

#include <string.h>
#include <assert.h>

static void
shiftdown(void *base, int i, int j, int len, int width, void *args,
	int (*swap)(void *, void *, void *),
        int (*compare)(const void *, const void *))
{
	assert(i < len);
	assert(j < len);
	
        while (j > i) {
		int k = (j - 1) / 2;

		void *a = (char *)base + j * width;
		void *b = (char *)base + k * width;

                if (compare(a, b) < 0) {
			swap(args, a, b);
                } else {
			break;
		}
		j = k;
        }
}

static void
shiftup(int *base, int j, int len, int width, void *args,
	int (*swap)(void *, void *, void *),
	int (*compare)(const void *, const void *))
{
	int i = j;
	int k = j * 2 + 1;
	int l = len - 1;

	assert(j < len);

        while (k < l) {
                int r = k + 1;

		if ((r < l) && (compare((char *)base + k * width,
		                        (char *)base + r * width) >= 0))
		{
			k = r;
		}

		swap(args, (char *)base + j * width, (char *)base + k * width);

                j = k;
		k = j * 2 + 1;
        }
        shiftdown(base, i, j, len, width, args, swap, compare);
}

void
heapq_push(void *base, size_t nel, size_t width, void *item, void *args,
	int (*swap)(void *, void *, void *),
        int (*compare)(const void *, const void *))
{
	memmove((char *)base + (nel - 1) * width, item, width);

        shiftdown(base, 0, nel - 1, nel, width, args, swap, compare);
}

void
heapq_pop(void *base, size_t nel, size_t width, void *item, void *args,
	int (*swap)(void *, void *, void *),
        int (*compare)(const void *, const void *))
{

	memmove(item, base, width);
	memmove(base, (char *)base + (nel - 1) * width, width);

        shiftup(base, 0, nel, width, args, swap, compare);
}

void
heapq_heapify(void *base, size_t nel, size_t width, void *args,
	int (*swap)(void *, void *, void *),
        int (*compare)(const void *, const void *))
{
	int i;
	for (i = nel / 2 - 1; i >= 0; i--) {
		shiftup(base, i, nel, width, args, swap, compare);
	}
}
