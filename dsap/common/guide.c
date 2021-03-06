/* guide.c - Search Guide handling */

#ifndef lint
static char *rcsid = "$Header: /xtel/isode/isode/dsap/common/RCS/guide.c,v 9.0 1992/06/16 12:12:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/dsap/common/RCS/guide.c,v 9.0 1992/06/16 12:12:39 isode Rel $
 *
 *
 * $Log: guide.c,v $
 * Revision 9.0  1992/06/16  12:12:39  isode
 * Release 8.0
 *
 */

/*
 *                                NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */

/*
	SYNTAX:
		Guide ::= [<objectclass> '#'] <Criteria>
		Criteria ::= CriteriaItem | CriteriaSet | '!' Criteria
		CrtieriaSet ::= ['('] Criteria '@' CriteriaSet [')'] |
				['('] Criteria '|' CriteriaSet [')']
		CriteriaItem ::= ['('] <attributetype> '$' <matchType> [')']
		matchType ::= "EQ" | "SUBSTR" | "GE" | "LE" | "APPROX"

		NadfGuide ::= <objectclass> '#' <Criteria> '#' <subset>
		subset ::= "baseObject" | "oneLevel" | "wholeSubtree"


	EXAMPLE:
		Person # commonName $ APPROX
		( organization $ EQ ) @ (commonName $ SUBSTR)
		( organization $ EQ )
			@ ((commonName $ SUBSTR) | (commonName $ EQ))

	NOTE:
		Use of @ for "and" as '&' get filtered out earlier!!!
*/

/* LINTLIBRARY */

#include "quipu/util.h"
#include "quipu/attrvalue.h"
#include "cmd_srch.h"
#include "quipu/syntaxes.h"

static free_CriteriaItem ();

static
Criteria_free (struct Criteria *arg) {
	struct Criteria *parm = arg;

	if (parm == NULL)
		return;

	switch (parm -> offset) {
	case Criteria_type:
		if (parm -> un.type)
			free_CriteriaItem (parm -> un.type),
							  parm -> un.type = NULL;
		break;

	case Criteria_and:
	case Criteria_or: {
		struct and_or_set *and_or_set;

		for (and_or_set = parm -> un.and_or; and_or_set;) {
			struct and_or_set *f_and_or_set = and_or_set -> and_or_next;

			if (and_or_set -> and_or_comp)
				Criteria_free (and_or_set -> and_or_comp),
							  and_or_set -> and_or_comp = NULL;

			if (and_or_set)
				free ((char *) and_or_set);
			and_or_set = f_and_or_set;
		}

		parm -> un.and_or = NULL;
	}
	break;

	case Criteria_not:
		if (parm -> un.not)
			Criteria_free (parm -> un.not),
						  parm -> un.not = NULL;
		break;
	}

	free ((char *) arg);
}

static
free_CriteriaItem (struct CriteriaItem *arg) {
	struct CriteriaItem *parm = arg;

	if (parm == NULL)
		return;

	free ((char *) arg);
}


static
guidefree (struct Guide *arg) {
	if (arg == NULL)
		return;

	/* will always be present if nadfGuide... */
	if (arg->objectClass)
		oid_free (arg->objectClass);

	Criteria_free (arg->criteria);

	free ((char *)arg);
}

static struct CriteriaItem *
CriteriaItem_cpy (struct CriteriaItem *arg) {
	struct CriteriaItem *parm = arg;
	struct CriteriaItem *res;

	if (parm == NULL)
		return NULL;

	res = (struct CriteriaItem *) smalloc (sizeof(struct CriteriaItem));

	res->offset = parm->offset;
	res->attrib = AttrT_cpy (parm->attrib);

	return (res);
}


static struct Criteria *
Criteria_cpy (struct Criteria *a) {
	struct Criteria *b;

	if (a == NULL)
		return NULL;

	b = (struct Criteria *) smalloc (sizeof(struct Criteria));
	b-> offset = a->offset;

	switch (a -> offset) {
	case Criteria_type:
		b->un.type = CriteriaItem_cpy (a->un.type);
		break;

	case Criteria_and:
	case Criteria_or: {
		struct and_or_set *and_or_set;
		struct and_or_set *ao_res = (struct and_or_set *)NULL;
		struct and_or_set *ao_tmp;

		ao_tmp = ao_res; /* OK lint ? */

		for (and_or_set = a -> un.and_or; and_or_set; and_or_set = and_or_set->and_or_next) {
			struct and_or_set *tmp;

			tmp = (struct and_or_set *) smalloc (sizeof(struct and_or_set));
			tmp->and_or_comp = Criteria_cpy(and_or_set->and_or_comp);
			tmp->and_or_next = (struct and_or_set *)NULL;
			if (ao_res == ((struct and_or_set *)NULL))
				ao_res = tmp;
			else
				ao_tmp->and_or_next = tmp;
			ao_tmp = tmp;
		}

		b->un.and_or = ao_res;
	}
	break;

	case Criteria_not:
		b->un.not = Criteria_cpy (a->un.not);
		break;
	}

	return (b);
}

