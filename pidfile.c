#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "die.h"

int
pidfile(const char *file)
{
	int  fd;
	int  flags;
	int  pidlen;
	char pid[32];
	struct flock lock;

	fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		die("pidfile: open %s %s", file, strerror(errno));
	}

	flags = fcntl(fd, F_GETFD);
	if (flags == -1) {
		die("pidfile: fcntl %s error", file);
	}

	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1) {
		die("pidfile: fcntl %s error", file);
	}

	/* lock file */
	lock.l_type   = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start  = 0;
	lock.l_len    = 0;
	if (fcntl(fd, F_SETLK, &lock) == -1) {
		if (errno  == EAGAIN || errno == EACCES) {
			die("pidfile: %s is locked", file);
		} else {
			die("pidfile: lock file %s fail", file);
		}
	}

	if (ftruncate(fd, 0) == -1) {
		die("pidfile: truncate %s error", file);
	}

	pidlen = snprintf(pid, sizeof(pid), "%ld\n", (long)getpid());
	if (write(fd, pid, pidlen) != pidlen) {
		die("pidfile: write %s error", file);
	}

	return fd;
}
