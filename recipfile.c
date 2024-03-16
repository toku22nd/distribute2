/* $Id$
 *
 * parse recipient file, and construct recipient address buffer
 *
 * by Shigeya Suzuki, April 1993
 * Copyright(c)1993, Shigeya Suzuki
 *
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef STDC_HEADERS
# include <stdio.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
# endif
# ifndef HAVE_STRRCHR
#  define strrchr rindex
# endif
#endif
#include <sysexits.h>
#include <errno.h>
  
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif

#include "longstr.h"
#include "memory.h"

static char *paren_err = "paren mismatch\n";
static char *quote_err = "quote mismatch\n";
static char *bracket_err = "angle bracket mismatch\n";
static char *memory_err = "insufficient memory\n";

extern void logandexit __P((int, char *, ...));
extern void logwarn __P((char *, ...));


/* normalizeaddr -- check one address and normalize it if necessary.
 */

char *
normalizeaddr(buf)
    char *buf;
{
    char *addr;
    char *hp;
    char *tp;
    char *cp;
    int addrfound = 0;
    int inp = 0;
    int inq = 0;

    if (strlen(buf) >= MAXADDRLEN) {
	logwarn("too large buf\n");
	return NULL;
    }

    addr = hp = malloc(sizeof(char) * MAXADDRLEN);

    if (hp == NULL) {
	logandexit(EX_UNAVAILABLE, memory_err);
    }

    *hp = '\0';

    for (tp = buf; *tp && ! addrfound; tp++) {
	if (! inp) {
	    if (! isblank(*tp) || inq) {
		*addr++ = *tp;
	 	*addr = '\0';
	    }
	}
	switch (*tp) {
	    case '\\':
		if (tp[1] != '\0') {
		    tp++;	/* ignore */
		}
		break;
	    case '"':	/* in quote */
		if (inq) {
		   inq = 0;
		} else if (! inp) {
		    inq = 1;
		}
		break;
	    case '(':	/* in paren */
		if (! inq) {
		    inp = 1;
		    if (addr != hp) {
			addr--;
			*addr = '\0';
		    }
		}
		break;
	    case ')':
		if (! inq) {
		    if (! inp) {
			logwarn(paren_err);
			free(hp);
			return NULL;
		    }
		    inp = 0;
		    tp++;
		    while (isblank(*tp)) {
			tp++;
		    }
		    tp--;
		} 
	
		break;
	    case '<':
		if (! inp && ! inq) {
		    cp = index(tp, '>');
	    	    if (cp != NULL) {
			tp++;
			strncpy(hp, tp, cp - tp);
			hp[cp - tp] = '\0';
			addrfound = 1;
		    } else {
			logwarn(bracket_err);
			free(hp);
			return NULL;
		    }
		}
		break;
	    case '>':
		if (! inq && ! inp) {
		    logwarn(bracket_err);
		    free(hp);
		    return NULL;
		}
		break;
	    default:
		break;
	}
    }
    if (inq) {
	logwarn(quote_err);
	free(hp);
	return NULL;
    }
    if (inp) {
	logwarn(paren_err);
	free(hp);
	return NULL;
    }
    if (strlen(hp) == 0) {
	logwarn("no From: address\n");
	free(hp);
	return NULL;
    }
    return hp;
}

/* fakefromaddr -- rewrite from address to list address
 */

char *
fakefromaddr(buf, list, host)
    char *buf;
    char *list;
    char *host;
{
    char *addr;
    char *hp;
    char *tp;
    char *bp;
    char *cp;
    int addrfound = 0;
    int inp = 0;
    int inq = 0;
    int addrlen;

    hp = malloc(sizeof(char) * MAXADDRLEN);

    if (hp == NULL) {
	logandexit(EX_UNAVAILABLE, memory_err);
    }

    bp = NULL;

    if (strlen(buf) >= MAXADDRLEN) {
	logwarn("too large buf\n");
	goto error;
    }

    addr = normalizeaddr(buf);
    if (addr == NULL) {
	goto error;
    }

    for (tp = buf; *tp && ! addrfound; tp++) {
	if (! inp) {
	    if ((! isblank(*tp) && *tp != '(') || inq) {
		if (bp == NULL) {
		    bp = tp;	/* address head */
		}
	    }
	}
	switch (*tp) {
	    case '\\':
		if (tp[1] != '\0') {
		    tp++;	/* ignore */
		}
		break;
	    case '"':	/* in quote */
		if (inq) {
		   inq = 0;
		} else if (! inp) {
		    inq = 1;
		}
		break;
	    case '(':	/* in paren */
		if (! inq) {
		    inp = 1;
		}
		break;
	    case ')':
		if (! inq) {
		    if (! inp) {
			logwarn(paren_err);
			free(addr);
			goto error;
		    }
		    inp = 0;
		    tp++;
		    while (isblank(*tp)) {
			tp++;
		    }
		    tp--;
		}
		break;
	    case '<':
		if (! inp && ! inq) {
		    tp++;
		    bp = tp;
		    cp = index(tp, '>');
	    	    if (cp != NULL) {
			strncpy(hp, tp, cp - tp);
			hp[cp - tp] = '\0';
			addrfound = 1;
		    } else {
		    	logwarn(bracket_err);
			free(addr);
			goto error;
		    }
		}
		break;
	    case '>':
		if (! inq && ! inp) {
		    logwarn(bracket_err);
		    free(addr);
		    goto error;
		}
		break;
	    default:
		break;
	}
    }
    if (inq) {
	logwarn(quote_err);
	free(addr);
	goto error;
    }
    if (inp) {
	logwarn(paren_err);
	free(addr);
	goto error;
    }

    addrlen = strlen(addr);

    strncpy(hp, buf, bp - buf);
    hp[bp - buf] = '\0';
    strcat(hp, list);
    strcat(hp, "@");
    strcat(hp, host);
    strcat(hp, bp + addrlen);

    free(addr);

    return hp;

error:
    sprintf(hp, "No Name <%s@%s>", list, host);
    return hp;
}


