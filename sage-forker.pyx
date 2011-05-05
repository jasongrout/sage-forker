import os
pipename = os.getenv("DOT_SAGE") + "forker"

cdef extern from 'fcntl.h':
    int unix_open "open"(char *pathname, int flags)
    int O_RDONLY
    int O_WRONLY
    int O_RDWR

cdef extern from 'unistd.h':
    int dup2(int oldfd, int newfd)
    int close(int fd)

cdef extern from 'stdlib.h':
    void exit(int status)

cdef extern from 'signal.h':
    int kill(int pid, int sig)
    int SIGHUP

cdef extern from 'sys/wait.h':
    int waitpid(int pid, int* status, int options)
    int WIFEXITED(int status)
    int WEXITSTATUS(int status)
    int WIFSIGNALED(int status)
    int WTERMSIG(int status)

cdef void setup_child(char* tty, int pid):
    # Open tty and copy the file descriptor to
    # stdin, stdout and stderr
    cdef int ttyfd = unix_open(tty, O_RDWR)
    # In case of failure, print a useful message and exit
    if ttyfd == -1:
        printf("Failed to open \"%s\"\n", tty);
        exit(0)
    dup2(ttyfd, 0)
    dup2(ttyfd, 1)
    dup2(ttyfd, 2)
    close(ttyfd)
    # Send a SIGHUP to pid to signal that the setup is done.
    # This also allow pid to take a note of *our* PID.
    kill(pid, SIGHUP)
    

from sage.misc.banner import banner
import sys
def sage_forker():
    # Create a pipe
    try:
        os.unlink(pipename)
    except OSError:
        pass
    os.mkfifo(pipename, 0600)

    cdef int childpid
    cdef int status
    while True:
        # Open the pipe and read a PID on the first line
        # and a tty filename (without \n) on the second.
        p = open(pipename, "r")
        pid = int(p.readline())
        tty = p.readline()
        p.close()

        # We create three child processes in a chain.
        # 3) The third child process is the actual Sage process which
        #    will be controlled by the tty
        # 2) The second waits for the third to exit, then sends a
        #    SIGHUP to the process controlling the tty.
        # 1) The first process simply exists and is wait()ed for by the
        #    parent process.
        # 0) The parent process wait()s for the first child to exit.
        #    After this is it no longer the parent of any child processes.
        #    We need this because otherwise we would have to wait() for
        #    our children.
        childpid = os.fork()
        if childpid == 0:
            # FIRST CHILD
            childpid = os.fork()
            if childpid == 0:
                # SECOND CHILD
                childpid = os.fork()
                if childpid == 0:
                    # THIRD CHILD
                    setup_child(tty, pid)
                    banner()
                    return
                # SECOND CHILD
                waitpid(childpid, &status, 0)
                if WIFSIGNALED(status):
                    kill(pid, WTERMSIG(status))
                else:
                    kill(pid, SIGHUP)
            exit(0)
        # Wait for the first child to exit
        waitpid(childpid, &status, 0)
        print "Forked to \"%s\" controlled by PID %d"%(tty,pid)
