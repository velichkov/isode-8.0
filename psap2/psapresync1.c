/* psapresync1.c - PPM: initiate resyncs */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/psap2/RCS/psapresync1.c,v 9.0 1992/06/16 12:29:42 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/psap2/RCS/psapresync1.c,v 9.0 1992/06/16 12:29:42 isode Rel $
 *
 *
 * $Log: psapresync1.c,v $
 * Revision 9.0  1992/06/16  12:29:42  isode
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
#include <signal.h>
#include "ppkt.h"

/*    P-RESYNCHRONIZE.REQUEST */

int
PReSyncRequest (int sd, int type, long ssn, int settings, PE *data, int ndata, struct PSAPindication *pi) {
	SBV	    smask;
	int     len,
			result;
	char   *base,
		   *realbase;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;

	toomuchP (data, ndata, NPDATA, "resync");
	missingP (pi);

	smask = sigioblock ();

	psapPsig (pb, sd);

	if ((result = info2ssdu (pb, pi, data, ndata, &realbase, &base, &len,
							 "P-RESYNCHRONIZE user-data", PPDU_RS)) != OK)
		goto out2;

	if ((result = SReSyncRequest (sd, type, ssn, settings, base, len, &sis))
			== NOTOK)
		if (SC_FATAL (sa -> sa_reason))
			ss2pslose (pb, pi, "SReSyncRequest", sa);
		else {
			ss2pslose (NULLPB, pi, "SReSyncRequest", sa);
			goto out1;
		}

out2:
	;
	if (result == NOTOK)
		freepblk (pb);
	else if (result == DONE)
		result = NOTOK;
out1:
	;
	if (realbase)
		free (realbase);
	else if (base)
		free (base);

	sigiomask (smask);

	return result;
}
