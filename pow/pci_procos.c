static char __attribute__ ((unused)) rcsid[] = "$Id: pci_procos.c,v 1.4 2011/06/30 13:30:17 lewis Exp $";
/* Property code for module POW-V   
   Started  05-OCT-92 by W.HEINZE              
   Modified 24-FEB-93 correction in STAQ property code
   Modified 16-APR-93 SETA added
   Modified 07-DEC-94 double ppm added
   Modified 13-DEC-95 added property DBPULS for double batch
   Modified 19-JUN-96 aquisition in PPM for non PPM power supplies
   Modified 26-MAY-97 modify STAQ property for PSB main power supply
   Modified 24-FEB-99 only allows setting corresponding to treatment code,
		      2nd acquisition unit, hwmn and hwmx (read from G64) added
   Modified 03-AUG-99 multiple power converters added
   Modified 09-NOV-99 STAQ modified for PSB MPS
   Modified 18-APR-01 Hardware min/max values can be overwritten with an EM call
   Modified 01-MAR-05 Adding multiple acquisitons per cycle
   Modified 04-JUL-06 Adding CCVTRM=8 for non-PPM double-batch property (CNGS)
   Modified 26-APR-07 Add proco r03ctlstmp forcing ctlstamp=0 for slaves
   Modified 17-FEB-09 correct bug in Actuation for multiple supplies (loop)

   Note: the macro sproco(pname, pname_dtr, EqmVal)       
         is expanded to following sequence:             

   procname(dtr,value,size,membno,plsline,coco)         

     pname_dtr *dtr      copy of table fields read by proco
     EqmVal    *value    proco values of type int or double
     int       size      number of elements in valar       
     int       membno    local member number [1..n]        
     int       plsline   pls line number                   
     int       *coco     complement code returned by proco 
     int       bls_num   block serial number

 * Got rid of host/network conversion and used generic host quick data io with all in host byte order
 * Now the new driver is used. The LIBRARY for quick data converts host structures to a form that
 * a power convertor understands. For other divices I will add the support when needed.
 * Julian Lewis Tue 28th June 2011

*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <string.h>

#include <gm/gm_def.h>
#include <gm/eqp_def.h>
#include <gm/proco_header.h>
#include <gm/access.h>

#define TYPE    1
#define SUB_FAMILY 0
#define NOL (struct quick_data_buffer *) 0

#define UPW(a) ((a >> 16) & 0x0ffff)
#define LOW(a) (a & 0x0ffff)

#include "libquick.h"
#include "pow_messages.h"

static int mil1533_init_done = 0;
static int mil1533_fh = (-1);
static short send_quick_data (struct quick_data_buffer *p)
{
    short cc;
    if (mil1533_init_done == 0) {
	if ((mil1533_fh = mil1553_init_quickdriver ()) < 0) {
	    perror ("mil1553_init_quickdriver");
	    return (-1);
	}
	mil1533_init_done = 1;
    }
//    printf("\n=========\nsend_quick_data:Before\n");
//    mil1553_print_msg(p,0,-1);
    cc = mil1553_send_quick_data(mil1533_fh, p);
//    printf("send_quick_data:cc:%d:After\n",(int) cc);
//    mil1553_print_msg(p,0,-1);
    return cc;
}

static short get_quick_data (struct quick_data_buffer *p)
{
    short cc;
    if (mil1533_init_done == 0) {
	if ((mil1533_fh = mil1553_init_quickdriver ()) < 0) {
	    perror ("mil1553_init_quickdriver");
	    return (-1);
	}
	mil1533_init_done = 1;
    }
//    printf("\n=========\nget_quick_data:Before\n");
//    mil1553_print_msg(p,1,-1);
    cc = mil1553_get_quick_data(mil1533_fh, p);
//    printf("get_quick_data:cc:%d:After\n",(int) cc);
//    mil1553_print_msg(p,1,-1);
    return cc;
}

/*====================================================*/
/* Return current time in ms for measurements         */
/*====================================================*/
static unsigned long GetCurrentTimeMsec (void)
{
    struct timeval tt;

    gettimeofday (&tt, NULL);
    return (tt.tv_sec * 1000 + (tt.tv_usec / 1000));
}				/* end of GetCurrentTimeMsec() */


/*****************************************************************************
   subroutine for PPM aquisition for non PPM power supplies
******************************************************************************/

static int getaq (int bls_num, int membno, int plsline, int *val, int *bffrp) /* variables for PPM acqu.*/
{
	int             puls, compl, el_arr[2], compl_arr[2];
	data_array      data_desc;

	/* read AQN data column directly for the corresponding pls line */

	/* Routine gm_getpulse can only be used if no multimachine EM */
	compl = gm_getpulse (bls_num, membno, plsline, FALSE, &puls);

	/* The following routine (find internal PPM line from EM plsline) */
	/* gives puls = 0 for non ppm equipment                           */
	/*  el_arr[0] = 1; el_arr[1] = membno;                            */
	/*  compl_arr[0] = compl_arr[1] = puls = 0;                       */
	/*  plstopuls (bls_num, el_arr, plsline, &puls, compl_arr);       */
	/*  compl = compl_arr[0];                                         */

	if (compl != 0)
		return (compl);

	data_desc.data = val;
	data_desc.type = EQM_TYP_INT;
	data_desc.size = 11;
	el_arr[0] = 1;
	el_arr[1] = membno;
	compl_arr[0] = 0;
	compl_arr[1] = 0;
	eqm_dtr_iv (bls_num, EQP_AQ, &data_desc, 11, el_arr, 2, -puls, compl_arr, 2);
	if (compl_arr[0] != 0)
		return (compl_arr[0]);

	data_desc.data = bffrp;
	data_desc.size = 1;
	compl_arr[0] = 0;
	compl_arr[1] = 0;
	eqm_dtr_iv (bls_num, EQP_BFFR, &data_desc, 1, el_arr, 2, -puls, compl_arr, 2);
	return (compl_arr[0]);
}

/*****************************************************************************
   subroutine for updating actuation for multiple power supplies - 17-FEB-09
******************************************************************************/

static int update_act (int bls_num, int membno, int valact) /* eq to update & actuation to store*/
{
	int  el_arr[2], compl_arr[2], actarr[6];
	data_array  data_desc;
	nonppm_ctrl_msg *dtrs1;
    
	data_desc.data = actarr;
	data_desc.type = EQM_TYP_INT;
	data_desc.size = 6;
	el_arr[0] = 1;
	el_arr[1] = membno;
	compl_arr[0] = 0;
	compl_arr[1] = 0;
	eqm_dtr_iv (bls_num, EQP_CCAC, &data_desc, 6, el_arr, 2, 0, compl_arr, 2);
	if (compl_arr[0] != 0)
		return (compl_arr[0]);

	/*now update actuation & change flag*/
        dtrs1 = (nonppm_ctrl_msg *) actarr;
	dtrs1->ccsact = valact;
	dtrs1->ccsact_change = 1;
	compl_arr[0] = 0;
	compl_arr[1] = 0;
	eqm_dtw_iv (bls_num, EQP_CCAC, &data_desc, 6, el_arr, 2, 0, compl_arr, 2);
	return (compl_arr[0]);
}
/*****************************************************************************
   subroutine for getting next element of multiple power supplies  17-FEB-09
   returns locelno or 0 if error
******************************************************************************/

static int get_nextmult (int bls_num, int membno) /* locelno */
{
	int  el_arr[2], compl_arr[2], master, nloc;
	data_array  data_desc;
    
	data_desc.data = &master;
	data_desc.type = EQM_TYP_INT;
	data_desc.size = 1;
	el_arr[0] = 1;
	el_arr[1] = membno;
	compl_arr[0] = 0;
	compl_arr[1] = 0;
	eqm_dtr_iv (bls_num, EQP_ELMSTR, &data_desc, 1, el_arr, 2, 0, compl_arr, 2);
	if (compl_arr[0] != 0)
		return (0);
	nloc = get_locelno(bls_num, master);
	return (nloc);
}

