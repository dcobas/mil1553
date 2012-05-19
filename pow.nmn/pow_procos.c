static char rcsid[] = "$Id: pow_procos.c,v 1.16 2009/03/12 16:20:52 nmn Exp $";
/* Property code for Equipment Module POW


   Started  16-FEB-90 by HEINZE                
   Modified 11-AUG-91 by Sicard, adapted for DSCs
	    15-JUN-94 by Heinze, adapted for double PPM
	    20-FEB-98 by Heinze, transition state, 15 bit power c., ppm acqisition for
				 non ppm power conv.
	    11-AUG-98 by Heinze, accepts 16 bit unipolar power supplies
	    08-DEC-99 by Heinze, OFF to power conv. not accepting OFF is converted to STBY

   File path: /userb/psco/pow/procos.c                          
   Compile with: make ... to obtain library

   Note: the macro sproco(pname, pname_dtr, EqmVal)       
         is expanded to following sequence:             

   procname(dtr,value,size,membno,plsline,coco,bls-num) 

     pname_dtr *dtr      copy of table fields read by proco
     EqmVal    *value    proco values of type int, double, short, float, char
     int       size      number of elements in valar       
     int       membno    local member number [1..n]        
     int       plsline   pls line number                   
     int       *coco     complement code returned by proco 
     int       bls-num   block-serial number               
*/

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <gm/gm_def.h>
#include <gm/eqp_def.h>
#include <gm/camac_def.h>
#include <gm/proco_header.h>
#include <gm/access.h>

#include <dscrt/quad_functions.h>

/* For actuation control ccsact one needs for CCATRM = 1, 2, 3 */
#define OFF     0xE1
#define ON      0xB4
#define STANDBY 0xD2
#define RESET   0x78

static int dummy = 0;		/*for quad_transf function */

static void my_usleep(int dly)
{
    struct timespec rqtp, rmtp;	/* 'nanosleep' time structure       */
    rqtp.tv_sec = 0;
    rqtp.tv_nsec = dly*1000;
    nanosleep(&rqtp, &rmtp);
}

/*******************************************************************
   subroutine for PPM aquisition for non PPM power supplies
********************************************************************/

/* read AQN data column directly for the corresponding pls line */
static int getaqnd (int bls_num, int membno, int plsline, int *val, int *err)
{
    int puls, compl, el_arr[2], compl_arr[2];
    data_array data_desc;


    /* Routine gm_getpulse can only be used if no multimachine EM */
    if ((compl = gm_getpulse (bls_num, membno, plsline, FALSE, &puls)) != 0)
	return (compl);

    data_desc.data = val;
    data_desc.type = EQM_TYP_INT;
    data_desc.size = 1;
    el_arr[0] = 1;
    el_arr[1] = membno;
    compl_arr[0] = 0;
    compl_arr[1] = 0;
    eqm_dtr_iv (bls_num, EQP_AQ, &data_desc, 1, el_arr, 2, -puls, compl_arr, 2);
    if (compl_arr[0] != 0) return (compl_arr[0]);

    data_desc.data = err;
    compl_arr[0] = 0;
    compl_arr[1] = 0;
    eqm_dtr_iv (bls_num, EQP_ERR2, &data_desc, 1, el_arr, 2, -puls, compl_arr, 2);
    return (compl_arr[0]);
}

/*****************************************************************************
R03AQN                Read CCV from hardware or AQ                      
*****************************************************************************/
/* reads from a connected channel the CCV either directly from
   the hardware (non ppm) or from the data column AQ (ppm). It does the
   necessary transformations to convert the read value into Ampere
 */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int aq;			/* RO  acquisition value                        */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment code                           */
    int grp;			/* RO  ppm                                      */
    double scal1;		/* RO  scaling factor                           */
    int elmstr;			/* RO  master equipment number if slave         */
    int hwmx;			/* RO  maximum amplitude (bit value)            */
} r03aqn_dtr;

