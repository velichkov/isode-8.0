/* na2norm.c - normalize NSAPaddr */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/compat/RCS/na2norm.c,v 9.0 1992/06/16 12:07:00 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/compat/RCS/na2norm.c,v 9.0 1992/06/16 12:07:00 isode Rel $
 *
 *
 * $Log: na2norm.c,v $
 * Revision 9.0  1992/06/16  12:07:00  isode
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
#include "general.h"
#include "manifest.h"
#include "isoaddrs.h"
#include "internet.h"
#include "tailor.h"

/* Encoding based on

   	"An interim approach to use of Network Addresses",
	S.E. Kille, January 16, 1989

 */

/*  */

struct NSAPaddr *
na2norm (struct NSAPaddr *na) {
	int	    ilen;
	char  *cp,
		  *dp;
	char    nsap[NASIZE * 2 + 1];
	struct hostent *hp;
	struct ts_interim *ts;
	static struct NSAPaddr nas;
	struct NSAPaddr *ca = &nas;

	if (na -> na_stack == NA_NSAP) {
		*ca = *na;	/* struct copy */
		return ca;
	}

	bzero ((char *) ca, sizeof *ca);
	ca -> na_stack = NA_NSAP;

	for (ts = ts_interim; ts -> ts_name; ts++)
		if (ts -> ts_subnet == na -> na_community)
			break;
	if (!ts -> ts_name) {
		SLOG (addr_log, LLOG_EXCEPTIONS, NULLCP,
			  ("unable to find community #%d", na -> na_community));
		return NULLNA;
	}

	cp = nsap;
	switch (na -> na_stack) {
	case NA_TCP:
		if ((hp = gethostbystring (na -> na_domain)) == NULL) {
			SLOG (addr_log, LLOG_EXCEPTIONS, NULLCP,
				  ("%s: unknown host", na -> na_domain));
			return NULLNA;
		}
#define	s2a(b)	(((int) (b)) & 0xff)
		sprintf (cp, "%03d%03d%03d%03d",
				 s2a (hp -> h_addr[0]),
				 s2a (hp -> h_addr[1]),
				 s2a (hp -> h_addr[2]),
				 s2a (hp -> h_addr[3]));
		cp += strlen (cp);
#undef	s2a

		if (na -> na_port) {
			sprintf (cp, "%05d", (int) ntohs (na -> na_port));
			cp += strlen (cp);

			if (na -> na_tset && na -> na_tset != NA_TSET_TCP) {
				sprintf (cp, "%05d", (int) na -> na_tset);
				cp += strlen (cp);
			}
		}
		break;

	case NA_X25:
		if (na -> na_community == SUBNET_INT_X25
				&& na -> na_cudflen == 0
				&& na -> na_pidlen == 0
				&& na -> na_dte[0] != '0') {	/* SEK - X121 form */
			/* should be more general */
			sprintf (nsap, "36%014s", na -> na_dte);
			ts = NULL;
			break;
		}

		if (ilen = na -> na_pidlen & 0xff)
			*cp++ = '1', dp = na -> na_pid;
		else if (ilen = na -> na_cudflen & 0xff)
			*cp++ = '2', dp = na -> na_cudf;
		else
			*cp++ = '0';
		if (ilen) {
			sprintf (cp, "%01d", ilen);
			cp += strlen (cp);

			for (; ilen-- > 0; cp += 3)
				sprintf (cp, "%03d", *dp++ & 0xff);
		}
		strcpy (cp, na -> na_dte);
		break;

	default:
		SLOG (addr_log, LLOG_EXCEPTIONS, NULLCP,
			  ("unknown address type 0x%x", na -> na_stack));
		return NULLNA;
	}

	cp = nsap, dp = ca -> na_address;
	if (ts) {
		bcopy (ts -> ts_prefix, dp, ts -> ts_length);
		dp += ts -> ts_length;
	}
	while (*cp) {
		*dp = (*cp++ - '0') << 4;
		if (*cp)
			*dp++ |= (*cp++ - '0') & 0x0f;
		else
			*dp++ |= 0x0f;
	}
	ca -> na_addrlen = dp - ca -> na_address;

	return ca;
}