/*****************************************************************************
R03ARRA1               Reads all acquisitions for MAPCs               
*****************************************************************************/

typedef struct {
	double  aq_ac[16];      /* RW  Buffer for multiple acquisitions         */
	int     namm;           /* RW  Buffer for number of acquisitions        */
	int     it_start;       /* RO  Interrupt number to start multiple acq.  */
	int     cnnt;           /* RO  Connection column                        */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} r03arra1_dtr; 

sproco(r03arra1,r03arra1_dtr,double)
{
	int       i;

	/* check if power supply is a MAPC */
	if ((dtr->it_start == 0) || (dtr->elmstr != 0) || (dtr->plsdbl != 0)) {
		*coco = EQP_ENPROPILL;
		return;
	}

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}
    
	/* check if namm has acceptable value */
	if (dtr->namm > 16)
		return;
    
	/* transfer the acquired values to the output */
	for (i = 0; i < dtr->namm; i++)
		value[i] = dtr->aq_ac[i];
}

/*****************************************************************************
R03NMEAS               Reads number of acquisitions done (for MAPCs)               
*****************************************************************************/

typedef struct {
	int     namm;           /* RW  Buffer for number of acquisitions        */
	int     it_start;       /* RO  Interrupt number to start multiple acq.  */
	int     cnnt;           /* RO  Connection column                        */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} r03nmeas_dtr; 

sproco(r03nmeas,r03nmeas_dtr,int)
{
	/* check if power supply is a MAPC */
	if ((dtr->it_start == 0) || (dtr->elmstr != 0) || (dtr->plsdbl != 0)) {
		*coco = EQP_ENPROPILL;
		return;
	}

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}
    
	/* transfer the value output */
	*value = dtr->namm;
}

/*****************************************************************************
R03MEAS              General proco reading calculated acq.values (for MAPCs)               
*****************************************************************************/

typedef struct {
	double  meas;           /* RW  Buffer for calculated value              */
	int     it_start;       /* RO  Interrupt number to start multiple acq.  */
	int     cnnt;           /* RO  Connection column                        */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} r03meas_dtr; 

sproco(r03meas,r03meas_dtr,double)
{
	/* check if power supply is a MAPC */
	if ((dtr->it_start == 0) || (dtr->elmstr != 0) || (dtr->plsdbl != 0)) {
		*coco = EQP_ENPROPILL;
		return;
	}

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}
    
	/* transfer the value output */
	*value = dtr->meas;
}

/*****************************************************************************
R03BAV                Reads complete acquisiton protocol                
*****************************************************************************/

typedef struct {
	int     aq[11];         /* RW  Acquisition protocol                     */
	int     cnnt;           /* RO  Connection column                        */
	int     bffr;           /* RO  Error column for transmission error      */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
	int     trm;            /* RO  Treatment code                           */
} r03bav_dtr; 

sproco(r03bav,r03bav_dtr,double)
{
	acq_msg *dtrs;          /* give dtrs same structure as acq_msg */
	int      val[11];       /* for PPM acquisition of non PPM power s. */
	int      bffrv;         /* error code also read from PPM column */
	int      aqnmsb;        /* ms bit of acquisition treatment code */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master */
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) { /* this is slave power supply */
		eqm_svse(EQP_POW, value, EQM_TYP_DOUBLE, RFLAG, dtr->elmstr, EQP_BUFACT, plsline, coco);
		return;
	}

	aqnmsb = (dtr->trm >> 8) & 0x8;     /* msb of aqntrm indicating ppm acq */
	if (aqnmsb == 0) {
		bffrv = dtr->bffr;
		dtrs = (acq_msg *) &(dtr->aq[0]);
	} else {         /* read AQN data column for the corresponding pls line */
		*coco = getaq (bls_num, membno, plsline, val, &bffrv);
		if (*coco != 0)
			return;
		dtrs = (acq_msg *) &(val[0]);   /* overlay data with acq_msg */
	}

	/* check if there was a wrong transfer from G64 */
	if (bffrv != 0)
		*coco = bffrv;       /* error in communication */
	if ((bffrv == 0) && ((dtrs->service == (1)) || (dtrs->service == (5))))
		*coco = EQP_SERVICERR;
	if ((bffrv == 0) && (dtrs->service != 0) && (*coco != EQP_SERVICERR))
		*coco = EQP_BADBUF;

	/* copy the message content into the output (value) */
	value[0] = (double) (dtrs->family);
	value[1] = (double) dtrs->type;
	value[2] = (double) dtrs->sub_family;
	value[3] = (double) (dtrs->member);
	value[4] = (double) (dtrs->service);
	value[5] = (double) (dtrs->cycle.machine);
	value[6] = (double) (dtrs->cycle.pls_line);
	value[7] = (double) (dtrs->protocol_date.sec);
	value[8] = (double) (dtrs->protocol_date.usec);
	value[9] = (double) (dtrs->specialist);
	value[10] = (double) dtrs->phys_status;
	value[11] = (double) dtrs->static_status;
	value[12] = (double) dtrs->ext_aspect;
	value[13] = (double) dtrs->status_qualif;
	value[14] = (double) (dtrs->busytime);
	value[15] = (dtrs->aqn);
	value[16] = (dtrs->aqn1);
	value[17] = (dtrs->aqn2);
	value[18] = (dtrs->aqn3);
}

/*****************************************************************************
R03BCV                Reads back complete control protocol              
*****************************************************************************/

typedef struct {
	int     ccac[6];        /* RW  Non ppm part of control protocol */
	int     ccva[5];        /* RW  PPM part of control protocol     */
	int     cnnt;           /* RO  Connection column                */
} r03bcv_dtr; 

sproco(r03bcv,r03bcv_dtr,double)
{
	nonppm_ctrl_msg *dtrs1;
	ppm_ctrl_msg    *dtrs2;       /* same structures as messages */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* copy non ppm part */
	dtrs1 = (nonppm_ctrl_msg *) dtr;
	value[0] = (double) (dtrs1->family);
	value[1] = (double) dtrs1->type;
	value[2] = (double) dtrs1->sub_family;
	value[3] = (double) (dtrs1->member);
	value[4] = (double) (dtrs1->service);
	value[5] = (double) (dtrs1->cycle.machine);
	value[6] = (double) (dtrs1->cycle.pls_line);
	value[7] = (double) (dtrs1->protocol_date.sec);
	value[8] = (double) (dtrs1->protocol_date.usec);
	value[9] = (double) (dtrs1->specialist);
	value[10] = (double) dtrs1->ccsact_change;
	value[11] = (double) dtrs1->ccsact;

	/* copy ppm part */
	dtrs2 = (ppm_ctrl_msg *) &(dtr->ccva[0]);
	value[12] = (dtrs2->ccv);
	value[13] = (dtrs2->ccv1);
	value[14] = (dtrs2->ccv2);
	value[15] = (dtrs2->ccv3);
	value[16] = (double) dtrs2->ccv_change;
	value[17] = (double) dtrs2->ccv1_change;
	value[18] = (double) dtrs2->ccv2_change;
	value[19] = (double) dtrs2->ccv3_change;
}

/*****************************************************************************
R03BUFV               Reads back control parameters as received by G64  
*****************************************************************************/

typedef struct {
	int     address1;       /* RO  BC/ RTI logical address          */
	int     cnnt;           /* RO  Connection column                */
} r03bufv_dtr; 