sproco (r03aqn, r03aqn_dtr, double)
{

    int trm_code, aqnd, aqnda = 0;
    int aqnmsb;
    int err;

    /* check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_DOUBLE, RFLAG, dtr->elmstr, EQP_AQN, plsline, coco);
	return;
    }

    trm_code = (dtr->trm >> 8) & 0x7;	/* except ms bit */
    if ((trm_code != 1) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    /* read value depending on ppm, non ppm, aqnmsb (ppm acq. for non ppm p.s.) */
    aqnmsb = (dtr->trm >> 8) & 0x8;

    if ((dtr->grp & PPMV_MASK) != 0)   /* ppm */
	aqnd = dtr->aq;
    else {			/* non ppm */
	if (aqnmsb == 0) {
	    *coco = quad_read ((unsigned long)(dtr->camna1 + A1_MASK), &aqnd);
	}
	else {
	    *coco = getaqnd (bls_num, membno, plsline, &aqnd, &err);
	    if (err) *coco = err;
	}
    }
    if (dtr->hwmx <= 0x7FFF) {      /* value not 16 bit unipolar */
	aqnda = aqnd & 0x7FFF;      /* mask out sign bit */
	if (dtr->hwmx <= 0x3FFF)
	    aqnda = aqnd & 0x3FFF;  /* mask out also MS bit */

	if (aqnd & 0x8000)
	    aqnd = -aqnda;
	else
	    aqnd = aqnda;
    }
    /* transform value aqnd into Ampere */
    *value = aqnd * dtr->scal1;
}


/*****************************************************************************
R03AQND               Read D register directly from QUAD                
*****************************************************************************/
/* reads the CCV register D directly from the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int cnnt;			/* RO  connection                               */
} r03aqnd_dtr;

sproco (r03aqnd, r03aqnd_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* read the register D */
    *coco = quad_read ((unsigned long)(dtr->camna1 + A1_MASK), value);
}


/*****************************************************************************
R03CCAD               Reads actuation directly from CCAC                
*****************************************************************************/
/* reads directly from the data column CCAC */

typedef struct
{
    int ccac;			/* RO  actuation column CCAC                    */
    int cnnt;			/* RO  connection                               */
    int elmstr;			/* RO  master equ. nr. if slave                 */
} r03ccad_dtr;

sproco (r03ccad, r03ccad_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, RFLAG, dtr->elmstr, EQP_CCAD, plsline, coco);
	return;
    }
    *value = dtr->ccac;
}


/*****************************************************************************
R03CCVD               Read CCV directly from CCVA                       
*****************************************************************************/
/* reads directly from the data column CCVA */

typedef struct
{
    int ccva;			/* RO  CCV value                                */
    int cnnt;			/* RO  connection                               */
} r03ccvd_dtr;

sproco (r03ccvd, r03ccvd_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    *value = dtr->ccva;
}


/*****************************************************************************
R03CCVX               Read CCV from data column                         
*****************************************************************************/
/* the CCV is read from the corresponding data column and transformed into Ampere */

typedef struct
{
    int ccva;			/* RO  Data column containing CCV               */
    int cnnt;			/* RO  Connect column                           */
    int trm;			/* RO  Treatment code                           */
    int grp;			/* RO  PPM column                               */
    double scal1;		/* RO  Scaling factor                           */
    int elmstr;			/* RO  Eqn. of master for reference power supl. */
    int hwmx;			/* RO  Maximum value in bits                    */
} r03ccvx_dtr;

