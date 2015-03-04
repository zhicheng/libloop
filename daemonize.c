#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/resource.h>

/*
 * ref:
 * http://0pointer.de/public/systemd-man/daemon.html
 */
int
daemonize(int nochdir, int noclose)
{
	int   sig;
	pid_t pid;
	sigset_t set;
	struct rlimit limit;

	if (!noclose) {
		/*
		 * Close all open file descriptors except STDIN, STDOUT, STDERR
		 * (i.e. the first three file descriptors 0, 1, 2).
		 * This ensures that no accidentally passed file descriptor stays around
		 * in the daemon process.
		 */
		if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
			return -1;
		}

		for(; limit.rlim_cur > 3; limit.rlim_cur--) {
			close(limit.rlim_cur - 1);
		}
	}

	/* Reset all signal handlers to their default. */
	for (sig = 1; sig <= NSIG; sig++) {
		signal(sig, SIG_DFL);
	}

	/* Reset the signal mask using sigprocmask(). */
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	/* Call fork(), to create a background process. */
	pid = fork();
	if (pid == -1) {
		return -1;
	} else if (pid != 0) {
		_exit(0);
	}

	/* Call setsid() to detach from any terminal and create an independent session. */
	if (setsid() == -1) {
		return -1;
	}

	/* Call fork() again, to ensure that the daemon can never re-acquire a terminal again. */
	signal(SIGHUP, SIG_IGN);
	pid = fork();
	if (pid == -1) {
		return -1;
	} else if (pid != 0) {
		_exit(0);
	}

	/*  connect /dev/null to STDIN, STDOUT, STDERR. */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	if (open("/dev/null", O_RDONLY) == -1) {
		return -1;
	}
	if (open("/dev/null", O_WRONLY) == -1) {
		return -1;
	}
	if (open("/dev/null", O_RDWR) == -1) {
		return -1;
	}

	/*
	 * reset the umask to 0, so that the file modes passed to
	 * open(), mkdir() and suchlike directly control the access mode
	 * of the created files and directories.
	 */
	umask(0);

	if (!nochdir) {
		/*
		 * change the current directory to the root directory (/),
		 * in order to avoid that the daemon involuntarily blocks
		 * mount points from being unmounted.
		 */
		if (chdir("/") == -1) {
			return -1;
		}
	}

	return 0;
}
