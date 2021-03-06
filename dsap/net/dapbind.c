/* dapbind.c - Establish directory association */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/dsap/net/RCS/dapbind.c,v 9.0 1992/06/16 12:14:05 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/dsap/net/RCS/dapbind.c,v 9.0 1992/06/16 12:14:05 isode Rel $
 *
 *
 * $Log: dapbind.c,v $
 * Revision 9.0  1992/06/16  12:14:05  isode
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


/* LINTLIBRARY */

#include "quipu/util.h"
#include "quipu/oid.h"
#include "quipu/dap2.h"
#include "../x500as/DAS-types.h"

extern  LLog    * log_dsap;
extern  int       dsap_ad;           /* Association descriptor */
extern  int       dsap_id;           /* Last id sent */
extern  char    * dsa_address;  /* address of default dsa */
struct PSAPaddr   dsa_bound;

static char * qlocalhost = "DAP";	/* DAP bind speed up */

int
ds_bind (struct ds_bind_arg *arg, struct ds_bind_error *error, struct ds_bind_arg *result) {
	/* reverse compatability for bind */
	arg->dba_auth_type = DBA_AUTH_SIMPLE;
	arg->dba_time1 = NULLCP;
	arg->dba_time2 = NULLCP;

	return (secure_ds_bind (arg,error,result));
}

int
secure_ds_bind (struct ds_bind_arg *arg, struct ds_bind_error *error, struct ds_bind_arg *result) {
	struct PSAPaddr             *addr;

	if (!dsa_address || ((addr = str2paddr (dsa_address)) == NULLPA)) {
		error->dbe_version = DBA_VERSION_V1988;
		error->dbe_type = DBE_TYPE_SERVICE;
		error->dbe_value = DSE_SV_INVALIDREFERENCE;
		if (dsa_address) {
			LLOG(log_dsap, LLOG_EXCEPTIONS, ("invalid name %s",dsa_address));
			sprintf (error->dbe_data,"invalid name %s",dsa_address);
			error->dbe_cc = strlen (error->dbe_data);
		} else {
			LLOG(log_dsap, LLOG_EXCEPTIONS, ("NULL address"));
			sprintf (error->dbe_data,"NULL address");
			error->dbe_cc = strlen (error->dbe_data);
		}
		return (DS_ERROR_LOCAL);
	}

	return(dap_bind(&(dsap_ad), arg, error, result, addr));
}

int
dap_bind (int *ad, struct ds_bind_arg *arg, struct ds_bind_error *error, struct ds_bind_arg *result, struct PSAPaddr *addr) {
	int				  ret;
	struct DAPconnect         dc_s;
	struct DAPconnect         *dc = &dc_s;
	struct DAPindication      di_s;
	struct DAPindication      *di = &di_s;

	DLOG(log_dsap, LLOG_DEBUG, ("dap_bind: Address: %s", paddr2str(addr,NULLNA)));
	bzero ((char *)dc, sizeof dc_s);
	error->dbe_cc = 0;

	ret = DapAsynBindRequest ( addr, arg, dc, di, ROS_SYNC);

	if (ret == NOTOK) {
		*error = dc->dc_un.dc_bind_err;
		return (DS_ERROR_CONNECT);
	}

	if (dc->dc_result == DC_RESULT) {

		dsa_bound = dc->dc_connect.acc_connect.pc_responding;	/* struct copy */

		(*ad) = dc->dc_sd;

		*result = dc->dc_un.dc_bind_res; /* struct copy */
		dc->dc_un.dc_bind_res.dba_dn = NULLDN; /* stop freeing it ! */

		DCFREE(dc);
		return (DS_OK);
	}

	if (dc->dc_result == DC_ERROR) {
		*error = dc->dc_un.dc_bind_err;

		DCFREE(dc);
		return (DS_X500_ERROR);
	}

	return (DS_ERROR_CONNECT);

}

/* 	DAP-BIND.REQUEST */

/* ARGSUSED */

int	  DapAsynBindReqAux (callingtitle, calledtitle, callingaddr,
						 calledaddr, prequirements, srequirements, isn, settings,
						 sf, bindarg, qos, dc, di, async)
