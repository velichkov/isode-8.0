/* cstrings.c - */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/dsap/common/RCS/cstrings.c,v 9.0 1992/06/16 12:12:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/dsap/common/RCS/cstrings.c,v 9.0 1992/06/16 12:12:39 isode Rel $
 *
 *
 * $Log: cstrings.c,v $
 * Revision 9.0  1992/06/16  12:12:39  isode
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


#include "psap.h"

static char arg_error [1024];
static char arg_flag [100];
int chase_flag = 2;
extern char * result_sequence;

int
reset_arg (void) {
	arg_error [0] = 0;
	arg_flag [0] = 0;
	chase_flag = 2;
	if (result_sequence)
		free (result_sequence);
	result_sequence = NULLCP;
}

print_arg_error (opt)
PS opt;
{
	if (arg_error [0] != 0) {
		ps_printf (opt,"'%s' ambiguous, specify\n%s",arg_flag,arg_error);
		return (OK);
	} else
		return (NOTOK);
}


int
test_arg (char *x, char *y, int c) {
	int count = 0;
	char * top, *topx;

	top = y;
	topx = x;

	if (*y == '-' )
		count--;

	for (; (*y != 0) || (*x != 0); y++) {
		if (*x == 0)
			if (count >= c)
				return (1);
			else {
				strcat (arg_error, top);
				strcat (arg_error, "\n");
				strcpy (arg_flag,topx);
				return (0);
			}
		if (chrcnv[*x] != chrcnv[*y])
			return (0);

		count++;
		x++;
	}

	if (count >= c)
		return (1);
	else {
		strcat (arg_error, top);
		strcat (arg_error, "\n");
		strcpy (arg_flag, topx);
		return (0);
	}
}