sproco (r03ccvx, r03ccvx_dtr, double)
{

    int trm_code, aqnd, aqnda;

    /* first check if the power supply is connected */
    if ((dtr->cnnt == 0) && (dtr->elmstr == 0)) { *coco = EQP_NOTCON; return; }

    trm_code = (dtr->trm >> 12) & 0xf;
    if ((trm_code != 1) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    aqnd = dtr->ccva;
    if (dtr->hwmx > 0x7FFF) {	/* no mask, value 16 bit unipolar */
	*value = aqnd * dtr->scal1;
	return;
    }
    aqnda = aqnd & 0x7FFF;	/* mask out sign bit */
    if (aqnd & 0x8000)
	aqnd = -aqnda;
    *value = aqnd * dtr->scal1;	/* transform value aqnd into Ampere */
}


/*****************************************************************************
R03CSAX               Read actuation from data column                   
*****************************************************************************/
/* reads the actuation bit pattern from CCAC and interpretes it */

typedef struct
{
    int ccac;			/* RO  actuation CCAC                           */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment code                           */
    int elmstr;			/* RO  master equ. nr. if slave                 */
} r03csax_dtr;

sproco (r03csax, r03csax_dtr, int)
{

    int trm_code, actuation;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, RFLAG, dtr->elmstr, EQP_CCSACT, plsline, coco);
	return;
    }

    /* check if the treatment code is accepted */
    trm_code = (dtr->trm >> 4) & 0xf;
    if (((trm_code < 1) || (trm_code > 3)) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    /* read pattern */
    actuation = dtr->ccac;
    switch (actuation) {
    case OFF:     *value = 0; break;
    case ON:      *value = 1; break;
    case STANDBY: *value = 2; break;
    case RESET:   *value = 3; break;
    default:      *coco = EQP_VOR;
    }
}


/*****************************************************************************
R03FUPA               Reads actuation pattern from the FUPA buffer      
*****************************************************************************/
/* reads the actuation bit pattern from FUPA and interpretes it */

typedef struct
{
    int fupa;			/* RO  Actuation buffer for temporary storage   */
    int cnnt;			/* RO  Connection                               */
    int trm;			/* RO  Treatment code                           */
    int elmstr;			/* RO  Master equ. nr. if slave                 */
} r03fupa_dtr;

sproco (r03fupa, r03fupa_dtr, int)
{

    int trm_code, actuation;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, RFLAG, dtr->elmstr, EQP_FUPBUF, plsline, coco);
	return;
    }

    /* check if the treatment code is accepted */
    trm_code = (dtr->trm >> 4) & 0xf;
    if (((trm_code < 1) || (trm_code > 3)) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    /* read pattern */
    actuation = dtr->fupa;
    switch (actuation) {
    case OFF:     *value = 0; break;
    case ON:      *value = 1; break;
    case STANDBY: *value = 2; break;
    case RESET:   *value = 3; break;
    default:      *coco = EQP_VOR;
    }
}


/*****************************************************************************
R03SAQD               Read C register from QUAD                         
*****************************************************************************/
/* reads the actuation register C directly from the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int cnnt;			/* RO  connection                               */
} r03saqd_dtr;

sproco (r03saqd, r03saqd_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* read the register C */
    *coco = quad_read ((unsigned long) dtr->camna1, value);
}


/*****************************************************************************
R03STAQ               Read actuation byte directly                      
*****************************************************************************/
/* reads the actuation pattern directly from the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment code                           */
} r03staq_dtr;

sproco (r03staq, r03staq_dtr, int)
{
    int actuation;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* read the actuation and cut off MS byte */
    *coco = quad_read ((unsigned long) dtr->camna1, &actuation);

    /* for BFA power converters (statrm=3), displace 2 bits */
    if ((dtr->trm & 0xF) == 3)
	actuation = (actuation & 0x3f) | ((actuation & 0x300) >> 2);
    *value = actuation & 0xFF;
}


/*****************************************************************************
R03STQ1               Read complete status of QUAD channel              
*****************************************************************************/
/* reads the status of the QUAD - ST channel */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int cnnt;			/* RO  Connection                               */
} r03stq1_dtr;

sproco (r03stq1, r03stq1_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    *coco = quad_status ((unsigned long) dtr->camna1, value);
}


/*****************************************************************************
W03CCAD               write A register of QUAD directly                 
*****************************************************************************/
/* writes directly into CCAC and the actuation register A of the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccac;			/* RW  CCAC column                              */
    int cnnt;			/* RO  connection                               */
    int elmstr;			/* RO  master equ. nr. if slave                 */
} w03ccad_dtr;

