/* rydsresult.c - ROSY: return result to invocation */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/rosy/RCS/rydsresult.c,v 9.0 1992/06/16 12:37:29 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/rosy/RCS/rydsresult.c,v 9.0 1992/06/16 12:37:29 isode Rel $
 *
 *
 * $Log: rydsresult.c,v $
 * Revision 9.0  1992/06/16  12:37:29  isode
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
#include "rosy.h"


#ifdef __STDC__
#define missingP(p) \
{ \
    if (p == NULL) \
        return rosaplose (roi, ROS_PARAMETER, NULLCP, \
                            "mandatory parameter \"%s\" missing", #p); \
}
#else
#define	missingP(p) \
{ \
    if (p == NULL) \
	return rosaplose (roi, ROS_PARAMETER, NULLCP, \
			    "mandatory parameter \"%s\" missing", "p"); \
}
#endif

/*    RESULT */

int	RyDsResult (sd, id, out, priority, roi)
int	sd;
int	id,
	priority;
caddr_t out;
struct RoSAPindication *roi;
{
	int	    result;
	PE	    pe;
	struct opsblk *opb;
	struct RyOperation *ryo;

	missingP (roi);

	if ((opb = findopblk (sd, id, OPB_RESPONDER)) == NULLOPB)
		return rosaplose (roi, ROS_PARAMETER, NULLCP,
						  "invocation %d not in progress on association %d",
						  id, sd);

	ryo = opb -> opb_ryo;
	if (!ryo -> ryo_result)
		return rosaplose (roi, ROS_PARAMETER, NULLCP,
						  "result not permitted with operation %s/%d",
						  ryo -> ryo_name, ryo -> ryo_op);

#ifdef PEPSY_DEFINITIONS
	if (ryo -> ryo_res_mod) {
#else
	if (ryo -> ryo_res_encode) {
#endif
#ifdef	notdef
		missingP (out);
#endif
		PY_pepy[0] = 0;
#ifdef PEPSY_DEFINITIONS
		if (enc_f (ryo -> ryo_res_index, ryo -> ryo_res_mod, &pe, 1, NULL,
				   NULLCP, out) == NOTOK)
#else
		if ((*ryo -> ryo_res_encode) (&pe, 1, NULL, NULLCP, out) == NOTOK)
#endif
			return rosaplose (roi, ROS_CONGEST, NULLCP,
							  "error encoding result for invocation %d of operation %s/%d [%s]",
							  opb -> opb_id, ryo -> ryo_name, ryo -> ryo_op, PY_pepy);
	} else {
		if (out)
			return rosaplose (roi, ROS_PARAMETER, NULLCP,
							  "result value not permitted with operation %s/%d",
							  ryo -> ryo_name, ryo -> ryo_op);

		pe = NULLPE;
	}

	if ((result = RoResultRequest (sd, id, ryo -> ryo_op, pe, priority, roi))
			!= NOTOK)
		freeopblk (opb);

	if (pe)
		pe_free (pe);

	return result;
}
