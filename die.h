#ifndef __DIE_H__
#define __DIE_H__

#include <stdlib.h>

#define die(fmt, ...) do {                                                    \
	fprintf(stderr, "%s:%d "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
	exit(0);                                                              \
} while(0)

#define die_errno(s) do {                       \
        die("%s: %s", (s), strerror(errno));    \
} while(0)

#define die_unless(a) do {                      \
	if (!(a)) {                             \
		die("%s", #a);                  \
	}                                       \
} while (0)

#define die_errno_unless(a) do {                \
	if (!(a)) {                             \
		die_errno(#a);                  \
	}                                       \
} while (0)

#endif /* __DIE_H__ */
