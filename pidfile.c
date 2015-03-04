#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

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
		return -1;
	}

	flags = fcntl(fd, F_GETFD);
	if (flags == -1) {
		return -1;
	}

	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1) {
		return -1;
	}

	/* lock file */
	lock.l_len    = 0;
	lock.l_start  = 0;
	lock.l_type   = F_WRLCK;
	lock.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK, &lock) == -1) {
		return -1;
	}

	if (ftruncate(fd, 0) == -1) {
		return -1;
	}

	pidlen = snprintf(pid, sizeof(pid), "%ld\n", (long)getpid());
	if (write(fd, pid, pidlen) != pidlen) {
		return -1;
	}

	return fd;
}
