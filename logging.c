/* $Id$
 *
 * Logging and error handling for distribute+archive
 *
 * Shigeya Suzuki, Dec 1993, October 1997
 * Copyright(c)1993-1997 Shigeya Suzuki
 */

#include <stdio.h>
#include <sysexits.h>
#include <stdlib.h>

#ifdef __STDC__
#include <stdarg.h>
#endif

#include "cdefs.h"

#ifdef TEST
#undef SYSLOG
#endif

#ifdef SYSLOG
# include <syslog.h>
#endif

#define LOGBUFSIZE	1024

#include "logging.h"
#include "memory.h"

/* Global
 */
static int print_error = 0;
static char* logbuf = NULL;

/* Initialization
 */
void
init_log(tag)
    char* tag;
{
#ifdef SYSLOG	
    openlog(tag, LOG_PID, SYSLOG_FACILITY);
#endif
#ifdef DEBUGLOG
    char logfile[80];
    sprintf(logfile, "/tmp/%s.log", tag);
    debuglog = fopen(, "a");
    if (debuglog == NULL) {
	logandexit(EX_UNAVAILABLE, "can't open debug log", logfile);
    }
    fprintf(debuglog, "---\n");
    fprintf(debuglog, "invoked: pid=%d\n", getpid());
    fflush(debuglog);
#endif
    logbuf = xmalloc(LOGBUFSIZE);
}


/* Toggle print error mode
 */
void
logging_setprinterror(flag)
    int flag;
{
    print_error = flag;
}


/* Several error handler
 */

void
logerror_buf(prefix)
    char* prefix;
{
#ifdef SYSLOG
    syslog(LOG_ERR, logbuf);
#endif
    if (print_error) {
	if (prefix != NULL)
	    fputs(prefix, stderr);
	fputs(logbuf, stderr);
	fputc('\n', stderr);
    }
}


#ifdef __STDC__

void
logerror(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(logbuf, fmt, ap);	/* vsnprintf is available everywhere?? */
    va_end(ap);
    logerror_buf("Error: ");
}

#else

void
logerror(fmt, a1, a2)
    char* fmt;
    char* a1;
    char* a2;
{
    sprintf(logbuf, fmt, a1, a2);
    logerror_buf("Error: ");
}

#endif


#ifdef __STDC__

void
logwarn(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(logbuf, fmt, ap);	/* vsnprintf is available everywhere?? */
    va_end(ap);
    logerror_buf("Warning: ");
}

#else

void
logwarn(fmt, a1, a2)
    char* fmt;
    char* a1;
    char* a2;
{
    sprintf(logbuf, fmt, a1, a2);
    logerror_buf("Warning: ");
}

#endif


#ifdef __STDC__

void
loginfo(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(logbuf, fmt, ap);
    va_end(ap);
    logerror_buf("Info: ");
}

#else

void loginfo(fmt, a1, a2)
    char* fmt;
    char* a1;
    char* a2;
{
    sprintf(logbuf, fmt, a1, a2);
    logerror_buf("Info: ");
}

#endif


/* log it then exit with error code
 */

#ifdef __STDC__
void
logandexit(int exitcode, char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(logbuf, fmt, ap);	/* vsnprintf is available everywhere?? */
    va_end(ap);

    logerror_buf(NULL);

    exit(exitcode);
}

#else

void
logandexit(exitcode, fmt, a1, a2)
    int exitcode;
    char* fmt;
    char* a1;
    char* a2;
{
    sprintf(logbuf, fmt, a1, a2);
    logerror_buf(NULL);
    exit(exitcode);
}

#endif




/* Abort with program error
 */
void
programerror()
{
    logandexit(EX_UNAVAILABLE, "program error");
}

