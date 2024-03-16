/* $Id$
 *
 * parse recipfile definitions
 */

#ifndef __RECIPFILE_H__
#define __RECIPFILE_H__

#ifndef __P
# include "cdefs.h"
#endif

extern char * normalizeaddr  __P((char*));
extern char * parserecipfile __P((char*, int));
extern char * fakefromaddr  __P((char*, char*, char*));

#endif	/* __PARSERECIPFILE_H__ */
