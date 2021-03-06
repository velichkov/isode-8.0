/* ftamdocument.c - FTAM document database */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/ftam/RCS/ftamdocument.c,v 9.0 1992/06/16 12:14:55 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/ftam/RCS/ftamdocument.c,v 9.0 1992/06/16 12:14:55 isode Rel $
 *
 *
 * $Log: ftamdocument.c,v $
 * Revision 9.0  1992/06/16  12:14:55  isode
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
#include "ftam.h"
#include "tailor.h"

/*    DATA */

static char *isodocuments = "isodocuments";

static FILE *servf = NULL;
static int   stayopen = 0;

static struct isodocument ids;

/*  */

int
setisodocument (int f) {
	if (servf == NULL)
		servf = fopen (isodefile (isodocuments, 0), "r");
	else
		rewind (servf);
	stayopen |= f;

	return (servf != NULL);
}


int
endisodocument (void) {
	if (servf && !stayopen) {
		fclose (servf);
		servf = NULL;
	}

	return 1;
}

/*  */

struct isodocument *
getisodocument (void) {
	char  *cp;
	struct isodocument *id = &ids;
	static char buffer[BUFSIZ + 1];
	static char *vec[NVEC + NSLACK + 1];

	if (servf == NULL
			&& (servf = fopen (isodefile (isodocuments, 0), "r")) == NULL)
		return NULL;

	if (id -> id_type)
		oid_free (id -> id_type);
	if (id -> id_abstract)
		oid_free (id -> id_abstract);
	if (id -> id_transfer)
		oid_free (id -> id_transfer);
	if (id -> id_model)
		oid_free (id -> id_model);
	if (id -> id_constraint)
		oid_free (id -> id_constraint);

	bzero ((char *) id, sizeof *id);

	while (fgets (buffer, sizeof buffer, servf) != NULL) {
		if (*buffer == '#')
			continue;
		if (cp = index (buffer, '\n'))
			*cp = NULL;
		if (str2vec (buffer, vec) < 6)
			continue;

		id -> id_entry = vec[0];

		if ((id -> id_type = str2oid (vec[1])) == NULLOID
				|| (id -> id_type = oid_cpy (id -> id_type)) == NULLOID)
			continue;

		if ((id -> id_abstract = str2oid (vec[2])) == NULLOID
				|| (id -> id_abstract = oid_cpy (id -> id_abstract))
				== NULLOID)
			goto free1;

		if ((id -> id_transfer = str2oid (vec[3])) == NULLOID
				|| (id -> id_transfer = oid_cpy (id -> id_transfer))
				== NULLOID)
			goto free2;

		if ((id -> id_model = str2oid (vec[4])) == NULLOID
				|| (id -> id_model = oid_cpy (id -> id_model)) == NULLOID)
			goto free3;

		if ((id -> id_constraint = str2oid (vec[5])) == NULLOID
				|| (id -> id_constraint = oid_cpy (id -> id_constraint))
				== NULLOID) {
			oid_free (id -> id_model);
			id -> id_model = NULLOID;

free3:
			;
			oid_free (id -> id_transfer);
			id -> id_transfer = NULLOID;

free2:
			;
			oid_free (id -> id_abstract);
			id -> id_abstract = NULLOID;

free1:
			;
			oid_free (id -> id_type);
			id -> id_type = NULLOID;

			continue;
		}

		return id;
	}

	return NULL;
}

/*  */

struct isodocument *
getisodocumentbyentry (char *entry) {
	struct isodocument *id;

	setisodocument (0);
	while (id = getisodocument ())
		if (strcmp (id -> id_entry, entry) == 0)
			break;
	endisodocument ();

	return id;
}

/*  */

struct isodocument *getisodocumentbytype (type)
OID	type;
{
	struct isodocument *id;

	setisodocument (0);
	while (id = getisodocument ())
		if (oid_cmp (id -> id_type, type) == 0)
			break;
	endisodocument ();

	return id;
}
