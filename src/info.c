#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <assert.h>

#include "libje_private.h"

// This code collects info from the environment to:
//          - populate session info into attributes of a 'g' NODE
//          - provide a readable unique session name for session-freeze tar file names
//          - capture start time for stats.

// This code is very likely Linux specific, and may need canoditionals for porting to other OS.

// The resulting info is collected into a buffer using minimal spacing g format
// e.g.      "session[progname=g username=ellson hostname=work .... ]

// There is an attribute pretty-printer function for when this is printed for the user.

// This info is gathered just once, so a statically sized buffer is used. It is only filled
// on the first call,  if session() is called again the same result is used.


#define SESSION_BUF_SIZE 1024

char * je_session(context_t *C)
{
    static char buf[SESSION_BUF_SIZE];
    static char *pos = &buf[0];  // NB. static. This initalization happens only once
    static struct passwd *pw;
    static struct utsname unamebuf;
	static struct timespec starttime;
    static uid_t uid;

    if (pos != &buf[0]) { // have we been here before?
        return buf;
    }

    je_append_string  (C, &pos, "session");
    je_append_token   (C, &pos, '[');

    je_append_string  (C, &pos, "progname");
    je_append_token   (C, &pos, '=');
    je_append_string  (C, &pos, C->progname);

    C->pid = getpid();
    je_append_string  (C, &pos, "pid");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->pid);

    uid = geteuid();
    pw = getpwuid(uid);
    if (!pw) {
        perror("Error - getpwuid(): ");
        exit(EXIT_FAILURE);
    }
    C->username = pw->pw_name;
    je_append_string  (C, &pos, "username");
    je_append_token   (C, &pos, '=');
    je_append_string  (C, &pos, C->username);

    if (uname(&unamebuf) != 0) {
        perror("Error - uname(): ");
        exit(EXIT_FAILURE);
    } 
    C->hostname = unamebuf.nodename;
    je_append_string  (C, &pos, "hostname");
    je_append_token   (C, &pos, '=');
    je_append_string  (C, &pos, C->hostname);

	if (clock_gettime(CLOCK_BOOTTIME, &(C->uptime)) != 0) {
        perror("Errror - clock_gettime(): ");
        exit(EXIT_FAILURE);
    }
    je_append_string  (C, &pos, "uptime");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->uptime.tv_sec);

	if (clock_gettime(CLOCK_REALTIME, &starttime) != 0) {
        perror("Errror - clock_gettime(): ");
        exit(EXIT_FAILURE);
    }
    je_append_string  (C, &pos, "starttime");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, starttime.tv_sec);

    je_append_token   (C, &pos, ']');

    assert(pos < buf+SESSION_BUF_SIZE);

    return buf;
}

#define STATS_BUF_SIZE 2048
#define TEN9 1000000000

char * je_stats(context_t *C)
{
    static char buf[STATS_BUF_SIZE];

    char *pos = &buf[0];  // NB non-static.  stats are updated and re-formatted on each call
	struct timespec nowtime;
	long runtime;    // runtime in nano-seconds

    je_append_string  (C, &pos, "stats");
    je_append_token   (C, &pos, '[');

	if (clock_gettime(CLOCK_BOOTTIME, &nowtime) != 0) {
        perror("Errror - clock_gettime(): ");
        exit(EXIT_FAILURE);
    }
	runtime = ((unsigned long)nowtime.tv_sec * TEN9 + (unsigned long)nowtime.tv_nsec)
            - ((unsigned long)(C->uptime.tv_sec) * TEN9 + (unsigned long)(C->uptime.tv_nsec));

    je_append_string  (C, &pos, "runtime");
    je_append_token   (C, &pos, '=');
    je_append_runtime (C, &pos, runtime/TEN9, runtime%TEN9);

    je_append_string  (C, &pos, "files");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_filecount);

    je_append_string  (C, &pos, "lines");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, 1 + (C->stat_lfcount ? C->stat_lfcount : C->stat_crcount));

    je_append_string  (C, &pos, "acts");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_actcount);

    je_append_string  (C, &pos, "acts_per_second");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_actcount*TEN9/runtime);

    je_append_string  (C, &pos, "sameas");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_sameas);

    je_append_string  (C, &pos, "patterns");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_patterncount);

    je_append_string  (C, &pos, "patternmatches");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_patternmatches);

    je_append_string  (C, &pos, "strings");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_stringcount);

    je_append_string  (C, &pos, "fragmentss");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_fragcount);

    je_append_string  (C, &pos, "inchars");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_inchars);

    je_append_string  (C, &pos, "chars_per_second");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_inchars + TEN9 / runtime);

    je_append_string  (C, &pos, "inbufsize");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, sizeof(inbuf_t));

    je_append_string  (C, &pos, "inbufmax");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_inbufmax);

    je_append_string  (C, &pos, "inbufnow");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_inbufnow);

    je_append_string  (C, &pos, "elemsize");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, size_elem_t);

    je_append_string  (C, &pos, "elemmax");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_elemmax);

    je_append_string  (C, &pos, "elemnow");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_elemnow);

    je_append_string  (C, &pos, "inbufmallocsize");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, INBUFALLOCNUM * sizeof(inbuf_t));

    je_append_string  (C, &pos, "inbufmalloccount");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_inbufmalloc);

    je_append_string  (C, &pos, "inbufmalloctotal");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_inbufmalloc * INBUFALLOCNUM * sizeof(inbuf_t));

    je_append_string  (C, &pos, "elemmallocsize");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, LISTALLOCNUM * size_elem_t);

    je_append_string  (C, &pos, "elemmalloccount");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_elemmalloc);

    je_append_string  (C, &pos, "elemmalloctotal");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, C->stat_elemmalloc * LISTALLOCNUM * size_elem_t);

    je_append_string  (C, &pos, "malloctotal");
    je_append_token   (C, &pos, '=');
    je_append_ulong   (C, &pos, (C->stat_elemmalloc * LISTALLOCNUM * size_elem_t)
                        + (C->stat_inbufmalloc * INBUFALLOCNUM * sizeof(inbuf_t)));

    je_append_token   (C, &pos, ']');

    assert(pos < buf+STATS_BUF_SIZE);

    return buf;
}