/* smuxentry.c - smuxEntry routines */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/snmp/RCS/smuxentry.c,v 9.0 1992/06/16 12:38:11 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/snmp/RCS/smuxentry.c,v 9.0 1992/06/16 12:38:11 isode Rel $
 *
 * Contributed by NYSERNet Inc.  This work was partially supported by the
 * U.S. Defense Advanced Research Projects Agency and the Rome Air Development
 * Center of the U.S. Air Force Systems Command under contract number
 * F30602-88-C-0016.
 *
 *
 * $Log: smuxentry.c,v $
 * Revision 9.0  1992/06/16  12:38:11  isode
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
#include "smux.h"
#include "tailor.h"

/*    DATA */

static char *smuxEntries = "snmpd.peers";

static FILE *servf = NULL;
static int  stayopen = 0;

static struct smuxEntry    ses;

/*  */

int	setsmuxEntry (f)
int	f;
{
	if (servf == NULL)
		servf = fopen (isodefile (smuxEntries, 0), "r");
	else
		rewind (servf);
	stayopen |= f;

	return (servf != NULL);
}


int	endsmuxEntry () {
	if (servf && !stayopen) {
		fclose (servf);
		servf = NULL;
	}

	return 1;
}

/*  */

struct smuxEntry  *getsmuxEntry () {
	int	    vecp;
	int i;
	struct smuxEntry *se = &ses;
	char  *cp;
	static char buffer[BUFSIZ + 1];
	static char *vec[NVEC + NSLACK + 1];
	static unsigned int elements[NELEM + 1];

	if (servf == NULL
			&& (servf = fopen (isodefile (smuxEntries, 0), "r")) == NULL)
		return NULL;

	bzero ((char *) se, sizeof *se);

	while (fgets (buffer, sizeof buffer, servf) != NULL) {
		if (*buffer == '#')
			continue;
		if (cp = index (buffer, '\n'))
			*cp = NULL;
		if ((vecp = str2vec (buffer, vec)) < 3)
			continue;

		if ((i = str2elem (vec[1], elements)) <= 1)
			continue;

		se -> se_name = vec[0];
		se -> se_identity.oid_elements = elements;
		se -> se_identity.oid_nelem = i;
		se -> se_password = vec[2];
		se -> se_priority = vecp > 3 ? atoi (vec[3]) : -1;

		return se;
	}

	return NULL;
}

/*  */

struct smuxEntry *getsmuxEntrybyname (name)
char   *name;
{
	struct smuxEntry *se;

	setsmuxEntry (0);
	while (se = getsmuxEntry ())
		if (strcmp (name, se -> se_name) == 0)
			break;
	endsmuxEntry ();

	return se;
}

/*  */

struct smuxEntry *getsmuxEntrybyidentity (identity)
OID	identity;
{
	struct smuxEntry *se;

	setsmuxEntry (0);

	/* Compare only the portion of the object identifier that is given in
	   the entry read from the smux.peers file, allowing a SMUX sub-agent
	   to provide its own version number as a suffix of its identity.  (EJP)
	 */
	while (se = getsmuxEntry ())
		if (identity -> oid_nelem >= se -> se_identity.oid_nelem
				&& elem_cmp (identity -> oid_elements,
							 se -> se_identity.oid_nelem,
							 se -> se_identity.oid_elements,
							 se -> se_identity.oid_nelem) == 0)
			break;

	endsmuxEntry ();

	return se;
}

