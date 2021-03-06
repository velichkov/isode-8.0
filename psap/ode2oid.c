/* ode2oid.c - object descriptor to object identifier */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/psap/RCS/ode2oid.c,v 9.0 1992/06/16 12:25:44 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/psap/RCS/ode2oid.c,v 9.0 1992/06/16 12:25:44 isode Rel $
 *
 *
 * $Log: ode2oid.c,v $
 * Revision 9.0  1992/06/16  12:25:44  isode
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

#ifdef FIXME
#include <psap.h>
OID	ode2oid (char* descriptor) {
	return NULLOID;
}
#else
/* LINTLIBRARY */

#include <stdio.h>
#include "psap.h"
#include "ppkt.h"

/* work around define collisions */
#undef missingP
#undef pylose
#include "rtpkt.h"

/* work around type clashes */
#undef missingP
#undef pylose
#undef toomuchP

#define ACSE
// AC_ASN below copied from acpkt.h"
#ifdef	ACSE
#define	AC_ASN		"acse pci version 1"
#if	USE_BUILTIN_OIDS
#define AC_ASN_OID	str2oid ("2.2.1.0.1")
#else
#define AC_ASN_OID	ode2oid (AC_ASN)
#endif
#endif

/*  */
#define ODECACHESIZE 10
static struct la_cache {
	char	*descriptor;
	int	ref;
	OID	oid;
} Cache[ODECACHESIZE];

static void
preloadcache (char *str) {
	struct la_cache *cp = &Cache[0];
	struct isobject *io;

	setisobject (0);
	while (io = getisobject ()) {
		if (strcmp (str, io -> io_descriptor) == 0 ||
				strcmp (DFLT_ASN, io -> io_descriptor) == 0 ||
				strcmp (AC_ASN, io -> io_descriptor) == 0 ||
				strcmp (BER, io -> io_descriptor) == 0 ||
				strcmp (RT_ASN, io -> io_descriptor) == 0) {
			if ((cp -> oid = oid_cpy (&io -> io_identity)) == NULLOID ||
					(cp -> descriptor = malloc ((unsigned) (strlen (io -> io_descriptor) + 1)))
					== NULLCP) {
				if (cp -> oid) {
					oid_free (cp -> oid);
					cp -> oid = NULLOID;
				}
			} else {
				strcpy (cp -> descriptor, io -> io_descriptor);
				cp -> ref = 1;
				cp ++;
			}
		}
	}
	endisobject ();
}

OID
ode2oid (char *descriptor) {
	struct isobject *io;
	int i, least;
	struct la_cache *cp, *cpn;
	static char firsttime = 0;

	if (firsttime == 0) {
		preloadcache (descriptor);
		firsttime = 1;
	}

	least = Cache[0].ref;
	for (cpn = cp = &Cache[0], i = 0; i < ODECACHESIZE; i++, cp++) {
		if (cp -> ref < least) {
			least = cp -> ref;
			cpn = cp;
		}
		if (cp -> ref <= 0)
			continue;
		if (strcmp (descriptor, cp -> descriptor) == 0) {
			cp -> ref ++;
			return cp -> oid;
		}
	}

	if ((io = getisobjectbyname (descriptor)) == NULL)
		return NULLOID;

	if (cpn -> oid)
		oid_free (cpn -> oid);
	if (cpn -> descriptor)
		free (cpn -> descriptor);

	cpn -> ref = 1;
	if ((cpn -> oid = oid_cpy (&io -> io_identity)) == NULLOID ||
			(cpn -> descriptor = malloc ((unsigned) (strlen (descriptor) + 1))) == NULLCP) {
		if (cpn -> oid) {
			oid_free (cpn -> oid);
			cpn -> oid = NULLOID;
		}
		cpn -> ref = 0;
	} else
		strcpy (cpn -> descriptor, descriptor);

	return (&io -> io_identity);
}

#ifdef DEBUG
int
free_oid_cache()  {
	struct la_cache *cp;
	int i;

	for (cp = &Cache[0], i = 0; i < ODECACHESIZE; i++, cp++) {
		if (cp->descriptor)
			free (cp->descriptor);

		if (cp->oid)
			oid_free(cp->oid);
	}
}
#endif
#endif
