/* rt2ssreleas1.c - RTPM: initiate release */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/rtsap/RCS/rt2ssreleas1.c,v 9.0 1992/06/16 12:37:45 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/rtsap/RCS/rt2ssreleas1.c,v 9.0 1992/06/16 12:37:45 isode Rel $
 *
 *
 * $Log: rt2ssreleas1.c,v $
 * Revision 9.0  1992/06/16  12:37:45  isode
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
#include "rtpkt.h"

static int  RtEndRequestAux ();

/*    RT-END.REQUEST (X.410 CLOSE.REQUEST) */

int
RtEndRequest (int sd, struct RtSAPindication *rti) {
	SBV	    smask;
	int     result;
	struct assocblk   *acb;

	missingP (rti);

	smask = sigioblock ();

	rtsapPsig (acb, sd);

	result = RtEndRequestAux (acb, rti);

	sigiomask (smask);

	return result;
}

/*  */

static int
RtEndRequestAux (struct assocblk *acb, struct RtSAPindication *rti) {
	int     result;
	struct SSAPindication   sis;
	struct SSAPindication *si = &sis;
	struct SSAPabort  *sa = &si -> si_abort;
	struct SSAPrelease  srs;
	struct SSAPrelease *sr = &srs;

	if (acb -> acb_flags & ACB_ACS)
		return rtsaplose (rti, RTS_OPERATION, NULLCP,
						  "not an association descriptor for RTS");
	if (!(acb -> acb_flags & ACB_INIT) && (acb -> acb_flags & ACB_TWA))
		return rtsaplose (rti, RTS_OPERATION, NULLCP, "not initiator");
	if (!(acb -> acb_flags & ACB_TURN))
		return rtsaplose (rti, RTS_OPERATION, NULLCP, "turn not owned by you");
	if (acb -> acb_flags & ACB_ACT)
		return rtsaplose (rti, RTS_OPERATION, NULLCP, "transfer in progress");
	if (acb -> acb_flags & ACB_PLEASE)
		return rtsaplose (rti, RTS_WAITING, NULLCP, NULLCP);

	if (SRelRequest (acb -> acb_fd, NULLCP, 0, NOTOK, sr, si) == NOTOK) {
		if (sa -> sa_peer)
			return ss2rtsabort (acb, sa, rti);

		result = ss2rtslose (acb, rti, "SRelRequest", sa);
	} else if (!sr -> sr_affirmative)
		result = rtpktlose (acb, rti, RTS_PROTOCOL, NULLCP,
							"other side refused to release connection");
	else {
		acb -> acb_fd = NOTOK;
		result = OK;
	}

	acb -> acb_flags &= ~ACB_STICKY;
	freeacblk (acb);

	return result;
}