static struct Guide *
guidecpy (struct Guide *a) {
	struct Guide * b;

	b = (struct Guide * ) smalloc (sizeof(struct Guide));

	if (a->objectClass)
		b->objectClass = oid_cpy (a->objectClass);
	else
		b->objectClass = NULLOID;

	b->criteria = Criteria_cpy (a->criteria);

	b -> subset = a -> subset;

	return (b);
}

#define NOCHOICE 255

static CMD_TABLE guide_tab [] = {
	"EQ",		choice_equality,
	"SUBSTR",	choice_substrings,
	"GE",		choice_greaterOrEqual,
	"LE",		choice_lessOrEqual,
	"APPROX",	choice_approximateMatch,
	0,		NOCHOICE
};

static struct CriteriaItem *
CriteriaItem_parse (char *str) {
	struct CriteriaItem * res;
	char * ptr;
	if ((str == NULLCP) || (*str == 0))
		return ((struct CriteriaItem *) NULL);

	res = (struct CriteriaItem *) smalloc (sizeof(struct CriteriaItem));

	if ((ptr = index (str,'$')) == NULLCP) {
		parse_error ("Seperator missing in CriteriaItem %s",str);
		return ((struct CriteriaItem *) NULL);
	}
	*ptr-- = 0;
	if (isspace (*ptr))
		*ptr = 0;
	ptr++;
	if ((res -> attrib = AttrT_new (str)) == NULLAttrT) {
		parse_error ("Unknown attribute type in CriteriaItem %s",str);
		return ((struct CriteriaItem *) NULL);
	}
	*ptr++ = '$';

	if ((res -> offset = cmd_srch(SkipSpace(ptr),guide_tab)) == NOCHOICE) {
		parse_error ("Unknown search type in CriteriaItem %s",ptr);
		return ((struct CriteriaItem *) NULL);
	}

	return (res);

}



static
getop (char *str, char *ch) {
	int             i,
					bracket = 0;

	for (i = 0; i < ((int)strlen (str)); i++) {
		if (bracket == 0 && (str[i] == '@' || str[i] == '|')) {
			*ch = str[i];
			return (i);
		}
		if (str[i] == '(')
			++bracket;
		if (str[i] == ')')
			--bracket;
		if (bracket < 0) {
			parse_error ("Too many close brackets",NULLCP);
			return (-2);
		}
	}
	return (-1);
}


static struct Criteria *
Criteria_parse (char *str) {
	int             gotit,
					bracketed;
	char            ch,
					och = '\0';
	struct Criteria *  result;

	struct and_or_set  * ao = (struct and_or_set *)NULL;
	struct and_or_set  * ao_ptr;

	result = (struct Criteria *) smalloc (sizeof(struct Criteria));

	FAST_TIDY(str);

	/* Got a multiple-component string for parsing */

	do {
		bracketed = FALSE;
		if ((gotit = getop (str, &ch)) == -2)
			return ((struct Criteria *)NULL);

		if (gotit < 0) {/* Match an open bracket. */
			if (*str == '(')
				if (str[strlen (str) - 1] == ')') {
					str[strlen (str) - 1] = '\0';
					++str;
					bracketed = TRUE;
				} else {
					parse_error ("Too many open brackets",NULLCP);
					return ((struct Criteria *)NULL);
				}

			if (och == '\0') {
				if (bracketed == TRUE) {
					gotit = 0;	/* Stop 'while' loop
							 * falling */
					continue;	/* Parse the internals */
				} else
					break;	/* Single item only */
			} else
				ch = och;	/* Use last operation */
		}
		if (och == '\0')/* Remember last operation */
			och = ch;
		else if (och != ch) {
			parse_error ("Can't Mix Operations.",NULLCP);
			return ((struct Criteria *)NULL);
		}
		if (gotit >= 0)	/* If got an op, make it null */
			str[gotit] = '\0';
		/* Recurse on the 'first' string */
		ao_ptr = (struct and_or_set*) smalloc (sizeof(struct and_or_set));
		if ((ao_ptr->and_or_comp = Criteria_parse (str)) == (struct Criteria *)NULL)
			return ((struct Criteria *)NULL);

		ao_ptr->and_or_next = (struct and_or_set*) NULL;
		if (ao != (struct and_or_set *)NULL)
			ao_ptr->and_or_next = ao;
		ao = ao_ptr;
		str += gotit + 1;

		if (gotit >= 0) {	/* Match an and symbol */
			if (och == '@') {
				result->offset = Criteria_and;
			} else {/* Match an or symbol */
				result->offset = Criteria_or;
			}
		}
		result->un.and_or = ao;
	} while (gotit >= 0);
	if (och == '\0') {
		if (*str == '!') {	/* Match a not symbol */
			result->offset = Criteria_not;
			if ((result->un.not = Criteria_parse (str + 1)) == (struct Criteria *)NULL)
				return ((struct Criteria *)NULL);
		} else {
			result->offset = Criteria_type;
			if ((result->un.type = CriteriaItem_parse(str)) == (struct CriteriaItem *)NULL)
				return ((struct Criteria *)NULL);
		}
	}
	return (result);
}