AEI			  callingtitle;
AEI			  calledtitle;
struct PSAPaddr		* callingaddr;
struct PSAPaddr		* calledaddr;
int			  prequirements;
int			  srequirements;
long			  isn;
int			  settings;
struct SSAPref		* sf;
struct ds_bind_arg	* bindarg;
struct QOStype		* qos;
struct DAPconnect	* dc;
struct DAPindication	* di;
int			  async;
{
	int			  result;
	OID			  app_ctx;
	struct PSAPctxlist	  pc_s;
	struct PSAPctxlist	* pc = &(pc_s);
	OID			  def_ctx;
	PE			  bindargpe;
	struct RoNOTindication	  rni_s;
	struct RoNOTindication	* rni = &(rni_s);
	struct AcSAPconnect	* acc = &(dc->dc_connect);
	OID                         acse_pci;

	/*
	* Set application-context, presentation context definition list,
	* and default-context.
	*/

	DLOG(log_dsap, LLOG_TRACE, ("DapAsynBindReqAux():"));

	if((acse_pci = oid_cpy(DIR_ACSE)) == NULLOID) {
		LLOG(log_dsap, LLOG_EXCEPTIONS, ("acse pci version 1 OID not found"));

		dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
		dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
		dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
		sprintf (dc->dc_un.dc_bind_err.dbe_data,"acse pci version 1 OID not found");
		dc->dc_un.dc_bind_err.dbe_cc = strlen (dc->dc_un.dc_bind_err.dbe_data);

		return (NOTOK);
	}

	app_ctx = oid_cpy (DIR_ACCESS_AC);

#ifdef USE_DEFAULT_CTX
	def_ctx = oid_cpy (DIR_ACCESS_AS);
#else
	def_ctx = NULLOID;
#endif

	pc->pc_nctx = 2;
	pc->pc_ctx[0].pc_id = DIR_ACCESS_PC_ID;
	pc->pc_ctx[0].pc_asn = oid_cpy(DIR_ACCESS_AS);
	pc->pc_ctx[0].pc_atn = NULLOID;
	pc->pc_ctx[1].pc_id = DIR_ACSE_PC_ID;
	pc->pc_ctx[1].pc_asn = acse_pci;
	pc->pc_ctx[1].pc_atn = NULLOID;


	/* Encode Bind Argument */
	if (encode_DAS_DirectoryBindArgument (&bindargpe, 1, 0, NULLCP, bindarg) != OK) {
		dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
		dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
		dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
		sprintf (dc->dc_un.dc_bind_err.dbe_data,"encoding bind args failed");
		dc->dc_un.dc_bind_err.dbe_cc = strlen (dc->dc_un.dc_bind_err.dbe_data);

		return (daplose (di, DA_ARG_ENC, NULLCP, "DAP BIND REQUEST"));
	}
	bindargpe->pe_context = DIR_ACCESS_PC_ID;

#ifdef PDU_DUMP
	pdu_dump (bindargpe,"arg",100);
#endif
	result = RoAsynBindRequest (app_ctx, callingtitle, calledtitle,
								callingaddr, calledaddr, pc, def_ctx, prequirements,
								srequirements, isn, settings, sf, bindargpe, qos,
								acc, rni, async);

	if (bindargpe != NULLPE)
		pe_free (bindargpe);

	if (result == NOTOK) {
		dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
		dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
		dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
		dc->dc_un.dc_bind_err.dbe_cc = rni->rni_cc;
		bcopy (rni->rni_data,dc->dc_un.dc_bind_err.dbe_data,rni->rni_cc);
		return (ronot2daplose (di, "DAP-BIND.REQUEST", rni));
	}

	/* Set the connection identifier */
	dc->dc_sd = acc->acc_sd;

	if (((!async) && (result == OK)) || (async && (result == DONE))) {
		/*
		* Check the type of return and attempt to decode user data.
		*/
		if (DapBindDecode (acc, dc, rni) != OK) {
			return (daplose (di, DA_RES_DEC, NULLCP, "DAP BIND REQUEST"));
		}
	}

	return (result);
}

int
DapAsynBindRequest (struct PSAPaddr *calledaddr, struct ds_bind_arg *bindarg, struct DAPconnect *dc, struct DAPindication *di, int async) {
	struct SSAPref		  sf_s;
	struct SSAPref		* sf = &(sf_s);
	struct QOStype		  qos;

	if (qlocalhost == NULLCP)
		qlocalhost = PLocalHostName();

	if ((sf = addr2ref (qlocalhost)) == NULL) {
		sf = &sf_s;
		bzero ((char *) sf, sizeof *sf);
	}

	bzero ((char *) &qos, sizeof qos);
	qos.qos_sversion = NOTOK;	/* Negotiate highest session */
	qos.qos_maxtime = NOTOK;	/* No time out specified */

	return (DapAsynBindReqAux (NULLAEI, NULLAEI, NULLPA, calledaddr,
							   PR_MYREQUIRE, ROS_MYREQUIRE, SERIAL_NONE, 0, sf,
							   bindarg, &qos, dc, di, async));
}

