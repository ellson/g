/* vim:set shiftwidth=4 ts=8 expandtab: */

/*************************************************************************
 * Copyright (c) 2017 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: John Ellson <john.ellson@gmail.com>
 *************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <assert.h>

// include local configuration
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

#include "thread.h"
#include "process.h"
#include "info.h"

#define BUF_SIZE 2048

// Runtime values  (sec.ns)
#define Ar(attr,valu_s,valu_ns) { \
     append_string  (THREAD, &pos, attr); \
     append_token   (THREAD, &pos, '='); \
     append_runtime (THREAD, &pos, valu_s, valu_ns); \
}
// String values
#define As(attr,valu) { \
     append_string  (THREAD, &pos, attr); \
     append_token   (THREAD, &pos, '='); \
     append_string  (THREAD, &pos, valu); \
}
// Integer values
#define Au(attr,valu) { \
     append_string  (THREAD, &pos, attr); \
     append_token   (THREAD, &pos, '='); \
     append_ulong   (THREAD, &pos, valu); \
}

/**
 * The info is formatted into a buffer using minimal
 * spacing g format
 * e.g.      "process[progname=g username=ellson hostname=work .... ]
 *
 * @param THREAD context
 * @return formatted string result
 */
void info_process(THREAD_t * THREAD)
{
    PROCESS_t *PROCESS = THREAD->PROCESS;

    char buf[BUF_SIZE];
    char *pos = &buf[0];

    // write in canonical g format
    // - minimal spacing - one SUBJECT per line
    // - alpha sorted nodes and attributes

    // FIXME - add:
    //    - prog basename ?   ... TMI ?
    //    - prog packagename

    // FIXME - Audit for security
    //          - don't provide too much info
 
    THREAD->sep = '\0';
    append_string  (THREAD, &pos, "process");
    append_token   (THREAD, &pos, '[');
    Au("elemmallocsize",        LISTALLOCNUM * sizeof(elem_t));
    Au("elempermalloc",         LISTALLOCNUM);
    Au("elemsize",              sizeof(elem_t));
    As("hostname",              PROCESS->hostname);
    Au("inbufcapacity",         INBUFSIZE);
    Au("inbufmallocsize",       INBUFALLOCNUM * sizeof(inbufelem_t));
    Au("inbufpermalloc",        INBUFALLOCNUM);
    Au("inbufsize",             sizeof(inbufelem_t));
    As("osmachine",             PROCESS->osmachine);
    As("osname",                PROCESS->osname);
    As("osrelease",             PROCESS->osrelease);
    Au("pid",                   PROCESS->pid);
    As("progname",              PROCESS->progname);
    Au("starttime",             PROCESS->starttime);
    Au("uptime",                PROCESS->uptime);
    As("username",              PROCESS->username);
    As("version",               PACKAGE_VERSION);
    Au("voidptrsize",           sizeof(void*));
    append_token   (THREAD, &pos, ']');
    assert(pos < buf+BUF_SIZE);

    fprintf(stdout,"%s\n", buf);
}


/**
 * format running thread stats as a string
 *
 * @param THREAD context
 * @return formatted string
 */