sproco(r03bufv,r03bufv_dtr,double)
{
	struct quick_data_buffer    send_buf;
	struct quick_data_buffer   *quickptr_req = &send_buf;
	struct quick_data_buffer    receive_buf;
	struct quick_data_buffer   *quickptr_ctl = &receive_buf;
	req_msg                    *req_ptr;
	ctrl_msg                   *ctrl_ptr;
	unsigned long clk;

	quickptr_req->bc = UPW(dtr->address1)-1; /* BC number */
	quickptr_req->rt = LOW(dtr->address1); /* RT number */
	quickptr_req->stamp = 0;
	quickptr_req->error = 0;
	quickptr_req->pktcnt = sizeof (req_msg);
	quickptr_req->next = NOL;

	/* Initialize request message */
	req_ptr = (req_msg *) &(quickptr_req->pkt);
	req_ptr->family         = (EQP_POW);
	req_ptr->type           = TYPE;
	req_ptr->sub_family     = SUB_FAMILY;
	req_ptr->member         = (membno);
	req_ptr->service        = (1);     /* read back ctrl msg */
	req_ptr->cycle.machine  = (gm_getmachine());
	req_ptr->cycle.pls_line = (plsline);
	req_ptr->specialist     = (0);

	/* Send request message to G64 */
	if (send_quick_data (quickptr_req) != 0) {
		*coco = EQP_QCKDATERR;
		return;
	}
	quickptr_ctl->bc = UPW(dtr->address1)-1; /* BC number */
	quickptr_ctl->rt = LOW(dtr->address1); /* RT number */
	quickptr_ctl->pktcnt = sizeof(ctrl_msg);
	quickptr_ctl->next = NOL;
	clk = GetCurrentTimeMsec() + 100;    /* timeout 100ms */

	/* Wait for the reply message */
	get_quick_data (quickptr_ctl);
	while ((GetCurrentTimeMsec() < clk) && (quickptr_ctl->error != 0)) {
		get_quick_data (quickptr_ctl);
	}
	if (GetCurrentTimeMsec() >= clk) {
		*coco = EQP_TIMOUT;
		return;
	}
	if (quickptr_ctl->error != 0) {
		switch (quickptr_ctl->error) {
		    case BC_not_connected:      *coco = EQP_BCNOTCON; return;
		    case RT_not_connected:      *coco = EQP_RTNOTCON; return;
		    case TB_not_set:            *coco = EQP_TBNOTSET; return;
		    case M1553_error:           *coco = EQP_M1553ERR; return;
		    case Bad_buffer:            *coco = EQP_BADBUF;   return;
		    default:                    *coco = EQP_ILLDTENT; return;
		}
	}

	/* transfer the received data to the EM values */
	ctrl_ptr = (ctrl_msg *) &(quickptr_ctl->pkt);

	/* check if wanted service was delivered */
	if (ctrl_ptr->service != (1))
		*coco = EQP_SERVICERR;

	value[0] = (double) (ctrl_ptr->family);
	value[1] = (double) ctrl_ptr->type;
	value[2] = (double) ctrl_ptr->sub_family;
	value[3] = (double) (ctrl_ptr->member);
	value[4] = (double) (ctrl_ptr->service);
	value[5] = (double) (ctrl_ptr->cycle.machine);
	value[6] = (double) (ctrl_ptr->cycle.pls_line);
	value[7] = (double) (ctrl_ptr->protocol_date.sec);
	value[8] = (double) (ctrl_ptr->protocol_date.usec);
	value[9] = (double) (ctrl_ptr->specialist);
	value[10] = (double) ctrl_ptr->ccsact_change;
	value[11] = (double) ctrl_ptr->ccsact;
	value[12] = (ctrl_ptr->ccv);
	value[13] = (ctrl_ptr->ccv1);
	value[14] = (ctrl_ptr->ccv2);
	value[15] = (ctrl_ptr->ccv3);
	value[16] = (double) ctrl_ptr->ccv_change;
	value[17] = (double) ctrl_ptr->ccv1_change;
	value[18] = (double) ctrl_ptr->ccv2_change;
	value[19] = (double) ctrl_ptr->ccv3_change;
}

/*****************************************************************************
R03CCSAV              Reads actuation from control protocol             
*****************************************************************************/

typedef struct {
	int     ccac[6];        /* RW  Non ppm part of control protocol         */
	int     cnnt;           /* RO  Connection column                        */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} r03ccsav_dtr; 

sproco(r03ccsav,r03ccsav_dtr,int)
{
	nonppm_ctrl_msg *dtrs;     /* dtrs = message structure */
	unsigned char    new_value;

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master */
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) { /* this is slave power supply */
		eqm_svse(EQP_POW, value, EQM_TYP_INT, RFLAG, dtr->elmstr, EQP_CCSACT, plsline, coco);
		return;
	}

	/* read "new" value from message */
	dtrs = (nonppm_ctrl_msg *) dtr;/* overlay */
	new_value = dtrs->ccsact;

	/* transform "new" coding of actuation into "old" one */
	switch (new_value) {
	    case 1: *value = 0; return;             /* off */
	    case 2: *value = 2; return;             /* stand-by */
	    case 3: *value = 1; return;             /* on */
	    case 4: *value = 3; return;             /* reset */
	    case 5: *value = 4; return;             /* ready for CBE */
	    default: *coco = EQP_VOR; return;
	}
}

/*****************************************************************************
R03CCVV               Reads CCV value (Ampere) from POW-V               
*****************************************************************************/

typedef struct {
	int     ccva[5];        /* RO  ppm part of control message              */
	int     cnnt;           /* RO  connect parameter (1=connected, 0=not)   */
	int     elmstr;         /* RO  Mbno. of referenced power supply         */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
	int     casesel;        /* RO  distinguishes the 4 cases CCV..CCV3      */
} r03ccvv_dtr; 

sproco(r03ccvv,r03ccvv_dtr,double)
{
	ppm_ctrl_msg *dtrs;        /* give dtrs structure as message */

	/* return if power supply not connected and not reference power supply */
	if ((dtr->cnnt == 0) && !(dtr->elmstr && dtr->plsdbl != (-1))) {
		*coco = EQP_NOTCON;
		return;
	}

	/* read wanted CCV value from control protocol */
	dtrs = (ppm_ctrl_msg *) dtr;/* overlay */
	switch (dtr->casesel) {
		case 0:
			*value = (dtrs->ccv);
			if (dtrs->ccv_change != 1)
				*coco = EQP_CCLNSET;
			return;
		case 1:
			*value = (dtrs->ccv1);
			if (dtrs->ccv1_change != 1)
				*coco = EQP_CCLNSET;
			return;
		case 2:
			*value = (dtrs->ccv2);
			if (dtrs->ccv2_change != 1)
				*coco = EQP_CCLNSET;
			return;
		case 3:
			*value = (dtrs->ccv3);
			if (dtrs->ccv3_change != 1)
				*coco = EQP_CCLNSET;
			return;
		default:
			*coco = EQP_ILLDTENT;
	}
}

/*****************************************************************************
R03CONFV              Reads the configuration of power supply           
*****************************************************************************/

typedef struct {
	int     address1;       /* RO  BC/ RTI logical address  */
	int     cnnt;           /* RO  Connection column        */
} r03confv_dtr; 

