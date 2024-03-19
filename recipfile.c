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
normalizeaddr(header)
    char *header;
{
    char *hp;			/* current position of header buffer */
    char *addr_h;		/* mail address head position */
    char *addr_end;		/* mail address end position */
    char *addr_p;		/* mail address current position */
    int in_paren = 0;		/* true if in paren */
    int in_quote = 0;		/* true if in quote */
    int in_address = 0;		/* *hp points mail address portion */
    int addrfound = 0;		/* true if <mail@addr> found */

    if (strlen(header) >= MAXADDRLEN) {
	logwarn("too large header\n");
	return NULL;
    }

    addr_p = addr_h = malloc(sizeof(char) * MAXADDRLEN);

    if (addr_h == NULL) {
	logandexit(EX_UNAVAILABLE, memory_err);
    }

    /* malloc() returns uninitialized buffer. make sure buffer is empty. */
    *addr_p = '\0';

    /* scan mail address part from header */
    for (hp = header; *hp && ! addrfound; hp++) {
	if (! in_paren) {
	    if ((! isblank(*hp) && *hp != '(') || in_quote) {
		in_address = 1;
		*addr_p++ = *hp;
	 	*addr_p = '\0';
	    } else {
		in_address = 0;
	    }
	}
	switch (*hp) {
	    case '\\':
		if (hp[1] != '\0') {
		    hp++;	/* ignore */
		}
		break;
	    case '"':	/* in quote */
		if (in_quote) {
		   in_quote = 0;
		} else if (! in_paren) {
		    in_quote = 1;
		}
		break;
	    case '(':	/* in paren */
		if (! in_quote) {
		    in_paren = 1;
		}
		break;
	    case ')':
		if (! in_quote) {
		    if (! in_paren) {
			logwarn(paren_err);
			free(addr_h);
			return NULL;
		    }
		    in_paren = 0;
		    hp++;
		    while (isblank(*hp)) {
			hp++;
		    }
		    hp--;
		    if (in_address) {
			*addr_p++ = '.';	/* replace (comment) to '.' */
	 		*addr_p = '\0';
		    }
		} 
	
		break;
	    case '<':
		if (! in_paren && ! in_quote) {
		    addr_end = index(hp, '>');
	    	    if (addr_end != NULL) {
			hp++;
			strncpy(addr_h, hp, addr_end - hp);
			addr_h[addr_end - hp] = '\0';
			addrfound = 1;
		    } else {
			logwarn(bracket_err);
			free(addr_h);
			return NULL;
		    }
		}
		break;
	    case '>':
		if (! in_quote && ! in_paren) {
		    logwarn(bracket_err);
		    free(addr_h);
		    return NULL;
		}
		break;
	    default:
		break;
	}
    }
    if (in_quote) {
	logwarn(quote_err);
	free(addr_h);
	return NULL;
    }
    if (in_paren) {
	logwarn(paren_err);
	free(addr_h);
	return NULL;
    }
    if (strlen(addr_h) == 0) {
	logwarn("no From: address\n");
	free(addr_h);
	return NULL;
    }
    return addr_h;
}

/* fakefromaddr -- rewrite from address to list address
 */