void info_thread(THREAD_t * THREAD)
{
    PROCESS_t *PROCESS = THREAD->PROCESS;
    uint64_t runtime, lend, itot, etot, istr;
    char percent[6];
    char buf[BUF_SIZE];
    char *pos = &buf[0];
    
#ifdef HAVE_CLOCK_GETTIME
    // Y2038-unsafe struct - but should be ok for uptime
    // ref: https://sourceware.org/glibc/wiki/Y2038ProofnessDesign
    static struct timespec uptime;
    static struct timespec nowtime;
    if (clock_gettime(CLOCK_BOOTTIME, &uptime))
        FATAL("clock_gettime()");
    if (clock_gettime(CLOCK_REALTIME, &nowtime))
        FATAL("clock_gettime()");
    runtime = ((uint64_t)uptime.tv_sec * TEN9 + (uint64_t)uptime.tv_nsec)
            - (PROCESS->uptime * TEN9 + PROCESS->uptime_nsec);
#else
    // Y2038-unsafe struct - but should be ok for uptime
    // ref: https://sourceware.org/glibc/wiki/Y2038ProofnessDesign
    static struct timeval nowtime;
    if (gettimeofday(&nowtime, NULL))
        FATAL("gettimeofday()");
    runtime = ((uint64_t)nowtime.tv_sec * TEN9 + (uint64_t)nowtime.tv_usec) * TEN3
            - (PROCESS->uptime * TEN9 + PROCESS->uptime_nsec);
#endif

    itot = PROCESS->PROC_INBUF.stat_inbufmalloc * INBUFALLOCNUM * sizeof(inbufelem_t);
    etot = PROCESS->PROC_LIST.stat_elemmalloc * LISTALLOCNUM * sizeof(elem_t);
    lend = (IO()->stat_lfcount ? IO()->stat_lfcount : IO()->stat_crcount);
    istr = TOKEN()->stat_instringshort + TOKEN()->stat_instringlong; 
    sprintf(percent,"%lu%%", (long)TOKEN()->stat_instringshort * 100 / istr);
  
    // write in canonical g format
    // - minimal spacing - one SUBJECT per line
    // - alpha sorted nodes and attributes

    THREAD->sep = '\0';
    append_string  (THREAD, &pos, "thread");
    append_token   (THREAD, &pos, '[');
    Au("charpersecond",         IO()->stat_incharcount+TEN9/runtime);
    Au("containdepth",          THREAD->stat_containdepth);
    Au("containdepthmax",       THREAD->stat_containdepthmax);
    Au("elemmalloccount",       PROCESS->PROC_LIST.stat_elemmalloc);
    Au("elemmalloctotal",       etot);
    Au("elemmax",               PROCESS->PROC_LIST.stat_elemmax);
    Au("elemnow",               PROCESS->PROC_LIST.stat_elemnow);
    Au("fragmax",               PROCESS->PROC_LIST.stat_fragmax);
    Au("fragnow",               PROCESS->PROC_LIST.stat_fragnow);
    Au("inactcount",            THREAD->stat_inactcount);
    Au("inactspersecond",       THREAD->stat_inactcount*TEN9/runtime);
    Au("inbufmalloccount",      PROCESS->PROC_INBUF.stat_inbufmalloc);
    Au("inbufmalloctotal",      itot);
    Au("inbufmax",              PROCESS->PROC_INBUF.stat_inbufmax);
    Au("inbufnow",              PROCESS->PROC_INBUF.stat_inbufnow);
    Au("incharcount",           IO()->stat_incharcount);
    Au("infilecount",           IO()->stat_infilecount);
    Au("infragcount",           TOKEN()->stat_infragcount);
    Au("inlinecount",           lend + 1);
    Au("instringcount",         istr);
    As("instringshort",         percent);
    Au("malloctotal",           itot+etot);
    Au("nowtime",               nowtime.tv_sec);
    Au("outactcount",           THREAD->stat_outactcount);
    Ar("runtime",               runtime/TEN9, runtime%TEN9);
    append_token   (THREAD, &pos, ']');
    assert(pos < buf+BUF_SIZE);

    fprintf(stdout,"%s\n", buf);
}

/**
 * format running container_stats as a string
 *
 * @param CONTAINER context
 * @return formatted string
 */
void info_container(CONTAINER_t * CONTAINER)
{
    THREAD_t *THREAD = CONTAINER->THREAD;

    char buf[BUF_SIZE];
    char *pos = &buf[0];
    
    THREAD->sep = '\0';
    append_string  (THREAD, &pos, "container");
    append_token   (THREAD, &pos, '[');
    Au("containercount",        CONTAINER->stat_containercount);
    Au("inactcount",            CONTAINER->stat_inactcount);
// FIXME - also need e.g.: dominactcount 
//         to gather the merged count
// FIXME - also need e.g.: threadoutactcount
//         to gain insight into act expansion
    Au("outactcount",           CONTAINER->stat_outactcount);
    Au("sameas",                CONTAINER->stat_sameas);
    append_token   (THREAD, &pos, ']');
    assert(pos < buf+BUF_SIZE);

    fprintf(stdout,"%s\n", buf);
}
