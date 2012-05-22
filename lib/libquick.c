/** @file libquick.c
 *  @brief Implement the *really* minimum needed quick data library
 *  Copyright CERN, 2012
 *  Julian Lewis, 2011-2012
 *  Juan David Gonzalez Cobas May 22, 2012
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#define __USE_XOPEN
#include <unistd.h>
#include "libquick.h"


/**
 * This is a header understood by RTI driven equipment such as a power supply or a relay box.
 * Notice I reversed the order of fields that are char pairs, don't ask thats just the
 * way it is.
 */

#define TX_BUF_SIZE	32
#define RX_BUF_SIZE	(TX_BUF_SIZE+1)
#define HEADER_SIZE 8
#define MESS_SIZE (TX_BUF_SIZE - HEADER_SIZE -1)

struct msg_header_s {
	short packet_size;
	char spare_1;
	char version;
	short source_address;
	short destination_address;
	char sequence;
	char packet_type;
	short source_transport;
	short destination_transport;
	char spare_2;
	char session_error;
};

/**
 * After diving into code, poor doccumentation etc
 * and with out a clue what I am doing, it turns
 * out that this is what you put in a header !!
 */

#define QDP_VERSION 3
#define QDP_SINGLE_PACKET 3
#define QDP_QUICK_TYPE 31

static void build_message_header(struct quick_data_buffer *quick_pt,
						 struct msg_header_s *msh) {

	static unsigned short dest_transport = 0x8000;

	memset(msh, 0, sizeof(*msh));

	msh->packet_size           = quick_pt->pktcnt;
	msh->version               = QDP_VERSION;
	msh->source_address        = ((quick_pt->bc+1) << 8) | 0xFF;
	msh->destination_address   = ((quick_pt->bc+1) << 8) | quick_pt->rt;
	msh->packet_type           = QDP_QUICK_TYPE;
	msh->sequence              = QDP_SINGLE_PACKET;
	msh->source_transport      = quick_pt->stamp;
	msh->destination_transport = dest_transport++;
	msh->session_error         = 0;

	if (dest_transport == 0xFFFF)
		dest_transport = 0x8000;
}

/**
 * @brief Open the mil1553 driver and initialize library
 * @return File handle greater than zero if successful, or zero on error
 *
 * The returned file handle is used in all subsequent calls to send and get.
 * Every thread should obtain its own file handle, there is no limit to the
 * number of concurrent threads that can use the library.
 * This is now thread safe, sorry I changed the API.
 */

#define MIL1553_DEV_PATH "/dev/mil1553"

int mil1553_init_quickdriver(void) {
	int cc;
	cc = open(MIL1553_DEV_PATH, O_RDWR, 0);
	return cc;
}

void mil1553_print_error(int cc) {

	char *cp;
	if (cc) {
		if (cc < 0)
			cc = -cc;
		cp = strerror(cc);
		fprintf(stderr,"QuickDataLib Error:%s - ",cp);

		/**
		 * Specific usage of system errors, extra information
		 */

		switch (cc) {

			case EFAULT:
				fprintf(stderr,"Bad BC number");
			break;

			case ENODEV:
				fprintf(stderr,"Bad RTI number or RTI down");
			break;

			case ETIME:
				fprintf(stderr,"User wait time out");
			break;

			case EINTR:
				fprintf(stderr,"Got a signal while in wait");
			break;

			case ENOMEM:
				fprintf(stderr,"Failed to allocate memory");
			break;

			case ENOTTY:
				fprintf(stderr,"Bad IOCTL number or call");
			break;

			case EACCES:
				fprintf(stderr,"Driver failed, generic code");
			break;

			case ETIMEDOUT:
				fprintf(stderr,"RTI didn't answer, STR.TIM");
			break;

			case EPROTO:
				fprintf(stderr,"RTI message error, STR.ME");
			break;

			case EBUSY:
				fprintf(stderr,"RTI busy, STR.BUY");
			break;

			case EAGAIN:
				fprintf(stderr,"Wrong service number read back, try again");
			break;

			case EINPROGRESS:
				fprintf(stderr,"QDP Transaction partial failure");
			break;

			default:
				fprintf(stderr,"Driver/System error");
		}
		fprintf(stderr,"\n");
	}
}

