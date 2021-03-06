/* ds_abandon.c -  */

#ifndef lint
static char *rcsid = "$Header: /xtel/isode/isode/quipu/RCS/ds_abandon.c,v 9.0 1992/06/16 12:34:01 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/quipu/RCS/ds_abandon.c,v 9.0 1992/06/16 12:34:01 isode Rel $
 *
 *
 * $Log: ds_abandon.c,v $
 * Revision 9.0  1992/06/16  12:34:01  isode
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


#include "quipu/util.h"
#include "quipu/attrvalue.h"
#include "quipu/abandon.h"
#include "quipu/dsp.h"
#include "quipu/ds_error.h"
#include "quipu/connection.h"

extern LLog * log_dsap;
/* ARGSUSED */

int
do_ds_abandon (struct ds_abandon_arg *arg, struct DSError *error) {
	DLOG (log_dsap,LLOG_TRACE,("ds_abandon"));

	error->dse_type = DSE_ABANDON_FAILED;
	error->ERR_ABANDON_FAIL.DSE_ab_problem = DSE_AB_CANNOTABANDON;
	error->ERR_ABANDON_FAIL.DSE_ab_invokeid = 0;
	return (NOTOK);
}

int
perform_abandon (struct task_act *tk) {
	struct task_act	* tk_tmp;
	struct task_act	**tk_p;
	int			  ab_id = tk->tk_dx.dx_arg.dca_dsarg.arg_ab.aba_invokeid;
	struct DSError	* err = &(tk->tk_resp.di_error.de_err);

	DLOG(log_dsap, LLOG_TRACE, ("perform_abandon"));

	tk_p = &(tk->tk_conn->cn_tasklist);
	for(tk_tmp = (*tk_p); tk_tmp!=NULLTASK; tk_tmp=tk_tmp->tk_next) {
		if(tk_tmp->tk_dx.dx_id == ab_id)
			break;

		tk_p = &(tk_tmp->tk_next);
	}
	if(tk_tmp == NULLTASK) {
		LLOG(log_dsap, LLOG_NOTICE, ("perform_abandon - cannot find task to abandon"));
		err->dse_type = DSE_ABANDON_FAILED;
		err->ERR_ABANDON_FAIL.DSE_ab_problem = DSE_AB_NOSUCHOPERATION;
		err->ERR_ABANDON_FAIL.DSE_ab_invokeid = ab_id;
		return(NOTOK);
	} else {
		DLOG(log_dsap, LLOG_DEBUG, ("perform_abandon - found task to abandon"));

		/* Slice out task to abandon */
		(*tk_p) = tk_tmp->tk_next;

		if(task_abandon(tk_tmp) != OK) {
			DLOG(log_dsap, LLOG_DEBUG, ("perform_abandon - task_abandon NOTOK"));
			err->dse_type = DSE_ABANDON_FAILED;
			err->ERR_ABANDON_FAIL.DSE_ab_problem = DSE_AB_CANNOTABANDON;
			err->ERR_ABANDON_FAIL.DSE_ab_invokeid = ab_id;
			return(NOTOK);
		} else {
			DLOG(log_dsap, LLOG_DEBUG, ("perform_abandon - task_abandon OK"));
			tk->tk_result = &(tk->tk_resp.di_result.dr_res);
			tk->tk_result->dcr_dsres.result_type = OP_ABANDON;
			return(OK);
		}
	}
}

int
task_abandon (struct task_act *tk) {
	struct oper_act	* on;

	DLOG(log_dsap, LLOG_TRACE, ("task_abandon"));

	for(on = tk->tk_operlist; on != NULLOPER; on = on->on_next_task) {
		on->on_state = ON_ABANDONED;
		on->on_task = NULLTASK;
		if (on->on_dsas) {
			di_desist (on->on_dsas);
			on -> on_dsas = NULL_DI_BLOCK;
		}
	}

	ds_error_free (&tk->tk_resp.di_error.de_err);
	tk->tk_resp.di_error.de_err.dse_type = DSE_ABANDONED;
	task_error(tk);

	return(OK);
}

