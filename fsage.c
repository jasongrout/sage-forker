/* Sage forker "client" */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>


/* PID of the Sage process that we are controlling. */
pid_t sage_pid = 0;

void hupaction(int signum, siginfo_t* info, void* context);
void hupaction2(int signum, siginfo_t* info, void* context);
void intaction(int signum, siginfo_t* info, void* context);

int main(int argc, char** argv)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));

	/* Signal handler for SIGHUP.  We will receive exactly
	 * one SIGHUP during startup from the Sage process. */
	sa.sa_sigaction = hupaction;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGHUP, &sa, NULL);

	/* Block SIGHUP */
	sigset_t defaultmask;
	sigset_t block;
	sigemptyset(&block);
	sigaddset(&block, SIGHUP);
	sigprocmask(SIG_BLOCK, &block, &defaultmask);

	/* Get filename of tty */
	const char* tty = ttyname(0);
	if (!tty) {perror("ttyname"); exit(3);}

	/* pipename = $HOME/.sage/forker */
	const char* home = getenv("HOME");
	if (!home) home = ".";
	char pipename[256];
	snprintf(pipename, sizeof(pipename), "%s/.sage/forker", home);

	/* Write our PID and tty to pipename */
	char buf[256];
	snprintf(buf, sizeof(buf), "%d\n%s", (int)getpid(), tty);
	int fd = open(pipename, O_WRONLY|O_NONBLOCK);
	if (fd < 0) {perror(pipename); exit(2);}
	write(fd, buf, strlen(buf));
	close(fd);

	/* Wait for 1 second for Sage to respond with a SIGHUP. */
	struct timespec tmout;
	tmout.tv_sec = 1;
	tmout.tv_nsec = 0;
	int r = pselect(0, NULL, NULL, NULL, &tmout, &defaultmask);
	sigprocmask(SIG_SETMASK, &defaultmask, NULL);

	if (!sage_pid)
	{
		printf("No answer from Sage, probably sage_forker() is not running.\n");
		exit(1);
	}

	/* Second handler for SIGHUP.  We will receive a SIGHUP when 
	 * Sage exits. */
	sa.sa_sigaction = hupaction2;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGHUP, &sa, NULL);

	/* Install signal handler for SIGINT.  We will forward any interrupts 
	 * to the Sage process. */
	sa.sa_sigaction = intaction;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &sa, NULL);

	/* Close stdin, stdout, stderr */
	close(0);
	close(1);
	close(2);

	/* If we exit, then the session will be closed,
	 * so we just hang out until Sage finishes. */
	for (;;)
	{
		/* Check every 5 seconds that Sage is still alive.  If it exits,
		 * we *should* receive a SIGHUP so the timeout is really only a
		 * backup. */
		tmout.tv_sec = 5;
		tmout.tv_nsec = 0;
		int r = pselect(0, NULL, NULL, NULL, &tmout, NULL);
		if (sage_pid)
		{
			/* Check whether sage_pid is still alive */
			if (kill(sage_pid, 0) == -1)
			{
				/* Sage quit, so we quit also */
				exit(0);
			}
		}
	}
}

void hupaction(int signum, siginfo_t* si, void* context)
{
	sage_pid = si->si_pid;
}

void hupaction2(int signum, siginfo_t* si, void* context)
{
	exit(0);
}

void intaction(int signum, siginfo_t* si, void* context)
{
	/* Forward interrupt to Sage */
	kill(sage_pid, signum);
}