sproco (w03ccad, w03ccad_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_CCAD, plsline, coco);
	return;
    }

    dtr->ccac = *value;
    /* write into register A */
    *coco = quad_transf ((unsigned long) dtr->camna1, *value, dummy, TRF_ACT);
}


/*****************************************************************************
W03CCVC               Write CCV in data column and hardware             
*****************************************************************************/
/* This proco w03ccvc checks the size of CCV value, converts  it  into  an
   integer   value   demanded   by   the  hardware,  writes  it  into  the
   corresponding data table ccva and sends  it  to  the  hardware  if  the
   channel is not declared disconnected and there is no ppm */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccva;			/* RW  CCVA data column                         */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment code                           */
    int grp;			/* RO  ppm                                      */
    double scal1;		/* RO  scaling factor                           */
    int hwmx;			/* RO  maximum value in bits                    */
    double mn;			/* RO  minimum in Ampere                        */
    double mx;			/* RO  maximum in Ampere                        */
    double ilim;		/* RO  inner limit in Ampere                    */
    int elmstr;			/* RO  Eqn. of master for reference power supp. */
} w03ccvc_dtr;

sproco (w03ccvc, w03ccvc_dtr, double)
{
    int trm_code, ccvd, ccvda;
    double rval;

    /* first check if the power supply is connected */
    if ((dtr->cnnt == 0) && (dtr->elmstr == 0)) { *coco = EQP_NOTCON; return; }

    /* check if the treatment code is accepted */
    trm_code = (dtr->trm >> 12) & 0xf;
    if ((trm_code != 1) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    /* check if the value is within its allowed limits */
    if ((*value < dtr->mn) || (*value > dtr->mx) || ((*value < dtr->ilim) && (*value > -dtr->ilim))) {
	*coco = EQP_LIMERR; return;
    }

    /* check that scaling factor is not zero and calculate value in bits */
    if (dtr->scal1 == 0) { *coco = EQP_SCLERR; return; }

    rval = *value / dtr->scal1;
    if (rval >= 0) {
	ccvd = rval + 0.5;
	ccvda = ccvd;
    }
    else {
	ccvd = rval - 0.5;
	ccvda = -ccvd;		/* ccvda is absolute value */
    }
    if (ccvd < 0)
	ccvd = ccvda | 0x8000;

    /* store the checked and transformed value into corresponding data column */
    dtr->ccva = ccvd;

    /* return if ppm bit is set (no direct writing into QUAD) or if reference power supply */
    if (((dtr->grp & PPMV_MASK) != 0) || (dtr->cnnt == 0)) return;

    /* now call the QUAD to transfer the CCV */
    *coco = quad_transf ((unsigned long) dtr->camna1, dummy, ccvd, TRF_CCV);
}


/*****************************************************************************
W03CCVD               write B register of QUAD directly                 
*****************************************************************************/
/* writes directly into CCVA and - if no PPM - to register B of the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccva;			/* RW  CCVA column                              */
    int cnnt;			/* RO  connection                               */
    int grp;			/* RO  PPM parameters                           */
} w03ccvd_dtr;

sproco (w03ccvd, w03ccvd_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    dtr->ccva = *value;

    /* return if ppm bit is set (no direct writing into QUAD) */
    if ((dtr->grp & PPMV_MASK) != 0) return;

    /* write into register B */
    *coco = quad_transf ((unsigned long) dtr->camna1, dummy, *value, TRF_CCV);
}


/*****************************************************************************
W03CLEAR              Clear transmission error in STE                   
*****************************************************************************/
/* puts the QUAD into read back mode and so resets a possible transmission
   error in a STE */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int cnnt;			/* RO  connection                               */
} w03clear_dtr;

sproco (w03clear, w03clear_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    *coco = quad_clear ((unsigned long) dtr->camna1);
}


/*****************************************************************************
W03CSAX               Write actuation                                   
*****************************************************************************/
/* writes the actuation bit pattern into the CCAC and FUPA data column and
   into the hardware if, and only if, the channel is connected.  The reset
   actuation, however, is treated differently.  It  is  written  into  the
   hardware  and  after  0.5  sec  the  old actuation contained in FUPA is
   written into CCAC and the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccac;			/* RW  actuation                                */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment                                */
    int fupa;			/* RW  actuation stored in FUPA                 */
    int elmstr;			/* RO  master equ. nr. if slave                 */
} w03csax_dtr;

