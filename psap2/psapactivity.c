/* psapactivity.c - PPM: activities */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/psap2/RCS/psapactivity.c,v 9.0 1992/06/16 12:29:42 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/psap2/RCS/psapactivity.c,v 9.0 1992/06/16 12:29:42 isode Rel $
 *
 *
 * $Log: psapactivity.c,v $
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

/*    P-CONTROL-GIVE.REQUEST */

int
PGControlRequest (int sd, struct PSAPindication *pi) {
	SBV	    smask;
	int     result;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;

	missingP (pi);

	smask = sigioblock ();

	psapPsig (pb, sd);

	if ((result = SGControlRequest (sd, &sis)) == NOTOK)
		if (SC_FATAL (sa -> sa_reason))
			ss2pslose (pb, pi, "SGControlRequest", sa);
		else {
			ss2pslose (NULLPB, pi, "SGControlRequest", sa);
			goto out1;
		}
	else
		pb -> pb_owned = 0;

	if (result == NOTOK)
		freepblk (pb);
out1:
	;
	sigiomask (smask);

	return result;
}

/*    P-ACTIVITY-START.REQUEST */

int
PActStartRequest (int sd, struct SSAPactid *id, PE *data, int ndata, struct PSAPindication *pi) {
	SBV	    smask;
	int     len,
			result;
	char   *base,
		   *realbase;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;

	toomuchP (data, ndata, NPDATA, "activity start");
	missingP (pi);

	smask = sigioblock ();

	psapPsig (pb, sd);

	if ((result = info2ssdu (pb, pi, data, ndata, &realbase, &base, &len,
							 "P-ACTIVITY-START user-data", PPDU_NONE)) != OK)
		goto out2;

	if ((result = SActStartRequest (sd, id, base, len, &sis)) == NOTOK)
		if (SC_FATAL (sa -> sa_reason))
			ss2pslose (pb, pi, "SActStartRequest", sa);
		else {
			ss2pslose (NULLPB, pi, "SActStartRequest", sa);
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

/*    P-ACTIVITY-RESUME.REQUEST */

int
PActResumeRequest (int sd, struct SSAPactid *id, struct SSAPactid *oid, long ssn, struct SSAPref *ref, PE *data, int ndata, struct PSAPindication *pi) {
	SBV	    smask;
	int     len,
			result;
	char   *base,
		   *realbase;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;

	toomuchP (data, ndata, NPDATA, "activity resume");
	missingP (pi);

	smask = sigioblock ();

	psapPsig (pb, sd);

	if ((result = info2ssdu (pb, pi, data, ndata, &realbase, &base, &len,
							 "P-ACTIVITY-RESUME user-data", PPDU_NONE)) != OK)
		goto out2;

	if ((result = SActResumeRequest (sd, id, oid, ssn, ref, base, len, &sis))
			== NOTOK)
		if (SC_FATAL (sa -> sa_reason))
			ss2pslose (pb, pi, "SActResumeRequest", sa);
		else {
			ss2pslose (NULLPB, pi, "SActResumeRequest", sa);
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

/*    P-ACTIVITY-{INTERRUPT,DISCARD}.REQUEST */

int
PActIntrRequestAux (int sd, int reason, struct PSAPindication *pi, IFP sfunc, char *stype) {
	SBV	    smask;
	int     result;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;

	missingP (pi);
	missingP (sfunc);
	missingP (stype);

	smask = sigioblock ();

	psapPsig (pb, sd);

	if ((result = (*sfunc) (sd, reason, &sis)) == NOTOK)
		if (SC_FATAL (sa -> sa_reason))
			ss2pslose (pb, pi, stype, sa);
		else {
			ss2pslose (NULLPB, pi, stype, sa);
			goto out1;
		}

	if (result == NOTOK)
		freepblk (pb);
out1:
	;
	sigiomask (smask);

	return result;
}

/*    P-ACTIVITY-{INTERRUPT,DISCARD}.RESPONSE */

int
PActIntrResponseAux (int sd, struct PSAPindication *pi, IFP sfunc, char *stype) {
	SBV	    smask;
	int     result;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;

	missingP (pi);
	missingP (sfunc);
	missingP (stype);

	smask = sigioblock ();

	psapPsig (pb, sd);

	if ((result = (*sfunc) (sd, &sis)) == NOTOK)
		if (SC_FATAL (sa -> sa_reason))
			ss2pslose (pb, pi, stype, sa);
		else {
			ss2pslose (NULLPB, pi, stype, sa);
			goto out1;
		}

	if (result == NOTOK)
		freepblk (pb);
out1:
	;
	sigiomask (smask);

	return result;
}
