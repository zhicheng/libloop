CC = cc

system_name := $(shell sh -c 'uname -s 2>/dev/null || echo not')


CFLAGS += -O2
CFLAGS += -Wall
CFLAGS += -Werror

LDFLAGS += -lssl
LDFLAGS += -lcrypto
LDFLAGS += -lpthread

SRC += buf.c
SRC += ssl.c
SRC += loop.c
SRC += heapq.c
SRC += socket.c
SRC += hashtable.c

OBJS = $(SRC:.c=.o)

ifeq ($(system_name),Linux)
	CFLAGS += -DUSE_EPOLL
	CFLAGS += -D__USE_GNU
	CFLAGS += -D_GNU_SOURCE
else ifeq ($(system_name),Darwin)
	CFLAGS += -DHAVE_SA_LEN
	CFLAGS += -DHAVE_SIN_LEN
	CFLAGS += -DUSE_KQUEUE
	CFLAGS += -D__APPLE_USE_RFC_2292
	CFLAGS += -D_GNU_SOURCE
	CFLAGS += -I/usr/local/opt/openssl/include	# Homebrew Version
	LDFLAGS += -L/usr/local/opt/openssl/lib		# Homebrew Version
else ifeq ($(system_name),FreeBSD)
	CFLAGS += -DUSE_KQUEUE
else
	CFLAGS += -DUSE_SELECT
endif

all: main

main: main.o $(OBJS) Makefile
	@$(CC) main.o $(OBJS) -o $@ $(LDFLAGS) 
	@echo CC main

loop.o: loop.c mux-epoll.c mux-kqueue.c mux-select.c Makefile
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

%.o: %.c Makefile
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	@rm main main.o $(OBJS)
	@echo clean main main.o $(OBJS)
