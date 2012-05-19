/************************************************************************/
/* filename: pow_messages.h                                             */
/* defines the message format for the power supplies protocol           */
/* created by W. Heinze                                                 */
/* date: 23.10.92                                                       */
/************************************************************************/
#ifndef _POW_MESSAGES_H_INCLUDE_
#define _POW_MESSAGES_H_INCLUDE_

/************************************************************************/
/* define common substructures                                          */
/************************************************************************/

/* standard format of date */
typedef struct {
	int             sec;
	int             usec;
} date;

/* accelerator and pls line number */
typedef struct {
	short           machine;
	short           pls_line;
} pls_event;

/************************************************************************/
/* define message structures as demanded by protocol                    */
/************************************************************************/

/* non ppm part of ctrl message */
typedef struct {
	short           family;             /* 00 */
	char            type;               /* 02 */
	char            sub_family;         /* 03 */
	short           member;             /* 04 */
	short           service;            /* 06 */
	pls_event       cycle;              /* 08 */
	date            protocol_date;      /* 12 */
	short           specialist;         /* 20 */
	char            ccsact_change;      /* 22 */
	unsigned        char ccsact;        /* 23 */
} nonppm_ctrl_msg;                          /* 24 bytes = 6 32bits values */

/* ppm part of ctrl message */
typedef struct {
	float           ccv;                /* 00 */
	float           ccv1;               /* 04 */
	float           ccv2;               /* 08 */
	float           ccv3;               /* 12 */
	char            ccv_change;         /* 16 */
	char            ccv1_change;        /* 17 */
	char            ccv2_change;        /* 18 */
	char            ccv3_change;        /* 19 */
} ppm_ctrl_msg;                             /* 20 bytes = 5 32bits values */

/* acquisition message protocol */
typedef struct {
	short           family;             /* 00 */
	char            type;               /* 02 */
	char            sub_family;         /* 03 */
	short           member;             /* 04 */
	short           service;            /* 06 */
	pls_event       cycle;              /* 08 */
	date            protocol_date;      /* 12 */
	short           specialist;         /* 20 */
	unsigned char   phys_status;        /* 22 */
	unsigned char   static_status;      /* 23 */
	unsigned char   ext_aspect;         /* 24 */
	unsigned char   status_qualif;      /* 25 */
	short           busytime;           /* 26 */
	float           aqn;                /* 28 */
	float           aqn1;               /* 32 */
	float           aqn2;               /* 36 */
	float           aqn3;               /* 40 */
} acq_msg;                                  /* 44 bytes = 22 16bits values */

/* message for immediate request */
typedef struct {
	short           family;             /* 00 */
	char            type;               /* 02 */
	char            sub_family;         /* 03 */
	short           member;             /* 04 */
	short           service;            /* 06 */
	pls_event       cycle;              /* 08 */
	date            protocol_date;      /* 12 */
	short           specialist;         /* 20 */
} req_msg;                                  /* 22/2 = 11 16bits values */

/* configuration message */
typedef struct {
	short           family;             /* 00 */
	char            type;               /* 02 */
	char            sub_family;         /* 03 */
	short           member;             /* 04 */
	short           service;            /* 06 */
	pls_event       cycle;              /* 08 */
	date            protocol_date;      /* 12 */
	short           specialist;         /* 20 */
	short           dummy;              /* 22 */
	float           i_nominal;          /* 24 */
	float           resolution;         /* 28 */
	float           i_max;              /* 32 */
	float           i_min;              /* 36 */
	float           di_dt;              /* 40 */
	float           mode;               /* 44 */
} conf_msg;                                 /* 48/2 = 24 16bits values */

/* read back control message */
typedef struct {
	short           family;             /* 00 */
	char            type;               /* 02 */
	char            sub_family;         /* 03 */
	short           member;             /* 04 */
	short           service;            /* 06 */
	pls_event       cycle;              /* 08 */
	date            protocol_date;      /* 12 */
	short           specialist;         /* 20 */
	char            ccsact_change;      /* 22 */
	unsigned char   ccsact;             /* 23 */
	float           ccv;                /* 24 */
	float           ccv1;               /* 28 */
	float           ccv2;               /* 32 */
	float           ccv3;               /* 36 */
	char            ccv_change;         /* 40 */
	char            ccv1_change;        /* 41 */
	char            ccv2_change;        /* 42 */
	char            ccv3_change;        /* 43 */
} ctrl_msg;                                 /* 44/2 = 22 16bits values */
#endif /* _POW_MESSAGES_H_INCLUDE_ */