sproco (w03csax, w03csax_dtr, int)
{
    int trm_code, actuation = OFF;
    time_t dlay3 = 510000;      /*510 millisecond */

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_CCSACT, plsline, coco);
	return;
    }

    /* check if the treatment code is accepted */
    trm_code = (dtr->trm >> 4) & 0xf;
    if ((trm_code == 1) || (trm_code == 7)) {
	switch (*value) {
	case 0: actuation = OFF;     break;
	case 1: actuation = ON;      break;
	case 2: actuation = STANDBY; break;
	case 3: actuation = RESET;   break;
	default:{ *coco = EQP_VOR; return; }
	}
    }
    else if (trm_code == 2) {
	switch (*value) {
	case 0: actuation = STANDBY; break; /* OFF puts power conv. not accepting OFF into STBY */
	case 1: actuation = ON;      break;
	case 2: actuation = STANDBY; break;
	default:{ *coco = EQP_VOR; return; }
	}
    }
    else if (trm_code == 3) {
	switch (*value) {
	case 0: actuation = OFF;   break;
	case 1: actuation = ON;    break;
	case 3: actuation = RESET; break;
	default:{ *coco = EQP_VOR; return; }
	}
    }
    else { *coco = EQP_BADTRM; return; }

    if ((*value >= 0) && (*value <= 2)) {   /* on, off or standby */
	dtr->fupa = actuation;
    }
    else {			/* reset */
	*coco = quad_transf ((unsigned long) dtr->camna1, actuation, dummy, TRF_ACT);
	if (*coco != EQP_NOERR) return;

	my_usleep (dlay3);
	actuation = dtr->fupa;
    }
    dtr->ccac = actuation;
    *coco = quad_transf ((unsigned long) dtr->camna1, actuation, dummy, TRF_ACT);
}


/*****************************************************************************
W03FULL               Write actuation into CCAC and hardware            
*****************************************************************************/
/* writes the OFF pattern into CCAC and the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccac;			/* RW  actuation                                */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment                                */
    int elmstr;			/* RO  master equ. nr. if slave                 */
} w03full_dtr;

sproco (w03full, w03full_dtr, int)
{

    int trm_code;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_FULLST, plsline, coco);
	return;
    }

    /* check if the treatment code is accepted */
    trm_code = (dtr->trm >> 4) & 0xf;
    if ((trm_code == 1) || (trm_code == 3))
	dtr->ccac = OFF;
    else if (trm_code == 2)
	dtr->ccac = STANDBY;
    else if (trm_code != 7) {
	*coco = EQP_BADTRM; return;
    }

    *coco = quad_transf ((unsigned long) dtr->camna1, dtr->ccac, dummy, TRF_ACT);
}


/*****************************************************************************
W03ICCV               Write incremental CCV                             
*****************************************************************************/
/* checks if the incremented CCV value is still in its accepted range  and
   writes it into CCVA and - if no ppm and the channel is connected - also
   into the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccva;			/* RW  CCVA                                     */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment code                           */
    int grp;			/* RO  ppm                                      */
    double scal1;		/* RO  scaling factor                           */
    int hwmx;			/* RO  maximum value in bits                    */
    double mn;			/* RO  minimum value in Ampere                  */
    double mx;			/* RO  maximum value in Ampere                  */
    double ilim;		/* RO  inner limit                              */
} w03iccv_dtr;

