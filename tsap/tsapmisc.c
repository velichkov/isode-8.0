/* tsapmisc.c - miscellany tsap functions */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/tsap/RCS/tsapmisc.c,v 9.0 1992/06/16 12:40:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/tsap/RCS/tsapmisc.c,v 9.0 1992/06/16 12:40:39 isode Rel $
 *
 *
 * $Log: tsapmisc.c,v $
 * Revision 9.0  1992/06/16  12:40:39  isode
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
#include "tpkt.h"
#ifdef SYS5
#include  <termio.h>
#else
#include <sys/ioctl.h>
#endif

/*    estimate of octets that might be returned */

int	TSelectOctets (sd, nbytes, td)
int	sd;
long   *nbytes;
struct TSAPdisconnect *td;
{
	int	    result;
	long    value;
	SBV	    smask;
	struct tsapblk *tb;

	missingP (nbytes);
	missingP (td);

	smask = sigioblock ();

	tsapPsig (tb, sd);

	result = OK;
	if (tb -> tb_nreadfnx) {
		if ((result = (*tb -> tb_nreadfnx) (tb, &value)) == NOTOK)
			value = 0L;
	} else {
#ifdef	FIONREAD
		if (ioctl (tb -> tb_fd, FIONREAD, (char *) &value) == NOTOK)
			value = 0L;
#endif

		switch (tb -> tb_flags & (TB_TP0 | TB_TP4)) {
		case TB_TCP:
		case TB_X25:
			if (value > DT_MAGIC && tb -> tb_len == 0)
				value -= DT_MAGIC;
			break;

		default:
			break;
		}
	}

	if (result == OK)
		value += (long) tb -> tb_len;
	*nbytes = value;

	sigiomask (smask);

	return result;
}

/*    get TSAPs */

int
TGetAddresses (int sd, struct TSAPaddr *initiating, struct TSAPaddr *responding, struct TSAPdisconnect *td) {
	SBV	    smask;
	struct tsapblk *tb;

	missingP (td);

	smask = sigioblock ();

	tsapPsig (tb, sd);

	if (initiating)
		copyTSAPaddrX (&tb -> tb_initiating, initiating);
	if (responding)
		copyTSAPaddrX (&tb -> tb_responding, responding);

	sigiomask (smask);

	return OK;
}

/*    define transport manager */

#ifdef	MGMT
int
TSetManager (int sd, IFP fnx, struct TSAPdisconnect *td) {
	SBV	    smask;
	struct tsapblk *tb;

	missingP (td);

	smask = sigioblock ();

	tsapPsig (tb, sd);

	tb -> tb_manfnx = fnx;

	sigiomask (smask);

	return OK;
}
#endif
