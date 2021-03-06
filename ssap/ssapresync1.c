/* ssapresync.c - SPM: initiate resyncs */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/ssap/RCS/ssapresync1.c,v 9.0 1992/06/16 12:39:41 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/ssap/RCS/ssapresync1.c,v 9.0 1992/06/16 12:39:41 isode Rel $
 *
 *
 * $Log: ssapresync1.c,v $
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

static int  SReSyncRequestAux ();

/*    S-RESYNCHRONIZE.REQUEST */

#define	dotoken(requires,shift,bit,type) \
{ \
    if (sb -> sb_requirements & requires) \
	switch (settings & (ST_MASK << shift)) { \
	    case ST_INIT_VALUE << shift: \
	    case ST_RESP_VALUE << shift: \
	    case ST_CALL_VALUE << shift: \
		break; \
 \
	    default: \
		return ssaplose (si, SC_PARAMETER, NULLCP, \
			"improper choice of %s token setting",type); \
	} \
}

/*  */

int
SReSyncRequest (int sd, int type, long ssn, int settings, char *data, int cc, struct SSAPindication *si) {
	SBV	    smask;
	int     result;
	struct ssapblk *sb;

	switch (type) {
	case SYNC_RESTART:
		break;

	case SYNC_ABANDON:
		if (ssn != SERIAL_NONE)
			return ssaplose (si, SC_PARAMETER, NULLCP,
							 "serial number inappropriate");
		break;

	case SYNC_SET:
		if (SERIAL_MIN > ssn || ssn > SERIAL_MAX + 1)
			return ssaplose (si, SC_PARAMETER, NULLCP,
							 "invalid serial number");
		break;

	default:
		return ssaplose (si, SC_PARAMETER, NULLCP,
						 "improper choice of type setting");
	}
	missingP (si);

	smask = sigioblock ();

	ssapRsig (sb, sd);
	toomuchP (sb, data, cc, SN_SIZE, "resync");

	result = SReSyncRequestAux (sb, type, ssn, settings, data, cc, si);

	sigiomask (smask);

	return result;
}

/*  */

static int
SReSyncRequestAux (struct ssapblk *sb, int type, long ssn, int settings, char *data, int cc, struct SSAPindication *si) {
	int     result;

	if (!(sb -> sb_requirements & SR_RESYNC))
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "resynchronize service unavailable");

	if ((sb -> sb_requirements & SR_ACTIVITY)
			&& !(sb -> sb_flags & SB_Vact))
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "no activity in progress");

	if ((sb -> sb_flags & SB_RA)
			&& SDoCollideAux (sb -> sb_flags & SB_INIT ? 1 : 0, type, ssn,
							  sb -> sb_rs, sb -> sb_rsn) == NOTOK)
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "resync in progress takes precedence");

	switch (type) {
	case SYNC_RESTART:
		if (sb -> sb_V_M < ssn || ssn < sb -> sb_V_R)
			return ssaplose (si, SC_OPERATION, NULLCP,
							 "bad choice for resync ssn, should be in [%d..%d]",
							 sb -> sb_V_R, sb -> sb_V_M);
		break;

	case SYNC_ABANDON:
		ssn = sb -> sb_V_M;
		break;

	case SYNC_SET:
		break;
	}

	dotokens ();

	if ((result = SWriteRequestAux (sb, SPDU_RS, data, cc, type, ssn, settings, NULLSD, NULLSD, NULLSR, si)) == NOTOK)
		freesblk (sb);
	else {
		sb -> sb_flags |= SB_RS, sb -> sb_flags &= ~(SB_RA | SB_EDACK | SB_ERACK);
		sb -> sb_rs = type;
		sb -> sb_rsn = ssn;
		sb -> sb_rsettings = sb -> sb_requirements & SR_TOKENS ? settings : 0;
	}

	return result;
}

#undef	dotoken
