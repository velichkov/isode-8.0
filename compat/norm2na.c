/* norm2na.c - normalize NSAPaddr to NSAP struct */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/compat/RCS/norm2na.c,v 9.0 1992/06/16 12:07:00 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/compat/RCS/norm2na.c,v 9.0 1992/06/16 12:07:00 isode Rel $
 *
 *
 * $Log: norm2na.c,v $
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
#include "psap.h"
#include "isoaddrs.h"
#include "tailor.h"
#include "internet.h"

/* Encoding on "unrealNS" addresses based on

   	"An interim approach to use of Network Addresses",
	S.E. Kille, January 16, 1989

 */

/*  */

int
norm2na (char *p, int len, struct NSAPaddr *na) {
	na -> na_stack = NA_NSAP;

	if ((len == 8) && ((p[0] == 0x36) || (p[0] == 0x37))) {
		int	xlen;			/* SEK - X121 form */
		char   *cp,
			   *cp2,
			   *dp;
		char	nsap[14];

		dp = nsap;
		for (cp2 = (cp = p + 1) + 7; cp < cp2; cp++) {
			int     j;

			if ((j = ((*cp & 0xf0) >> 4)) > 9)
				goto concrete;
			*dp++ = j + '0';

			if ((j = (*cp & 0x0f)) > 9) {
				if (j != 0x0f)
					goto concrete;
			} else
				*dp++ = j + '0';
		}

		for (cp = nsap, xlen = 14; *cp == '0'; cp++, xlen--)
			continue;
		na -> na_dtelen = xlen;
		for (cp2 = na -> na_dte; xlen-- > 0; )
			*cp2++ = *cp++;
		*cp2 = NULL;
		na -> na_stack = NA_X25;
		na -> na_community = SUBNET_INT_X25;
	} else {
		struct ts_interim *ts,
				   *tp;

		tp = NULL;
		for (ts = ts_interim; ts -> ts_name; ts++)
			if (len > ts -> ts_length
					&& (tp == NULL || ts -> ts_length > tp -> ts_length)
					&& bcmp (p, ts -> ts_prefix, ts -> ts_length) == 0)
				tp = ts;
		if (tp) {
			int	    i,
					ilen,
					rlen;
			char   *cp,
				   *dp,
				   *ep;
			char    nsap[NASIZE * 2 + 1];

			if (tp -> ts_syntax == NA_NSAP)
				goto lock_and_load;
			dp = nsap;
			for (cp = p + tp -> ts_length, ep = p + len;
					cp < ep;
					cp++) {
				int     j;

				if ((j = ((*cp & 0xf0) >> 4)) > 9) {
concrete:
					;
					LLOG (addr_log, LLOG_EXCEPTIONS,
						  ("invalid concrete encoding"));
					goto realNS;
				}
				*dp++ = j + '0';

				if ((j = (*cp & 0x0f)) > 9) {
					if (j != 0x0f)
						goto concrete;
				} else
					*dp++ = j + '0';
			}
			*dp = NULL;

			cp = nsap;
lock_and_load:
			;
			na -> na_community = tp -> ts_subnet;
			switch (na -> na_stack = tp -> ts_syntax) {
			case NA_NSAP:
				goto unrealNS;

			case NA_X25:
				if ((int)strlen (cp) < 1) {
					LLOG (addr_log, LLOG_EXCEPTIONS,
						  ("missing DTE+CUDF indicator: %s", nsap));
					goto realNS;
				}
				sscanf (cp, "%1d", &i);
				cp += 1;
				switch (i) {
				case 0:	/* DTE only */
					break;

				case 1:	/* DTE+PID */
				case 2:	/* DTE+CUDF */
					if ((int)strlen (cp) < 1) {
						LLOG (addr_log, LLOG_EXCEPTIONS,
							  ("missing DTE+CUDF indicator: %s",
							   nsap));
						goto realNS;
					}
					sscanf (cp, "%1d", &ilen);
					cp += 1;
					rlen = ilen * 3;
					if ((int)strlen (cp) < rlen) {
						LLOG (addr_log, LLOG_EXCEPTIONS,
							  ("bad DTE+CUDF length: %s", nsap));
						goto realNS;
					}
					if (i == 1) {
						if (ilen > NPSIZE) {
							LLOG (addr_log, LLOG_EXCEPTIONS,
								  ("PID too long: %s", nsap));
							goto realNS;
						}
						dp = na -> na_pid;
						na -> na_pidlen = ilen;
					} else {
						if (ilen > CUDFSIZE) {
							LLOG (addr_log, LLOG_EXCEPTIONS,
								  ("CUDF too long: %s", nsap));
							goto realNS;
						}
						dp = na -> na_cudf;
						na -> na_cudflen = ilen;
					}
					for (; rlen > 0; rlen -= 3) {
						sscanf (cp, "%3d", &i);
						cp += 3;

						if (i > 255) {
							LLOG (addr_log, LLOG_EXCEPTIONS,
								  ("invalid PID/CUDF: %s", nsap));
							goto realNS;
						}
						*dp++ = i & 0xff;
					}
					break;

				default:
					LLOG (addr_log, LLOG_EXCEPTIONS,
						  ("invalid DTE+CUDF indicator: %s", nsap));
					goto realNS;
				}
				strcpy (na -> na_dte, cp);
				na -> na_dtelen = strlen (na -> na_dte);
				break;

			case NA_TCP:
				if ((int)strlen (cp) < 12) {
					LLOG (addr_log, LLOG_EXCEPTIONS,
						  ("missing IP address: %s", nsap));
					goto realNS;
				}
				{
					int	    q[4];

					sscanf (cp, "%3d%3d%3d%3d", q, q + 1, q + 2,
							q + 3);
					sprintf (na -> na_domain,
							 "%d.%d.%d.%d", q[0], q[1], q[2], q[3]);
				}
				cp += 12;

				if (*cp) {
					if ((int)strlen (cp) < 5) {
						LLOG (addr_log, LLOG_EXCEPTIONS,
							  ("missing port: %s", nsap));
						goto realNS;
					}
					sscanf (cp, "%5d", &i);
					cp += 5;
					na -> na_port = htons ((u_short) i);

					if (*cp) {
						if ((int)strlen (cp) < 5) {
							LLOG (addr_log, LLOG_EXCEPTIONS,
								  ("missing tset: %s", nsap));
							goto realNS;
						}
						sscanf (cp, "%5d", &i);
						cp += 5;
						na -> na_tset = (u_short) i;

						if (*cp)
							LLOG (addr_log, LLOG_EXCEPTIONS,
								  ("extra TCP information: %s", nsap));
					}
				}
				break;

			default:
				LLOG (addr_log, LLOG_NOTICE,
					  ("unknown syntax %d for DSP: %s", ts -> ts_syntax,
					   nsap));
				goto realNS;
			}
		} else {
realNS:
			;
			na -> na_stack = NA_NSAP;
			na -> na_community = SUBNET_REALNS;
unrealNS:
			;
			if (len > sizeof na -> na_address) {
				LLOG (addr_log, LLOG_EXCEPTIONS,
					  ("NSAP address too long: %d octets", len));
				return NOTOK;
			}
			bcopy (p, na -> na_address, na -> na_addrlen = len);
		}
	}

	return OK;
}