sproco(r03confv,r03confv_dtr,double)
{
	struct quick_data_buffer    send_buf;
	struct quick_data_buffer   *quickptr_req = &send_buf;
	struct quick_data_buffer    receive_buf;
	struct quick_data_buffer   *quickptr_ctl = &receive_buf;
	req_msg                    *req_ptr;
	conf_msg                   *conf_ptr;
	unsigned long clk;

	quickptr_req->bc = UPW(dtr->address1)-1; /* BC number */
	quickptr_req->rt = LOW(dtr->address1); /* RT number */
	quickptr_req->stamp = 0;
	quickptr_req->error = 0;
	quickptr_req->pktcnt = sizeof (req_msg);
	quickptr_req->next = NOL;

	/* Initialize request message */
	req_ptr = (req_msg *) &(quickptr_req->pkt);
	req_ptr->family         = (EQP_POW);
	req_ptr->type           = TYPE;
	req_ptr->sub_family     = SUB_FAMILY;
	req_ptr->member         = (membno);
	req_ptr->service        = (5);     /* read back ctrl msg */
	req_ptr->cycle.machine  = (gm_getmachine());
	req_ptr->cycle.pls_line = (plsline);
	req_ptr->specialist     = (0);

	/* Send request message to G64 */
	if (send_quick_data (quickptr_req) != 0) {
		*coco = EQP_QCKDATERR;
		return;
	}
	quickptr_ctl->bc = UPW(dtr->address1)-1; /* BC number */
	quickptr_ctl->rt = LOW(dtr->address1); /* RT number */
	quickptr_ctl->pktcnt = sizeof(ctrl_msg);
	quickptr_ctl->next = NOL;
	clk = GetCurrentTimeMsec() + 100;    /* timeout 100ms */

	/* Wait for the reply message */
	get_quick_data (quickptr_ctl);
	while ((GetCurrentTimeMsec() < clk) && (quickptr_ctl->error != 0)) {
		get_quick_data (quickptr_ctl);
	}
	if (GetCurrentTimeMsec() >= clk) {
		*coco = EQP_TIMOUT;
		return;
	}
	if (quickptr_ctl->error != 0) {
		switch (quickptr_ctl->error) {
			case BC_not_connected:  *coco = EQP_BCNOTCON; return;
			case RT_not_connected:  *coco = EQP_RTNOTCON; return;
			case TB_not_set:        *coco = EQP_TBNOTSET; return;
			case M1553_error:       *coco = EQP_M1553ERR; return;
			case Bad_buffer:        *coco = EQP_BADBUF;   return;
			default:                *coco = EQP_ILLDTENT; return;
		}
	}

	/* transfer the received data to the EM values */
	conf_ptr = (conf_msg *) &(quickptr_ctl->pkt);

	/* check if wanted service was delivered */
	if (conf_ptr->service != (5))
	    *coco = EQP_SERVICERR;

	value[0] = (double) (conf_ptr->family);
	value[1] = (double) conf_ptr->type;
	value[2] = (double) conf_ptr->sub_family;
	value[3] = (double) (conf_ptr->member);
	value[4] = (double) (conf_ptr->service);
	value[5] = (double) (conf_ptr->cycle.machine);
	value[6] = (double) (conf_ptr->cycle.pls_line);
	value[7] = (double) (conf_ptr->protocol_date.sec);
	value[8] = (double) (conf_ptr->protocol_date.usec);
	value[9] = (double) (conf_ptr->specialist);
	value[10] = (conf_ptr->i_nominal);
	value[11] = (conf_ptr->resolution);
	value[12] = (conf_ptr->i_max);
	value[13] = (conf_ptr->i_min);
	value[14] = (conf_ptr->di_dt);
	value[15] = (conf_ptr->mode);
}

/*****************************************************************************
R03DATEV              Reads date in LynxOS encoded form                 
*****************************************************************************/

typedef struct {
	int     aq[11];         /* RW  Contains acquisition message             */
	int     cnnt;           /* RO  Connection column                        */
	int     bffr;           /* RO  Error column acquisition                 */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
	int     alarmflg;       /* RO  Alarm flag, 1 if surveyed                */
	int     trm;            /* RO  Treatment code                           */
} r03datev_dtr; 

sproco(r03datev,r03datev_dtr,int)
{
	acq_msg *dtrs;          /* give dtrs same structure as acq_msg */
	int      val[11];       /* for PPM acquisition of non PPM power s. */
	int      bffrv;         /* error code also read from PPM column */
	int      aqnmsb;        /* ms bit of acquisition treatment code */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master,
	   for multiple power converters, call next one (elmstr) if alarm flag is 0 */
	if (dtr->elmstr > 0 && (dtr->plsdbl != (-1) || (dtr->plsdbl == (-1) && dtr->alarmflg == 0))) {
		eqm_svse(EQP_POW, value, EQM_TYP_INT, RFLAG, dtr->elmstr, EQP_DATE, plsline, coco);
		return;
	}

	aqnmsb = (dtr->trm >> 8) & 0x8;     /* msb of aqntrm indicating ppm acq */
	if (aqnmsb == 0) {
		bffrv = dtr->bffr;
		dtrs = (acq_msg *) &(dtr->aq[0]);
	} else {         /* read AQN data column for the corresponding pls line */
		*coco = getaq (bls_num, membno, plsline, val, &bffrv);
		if (*coco != 0)
			return;
		dtrs = (acq_msg *) &(val[0]);   /* overlay data with acq_msg */
	}

	/* check if there was a wrong transfer from G64 */
	if (bffrv != 0)
		*coco = bffrv;       /* error in communication */
	if ((bffrv == 0) && ((dtrs->service == (1)) || (dtrs->service == (5))))
		*coco = EQP_SERVICERR;
	if ((bffrv == 0) && (dtrs->service != 0) && (*coco != EQP_SERVICERR))
		*coco = EQP_BADBUF;

	value[0] = (dtrs->protocol_date.sec);
	value[1] = (dtrs->protocol_date.usec);
}

/*****************************************************************************
R03DBPUL              Reads if single (0) or double (1) pulsing         
*****************************************************************************/

typedef struct {
	int     trm;            /* RO  Treatment code                   */
	int     ccva[5];        /* RW  PPM part of control protocol     */
	int     cnnt;           /* RO  Connection column                */
} r03dbpul_dtr; 

sproco(r03dbpul,r03dbpul_dtr,int)
{
	ppm_ctrl_msg *dtrs;     /* dtrs = message structure */
	int aqntrm_msb;         /* aqn treatment code minus ms bit */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}
	aqntrm_msb = (dtr->trm >> 8) & 0x7;
	dtrs = (ppm_ctrl_msg *) &(dtr->ccva[0]);        /* overlay */

	if ((aqntrm_msb == 6) && ((dtrs->ccv1) != 0.))
		*value = 1;
	else
		*value = 0;
}

/*****************************************************************************
R03MNMX               Reads column MN and coco from ERR6                
*****************************************************************************/

typedef struct {
	float   hwmnx;          /* RO  Hardware min/max (float !)               */
	int     erres;          /* RO  Completion code written by RT program    */
	int     cnnt;           /* RO  Connection data column                   */
	int     select;         /* RO  Selects if hwmx(=1) or hwmn(=2) is read  */
	int     elmstr;         /* RO  Element nr. of master power converter    */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} r03mnmx_dtr; 

sproco(r03mnmx,r03mnmx_dtr,double)
{
	int    property;

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}
	/* for slave power supplies, make same operation for corresponding master */
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) { /* this is slave power supply */
		switch (dtr->select)  {   /* decide with which property */
		    case 1: property = EQP_MAXV1; break;
		    case 2: property = EQP_MINV1; break;
		    default: *coco = EQP_ILLDTENT; return;
		}
		eqm_svse(EQP_POW, value, EQM_TYP_DOUBLE, RFLAG, dtr->elmstr, property, plsline, coco);
		return;
	}
	*value = dtr->hwmnx;
	*coco = dtr->erres;
}

/*****************************************************************************
R03STMP               Overwrites the Alarm's one AQNSTAMP property
*****************************************************************************/

