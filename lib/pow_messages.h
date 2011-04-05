/**
 * file pow_messages.h
 *
 * Defines the message format for the power supplies protocol.
 *
 * author Yury GEORGIEVSKIY
 * author Julian Lewis
 *
 * date Created on 12/12/2008
 * Modified for driver rewrite and clarity Julian
 *
 * Derived from original header, written by W. Heinze 23/10/1992
 */
#ifndef _POW_MESSAGES_H_INCLUDE_
#define _POW_MESSAGES_H_INCLUDE_

#define TYPE       1
#define SUB_FAMILY 0
#define POW_FAM    3
#define MAX_NB_RT  8

typedef struct {
	int sec;
	int usec;
} date;

typedef struct {
	short machine;
	short pls_line;
} pls_event;

typedef enum {
	RS_REF  = 0,
	RS_ECHO = 1,
	RS_CONF = 5
} rs_t;

typedef enum {
	AC_NOTHING = 0,
	AC_OFF     = 1,
	AC_STANDBY = 2,
	AC_ON      = 3,
	AC_RESET   = 4
} act_t;

/**
 * This is a common part for all QDP messages.
 * It is 22 bytes long (11 words) when __packed__ and
 * 24 bytes long when it is not.
 */

typedef struct {
	short     family;
	char      type;
	char      sub_family;
	short     member;
	short     service;
	pls_event cycle;
	date      protocol_date;
	short     specialist;
} req_msg;

/**
 * Acquisition message protocol (44 bytes) 22 words
 * Field description is taken from G64 TACQ_MSG definition in types.h file
 */

typedef struct {

	/* req_msg */

	short     family;
	char      type;
	char      sub_family;
	short     member;
	short     service;
	pls_event cycle;
	date      protocol_date;
	short     specialist;

	/* specific part */

	unsigned char phys_status; /*  0 - PHY_STUS_NOT_DEFINED
				       1 - PHY_STUS_OPERATIONAL
				       2 - PHY_STUS_PARTIALLY_OPERATIONAL
				       3 - PHY_STUS_NOT_OPERATIONAL
				       4 - PHY_STUS_NEEDS_COMMISIONING */

	unsigned char static_status; /* enum TConverterState from types.h */
	unsigned char ext_aspect;    /* enum Texternal_aspects from types.h */
	unsigned char status_qualif; /* bitfield.
					b0 - interlock fault
					b1 - unresettable fault
					b2 - resettable fault
					b3 - busy
					b4 - warning
					b5 - CCV out of limits in G64
					(CCV == current control value)
					b6 - forewarning pulse missing
					b7 - veto security */
	short         busytime;
	float         aqn;
	float         aqn1;
	float         aqn2;
	float         aqn3;
} acq_msg;


/**
 * Configuration message (48 bytes) 24 words.
 */

typedef struct {

	/* req_msg */

	short     family;
	char      type;
	char      sub_family;
	short     member;
	short     service;
	pls_event cycle;
	date      protocol_date;
	short     specialist;

	/* specific part */

	short dummy;
	float i_nominal;
	float resolution;
	float i_max;
	float i_min;
	float di_dt;
	float mode;
} conf_msg;


/**
 * read back control message (44 bytes long) 22 words
 * www page: http://wwwpsco.cern.ch/private/gm/gmdescrip/POW-V.html
 */

typedef struct {

  /* req_msg */

	short     family;
	char      type;
	char      sub_family;
	short     member;
	short     service;
	pls_event cycle;
	date      protocol_date;
	short     specialist;

  /* specific part */

	char          ccsact_change;
	unsigned char ccsact;
	float         ccv;
	float         ccv1;
	float         ccv2;
	float         ccv3;
	char          ccv_change;
	char          ccv1_change;
	char          ccv2_change;
	char          ccv3_change;
} ctrl_msg;

/**
 * Non PPM part of ctrl message (24 bytes) 12 words
 */

typedef struct {

  /* req_msg */

	short     family;
	char      type;
	char      sub_family;
	short     member;
	short     service;
	pls_event cycle;
	date      protocol_date;
	short     specialist;

  /* specific part */

	char          ccsact_change;
	unsigned char ccsact;
} nonppm_ctrl_msg;


/**
 * PPM part of ctrl message (20 bytes) 10 words
 */

typedef struct {
	float ccv;
	float ccv1;
	float ccv2;
	float ccv3;
	char  ccv_change;
	char  ccv1_change;
	char  ccv2_change;
	char  ccv3_change;
} ppm_ctrl_msg;

#endif /* _POW_MESSAGES_H_INCLUDE_ */
