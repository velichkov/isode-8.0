/* sunos4-1.h - site configuration file for SunOS release 4.1 */

/*
 * $Header: /xtel/isode/isode/config/RCS/sunos4-1.h,v 9.0 1992/06/16 12:08:13 isode Rel $
 *
 *
 * $Log: sunos4-1.h,v $
 * Revision 9.0  1992/06/16  12:08:13  isode
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


#ifndef	_CONFIG_
#define	_CONFIG_

#define	BSD42			/* Berkeley UNIX */
#define	SUNOS4			/*   with Sun's enhancements */
#define	SUNOS41			/*     SunOS 4.1 */
#define	WRITEV			/*   (sort of) real Berkeley UNIX */
#define	BSD43			/*   4.3BSD or later */

#define	VSPRINTF		/* has vprintf(3s) routines */

#define	TCP			/* has TCP/IP (of course) */
#define	SOCKETS			/*   provided by sockets */

#define	GETDENTS		/* has getdirent(2) call */
#define	NFS			/* network filesystem -- has getdirentries */
#define TEMPNAM                 /* has tempnam (3) call */
#endif
