#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include "daemonize.h"

void daemonize(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        syslog(LOG_ERR, "fork failed, cannot daemonize");
        exit(1);
    }

    if (pid > 0) {
        // Parent process: nothing left to do, exit immediately so the
        // shell prompt returns right away.
        exit(0);
    }

    // ----- Only the child reaches this point -----

    if (setsid() < 0) {
        exit(1);
    }

    if (chdir("/") < 0) {
        exit(1);
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
}
