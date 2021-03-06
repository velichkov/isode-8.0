/* acsapblock.c - manage association blocks */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/acsap/RCS/acsapblock.c,v 9.0 1992/06/16 12:05:59 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/acsap/RCS/acsapblock.c,v 9.0 1992/06/16 12:05:59 isode Rel $
 *
 *
 * $Log: acsapblock.c,v $
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
#include "acpkt.h"

/*    DATA */

static int  once_only = 0;
static struct assocblk assocque;
static struct assocblk *ACHead = &assocque;

/*    ASSOCIATION BLOCKS */

struct assocblk *
newacblk()  {
	struct assocblk *acb;

	acb = (struct assocblk   *) calloc (1, sizeof *acb);
	if (acb == NULL)
		return NULL;

	acb -> acb_fd = NOTOK;
	acb -> acb_actno = 1;

	if (once_only == 0) {
		ACHead -> acb_forw = ACHead -> acb_back = ACHead;
		once_only++;
	}

	insque (acb, ACHead -> acb_back);

	return acb;
}

/*  */

int
freeacblk (struct assocblk *acb) {
	if (acb == NULL)
		return;

	if (acb -> acb_flags & ACB_STICKY) {
		acb -> acb_flags &= ~ACB_STICKY;
		return;
	}

	if (acb -> acb_fd != NOTOK && acb -> acb_uabort)
		if (acb -> acb_flags & ACB_ACS) {
			if (acb -> acb_flags & ACB_RTS) {/* recurse */
				struct AcSAPindication  acis;

				(*acb -> acb_uabort) (acb -> acb_fd, NULLPEP, 0, &acis);
				return;
			} else {
				struct PSAPindication   pis;

				(*acb -> acb_uabort) (acb -> acb_fd, NULLPEP, 0, &pis);
			}
		} else {
			struct SSAPindication   sis;

			(*acb -> acb_uabort) (acb -> acb_fd, NULLCP, 0, &sis);
		}

	if (acb -> acb_flags & ACB_FINISH)
		ACFFREE (&acb -> acb_finish);

	if (acb -> acb_context)
		oid_free (acb -> acb_context);
	if (acb -> acb_retry)
		pe_free (acb -> acb_retry);

	FREEACB (acb);

	if (acb -> acb_apdu)
		pe_free (acb -> acb_apdu);

	remque (acb);

	free ((char *) acb);
}

/*  */

struct assocblk *
findacblk (int sd) {
	struct assocblk *acb;

	if (once_only == 0)
		return NULL;

	for (acb = ACHead -> acb_forw; acb != ACHead; acb = acb -> acb_forw)
		if (acb -> acb_fd == sd)
			return acb;

	return NULL;
}