int
DapAsynBindRetry (int sd, int do_next_nsap, struct DAPconnect *dc, struct DAPindication *di) {
	int			  result;
	struct RoNOTindication	  rni_s;
	struct RoNOTindication	* rni = &(rni_s);
	struct AcSAPconnect	* acc = &(dc->dc_connect);

	result = RoAsynBindRetry (sd, do_next_nsap, acc, rni);

	if (result == NOTOK) {
		return (ronot2daplose (di, "DAP-BIND.RETRY", rni));
	}

	if (result == DONE) {
		if (DapBindDecode (acc, dc, rni) != OK) {
			return (daplose (di, DA_RES_DEC, NULLCP, "DAP BIND RETRY"));
		}
	}

	return (result);
}

int
DapBindDecode (struct AcSAPconnect *acc, struct DAPconnect *dc, struct RoNOTindication *rni) {
	struct ds_bind_arg  * bind_res;
	struct ds_bind_error        * bind_err;

	switch(acc->acc_result) {
	case ACS_ACCEPT:
		DLOG(log_dsap, LLOG_NOTICE, ("DapBindDecode ACCEPT"));
		if ((acc->acc_ninfo == 1) && (acc->acc_info[0] != NULLPE)) {
#ifdef PDU_DUMP
			pdu_dump (acc->acc_info[0],"res",100);
#endif
			if(decode_DAS_DirectoryBindResult(acc->acc_info[0], 1, NULLIP, NULLVP, &bind_res) != OK) {
				LLOG (log_dsap,LLOG_EXCEPTIONS,( "Unable to parse DirectoryBindResult"));
				dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
				dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
				dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
				sprintf (dc->dc_un.dc_bind_err.dbe_data,"decoding bind result failed");
				dc->dc_un.dc_bind_err.dbe_cc = strlen (dc->dc_un.dc_bind_err.dbe_data);
				dc->dc_result = DC_REJECT;
				return (NOTOK);
			}
			dc->dc_result = DC_RESULT;
			dc->dc_un.dc_bind_res = *bind_res; /* struct copy */
			free ((char *)bind_res);
		} else {
			LLOG(log_dsap, LLOG_EXCEPTIONS, ("No DirectoryBindResult"));
			dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
			dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
			dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
			sprintf (dc->dc_un.dc_bind_err.dbe_data,"bind result missing");
			dc->dc_un.dc_bind_err.dbe_cc = strlen (dc->dc_un.dc_bind_err.dbe_data);
			dc->dc_result = DC_REJECT;
			return (NOTOK);
		}
		break;

	case ACS_PERMANENT:
		/*
		* Get the DirectoryBindError
		*/
		DLOG(log_dsap, LLOG_NOTICE, ("DapBindDecode PERMANENT"));
		if ((acc->acc_ninfo == 1) && (acc->acc_info[0] != NULLPE)) {
#ifdef PDU_DUMP
			pdu_dump (acc->acc_info[0],"err",100);
#endif
			if(decode_DAS_DirectoryBindError(acc->acc_info[0], 1, NULLIP, NULLVP, &bind_err) != OK) {
				dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
				dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
				dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
				sprintf (dc->dc_un.dc_bind_err.dbe_data,"decoding bind error failed");
				dc->dc_un.dc_bind_err.dbe_cc = strlen (dc->dc_un.dc_bind_err.dbe_data);
				LLOG(log_dsap, LLOG_EXCEPTIONS, ("Unable to decode DirectoryBindError"));
				dc->dc_result = DC_REJECT;
				return (NOTOK);
			}
			dc->dc_result = DC_ERROR;
			dc->dc_un.dc_bind_err = *bind_err;  /* struct copy */
			free ((char *)bind_err);
		} else {
			dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
			dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
			dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
			sprintf (dc->dc_un.dc_bind_err.dbe_data,"no bind error");
			dc->dc_un.dc_bind_err.dbe_cc = strlen (dc->dc_un.dc_bind_err.dbe_data);
			LLOG(log_dsap, LLOG_EXCEPTIONS, ("No DirectoryBindError"));
			dc->dc_result = DC_REJECT;
			return (NOTOK);
		}
		break;

	default:
		LLOG (log_dsap,LLOG_EXCEPTIONS,( "Association rejected: [%s]",
										 AcErrString(acc->acc_result)));

		dc->dc_un.dc_bind_err.dbe_version = DBA_VERSION_V1988;
		dc->dc_un.dc_bind_err.dbe_type = DBE_TYPE_SERVICE;
		dc->dc_un.dc_bind_err.dbe_value = DSE_SV_UNAVAILABLE;
		dc->dc_un.dc_bind_err.dbe_cc = rni->rni_cc;
		bcopy (rni->rni_data,dc->dc_un.dc_bind_err.dbe_data,rni->rni_cc);
		dc->dc_result = DC_REJECT;


		return (NOTOK);

	} /* switch acc->acc_result */

	return(OK);
}