typedef struct {
	int     elmstr;         /* RO   Eqipment nr. of slave power supply      */
	int     plsdbl;         /* RO   DBL condition or -1 for multiple p.c.   */
	int     alarmflg;       /* RO   Alarm flag, 1 if surveyed               */
	double  aqstmp;         /* R0   Acquisition stamp PPM                   */
} r03stmp_dtr;

sproco(r03stmp,r03stmp_dtr,double)
{
	/* for slave power supplies, make same operation for corresponding master,
	   for multiple power converters, call next one (elmstr) if alarm flag is 0 */
	if (dtr->elmstr > 0 && (dtr->plsdbl != (-1) || (dtr->plsdbl == (-1) && dtr->alarmflg == 0))) {
		eqm_svse(EQP_POW, value, EQM_TYP_DOUBLE, RFLAG, dtr->elmstr, EQP_AQNSTAMP, plsline, coco);
		return;
	}
	*value = dtr->aqstmp;
}
/*****************************************************************************
R03CTLSTMP               Overwrites the base CTLSTAMP property
*****************************************************************************/

typedef struct {
	int     elmstr;         /* RO   Eqipment nr. of slave power supply      */
	int     plsdbl;         /* RO   DBL condition or -1 for multiple p.c.   */
	double  ctlstmp;        /* R0   Acquisition stamp PPM                   */
} r03ctlstmp_dtr;

sproco(r03ctlstmp,r03ctlstmp_dtr,double)
{
	/* for slave power supplies, force stamp=0  to force reading staq*/
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) /* this is slave power supply */
		*value =0;
	else
		*value = dtr->ctlstmp;
}

/*****************************************************************************
R03RIV                Read one integer value from acquisition protocol  
*****************************************************************************/

typedef struct {
	int     aq[11];         /* RW  Acquisition protocol                     */
	int     cnnt;           /* RO  Connection column                        */
	int     bffr;           /* RO  Error column for transmission error      */
	int     elmstr;         /* RO  Eqipment nr. of slave power supply       */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
	int     alarmflg;       /* RO  Alarm flag, 1 if surveyed                */
	int     casesel;        /* RO  Case selects which value is read         */
	int     trm;            /* RO  Treatment code                           */
} r03riv_dtr; 

sproco(r03riv,r03riv_dtr,int)
{
	acq_msg *dtrs;          /* give dtrs same structure as acq_msg */
	int     property;
	int     val[11];        /* for PPM acquisition of non PPM power s. */
	int     bffrv;          /* error code also read from PPM column */
	int     aqnmsb;         /* ms bit of acquisition treatment code */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master,
	   for multiple power converters, call next one (elmstr) if alarm flag is 0 */
	if (dtr->elmstr > 0 && (dtr->plsdbl != (-1) || (dtr->plsdbl == (-1) && dtr->alarmflg == 0))) {
		switch (dtr->casesel)  {   /* decide with which property */
			case 1: property = EQP_TBIT; break;
			case 2: property = EQP_PSTAT; break;
			case 3: property = EQP_STAQ1; break;
			case 4: property = EQP_ASPEC; break;
			case 5: property = EQP_QUALIF; break;
			case 6: property = EQP_BSYTIM; break;
			default: *coco = EQP_ILLDTENT; return;
		}
		eqm_svse(EQP_POW, value, EQM_TYP_INT, RFLAG, dtr->elmstr, property, plsline, coco);
		return;
	}

	aqnmsb = (dtr->trm >> 8) & 0x8;     /* msb of aqntrm indicating ppm acq */
	if (aqnmsb == 0) {
		bffrv = dtr->bffr;
		dtrs = (acq_msg *) &(dtr->aq[0]);
	} else {         /* read AQN data column for the corresponding pls line */
		*coco = getaq (bls_num, membno, plsline, val, &bffrv);
		if (*coco != 0)
			return;
		dtrs = (acq_msg *) &(val[0]);   /* overlay data with acq_msg */
	}

	/* check if there was a wrong transfer from G64 */
	if (bffrv != 0)  *coco = bffrv;       /* error in communication */
	if ((bffrv == 0) && ((dtrs->service == (1)) || (dtrs->service == (5))))
		*coco = EQP_SERVICERR;
	if ((bffrv == 0) && (dtrs->service != 0) && (*coco != EQP_SERVICERR))
		*coco = EQP_BADBUF;

	/* select value corresponding to case data column */
	switch (dtr->casesel) {
	    case 1:     *value = (dtrs->specialist); return;
	    case 2:     *value = dtrs->phys_status; return;
	    case 3:     *value = dtrs->static_status; return;
	    case 4:     *value = dtrs->ext_aspect; return;
	    case 5:     *value = dtrs->status_qualif; return;
	    case 6:     *value = (dtrs->busytime); return;
	    default:    *coco = EQP_ILLDTENT; return;
	}
}

/*****************************************************************************
R03RRV                Reads real value from acquisition protocol        
*****************************************************************************/

typedef struct {
	int     aq[11];         /* RW  Acquisition protocol                     */
	int     cnnt;           /* RO  Connection column                        */
	int     bffr;           /* RO  Error column for transmission errors     */
	int     elmstr;         /* RO  Equipment nr. of master element          */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
	int     alarmflg;       /* RO  Alarm flag, 1 if surveyed                */
	int     casesel;        /* RO  Case select which value is read          */
	int     trm;            /* RO  Treatment code                           */
} r03rrv_dtr; 

sproco(r03rrv,r03rrv_dtr,double)
{
	acq_msg *dtrs;          /* give dtrs same structure as acq_msg */
	int     property;
	int     val[11];        /* for PPM acquisition of non PPM power s. */
	int     bffrv;          /* error code also read from PPM column */
	int     aqnmsb;         /* ms bit of acquisition treatment code */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master,
	   for multiple power converters, call next one (elmstr) if alarm flag is 0 */
	if (dtr->elmstr > 0 && (dtr->plsdbl != (-1) || (dtr->plsdbl == (-1) && dtr->alarmflg == 0))) {
		switch (dtr->casesel)  {   /* decide with which property */
			case 0: property = EQP_AQN; break;
			case 1: property = EQP_AQN1; break;
			case 2: property = EQP_AQN2; break;
			case 3: property = EQP_AQN3; break;
			default: *coco = EQP_ILLDTENT; return;
		}
		eqm_svse(EQP_POW, value, EQM_TYP_DOUBLE, RFLAG, dtr->elmstr, property, plsline, coco);
		return;
	}

	/* for non ppm power supplies whose acquisition is ppm because of supression of
	   acquisition timing pulses for certain cycles - indicated by treatment code */
	aqnmsb = (dtr->trm >> 8) & 0x8;     /* msb of aqntrm indicating ppm acq */
	if (aqnmsb == 0) {
		bffrv = dtr->bffr;
		dtrs = (acq_msg *) &(dtr->aq[0]);
	} else {         /* read AQN data column for the corresponding pls line */
		*coco = getaq (bls_num, membno, plsline, val, &bffrv);
		if (*coco != 0)
			return;
		dtrs = (acq_msg *) &(val[0]);   /* overlay data with acq_msg */
	}

	/* check if there was a wrong transfer from G64 */
	if (bffrv != 0)
		*coco = bffrv;       /* error in communication */
	if ((bffrv == 0) && ((dtrs->service == (1)) || (dtrs->service == (5))))
		*coco = EQP_SERVICERR;
	if ((bffrv == 0) && (dtrs->service != 0) && (*coco != EQP_SERVICERR))
		*coco = EQP_BADBUF;

	/* select value corresponding to case data column */
	switch (dtr->casesel) {
		case 0: *value = (dtrs->aqn); return;
		case 1: *value = (dtrs->aqn1); return;
		case 2: *value = (dtrs->aqn2); return;
		case 3: *value = (dtrs->aqn3); return;
		default: *coco = EQP_ILLDTENT; return;
	}
}

