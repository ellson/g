/* vim:set shiftwidth=4 ts=8 expandtab: */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYSINFO
#include <sys/sysinfo.h>
#else
#ifndef HAVE_CLOCK_GETTIME
#include <sys/time.h>
#endif
#endif

// private includes
#include "thread.h"
#include "session.h"
//
// public include
#include "libg_session.h"

void session(int *pargc, char *argv[], int optind)
{
    SESSION_t session;
    struct passwd *pw;
    struct utsname unamebuf;
    uid_t uid;
    pid_t pid;

#if defined(HAVE_CLOCK_GETTIME)
    // Y2038-unsafe struct - but should be ok for uptime
    // ref: https://sourceware.org/glibc/wiki/Y2038ProofnessDesign
    struct timespec uptime;   
    struct timespec starttime; 
    if (clock_gettime(CLOCK_BOOTTIME, &(uptime)))
        FATAL("clock_gettime()");
    session.uptime = uptime.tv_sec;
    session.uptime_nsec = uptime.tv_nsec;
    if (clock_gettime(CLOCK_REALTIME, &(starttime)))
        FATAL("clock_gettime()");
    session.starttime = starttime.tv_sec;
    session.starttime_nsec = starttime.tv_nsec;
#else
    // Y2038-unsafe struct - but should be ok for uptime
    // ref: https://sourceware.org/glibc/wiki/Y2038ProofnessDesign
    struct timeval uptime;   
    struct timeval starttime;   
    if (gettimeofday(&(uptime), NULL))
        FATAL("gettimeofday()");
    session.uptime = uptime.tv_sec;
    session.uptime_nsec = uptime.tv_usec * TEN3;
    if (gettimeofday(&starttime, NULL))
        FATAL("gettimeofday()");
    session.starttime = starttime.tv_sec;
    session.starttime_nsec = starttime.tv_usec * TEN3;
#endif

    pid = geteuid();
    session.pid = pid;
    uid = geteuid();
    if (!(pw = getpwuid(uid)))
        FATAL("getpwuid()");
    session.username = pw->pw_name;
    if (uname(&unamebuf))
        FATAL("uname()");
    session.hostname = unamebuf.nodename;
    session.osname = unamebuf.sysname;
    session.osrelease = unamebuf.release;
    session.osmachine = unamebuf.machine;

    // run a THREAD to process the input
    session.THREAD = thread(&session, pargc, argv, optind);
}