char *
fakefromaddr(header, listname, hostname)
    char *header;
    char *listname;
    char *hostname;
{
    char *hp;			/* current position of header buffer */
    char *addr_h;		/* mail address head position */
    char *addr_end;		/* mail address end position */
    char *addr_p;		/* mail address current position */
    char *comment_h;		/* comment buffer head position */
    char *comment_p;		/* comment buffer current position */
    int in_paren = 0;		/* true if in paren */
    int in_quote = 0;		/* true if in quote */
    int addrfound = 0;		/* true if <mail@addr> found */

    addr_h = malloc(sizeof(char) * MAXADDRLEN);

    if (addr_h == NULL) {
	logandexit(EX_UNAVAILABLE, memory_err);
    }

    comment_p = comment_h = malloc(sizeof(char) * MAXADDRLEN);

    if (comment_p == NULL) {
	logandexit(EX_UNAVAILABLE, memory_err);
    }

    addr_p = NULL;

    if (strlen(header) >= MAXADDRLEN) {
	logwarn("too large header\n");
	goto error;
    }

    /* scan mail address part from header */
    for (hp = header; *hp && ! addrfound; hp++) {
	if (! in_paren) {
	    if ((! isblank(*hp) && *hp != '(') || in_quote) {
		if (addr_p == NULL) {
		    addr_p = hp;	/* mail address head */
		} else {
		    addr_end = hp + 1;
		}
	    }
	}
	switch (*hp) {
	    case '\\':
		if (hp[1] != '\0') {
		    hp++;	/* ignore */
		}
		break;
	    case '"':	/* in quote */
		if (in_quote) {
		   in_quote = 0;
		} else if (! in_paren) {
		    in_quote = 1;
		}
		break;
	    case '(':	/* in paren */
		if (! in_quote) {
		    in_paren = 1;
		    if (addr_p != NULL) {
			*comment_p++ = ' ';
			*comment_p++ = *hp;
			*comment_p = '\0';
		    }
		}
		break;
	    case ')':
		if (! in_quote) {
		    if (! in_paren) {
			logwarn(paren_err);
			goto error;
		    }
		    in_paren = 0;
		    if (addr_p != NULL) {
			*comment_p++ = *hp;
			*comment_p = '\0';
		    }
		    hp++;
		    while (isblank(*hp)) {
			hp++;
		    }
		    hp--;
		}
		break;
	    case '<':
		if (! in_paren && ! in_quote) {
		    hp++;
		    addr_p = hp;
		    addr_end = index(hp, '>');
	    	    if (addr_end != NULL) {
			strncpy(addr_h, hp, addr_end - hp);
			addr_h[addr_end - hp] = '\0';
			addrfound = 1;
		    } else {
		    	logwarn(bracket_err);
			goto error;
		    }
		}
		break;
	    case '>':
		if (! in_quote && ! in_paren) {
		    logwarn(bracket_err);
		    goto error;
		}
		break;
	    default:
		if (in_paren) {
		    if (addr_p != NULL) {
			*comment_p++ = *hp;
			*comment_p = '\0';
		    }
		}
		break;
	}
    }
    if (in_quote) {
	logwarn(quote_err);
	goto error;
    }
    if (in_paren) {
	logwarn(paren_err);
	goto error;
    }

    strncpy(addr_h, header, addr_p - header);
    addr_h[addr_p - header] = '\0';
    strcat(addr_h, listname);
    strcat(addr_h, "@");
    strcat(addr_h, hostname);
    if (addrfound) {
	strcat(addr_h, addr_end);
    } else if (comment_p != comment_h) {
	strcat(addr_h, comment_h);
    }

    free(comment_h);

    return addr_h;

error:
    sprintf(addr_h, "No Name <%s@%s>", listname, hostname);
    free(comment_h);
    return addr_h;
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
    test("\"hello <shigeya@foretune.co.jp> (Shigeya 'too busy' Suzuki)");
    test("sh\"i\"\"g\"  eya  @f o\"r\"  et  \"un\"e \".c\"o\".\"j   p");
    test("  (hello) sh\"i\"\"g\" (world)  eya  @f o\"r\"  et  \"un\"e \".c\"o\".\"j   p (japan)");
    test("  (hello) sh\"i\"\"g\" (world)  eya  @f o\"r\"  et  \"un\"e \".c\"o\".\"j   p (japan) .org");
    test("foo.bar@wide.ad.jp");
    test("  (hello) foo(world)var@wide.ad.jp");
    test("  (hello) foo(world)bar@wide.ad.jp (japan)");
    test(" foo.bar@wide.ad.jp");
    test("\"foo\"\".bar\"@wide.ad.jp");
    test("\"foo\" \t . \t \"bar\"@wide.ad.jp");
    test("\"\" foo.bar@wide.ad.jp");
    test("\" \" foo.bar@wide.ad.jp");
    test("test (comment) hello <shigeya@wide.ad.jp> TEST (COMMENT) HELLO");
}

test(s1)
    char *s1;
{
    char buf[1024];
    strcpy(buf, s1);
    printf ("target = '%s'\nresult = '%s'\n", s1, normalizeaddr(buf));
    printf ("fake = '%s'\n", fakefromaddr(buf, "fake", "address.com"));
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