/*****************************************************************************
R03STAQV              Reads status acquisition byte encoded in old way  
*****************************************************************************/

typedef struct {
	int     aq[11];         /* RW  Acquisition protocol                     */
	int     cnnt;           /* RO  Connection column                        */
	int     bffr;           /* RO  Error column acquisition                 */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
	int     alarmflg;       /* RO  Alarm flag, 1 if surveyed                */
	int     trm;            /* RO  Treatment code                           */
} r03staqv_dtr; 

sproco(r03staqv,r03staqv_dtr,int)
{
	unsigned char   ph_st,  /* physical status */
			st_st,  /* statis status */
			ex_as,  /* external aspects */
			st_qu;  /* status qualifier */
	unsigned short  staq;   /* status word acquired with STAQ property */
	acq_msg *dtrs;          /* give dtrs same structure as acq_msg */
	int      val[11];       /* for PPM acquisition of non PPM power s. */
	int      bffrv;         /* error code also read from PPM column */
	int      statrm;        /* STATRM is 5 for PSB main power supply */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master,
	   for multiple power converters, call next one (elmstr) if alarm flag is 0 */
	if (dtr->elmstr > 0 && (dtr->plsdbl != (-1) || (dtr->plsdbl == (-1) && dtr->alarmflg == 0))) {
		eqm_svse(EQP_POW, value, EQM_TYP_INT, RFLAG, dtr->elmstr, EQP_STAQ, plsline, coco);
		return;
	}

	/* compose STAQ always from AQ column 0 - no ppm even for ppm power supplies */

	*coco = getaq (bls_num, membno, 0, val, &bffrv);
	if (*coco != 0)
		return;
	dtrs = (acq_msg *) &(val[0]);   /* overlay data with acq_msg in column 0 */

	/* check if there was a wrong transfer from G64 */
	if (bffrv != 0)
		*coco = bffrv;       /* error in communication */
	if ((bffrv == 0) && ((dtrs->service == (1)) || (dtrs->service == (5))))
		*coco = EQP_SERVICERR;
	if ((bffrv == 0) && (dtrs->service != 0) && (*coco != EQP_SERVICERR))
		*coco = EQP_BADBUF;

	ph_st = dtrs->phys_status;
	st_st = dtrs->static_status;
	ex_as = dtrs->ext_aspect;
	st_qu = dtrs->status_qualif;

	/* calculate bit 0 to 7 of staq by combining above mentioned parameters */
	staq = 0;
	if (st_st == 1 || st_st == 2)     staq = st_st;           /* bit0 (off), bit1 (st-by) */
	if (st_st == 3)                   staq |= 4;              /* bit2 (on) */
	if (!(st_qu & 2) && !(st_qu & 4)) staq |= 8;              /* bit3 (no fault) */
	if (ph_st == 1 || ph_st == 2)     staq |= 16;             /* bit4 (up), also if partially operational */
	if (ex_as == 3)                   staq |= 32;             /* bit5 (remote) */
	if (!(st_qu & 16))                staq |= 64;             /* bit6 (no warning) */
	if (!(st_qu & 1))                 staq |= 128;            /* bit7 (no interlock) */

	/* overwrite bits 3, 4, 6, 7 for statrm = 5 (main PSB power supply) */
	statrm = (dtr->trm) & 0xf;
	if (statrm == 5) {
		if (st_qu != 0) staq &= 0xf7;
		if (ph_st == 3) staq &= 0xef; else staq |= 0x10;
		if (ph_st == 2) staq &= 0xbf; else staq |= 0x40;
		if (st_qu & 9)  staq &= 0x7f;
	}
	staq |= st_qu << 8;  /* add complete status qualifier byte to staq */
	*value = (int) staq;
}

/*****************************************************************************
W03CCSAV              Write actuation into control protocol and FUPA    
*****************************************************************************/

typedef struct {
	int     ccac[6];        /* RW  Non ppm part of control protocol         */
	int     trm;            /* RO  Treatment code                           */
	int     cnnt;           /* RO  Connection column                        */
	int     fupa;           /* RW  Contains actual actuation except reset   */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} w03ccsav_dtr; 

sproco(w03ccsav,w03ccsav_dtr,int)
{
	nonppm_ctrl_msg *dtrs;  /* dtrs = message structure */
	ctrl_msg *ctrl;
	unsigned int new_value;
	int ccatrm, fen, tmp,j; /* treatment code for writing actuation */

	struct quick_data_buffer pkt;      /* control buffers */
	data_array              data;
	struct timeval     da;
	int adr1;
	int el[2], co[2];


	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master */
	if (dtr->elmstr > 0 && dtr->plsdbl != (-1)) {/* this is slave power supply */
		eqm_svse(EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_CCSACT, plsline, coco);
		return;
	}


	/* check treatment code for CCSACT control */
	ccatrm = (dtr->trm >> 4) & 0xF;
	if (!ccatrm) {
		*coco = EQP_BADTRM;
		return;
	}

	/* transform old coding of actuation into new one */
	switch (*value) {
		case 0:
			if (ccatrm != 1 && ccatrm != 3) {
				*coco = EQP_VOR;
				return;
			}
			new_value = 1;      /* off */
			break;
		case 1:
			if (ccatrm != 1 && ccatrm != 2 && ccatrm != 3) {
				*coco = EQP_VOR;
				return;
			}
			new_value = 3;      /* on */
			break;
		case 2:
			if (ccatrm != 1 && ccatrm != 2) {
				*coco = EQP_VOR;
				return;
			}
			new_value = 2;      /* stand-by */
			break;
		case 3:
			if (ccatrm != 1 && ccatrm != 2 && ccatrm != 4) {
				*coco = EQP_VOR;
				return;
			}
			new_value = 4;      /* reset */
			break;
		case 4:
			if (ccatrm != 4) {
				*coco = EQP_VOR;
				return;
			}
			new_value = 5;      /* ready for P.S. CBE */
			break;
		default:
			*coco = EQP_VOR;
			return;
	}

	/* write "new" (and verified) actuation into fupa */
	if (new_value != 4)
		dtr->fupa = new_value;

	/* write "new" (and verified) actuation into message */
	dtrs = (nonppm_ctrl_msg *) &dtr->ccac[0];/* overlay */
	dtrs->ccsact = new_value;
	dtrs->ccsact_change = 1;

	/* for multiple power converters */
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) { /* this is slave power supply */
		fen = get_locelno(bls_num, dtr->elmstr); /*get next in list*/
		j=0;
		while ((membno != fen) && (fen > 0) && (j<32)) { /*stop when master is initial elno or err*/
			update_act(bls_num, fen, new_value);  /*write actuation*/
			tmp = get_nextmult(bls_num, fen);
			fen = tmp;
			j++; /*to get out of loop if bad config*/
		};

	}
	/* Write directly OFF to 1553 */
	if (new_value == 1) {
		/* Get BC/RT from address1 column */
		data.data = &adr1;
		data.type = EQM_TYP_INT;
		data.size = 1;
		el[0] = 1;
		el[1] = membno;
		co[0] = co[1] = 0;
		eqm_dtr_iv(bls_num, EQP_ADDRESS1, &data, 1, el, 2, plsline, co, 2);
		if (co[0])
			return;          /* No ADDRESS1 column */

		gettimeofday(&da, NULL);  /* get TOD (Unix format) */
		memset((char *)&pkt, 0, sizeof(struct quick_data_buffer));
		pkt.bc = UPW(adr1)-1;
		pkt.rt = LOW(adr1);
		pkt.stamp = membno;
		pkt.next = NULL;
		ctrl = (ctrl_msg *) &pkt.pkt[0];
		memcpy(ctrl, dtrs, sizeof(nonppm_ctrl_msg));
		ctrl->protocol_date.sec = (da.tv_sec);
		ctrl->protocol_date.usec = (da.tv_usec);

		pkt.pktcnt = sizeof(ctrl_msg);      /* size of control message = 44 */
		if (send_quick_data(&pkt) != 0) {   /* MIL-1553 error (encoded in errno) */
			perror("send_quick_data");
		}
	}
}

