/* isoalias.c - application entity info --  directory service utilities */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/acsap/RCS/isoalias.c,v 9.0 1992/06/16 12:05:59 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/acsap/RCS/isoalias.c,v 9.0 1992/06/16 12:05:59 isode Rel $
 *
 *
 * $Log: isoalias.c,v $
 * Revision 9.0  1992/06/16  12:05:59  isode
 * Release 8.0
 *
 */

/*
 *				  NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */


/* LINTLIBRARY */

#include <stdio.h>
#include "general.h"
#include "manifest.h"
#include "tailor.h"

/*    DATA */

static char *isoaliases = "isoaliases";


#define	PBUCKETS	128
#define	PHASH(nm) \
    (((nm)[1]) ? (((chrcnv[((nm)[0])] - chrcnv[((nm)[1])]) & 0x1f) \
		  	+ ((chrcnv[(nm)[2]]) & 0x5f)) \
	       : (chrcnv[(nm)[0]]) & 0x7f)

struct pair {
	char   *p_name;
	char   *p_value;

	struct pair *p_chain;
};

static int inited = 0;
static struct pair *Pbuckets[PBUCKETS];

static int  read_aliases ();
static int  read_file ();

/*  */

char *
alias2name (char *name) {
	struct pair *p;

	if (!inited)
		read_aliases ();

	for (p = Pbuckets[PHASH (name)];
			p && lexequ (p -> p_name, name);
			p = p -> p_chain)
		continue;

	return (p ? p -> p_value : NULL);
}

/*  */

static int
read_aliases()  {
	char   *hp;
	char   buffer[BUFSIZ];

	if (inited)
		return;
	inited = 1;

	bzero ((char *) Pbuckets, sizeof Pbuckets);

	read_file (isodefile (isoaliases, 0));

	if ((hp = getenv ("HOME")) == NULL)
		hp = ".";
	sprintf (buffer, "%s/.isode_aliases", hp);
	read_file (buffer);
}

/*  */

static int
read_file (char *file) {
	char *cp;
	char   buffer[BUFSIZ + 1],
		   *vec[NVEC + NSLACK + 1];
	FILE *fp;

	if ((fp = fopen (file, "r")) == NULL)
		return;

	while (fgets (buffer, sizeof buffer, fp)) {
		if (*buffer == '#')
			continue;
		if (cp = index (buffer, '\n'))
			*cp = NULL;
		if (str2vec (buffer, vec) < 2)
			continue;

		if (add_alias (vec[0], vec[1]) == NOTOK)
			break;
	}

	fclose (fp);
}

/*  */

int
add_alias (char *name, char *value) {
	int	    i;
	struct pair *p;

	if (!inited)
		read_aliases ();

	if ((p = (struct pair *) calloc (1, sizeof *p)) == NULL) {
		SLOG (addr_log, LLOG_EXCEPTIONS, NULLCP,
			  ("calloc of alias structure failed"));
		return NOTOK;
	}

	if ((p -> p_name = malloc ((unsigned) (strlen (name) + 1))) == NULL
			|| (p -> p_value = malloc ((unsigned) (strlen (value) + 1)))
			== NULL) {
		SLOG (addr_log, LLOG_EXCEPTIONS, NULLCP,
			  ("malloc of alias structure failed"));
		if (p -> p_name)
			free (p -> p_name);
		free ((char *) p);
		return NOTOK;
	}
	strcpy (p -> p_name, name);
	strcpy (p -> p_value, value);

	p -> p_chain = Pbuckets[i = PHASH (p -> p_name)];
	Pbuckets[i] = p;

	return OK;
}

#ifdef DEBUG
int
free_isode_alias()  {
	int i;
	struct pair *p, *np;

	for (i=0; i<PBUCKETS; i++)
		for (p = Pbuckets[i]; p != (struct pair *)NULL ; p = np) {
			np = p -> p_chain;
			free (p->p_name);
			free (p->p_value);
			free ((char *)p);
		}

}
#endif