#ifndef TEST
char *
parserecipfile(filename, errormode)
    char *filename;
    int errormode;
{
    FILE *recipf;
    char buf[1024];
    struct longstr recipbuf;
    int first = 1;
    
    if ((recipf = fopen(filename, "r")) == NULL) {
	if (errormode == 1)
	    logandexit(EX_NOINPUT,
		       "can't open receipt address file '%s'\n", filename);
	else
	    return NULL;
    }
    
    ls_init(&recipbuf);
    ls_reset(&recipbuf);

    while (fgets(buf, sizeof buf, recipf) != NULL) {
	char *p;
	if ((p = strchr(buf, '\n')) != NULL)/* remove trailing LF */
	    *p = '\0';
	if ((p = strchr(buf, '#')) != NULL) /* remove comments, if exists */
	    *p = '\0';

	if (buf[0] != '\0') {	/* if it is *NOT* newline.. */
	    char * namep = normalizeaddr(buf);
	    if (namep != NULL) {
		if (!first) {
		    ls_appendstr(&recipbuf, " ");
		}
		else {
		    first = 0;
		}

		ls_appendstr(&recipbuf, namep);
	    }
	}
    }

#ifdef DEBUGLOG
    {extern FILE* debuglog;
     fprintf(debuglog, "paraserecipfile: %s\n", recipbuf.ls_buf);
    }
#endif
    return recipbuf.ls_buf;
}
#endif



#ifdef TEST
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

main(ac, av)
    int ac;
    char **av;
{
#if 0
    if (ac != 1) {
	char * p = parserecipfile(av[1]);
	puts("\noneliine test:\n");
	puts(p);
    }
#endif    
    test("   \"\033$B>H4nL>\033(J \033$BD+CK\033(J\" <jl@foretune.co.jp>");
    test("Shigeya Suzuki <shigeya@foretune.co.jp>");
    test("shigeya@foretune.co.jp (Shigeya Suzuki)");
    test("Shigeya \"too busy\" Suzuki <shigeya@foretune.co.jp>");
    test("\"Shigeya 'too busy' Suzuki\" <shigeya@foretune.co.jp>");
    test("shigeya@foretune.co.jp (Shigeya \"too busy\" Suzuki)");
    test("shigeya@foretune.co.jp (Shigeya 'too busy' Suzuki)");
    test("\":users 1\"@foretune.co.jp");
    test("<\":users 1\"@foretune.co.jp>");
    test("hello   <\":users 1\"@foretune.co.jp>   hello (hello)");
    test("(hello)   \":users 1\"@foretune.co.jp");
    test("=?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?= <shigeya@foretune.co.jp>");
    test("shigeya@foretune.co.jp   (<Shigeya Suzuki>)");
    test("=?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=\n =?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=   <shigeya@foretune.co.jp>");
    test("shigeya@foretune.co.jp   (=?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=\n =?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=)");
    test("  <shigeya@foretune.co.jp>   (=?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=\n =?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=)");
    test("  shigeya   <shigeya@foretune.co.jp>   (=?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=\n =?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=)");
    test("(shigeya)   <shigeya@foretune.co.jp>   (=?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=\n =?ISO-2022-JP?B?GyRCTmtMWkxQOkgbKEI=?=)");
    test("shigeya@foretune.co.jp (Shigeya 'too busy' Suzuki");
    test("shigeya@foretune.co.jp Shigeya 'too busy' Suzuki)");
    test("hello <shigeya@foretune.co.jp (Shigeya 'too busy' Suzuki)");
    test("\"hello<>>(()))\" <shigeya@foretune.co.jp> (Shigeya 'too busy' Suzuki)");
    test("\"shigeya@foretune.co.jp\" (Shigeya 'too busy' Suzuki)");
    test("\"Yoshitaka Tokugawa \\\"<<>>>(()))\\\" test\" <toku@tokugawa.org>");
}

test(s1)
    char *s1;
{
    char buf[1024];
    strcpy(buf, s1);
    printf ("target = '%s'\nresult = '%s'\n", s1, normalizeaddr(buf));
    printf ("fake = '%s'\n", fakefromaddr(buf, "test", "tokugawa.or.jp"));
}

void
#ifdef __STDC__
logandexit(int exitcode, char* fmt, ...)
#else
logandexit(exitcode, fmt, va_alist)
	int exitcode;
	char *fmt;
	va_dcl
#endif
{
    va_list ap;

    fprintf(stderr, "[EXITCODE=%d] ", exitcode);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(exitcode);
}

void
#ifdef __STDC__
logwarn(char* fmt, ...)
#else
logwarn(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/* allocate or fail
 */
char*
xmalloc(len)
    size_t len;
{
    void *p = malloc(len);
    if (p == NULL) {
	fprintf(stderr, "Can't allocate buf. size=%d\n", len);
	exit(1);
    }
    return p;
}

/* reallocate or fail
 */
char*
xrealloc(p, len)
    char *p;
    size_t len;
{
    void *np;

    if (p == NULL) {
	fprintf(stderr, "Invalid pointer for realloc (0x%x)\n", p);
	exit(1);
    }

    np = realloc(p,len);

    if (np == NULL) {
	fprintf(stderr, "Can't reallocate buf. size=%d (%d)\n", len, errno);
	exit(1);
    }
    return np;
}
#endif
