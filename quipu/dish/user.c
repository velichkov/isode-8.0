/* user.c - */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/quipu/dish/RCS/user.c,v 9.0 1992/06/16 12:35:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/quipu/dish/RCS/user.c,v 9.0 1992/06/16 12:35:39 isode Rel $
 *
 *
 * $Log: user.c,v $
 * Revision 9.0  1992/06/16  12:35:39  isode
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


#include "quipu/util.h"
#include "quipu/read.h"
#include "quipu/sequence.h"

extern struct dua_sequence * current_sequence;
extern struct dua_sequence * top_sequence;

#include "isoaddrs.h"

#define	OPT	(!frompipe || rps -> ps_byteno == 0 ? opt : rps)
#define	RPS	(!frompipe || opt -> ps_byteno == 0 ? rps : opt)
extern	char	frompipe;
extern	PS	opt, rps;

extern  char    fred_flag;

static new_alias ();

int
call_ds (int argc, char **argv) {
	extern char bound;
	extern char * myname;
	extern char * dsa_address;
	extern char * isodeversion;
	extern char * dsapversion;
	extern char * dishversion;
	extern DN 	fixed_pos;
	extern DN 	user_name;
	extern struct PSAPaddr dsa_bound;

	fred_flag = FALSE;

	if (argc > 1 && test_arg (argv[1], "-fred", 4)) {
		fred_flag = TRUE;
		argc--, argv++;
	}

	if (argc > 1) {
		if (test_arg (argv[1],"-sequence",1)) {
			show_sequence (RPS,argv[2],fred_flag);
			return;
		} else if (test_arg (argv[1], "-alias", 1)) {
			if (argc > 2)
				new_alias (argv[2]);
			else
				Usage (argv[0]);
			return;
		} else if (test_arg (argv[1], "-version", 1)) {
			ps_printf (RPS,"ISODE version %s\n",isodeversion);
			ps_printf (RPS,"DSAP  version %s\n",dsapversion);
			ps_printf (RPS,"DISH  version %s\n",dishversion);
			return;
		} else if (test_arg (argv[1], "-user", 1)) {
			if (fred_flag && user_name)
				ufn_dn_print_aux (RPS, user_name, NULLDN, 0);
			else
				dn_print (RPS, user_name, EDBOUT);
			ps_print (RPS, "\n");
			return;
		} else if (test_arg (argv[1], "-syntax", 2)) {
			int i;
			char * syntax2str();
			for (i=1; i<MAX_AV_SYNTAX; i++) {
				if (syntax2str(i) == NULLCP)
					return;
				ps_printf (RPS, "%s\n",syntax2str(i));
			}
			return;
		} else {
			Usage (argv[0]);
			return;
		}
	}

	if (bound) {
		ps_printf (RPS, "Connected to ");
		if (strcmp (myname, dsa_address))
			ps_printf (RPS, "%s at ", myname);
		ps_printf (RPS, "%s\n", pa2str (&dsa_bound));
	} else
		ps_print (RPS,"Not connected to a DSA (cache exists)\n");
	ps_print (RPS,"Current position: ");
	if (fred_flag && fixed_pos)
		ufn_dn_print_aux (RPS, fixed_pos, NULLDN, 0);
	else
		ps_print (RPS, "@"), dn_print (RPS, fixed_pos, EDBOUT);
	ps_print (RPS, "\nUser name: ");
	if (fred_flag && user_name)
		ufn_dn_print_aux (RPS, user_name, NULLDN, 0);
	else
		ps_print (RPS, "@"), dn_print (RPS, user_name, EDBOUT);
	ps_print (RPS, "\n");
	if (current_sequence != NULL_DS)
		ps_printf (RPS,"Current sequence: %s\n",current_sequence->ds_name);
	if (frompipe)
		ps_printf (RPS, "DAP-listener: %s\n", getenv ("DISHPROC"));
}


/*  */

static
new_alias (char *cp) {
	int	    seqno;
	DN	    sdn;

	if ((sdn = str2dn (*cp != '@' ? cp : cp + 1)) == NULLDN) {
		ps_printf (OPT, "Invalid DN for alias: %s\n", cp);
		return;
	}

	set_sequence ("default");
	if (seqno = add_sequence (sdn)) {
		ps_printf (RPS, "%-3d ", seqno);
		if (fred_flag && sdn)
			ufn_dn_print_aux (RPS, sdn, NULLDN, 0);
		else
			ps_print (RPS, "@"), dn_print (RPS, sdn, EDBOUT);
		ps_print (RPS, "\n");
	}
}

dish_error (ps,error)
PS ps;
struct DSError * error;
{
	struct access_point * ap;
	extern char neverefer;
	extern int chase_flag;

	if (error->dse_type == DSE_ABANDONED) {
		ps_print (ps,"(DAP call interrupted - abandon successful)\n");
		return (0);
	}

	if (error->dse_type == DSE_ABANDON_FAILED) {
		ps_print (ps,"(DAP call interrupted - abandon unsuccessful)\n");
		return (0);
	}

	if (error->dse_type == DSE_INTRERROR) {
		ps_print (ps,"(DAP call interrupted)\n");
		return (0);
	}

	if ((error->dse_type != DSE_REFERRAL)
			|| ((chase_flag == 0) && neverefer)
			|| (chase_flag == 1)) {
		ds_error (ps,error);
		return (0);
	}


	if (error->ERR_REFERRAL.DSE_ref_candidates == NULLCONTINUATIONREF) {
		ps_print (ps,"*** Referral error (but no reference !!!) ***\n");
		return (0);
	}

	for (ap = error->ERR_REFERRAL.DSE_ref_candidates->cr_accesspoints;
			ap != NULLACCESSPOINT; ap=ap->ap_next) {

		if (chase_flag != 2) {
			ps_print (ps,"Reference to another DSA for '");
			ufn_dn_print_aux (ps,
							  error->ERR_REFERRAL.DSE_ref_candidates->cr_name,
							  NULLDN, 0);
			ps_print (ps,"Chase reference to the DSA '");
			ufn_dn_print_aux (ps,ap->ap_name, NULLDN, 0);

			if (!yesno ("' ? ") == FALSE)
				continue;
		} else if (!frompipe) {
			ps_print (ps,"Reference to another DSA ('");
			ufn_dn_print (ps,ap->ap_name, 0);
			ps_print (ps,"') for '");
			ufn_dn_print (ps,
						  error->ERR_REFERRAL.DSE_ref_candidates->cr_name,0);
			ps_print (ps,"'...\n");
			ps_flush (ps);
		}

		if (referral_bind (ap->ap_address) != 0)
			return (1);

		if (chase_flag == 2)
			break; /* only try first - otherwise possible looping */
	}
	return (0);
}
