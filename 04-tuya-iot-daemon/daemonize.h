#ifndef DAEMONIZE_H
#define DAEMONIZE_H

// Turns the calling process into a background daemon: forks (the parent
// exits from inside this function and never returns), detaches from the
// controlling terminal (setsid), changes working directory to "/", resets
// umask, and redirects stdin/stdout/stderr to /dev/null.
//
// Only the child process returns from this function - by the time it
// does, printf()/scanf() no longer go anywhere useful, so use syslog()
// for all logging from this point on.
void daemonize(void);

#endif