/* =============================================================== */
/* These routines swap words in the message so as to be compatible */
/* with the POW equipment module. The byte order will end up the   */
/* same as with Yuries old driver (possibly network format)        */

/**
  * @brief send a raw quick data buffer network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_send_raw_quick_data_net(int fn, struct quick_data_buffer *quick_pt) {

	struct msg_header_s msh;
	struct quick_data_buffer *qptr;
	unsigned short *wptr;
	int i, j, cc, wc, occ;
	unsigned short txbuf[RX_BUF_SIZE];

	occ = 0;    /* Clear overall completion code */

	qptr = quick_pt;
	while (qptr) {

		build_message_header(qptr,&msh);
		wptr = (unsigned short *) &msh;
		for (i=0; i<HEADER_SIZE; i++)
			txbuf[i] = wptr[i];

		wc = (qptr->pktcnt + 1)/2;
		if (wc > MESS_SIZE)
			wc = MESS_SIZE;
		wc += HEADER_SIZE;

		wptr = (unsigned short *) qptr->pkt;
		for (i=HEADER_SIZE, j=0; i<wc; i++,j++)
			swab(&wptr[j],&txbuf[i],sizeof(short));

		cc = rtilib_send_eqp(fn,qptr->bc,qptr->rt,wc,txbuf);
		if (cc) {
			qptr->error = (short) cc;
			occ = EINPROGRESS;  /* Overall cc error, continue with next */
		} else
			qptr->error = 0;

		qptr = qptr->next;
	}
	return occ;
}

/**
  * @brief get a raw quick data buffer network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_get_raw_quick_data_net(int fn, struct quick_data_buffer *quick_pt) {

	struct msg_header_s *msh;
	struct quick_data_buffer *qptr;
	unsigned short *wptr;
	int i, j, cc, wc, occ;
	unsigned short rxbuf[RX_BUF_SIZE], str, rti;

	occ = 0;    /* Clear overall completion code */

	qptr = quick_pt;
	while (qptr) {

		wc = (qptr->pktcnt + 1)/2;
		if (wc > MESS_SIZE)
			wc = MESS_SIZE;
		wc += HEADER_SIZE;

		cc = rtilib_recv_eqp(fn,qptr->bc,qptr->rt,wc,rxbuf);
		if (cc) {
			qptr->error = (short) cc;
			occ = EINPROGRESS;  /* Overall cc error, continue with next */
			goto Next_qp;
		}
		str = rxbuf[0];
		if (str & STR_TIM) {
			qptr->error = ETIMEDOUT;
			occ = EINPROGRESS;  /* Overall cc error, continue with next */
			goto Next_qp;
		}
		if (str & STR_ME) {
			qptr->error = EPROTO;
			occ = EINPROGRESS;  /* Overall cc error, continue with next */
			goto Next_qp;
		}
		if (str & STR_BUY) {
			qptr->error = EBUSY;
			occ = EINPROGRESS;  /* Overall cc error, continue with next */
			goto Next_qp;
		}
		rti = (str & STR_RTI_MASK) >> STR_RTI_SHIFT;
		if (qptr->rt != rti) {
			qptr->error = ENODEV;
			occ = EINPROGRESS;  /* Overall cc error, continue with next */
			goto Next_qp;
		}
		msh = (struct msg_header_s *) &rxbuf[1];
		wptr = (unsigned short *) qptr->pkt;
		for (i=0,j=HEADER_SIZE+1; i<wc; i++,j++)
			swab(&rxbuf[j],&wptr[i],sizeof(short));

		qptr->error = 0;
Next_qp:        qptr = qptr->next;
	}
	return occ;
}