sproco (w03iccv, w03iccv_dtr, double)
{
    int aqnd, aqnda, ccvd, ccvda, trm_code;
    double rval;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    trm_code = (dtr->trm >> 12) & 0xf;
    if ((trm_code != 1) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    /* read the actual value from CCVA */
    aqnd = dtr->ccva;
    aqnda = aqnd & 0x7FFF;	/* mask out sign bit */
    if (aqnd & 0x8000)
	aqnd = -aqnda;

    /* calculate actual CCV */
    *value += aqnd * dtr->scal1;

    /* do the control */
    /* check if the value is within its allowed limits */
    if ((*value < dtr->mn) || (*value > dtr->mx) || ((*value < dtr->ilim) && (*value > -dtr->ilim))) {
	*coco = EQP_LIMERR; return;
    }

    /* check that scaling factor is not zero and calculate value in bits */
    if (dtr->scal1 == 0) { *coco = EQP_SCLERR; return; }

    rval = *value / dtr->scal1;
    if (rval >= 0) {
	ccvd = rval + 0.5;
	ccvda = ccvd;
    }
    else {
	ccvd = rval - 0.5;
	ccvda = -ccvd;		/* ccvda is absolute value */
    }
    if (ccvd < 0)
	ccvd = ccvda | 0x8000;

    /* store the checked and transformed value into corresponding data column */
    dtr->ccva = ccvd;

    /* return if ppm bit is set (no direct writing into QUAD) */
    if ((dtr->grp & PPMV_MASK) != 0) return;

    /* now call the QUAD to transfer the CCV */
    *coco = quad_transf ((unsigned long) dtr->camna1, dummy, ccvd, TRF_CCV);
}


/*****************************************************************************
W03INIT               Initialize QUAD per channel                       
*****************************************************************************/
/* initializes the QUAD and writes CCAC and CCVA into one channel  without
   disturbing the other ones */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int cnnt;			/* RO  connection                               */
    int ccac;			/* RO  ccac                                     */
    int ccva;			/* RO  ccva                                     */
    int elmstr;			/* RO  master equ. nr. if slave                 */
} w03init_dtr;

sproco (w03init, w03init_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    *coco = quad_init ((unsigned long) dtr->camna1, dtr->ccac, dtr->ccva);
}


/*****************************************************************************
W03PAUSE              Writes standby into CCAC and hardware             
*****************************************************************************/
/* write the standby pattern into CCAC and the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccac;			/* RW  actuation                                */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  connection                               */
    int elmstr;			/* RO  master equ. nr.                          */
} w03pause_dtr;

sproco (w03pause, w03pause_dtr, int)
{

    int trm_code;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_PAUSE, plsline, coco);
	return;
    }

    /* check if the treatment code is accepted */
    trm_code = (dtr->trm >> 4) & 0xf;
    if (((trm_code < 1) || (trm_code > 3)) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    dtr->ccac = STANDBY;
    *coco = quad_transf ((unsigned long) dtr->camna1, STANDBY, dummy, TRF_ACT);
}


/*****************************************************************************
W03RSETP              Reset power supply and establish old actuation    
*****************************************************************************/
/* writes the reset pattern into the hardware, waits 0.5s and  then  sends
   the actuation contained in CCAC to the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccac;			/* RO  actuation                                */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment code                           */
    int elmstr;			/* RO  master equ. nr.                          */
} w03rsetp_dtr;

sproco (w03rsetp, w03rsetp_dtr, int)
{
    int trm_code;
    time_t dlay3 = 510000;	/*510 millisecond */

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_RSET, plsline, coco);
	return;
    }

    /* check if the treatment code is accepted */
    trm_code = (dtr->trm >> 4) & 0xf;
    if (((trm_code < 1) || (trm_code > 3)) && (trm_code != 7)) { *coco = EQP_BADTRM; return; }

    *coco = quad_transf ((unsigned long) dtr->camna1, RESET, dummy, TRF_ACT);
    if (*coco != EQP_NOERR) return;

    my_usleep (dlay3);
    *coco = quad_transf ((unsigned long) dtr->camna1, dtr->ccac, dummy, TRF_ACT);
}


/*****************************************************************************
W03SETA               Set actuation operation                           
*****************************************************************************/
/* reads the actuation pattern from FUPA and writes it into CCAC and  into
   the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccac;			/* RW  actuation                                */
    int cnnt;			/* RO  connection                               */
    int trm;			/* RO  treatment                                */
    int fupa;			/* RO  backup storage of actuation              */
    int elmstr;			/* RO  master equ. nr.                          */
} w03seta_dtr;

