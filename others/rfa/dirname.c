/*
 * RFA - Remote File Access
 *
 * Access and Management for a partial file system tree that exists
 * at two sites either as master files or slave files
 *
 * dirname.c : create local filenames
 *
 * Contributed by Oliver Wenzel, GMD Berlin, 1990
 *
 * $Header: /xtel/isode/isode/others/rfa/RCS/dirname.c,v 9.0 1992/06/16 12:47:25 isode Rel $
 *
 * $Log: dirname.c,v $
 * Revision 9.0  1992/06/16  12:47:25  isode
 * Release 8.0
 *
 */

#ifndef       lint
static char *rcsid = "$Header: /xtel/isode/isode/others/rfa/RCS/dirname.c,v 9.0 1992/06/16 12:47:25 isode Rel $";
#endif

/*
 *                              NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include "general.h"
#include "rfa.h"

static char p[MAXPATHLEN];
char *fsBase = FS_BASE;

extern char *rindex();
extern char *realpath();

int commandMode = 0;

char *
basename (char *fn) {
	char *f;

	if (f = rindex(fn,'/'))
		return f+1;
	else
		return fn;
}


char *
dirname (char *fn) {
	static char buf[MAXPATHLEN];
	char *f;

	strcpy(buf, fn);
	if ((f = rindex(buf,'/')) && (f != buf)) {

		*f = '\0';
		return buf;
	} else
		return "/";
}


char *
makeFN (char *fn) {
	return makeFN2("", fn);
}

char *
makeFN2 (char *dir, char *fn) {
	char *s = p;

	strcpy(s,fsBase);
	s += strlen(fsBase);
	if (*(s-1) != '/')
		*(s++) = '/';
	while (*dir == '/')
		dir++;
	if (*dir != '\0') {
		strcpy(s, dir);
		s += strlen(dir);
	}
	if (*(s-1) != '/')
		*(s++) = '/';

	while (*fn == '/')
		fn++;
	strcpy(s, fn);

	return p;
}


char *
getRelativeFN (char *fn) {
	if (strncmp(fsBase, fn, strlen(fsBase)))
		return NULL;
	return fn + strlen(fsBase);
}


char *
expandSymLinks (char *path) {
	static char exp[MAXPATHLEN];
	char *r;

	/*--- expand symbolic links in fn ---*/
	if (realpath(makeFN(path), exp) == NULL) {
		if (errno != ENOENT) {
			sprintf(rfaErrStr,"%s - %s", sys_errname(errno),getRelativeFN(exp));
			return NULL;
		}
	}
	if ((r = getRelativeFN(exp)) == NULL) {
		sprintf(rfaErrStr, "%s not within RFA subtree", exp);
		return NULL;
	}
	return r;
}


char *
realPath3 (char *dir, char *path1, char *path2) {
	char *s, *s1, *rp;
	static char realp[MAXPATHLEN];
	char givenp[MAXPATHLEN];

	s = givenp;
	strcpy(s, dir);
	s += strlen(dir);
	if (*path1) {
		*(s++) = '/';
		strcpy(s, path1);
		s += strlen(path1);
	}
	if (*path2) {
		*(s++) = '/';
		strcpy(s, path2);
	}

	rp = realp;
	*(rp++) = '/';

	for (s = givenp; *s;) {
		while(*s == '/')
			s++;
		if (*s == '.')  {
			s1 = s+1;
			if (*(s1) == '/') {
				s += 2;
				continue;
			}
			if (*s1 == '.' && ((*(s1+1) == '/')||(*(s1-1) == '\0'))) {
				if ((rp - 1) != realp) {
					for( rp -= 2; *rp != '/'; rp--)
						;
					rp++;
				}
				s += 3;
				continue;
			}
		}
		for (; *s && *s != '/'; s++, rp++)
			*rp = *s;
		*(rp++) = *(s++);
	}
	*rp = '\0';

	return realp;
}


char *
realPath (char *dir, char *path) {
	return realPath3("", dir, path);
}


/*--------------------------------------------------------------*/
/*  getRfaContext						*/
/*--------------------------------------------------------------*/
char *
getRfaContext (char *cwd, char *fn) {
	char *rp;
	char buf[MAXPATHLEN];

	if (*fn == '@')
		rp = realPath(fsBase, fn+1);
	else if (*fn == '/')
		rp = realPath("/", fn);
	else if(commandMode) {
		getwd(buf);
		rp = realPath(buf, fn);
	} else
		rp = realPath3(fsBase, cwd, fn);

	if (strncmp(fsBase, rp, strlen(fsBase)))
		return NULL;

	/*--- extract realtive path ---*/
	rp += strlen(fsBase);

	return rp;
}


