/* ssapminor1.c - SPM: initiate minorsyncs */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/ssap/RCS/ssapminor1.c,v 9.0 1992/06/16 12:39:41 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/ssap/RCS/ssapminor1.c,v 9.0 1992/06/16 12:39:41 isode Rel $
 *
 *
 * $Log: ssapminor1.c,v $
 * Revision 9.0  1992/06/16  12:39:41  isode
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
#include "spkt.h"

static int  SMinSyncRequestAux ();

/*    S-MINOR-SYNC.REQUEST */

int
SMinSyncRequest (int sd, int type, long *ssn, char *data, int cc, struct SSAPindication *si) {
	SBV	    smask;
	int     result;
	struct ssapblk *sb;

	switch (type) {
	case SYNC_CONFIRM:
	case SYNC_NOCONFIRM:
		break;

	default:
		return ssaplose (si, SC_PARAMETER, NULLCP,
						 "improper choice of type setting");
	}
	missingP (ssn);
	missingP (si);

	smask = sigioblock ();

	ssapPsig (sb, sd);
	toomuchP (sb, data, cc, SN_SIZE, "minorsync");

	result = SMinSyncRequestAux (sb, type, ssn, data, cc, si);

	sigiomask (smask);

	return result;
}

/*  */

static int
SMinSyncRequestAux (struct ssapblk *sb, int type, long *ssn, char *data, int cc, struct SSAPindication *si) {
	int     result;

	if (!(sb -> sb_requirements & SR_MINORSYNC))
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "minor synchronize service unavailable");

	if ((sb -> sb_requirements & SR_DAT_EXISTS)
			&& !(sb -> sb_owned & ST_DAT_TOKEN))
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "data token not owned by you");

	if (!(sb -> sb_owned & ST_MIN_TOKEN))
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "minorsync token not owned by you");

	if ((sb -> sb_requirements & SR_ACTIVITY)
			&& !(sb -> sb_flags & SB_Vact))
		return ssaplose (si, SC_OPERATION, NULLCP, "no activity in progress");

	if (sb -> sb_flags & SB_MAA)
		return ssaplose (si, SC_OPERATION, "awaiting your majorsync response");

	if ((result = SWriteRequestAux (sb, SPDU_MIP, data, cc, type,
									*ssn = sb -> sb_V_M, 0, NULLSD, NULLSD, NULLSR, si)) == NOTOK)
		freesblk (sb);
	else {
		if (sb -> sb_flags & SB_Vsc) {
			sb -> sb_V_A = sb -> sb_V_M;
			sb -> sb_flags &= ~SB_Vsc;
		}
		sb -> sb_V_M++;
	}

	return result;
}