static struct Guide *
guideparse (char *str) {
	char *ptr;
	struct Guide * res;

	res = (struct Guide *) smalloc (sizeof (struct Guide));
	res -> subset = -1;

	if ((ptr = index (str,'#')) != NULLCP) {
		*ptr-- = 0;
		if (isspace (*ptr))
			*ptr = 0;
		ptr++;
		if (( res->objectClass = name2oid(str)) == NULLOID) {
			parse_error ("Unknown class in Guide %s",str);
			guidefree (res);
			return ((struct Guide *)NULL);
		}
		*ptr++ = '#';
		str = ptr;
	} else
		res->objectClass = NULLOID;

	if ((res->criteria = Criteria_parse (SkipSpace(str)))
			== (struct Criteria *)NULL) {
		guidefree (res);
		return ((struct Guide *)NULL);
	}

	return (res);
}

static CMD_TABLE subset_tab[] = {
	"BASEOBJECT",	0,
	"ONELEVEL",		1,
	"WHOLESUBTREE",	2,
	NULL,	       -1
};

static struct Guide *
nadfparse (char *str) {
	char   *ptr1,
		   *ptr2;
	struct Guide *res;

	if (!(ptr1 = index (str, '#'))) {
		parse_error ("missing class in NADF Guide %s", str);
		return NULL;
	}
	if (!(ptr2 = index (ptr1 + 1, '#'))) {
		parse_error ("missing subset in NADF Guide %s", str);
		return NULL;
	}
	*ptr2 = NULL;
	res = guideparse (str);
	*ptr2 = '#';
	if (!res)
		return NULL;
	if ((res -> subset = cmd_srch (SkipSpace (ptr2 + 1), subset_tab)) == -1) {
		parse_error ("Unknown subset type in NADF Guide %s", str);
		guidefree (res);
		return NULL;
	}

	return res;
}


static CriteriaItem_print(ps,parm,format)
PS ps;
struct CriteriaItem * parm;
int format;
{
	char *ptr;

	if (parm == NULL)
		return;

	if ((ptr = rcmd_srch ((int)parm->offset,guide_tab)) == NULLCP)
		ptr = "UNKNOWN !!!";

	if (format == READOUT) {
		ps_printf (ps,"%s on ",ptr);
		AttrT_print (ps,parm->attrib,EDBOUT);
	} else {
		AttrT_print (ps,parm->attrib,format);
		ps_printf (ps,"$%s",ptr);
	}


}


static Criteria_print (ps,a,format)
PS ps;
struct Criteria * a;
int format;
{
	char * sep;

	if (format == READOUT)
		sep = " OR ";
	else
		sep = "|";

	if (a == NULL)
		return;

	switch (a -> offset) {
	case Criteria_type:
		CriteriaItem_print (ps,a->un.type,format);
		break;
	case Criteria_and:
		if (format == READOUT)
			sep = " AND ";
		else
			sep = "@";
	case Criteria_or: {
		struct and_or_set *and_or_set;
		char * tmp = NULLCP;

		for (and_or_set = a -> un.and_or; and_or_set; and_or_set = and_or_set->and_or_next) {
			if (tmp != NULLCP)
				ps_print (ps,tmp);
			ps_print (ps,"(");
			Criteria_print(ps,and_or_set->and_or_comp,format);
			ps_print (ps,")");
			tmp = sep;
		}
	}
	break;

	case Criteria_not:
		if (format == READOUT)
			ps_print (ps,"NOT ");
		else
			ps_print (ps,"!");
		ps_print (ps,"(");
		Criteria_print (ps,a->un.not,format);
		ps_print (ps,")");
		break;
	}
}