/*****************************************************************************
W03CCVV               Writes CCV values (Ampere) into POW-V             
*****************************************************************************/

typedef struct {
	int     ccva[5];        /* RW  ppm part of ctrl message                 */
	int     trm;            /* RO  treatment code                           */
	double  mn;             /* RO  minimum CCV value                        */
	double  mx;             /* RO  maximum CCV value                        */
	double  ilim;           /* RO  inner limit (forbidden range) for CCV    */
	int     cnnt;           /* RO  connect parameter (1=connected, 0=not)   */
	int     elmstr;         /* RO  Mbno. of referenced power supply         */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
	int     casesel;        /* RO  distinguishes the 4 cases of CCV..CCV3   */
} w03ccvv_dtr; 

sproco(w03ccvv,w03ccvv_dtr,double)
{
	ppm_ctrl_msg *dtrs;      /* dtrs = message structure */
	int ccvtrm;              /* treatment code for ccv control */

	/* return if power supply not connected and not reference power supply */
	if ((dtr->cnnt == 0) && !(dtr->elmstr && dtr->plsdbl != (-1))) {
		*coco = EQP_NOTCON;
		return;
	}

	/* check treatment code for CCV control */
	ccvtrm = (dtr->trm >> 12) & 0xF;
	if (!ccvtrm) {
		*coco = EQP_BADTRM;
		return;
	}

	/* check if CCV value is inside limits */
	if (*value < dtr->mn || *value > dtr->mx || (*value < dtr->ilim && *value > -dtr->ilim)) {
		*coco = EQP_LIMERR;
		return;
	}

	/* write value corresponding to case data column */
	dtrs = (ppm_ctrl_msg *) &(dtr->ccva[0]);        /* overlay */
	switch (dtr->casesel) {
		case 0:
			(dtrs->ccv = *value);
			dtrs->ccv_change = 1;
			return;
		case 1:
			if (ccvtrm == 1) {
			   *coco = EQP_ENPROPILL;
			   return;
			}
			(dtrs->ccv1 = *value);
			dtrs->ccv1_change = 1;
			return;
		case 2:
			if (ccvtrm == 1 || ccvtrm == 2 || ccvtrm == 6 || ccvtrm == 8) {
			   *coco = EQP_ENPROPILL;
			   return;
			}
			(dtrs->ccv2 = *value);
			dtrs->ccv2_change = 1;
			return;
		case 3:
			if (ccvtrm == 1 || ccvtrm == 2 || ccvtrm == 3 || ccvtrm == 6 || ccvtrm == 8) {
			   *coco = EQP_ENPROPILL;
			   return;
			}
			(dtrs->ccv3 = *value);
			dtrs->ccv3_change = 1;
			return;
		default:
		    *coco = EQP_ILLDTENT;
	}
}

/*****************************************************************************
W03DBPUL              writes single (0) or double (1) pulsing into p.s. 
*****************************************************************************/

typedef struct {
 int      trm             ; /* RO  Treatment code                           */
 int      ccva[5]         ; /* RW  PPM part of control protocol             */
 int      cnnt            ; /* RO  connection column                        */
} w03dbpul_dtr; 

sproco(w03dbpul,w03dbpul_dtr,int)
{
	ppm_ctrl_msg   *dtrs;   /* dtrs = message structure */
	int             ccvtrm; /* ccv treatment code */
	int             i;
	int             el[2];
	int             co[2];
	ppm_ctrl_msg    dtva;
	data_array      data;


	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}
	ccvtrm = (dtr->trm >> 12) & 0xf;
	dtrs = (ppm_ctrl_msg *) &(dtr->ccva[0]);        /* overlay */

	if ((ccvtrm != 6) && (ccvtrm != 8) && (*value != 0))  {
		*coco = EQP_ENPROPILL;
		return;
	}
	if ((ccvtrm == 6) || (ccvtrm == 8)) {
		(dtrs->ccv1 = (*value == 0) ? 0. : 1.);
		dtrs->ccv1_change = 1;
	}
	if (ccvtrm == 8) {
		data.data = &dtva;
		data.type = EQM_TYP_INT;
		data.size = 5;
		el[0] = 1;
		el[1] = membno;
		co[0] = co[1] = 0;

		/* Non-PPM DBPULS property */
		for (i=0; i<= TgmLINES_IN_GROUP ; i++) {
			eqm_dtr_iv(bls_num, EQP_CCVA, &data, 5, el, 2, -i, co, 2);
			if (co[0] != 0) {
				if (co[1] == EQP_PULSILL)
					break;
				*coco = co[1];
				return;
			}
			(dtva.ccv1 = (*value == 0) ? 0. : 1.);
			dtva.ccv1_change = 1;
			eqm_dtw_iv(bls_num, EQP_CCVA, &data, 5, el, 2, -i, co, 2);
			if (co[0] != 0) {
				*coco=co[1];
				return;
			}
		}
	}
}

/*****************************************************************************
W03INCVV              Increments CCV                                    
*****************************************************************************/

typedef struct {
	int     ccva[5];        /* RW  PPM part of control protocol             */
	int     trm;            /* RO  Treatment code                           */
	double  mn;             /* RO  Minimum CCV value                        */
	double  mx;             /* RO  Maximum CCV value                        */
	double  ilim;           /* RO  Inner limit (forbidden range) for CCV    */
	int     cnnt;           /* RO  Connection column                        */
} w03incvv_dtr; 

sproco(w03incvv,w03incvv_dtr,double)
{
	ppm_ctrl_msg *dtrs;        /* dtrs = message structure */
	double new_value;

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* calculate new CCV value */
	dtrs = (ppm_ctrl_msg *) &(dtr->ccva[0]);        /* overlay */
	new_value = *value + (double) (dtrs->ccv);

	/* check treatment code for CCV control */
	if (!(dtr->trm >> 12) & 0xF) {
		*coco = EQP_BADTRM;
		return;
	}

	/* check if new CCV value is inside limits */
	if (new_value < dtr->mn || new_value > dtr->mx || (new_value < dtr->ilim && new_value > -dtr->ilim)) {
		*coco = EQP_LIMERR;
		return;
	}

	/* write value into ctrl message */
	(dtrs->ccv = new_value);
	dtrs->ccv_change = 1;
}

/*****************************************************************************
W03INITV              Initializes the control protocol for RT program   
*****************************************************************************/

typedef struct {
 int      ccac[6]         ; /* RW  Non ppm part of control protocol         */
 int      trm             ; /* RO  treatment code                           */
} w03initv_dtr; 

sproco(w03initv,w03initv_dtr,int)

