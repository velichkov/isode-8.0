/* ssaprelease1.c - SPM: initiate release */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/ssap/RCS/ssaprelease1.c,v 9.0 1992/06/16 12:39:41 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/ssap/RCS/ssaprelease1.c,v 9.0 1992/06/16 12:39:41 isode Rel $
 *
 *
 * $Log: ssaprelease1.c,v $
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

static int  SRelRequestAux ();
static int  SRelRetryRequestAux ();

/*    S-RELEASE.REQUEST */

int
SRelRequest (int sd, char *data, int cc, int secs, struct SSAPrelease *sr, struct SSAPindication *si) {
	SBV	    smask;
	int     result;
	struct ssapblk *sb;

	missingP (sr);
	missingP (si);

	smask = sigioblock ();

	ssapPsig (sb, sd);
	toomuchP (sb, data, cc, SF_SIZE, "release");

	result = SRelRequestAux (sb, data, cc, secs, sr, si);

	sigiomask (smask);

	return result;
}

/*  */

#define	dotoken(requires,shift,bit,type) \
{ \
    if ((sb -> sb_requirements & requires) && !(sb -> sb_owned & bit)) \
	return ssaplose (si, SC_OPERATION, NULLCP, \
		    "%s token not owned by you", type); \
}


static int
SRelRequestAux (struct ssapblk *sb, char *data, int cc, int secs, struct SSAPrelease *sr, struct SSAPindication *si) {
	struct ssapkt *s;

	dotokens ();

	if (sb -> sb_flags & SB_CD)
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "capability data request in progress");
	if (sb -> sb_flags & SB_CDA)
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "awaiting your capability data response");
	if (sb -> sb_flags & SB_GTC)
		return ssaplose (si, SC_OPERATION, NULLCP,
						 "give control request in progress");
	if (sb -> sb_flags & SB_MAA)
		return ssaplose (si, SC_OPERATION, "awaiting your majorsync response");
	if (sb -> sb_flags & SB_RELEASE)
		return ssaplose (si, SC_OPERATION, "release already in progress");

	if (sb -> sb_xspdu || sb -> sb_spdu)
		return ssaplose (si, SC_WAITING, NULLCP, NULLCP);

	if ((s = newspkt (SPDU_FN)) == NULL)
		return ssaplose (si, SC_CONGEST, NULLCP, "out of memory");
	if (cc > 0) {
		s -> s_mask |= SMASK_UDATA_PGI;
		s -> s_udata = data, s -> s_ulen = cc;
	} else
		s -> s_udata = NULL, s -> s_ulen = 0;

	sb -> sb_retry = s;

	return SRelRetryRequestAux (sb, secs, sr, si);
}

#undef	dotoken

/*    S-RELEASE-RETRY.REQUEST (pseudo) */

int
SRelRetryRequest (int sd, int secs, struct SSAPrelease *sr, struct SSAPindication *si) {
	SBV	    smask;
	int	    result;
	struct ssapblk *sb;

	missingP (sr);
	missingP (si);

	smask = sigioblock ();

	if ((sb = findsblk (sd)) == NULL)
		result = ssaplose (si, SC_PARAMETER, NULLCP,
						   "invalid session descriptor");
	else if (!(sb -> sb_flags & SB_RELEASE))
		result = ssaplose (si, SC_OPERATION, "release not in progress");
	else
		result = SRelRetryRequestAux (sb, secs, sr, si);

	sigiomask (smask);

	return result;
}

/*  */

static int
SRelRetryRequestAux (struct ssapblk *sb, int secs, struct SSAPrelease *sr, struct SSAPindication *si) {
	int	    code,
			result;
	struct ssapkt *s;

	if (sb -> sb_flags & SB_RELEASE)
		goto waiting;

	code = SPDU_FN;

again:
	;
	if (((s = sb -> sb_retry) -> s_code = code) == SPDU_FN) {
		s -> s_mask |= SMASK_FN_DISC;
		s -> s_fn_disconnect = FN_DISC_RELEASE;
	}

	result = spkt2sd (s, sb -> sb_fd, 0, si);

	if (s -> s_code == SPDU_FN) {
		s -> s_mask &= ~(SMASK_UDATA_PGI | SMASK_FN_DISC);
		s -> s_udata = NULL, s -> s_ulen = 0;
		s -> s_fn_disconnect = 0;
	}

	if (result == NOTOK)
		goto out1;

waiting:
	;
	if ((s = sb2spkt (sb, si, secs, NULLTX)) == NULL) {
		struct SSAPabort  *sa = &si -> si_abort;

		if (sa -> sa_reason == SC_TIMER) {
			sb -> sb_flags |= SB_RELEASE;

			return NOTOK;
		}

		goto out2;
	}

	bzero ((char *) sr, sizeof *sr);
	switch (s -> s_code) {
	case SPDU_FN:
		freespkt (s);
		code = SPDU_DN;
		goto again;

	case SPDU_DN:
		sr -> sr_affirmative = 1;
		copySPKTdata (s, sr);
		freespkt (s);
		freesblk (sb);
		return OK;

	case SPDU_NF:
		if (!(sb -> sb_requirements & SR_RLS_EXISTS)
				|| !(sb -> sb_owned & ST_RLS_TOKEN))
			goto bad_nf;
		sr -> sr_affirmative = 0;
		copySPKTdata (s, sr);
		freespkt (s);
		return OK;

	case SPDU_RS:
		if (sb -> sb_spdu)	/* XXX */
			freespkt (sb -> sb_spdu);
		sb -> sb_spdu = s;
		return ssaplose (si, SC_WAITING, NULLCP, NULLCP);

	case SPDU_AB:
		si -> si_type = SI_ABORT;
		{
			struct SSAPabort  *sa = &si -> si_abort;

			if (!(sa -> sa_peer = (s -> s_ab_disconnect & AB_DISC_USER)
								  ? 1 : 0))
				sa -> sa_reason = SC_ABORT;
			sa -> sa_info = s -> s_udata, sa -> sa_cc = s -> s_ulen;
			sa -> sa_realinfo = s -> s_udata, s -> s_udata = NULL;
		}
		break;

	default:
bad_nf:
		;
		spktlose (sb -> sb_fd, si, SC_PROTOCOL, NULLCP,
				  "session protocol mangled: not expecting 0x%x",
				  s -> s_code);
		break;
	}

out2:
	;
	freespkt (s);
out1:
	;
	freesblk (sb);

	return NOTOK;
}