static guideprint (ps,a,format)
PS ps;
struct Guide * a;
int format;
{
	if (a->objectClass) {
		if (format == READOUT) {
			ps_print (ps,"Class: ");
			oidprint (ps,a->objectClass,format);
			ps_print (ps,", search for: ");
		} else {
			oidprint (ps,a->objectClass,format);
			ps_print (ps," # ");
		}
	}

	Criteria_print (ps,a->criteria,format);

	if (a -> subset >= 0) {
		char *ptr = rcmd_srch (a -> subset, subset_tab);

		ps_printf (ps, format == READOUT ? " using %s search" : " # %s",
				   ptr ? ptr : "UNKNOWN !!!");
	}
}

static PE guideenc (m)
struct Guide * m;
{
	PE ret_pe;

	encode_SA_Guide (&ret_pe,0,0,NULLCP,m);

	return (ret_pe);
}

static struct Guide * guidedec (pe)
PE pe;
{
	struct Guide * m;

	if (decode_SA_Guide (pe,1,NULLIP,NULLVP,&m) == NOTOK)
		return ((struct Guide *) NULL);
	m -> subset = -1;

	return (m);
}

static PE nadfenc (m)
struct Guide * m;
{
	PE ret_pe;

	encode_SA_NadfGuide (&ret_pe,0,0,NULLCP,m);

	return (ret_pe);
}

static struct Guide * nadfdec (pe)
PE pe;
{
	struct Guide * m;

	if (decode_SA_NadfGuide (pe,1,NULLIP,NULLVP,&m) == NOTOK)
		return ((struct Guide *) NULL);

	return (m);
}

static
criteriaItem_cmp (struct CriteriaItem *a, struct CriteriaItem *b) {

	if (a == NULL)
		return (b==NULL ? 0 : -1);
	if (b == NULL)
		return (1);

	if (a->offset != b->offset)
		return (a->offset>b->offset ? 1 : -1);

	return (AttrT_cmp (a->attrib,b->attrib));
}

static
criteria_cmp (struct Criteria *a, struct Criteria *b) {
	int result;

	if (a==NULL)
		return(b==NULL ? 0 : -1);
	if (b==NULL)
		return(1);

	if (a->offset != b->offset)
		return (a->offset > b->offset ? 1 : -1);

	switch (a -> offset) {
	case Criteria_type:
		return (criteriaItem_cmp (a->un.type, b->un.type));

	case Criteria_and:
	case Criteria_or: {
		struct and_or_set *a_set;
		struct and_or_set *b_set;

		for (a_set = a->un.and_or; a_set ; a_set = a_set->and_or_next) {
			for (b_set = b->un.and_or; b_set ; b_set = b_set->and_or_next) {
				if ((result=criteria_cmp (a_set -> and_or_comp,b_set->and_or_comp)) == 0)
					break;
			}
			if (result != 0)
				return (1);
		}
		for (b_set = b->un.and_or; b_set ; b_set = b_set->and_or_next) {
			for (a_set = a->un.and_or; a_set ; a_set = a_set->and_or_next) {
				if ((result=criteria_cmp (a_set -> and_or_comp,b_set->and_or_comp)) == 0)
					break;
			}
			if (result != 0)
				return (-1);
		}
		return (0);

	}

	case Criteria_not:
		result = criteria_cmp (a->un.not,b->un.not);
		break;
	}

	return (result);
}

static
guidecmp (struct Guide *a, struct Guide *b) {
	int i;
	if (a == (struct Guide *)NULL)
		if (b == (struct Guide *)NULL)
			return (0);
		else
			return (1);

	if (b==(struct Guide *)NULL)
		return (-1);

	if ((i=oid_cmp(a->objectClass,b->objectClass)) == 0
			&& (i = criteria_cmp(a->criteria,b->criteria)) == 0) {
		if ((i = b -> subset - a -> subset) > 0)
			i = 1;
		else if (i < 0)
			i = -1;
	}

	return (i);
}

int
guide_syntax (void) {
	add_attribute_syntax ("Guide",
						  (IFP) guideenc,	(IFP) guidedec,
						  (IFP) guideparse,guideprint,
						  (IFP) guidecpy,	guidecmp,
						  guidefree,	NULLCP,
						  NULLIFP,	TRUE);

	add_attribute_syntax ("NadfGuide",
						  (IFP) nadfenc,	(IFP) nadfdec,
						  (IFP) nadfparse,guideprint,
						  (IFP) guidecpy,	guidecmp,
						  guidefree,	NULLCP,
						  NULLIFP,	TRUE);

}