{
	nonppm_ctrl_msg *dtrs1;
	ppm_ctrl_msg    *dtrs2;       /* same structures as messages */
	data_array       data_desc;
	int     el_arr[2], compl_arr[2]; /* to access data columns */
	int     ccvtrm;                 /* ccv treatment code */
	int     ccatrm;                 /* cca treatment code */
	int     val[5];                 /* intermediate ccva table */
	int     i, puls;
	int     max_pls_line;

	dtrs1 = (nonppm_ctrl_msg *) &(dtr->ccac[0]);

	/* initialize header */
	ccatrm = (dtr->trm >> 4) & 0xf;

	dtrs1->family = (3);
	dtrs1->type = 1;
	dtrs1->sub_family = 0;
	dtrs1->member = (membno);
	dtrs1->service = (0);
	dtrs1->cycle.machine = (gm_getmachine());
	dtrs1->specialist = (0);

	if (ccatrm == 0) {  /* reset actuation fields */
		dtrs1->ccsact_change = 0;
		dtrs1->ccsact = 0;
	}

	/* initialize zero elements of protocol */
	ccvtrm = (dtr->trm >> 12) & 0xf;

	data_desc.data = val;
	data_desc.type = EQM_TYP_INT;
	data_desc.size = 5;
	el_arr[0] = 1;
	el_arr[1] = membno;

	max_pls_line = rdbls_gtfield (bls_num, PPM_NR);

	for (puls=0; puls<=max_pls_line; ++puls) {    /* for all ppm columns */
		compl_arr[0] = 0;
		compl_arr[1] = 0;
		dtrs2 = (ppm_ctrl_msg *) val;       /* overlay */
		eqm_dtr_iv(bls_num, EQP_CCVA, &data_desc, 5, el_arr, 2, -puls, compl_arr, 2);
		if (compl_arr[0]) {
			*coco =  compl_arr[0];
			return;
		}
		if (ccvtrm == 0) {                  /* reset all ccva */
			for (i=0; i<5; ++i)
				val[i] = 0;
		}
		if ((ccvtrm == 1) || (ccvtrm == 7)) {   /* ccv is valid */
			dtrs2->ccv1 = 0;
			dtrs2->ccv2 = 0;
			dtrs2->ccv3 = 0;
			dtrs2->ccv1_change = 0;
			dtrs2->ccv2_change = 0;
			dtrs2->ccv3_change = 0;
		}
		if (ccvtrm == 2 || ccvtrm == 6 || ccvtrm == 8) {   /* ccv and ccv1 are valid */
			dtrs2->ccv2 = 0;
			dtrs2->ccv3 = 0;
			dtrs2->ccv2_change = 0;
			dtrs2->ccv3_change = 0;
		}
		compl_arr[0] = 0;
		compl_arr[1] = 0;
		eqm_dtw_iv(bls_num, EQP_CCVA, &data_desc, 5, el_arr, 2, -puls, compl_arr, 2);
		if (compl_arr[0]) {
			*coco =  compl_arr[0];
			return;
		}
	}
}

/*****************************************************************************
W03MNMX              Writes column MN and MX
*****************************************************************************/

typedef struct {
 float    hwmnx           ; /* WO  Hardware min/max (float !)               */
 int      erres           ; /* WO  Completion code written by RT program    */
 int      cnnt            ; /* RO  Connection data column                   */
 int      select          ; /* RO  Selects hwmx(=1) or hwmn(=2)             */
 int      elmstr          ; /* RO  Element nr. of master power converter    */
 int      plsdbl          ; /* RO  DBL condition or -1 for multiple p.c.    */
} w03mnmx_dtr;

sproco(w03mnmx,w03mnmx_dtr,double)
{
	int     property;

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}
	/* for slave power supplies, make same operation for corresponding master */
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) { /* this is slave power supply */
		switch (dtr->select)  {   /* decide with which property */
		    case 1: property = EQP_MAXV1; break;
		    case 2: property = EQP_MINV1; break;
		    default: *coco = EQP_ILLDTENT; return;
		}
		eqm_svse(EQP_POW, value, EQM_TYP_DOUBLE, WFLAG, dtr->elmstr, property, plsline, coco);
		return;
	}
	dtr->hwmnx = *value;
	dtr->erres = 0;
}


/*****************************************************************************
W03SETAV              Writes actuation from FUPA into protocol          
*****************************************************************************/

typedef struct {
	int     ccac[6];        /* RW  Non ppm part of control protocol         */
	int     fupa;           /* RO  Actual actuation except reset            */
	int     trm;            /* RO  Treatment code                           */
	int     cnnt;           /* RO  Connection column                        */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} w03setav_dtr; 

sproco(w03setav,w03setav_dtr,int)
{
	nonppm_ctrl_msg *dtrs;  /* dtrs = message structure */
	int fen, tmp, j;

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master */
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) { /* this is slave power supply */
		eqm_svse(EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_SETA, plsline, coco);
		return;
	}

	/* check treatment code for actuation control */
	if (!(dtr->trm >> 4) & 0xF) {
		*coco = EQP_BADTRM;
		return;
	}
	/* write actual actuation from fupa into message */
	dtrs = (nonppm_ctrl_msg *) dtr;/* overlay */
	dtrs->ccsact = dtr->fupa;
	dtrs->ccsact_change = 1;

	/* for multiple power converters */
	if (dtr->elmstr > 0 && dtr->plsdbl == (-1)) {
		fen = get_locelno(bls_num, dtr->elmstr); /*get next in list*/
		j=0;
		while ((membno != fen) && (fen > 0) && (j<32)) { /* stop when master is initial elno or err */
			update_act(bls_num, fen, dtr->fupa);     /* write actuation */
			tmp = get_nextmult(bls_num, fen);
			fen = tmp;
			j++;
		};

	}
}

/*****************************************************************************
W03TES4               Tests if CCV is within allowed limits             
*****************************************************************************/

typedef struct {
 int      trm             ; /* RO  treatment code                           */
 double   mn              ; /* RO  minimum CCV value                        */
 double   mx              ; /* RO  maximum CCV value                        */
 double   ilim            ; /* RO  inner limit (forbidden range)            */
 int      cnnt            ; /* RO  connect parameter                        */
 int      elmstr          ; /* RO  Mbno. of referenced power supply         */
 int      plsdbl          ; /* RO  DBL condition or -1 for multiple p.c.    */
} w03tes4_dtr; 

sproco(w03tes4,w03tes4_dtr,double)
{
	/* return if power supply not connected and not reference power supply */
	if ((dtr->cnnt == 0) && !(dtr->elmstr && dtr->plsdbl != (-1))) {
	    *coco = EQP_NOTCON;
	    return;
	}

	/* check treatment code for CCV control */
	if (!(dtr->trm >> 12) & 0xF) {
	    *coco = EQP_BADTRM;
	    return;
	}

	/* check if CCV value is inside limits */
	if (*value < dtr->mn || *value > dtr->mx || (*value < dtr->ilim && *value > -dtr->ilim)) {
	    *coco = EQP_LIMERR;
	    return;
	}
}

/*****************************************************************************
W03TBITV              Set "Specialist facility" TBIT in control protocol
*****************************************************************************/

typedef struct {
	int     ccac[6];        /* RW  Non ppm part of control protocol         */
	int     cnnt;           /* RO  Connection column                        */
	int     elmstr;         /* RO  Equipment nr. of master power supply     */
	int     plsdbl;         /* RO  DBL condition or -1 for multiple p.c.    */
} w03tbitv_dtr;

sproco(w03tbitv,w03tbitv_dtr,int)
{
	nonppm_ctrl_msg *dtrs;     /* dtrs = message structure */

	/* check if power supply is connected */
	if (dtr->cnnt == 0) {
		*coco = EQP_NOTCON;
		return;
	}

	/* for slave power supplies, make same operation for corresponding master */
	if ((dtr->elmstr > 0) && (dtr->plsdbl != (-1))) { /* this is slave power supply */
		eqm_svse(EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_TBIT, plsline, coco);
		return;
	}

	/* write testbit into non ppm message */
	dtrs = (nonppm_ctrl_msg *) dtr;/* overlay */
	dtrs->specialist = ((short) *value);
}