sproco (w03seta, w03seta_dtr, int)
{
    int actuation;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* if slave power supply, call for the corresponding master */
    if (dtr->elmstr > 0) {
	eqm_svse (EQP_POW, value, EQM_TYP_INT, WFLAG, dtr->elmstr, EQP_SETA, plsline, coco);
	return;
    }

    actuation = dtr->fupa;
    dtr->ccac = actuation;
    *coco = quad_transf ((unsigned long) dtr->camna1, actuation, dummy, TRF_ACT);
}


/*****************************************************************************
W03SETV               Set CCV                                           
*****************************************************************************/
/* takes the CCV from CCVA and transfers it to the hardware */

typedef struct
{
    int camna1;			/* RO  CAMAC address                            */
    int ccva;			/* RO  CCVA                                     */
    int cnnt;			/* RO  connection                               */
} w03setv_dtr;

sproco (w03setv, w03setv_dtr, int)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* now call the QUAD to transfer the CCV */
    *coco = quad_transf ((unsigned long) dtr->camna1, dummy, dtr->ccva, TRF_CCV);
}


/*****************************************************************************
W03TEINC              Check that CCV is correct if incremented          
*****************************************************************************/
/* tests if the CCV incremented by the value is inside the limits given by
   the table entries, but does not overwrite the old values */

typedef struct
{
    int ccva;			/* RO                                           */
    int cnnt;			/* RO                                           */
    int trm;			/* RO                                           */
    int grp;			/* RO                                           */
    double scal1;		/* RO                                           */
    int hwmx;			/* RO                                           */
    double mn;			/* RO                                           */
    double mx;			/* RO                                           */
    double ilim;		/* RO                                           */
} w03teinc_dtr;

sproco (w03teinc, w03teinc_dtr, double)
{
    int trm_code, aqnd, aqnda;
    double loc_val;

    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    trm_code = (dtr->trm >> 12) & 0xF;
    if ((trm_code != 1) && (trm_code != 7)) {
	*coco = EQP_BADTRM; return;
    }

    /* the CCV is read from the data column CCVA */
    aqnd = dtr->ccva;
    aqnda = aqnd & 0x7FFF;	/* mask out sign bit */
    if (aqnd & 0x8000)
	aqnd = -aqnda;

    /* calculate new value by adding the increment */
    loc_val = *value + aqnd * dtr->scal1;

    /* check if this value is within its allowed limits */
    if ((loc_val < dtr->mn) || (loc_val > dtr->mx) || ((loc_val < dtr->ilim) && (loc_val > -dtr->ilim))) {
	*coco = EQP_LIMERR; return;
    }

    /* check that scaling factor is not zero and calculate value in bits */
    if (dtr->scal1 == 0) { *coco = EQP_SCLERR; return; }
}


/*****************************************************************************
W03TEVAL              Checks CCV but does not change control            
*****************************************************************************/
/* tests if the value is inside the limits given by the table entries, but
   does not overwrite the old values */

typedef struct
{
    int ccva;			/* RO                                           */
    int cnnt;			/* RO                                           */
    int trm;			/* RO                                           */
    int grp;			/* RO                                           */
    double scal1;		/* RO                                           */
    int hwmx;			/* RO                                           */
    double mn;			/* RO                                           */
    double mx;			/* RO                                           */
    double ilim;		/* RO                                           */
} w03teval_dtr;

sproco (w03teval, w03teval_dtr, double)
{
    /* first check if the power supply is connected */
    if (dtr->cnnt == 0) { *coco = EQP_NOTCON; return; }

    /* check if the value is within its allowed limits */
    if ((*value < dtr->mn) || (*value > dtr->mx) || ((*value < dtr->ilim) && (*value > -dtr->ilim))) {
	*coco = EQP_LIMERR; return;
    }

    /* check that scaling factor is not zero and calculate value in bits */
    if (dtr->scal1 == 0) { *coco = EQP_SCLERR; return; }
}
