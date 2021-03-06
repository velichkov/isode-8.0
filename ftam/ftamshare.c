/* ftamshare.c - FPM: encode/decode shared ASE information */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/ftam/RCS/ftamshare.c,v 9.0 1992/06/16 12:14:55 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/ftam/RCS/ftamshare.c,v 9.0 1992/06/16 12:14:55 isode Rel $
 *
 *
 * $Log: ftamshare.c,v $
 * Revision 9.0  1992/06/16  12:14:55  isode
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
#include "fpkt.h"

/*  */

struct type_FTAM_Shared__ASE__Information *shared2fpm (fsb, sharedASE, fti)
struct ftamblk *fsb;
PE	sharedASE;
struct FTAMindication *fti;
{
	struct type_FTAM_Shared__ASE__Information *fpm;

	if ((fpm = (struct type_FTAM_Shared__ASE__Information *)
			   calloc (1, sizeof *fpm)) == NULL) {
		ftamlose (fti, FS_GEN (fsb), 1, NULLCP, "out of memory");
		if (fpm)
			free_FTAM_Shared__ASE__Information (fpm);
		return NULL;
	}

	fpm -> indirect__reference = sharedASE -> pe_context;
	fpm -> encoding -> offset = choice_UNIV_0_single__ASN1__type;
	(fpm -> encoding -> un.single__ASN1__type = sharedASE) -> pe_refcnt++;

	return fpm;
}

/*  */

int	fpm2shared (fsb, fpm, sharedASE, fti)
struct ftamblk *fsb;
struct type_FTAM_Shared__ASE__Information *fpm;
PE    *sharedASE;
struct FTAMindication *fti;
{
	PE	    pe;

	if (fpm -> encoding -> offset != choice_UNIV_0_single__ASN1__type)
		return ftamlose (fti, FS_GEN (fsb), 1, NULLCP,
						 "shared ASE information not single-ASN1-type");

	if ((pe = pe_cpy (fpm -> encoding -> un.single__ASN1__type)) == NULLPE)
		ftamlose (fti, FS_GEN (fsb), 1, NULLCP, "out of memory");
	(*sharedASE = pe) -> pe_context = fpm -> indirect__reference;

	return OK;
}
