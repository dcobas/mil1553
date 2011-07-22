/**
 * Implement the minimum needed quick data library
 * Julian Lewis Thu 17/March/2011 BE/CO/HT
 */

/**
 * This code was derrived by experimenting with the hardware.
 * The way data must be serialized and deserialized is not always symetric.
 * It was quite a challenge to find out what I needed to do to make things work.
 * I would strongly advise against anyone using the raw data IO calls on a power supply.
 * I have added the correct logic for serializing the power supply data structures.
 *
 * In conclusion unless you are some sort of pervert don't even think about calling
 *    mil1553_send_raw_quick_data or mil1553_get_raw_quick_data
 *
 * Instead use
 *    mil1553_send_quick_data or mil1553_get_quick_data
 * which do the correct conversions for you to and from native format
 */

#include <libquick.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>

void swab(const void *from, void *to, ssize_t n);

/**
 * This is a header understood by RTI driven equipment such as a power supply or a relay box.
 * Notice I reversed the order of fields that are char pairs, don't ask thats just the
 * way it is.
 */

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

static struct msg_header_s *build_message_header(struct quick_data_buffer *quick_pt) {

	static unsigned short dest_transport = 0x8000;
	struct msg_header_s *msh;

	msh = (struct msg_header_s *) malloc(sizeof(struct msg_header_s));
	if (msh) {

		bzero((void *) msh, sizeof(struct msg_header_s));

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
	return msh;
}

#ifdef DEBUG
void mil1553_print_message_header(struct msg_header_s *msh) {

	printf("\n=====>\n");

	printf("  PacketSize:          0x%04hX %03d\n",msh->packet_size,msh->packet_size);
	printf("  Version:             0x%04hx %03d\n",msh->version,msh->version);
	printf("  Spare_1:             0x%04hx %03d\n",msh->spare_1,msh->spare_1);
	printf("  SourceAddress:       0x%04hx %03d\n",msh->source_address,msh->source_address);
	printf("  DestinationAddress:  0x%04hx %03d\n",msh->destination_address,msh->destination_address);
	printf("  PacketType:          0x%04hx %03d\n",msh->packet_type,msh->packet_type);
	printf("  Sequence:            0x%04hx %03d\n",msh->sequence,msh->sequence);
	printf("  SourceTransport:     0x%04hx %03d\n",msh->source_transport,msh->source_transport);
	printf("  DestinationTransport:0x%04hx %03d\n",msh->destination_transport,msh->destination_transport);
	printf("  SessionError:        0x%04hx %03d\n",msh->session_error,msh->session_error);
	printf("  Spare_2:             0x%04hx %03d\n",msh->spare_2,msh->spare_2);

	printf("<=====\n");
}
#endif

/**
 * @brief Open the mil1553 driver and initialize library
 * @return File handle greater than zero if successful, or zero on error
 *
 * The returned file handle is used in all subsequent calls to send and get.
 * Every thread should obtain its own file handle, there is no limit to the
 * number of concurrent threads that can use the library.
 * This is now thread safe, sorry I changed the API.
 */

int mil1553_init_quickdriver(void) {
	return milib_handle_open();
}

/**
  * @brief send a raw quick data buffer
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_send_raw_quick_data(int fn, struct quick_data_buffer *quick_pt) {

	struct msg_header_s *msh;
	struct quick_data_buffer *qptr;
	unsigned short *wptr;
	int i, j, cc, wc, occ;
	unsigned short txbuf[TX_BUF_SIZE];

	occ = 0;    /* Clear overall completion code */

	qptr = quick_pt;
	while (qptr) {

		msh  = build_message_header(qptr);
		wptr = (unsigned short *) msh;
#ifdef DEBUG
		mil1553_print_message_header(msh);
#endif
		for (i=0; i<HEADER_SIZE; i++)
			txbuf[i] = wptr[i];
		free(msh);

		wc = (qptr->pktcnt + 1)/2;
		if (wc > MESS_SIZE)
			wc = MESS_SIZE;
		wc += HEADER_SIZE;

		wptr = (unsigned short *) qptr->pkt;
		for (i=HEADER_SIZE, j=0; i<wc; i++,j++)
			txbuf[i] = wptr[j];

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
  * @brief get a raw quick data buffer
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_get_raw_quick_data(int fn, struct quick_data_buffer *quick_pt) {

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
			occ = cc;  /* Overall cc error, continue with next */
			goto Next_qp;
		}
		str = rxbuf[0];
		if (str & STR_TIM) {
			qptr->error = ETIMEDOUT;
			occ = qptr->error;
			goto Next_qp;
		}
		if (str & STR_ME) {
			qptr->error = EPROTO;
			occ = qptr->error;
			goto Next_qp;
		}
		if (str & STR_BUY) {
			qptr->error = EBUSY;
			occ = qptr->error;
			goto Next_qp;
		}
		rti = (str & STR_RTI_MASK) >> STR_RTI_SHIFT;
		if (qptr->rt != rti) {
			qptr->error = ENODEV;
			occ = qptr->error;
			goto Next_qp;
		}
		msh = (struct msg_header_s *) &rxbuf[1];
#ifdef DEBUG
		mil1553_print_message_header(msh);
#endif
		wptr = (unsigned short *) qptr->pkt;
		for (i=0,j=HEADER_SIZE+1; i<wc; i++,j++)
			wptr[i] = rxbuf[j];

		qptr->error = 0;
Next_qp:        qptr = qptr->next;
	}
	return occ;
}

/**
 * ===================================================================
 * About power supplies and how to exchange data with one.
 * The data layout is little_middle endian, most of the time.
 * Little_middle endian is Little endian word order and Big endian byte order.
 * There are exceptions to this rule:
 * (1) floats when writing are pure Big endian.
 * (2) Consecutive char fields in a structure are Little endian.
 *
 * This took me a while to find out !!!
 *
 * (3) Well if you read back the values you wrote the VHDL has
 *     swapped the words so you don't need to swap them again.
 *     But the read back byte order is big endian so you must byte swap.
 */

/* (1) Swap words in a float */

static void FloatWordSwap(float *f) {

	unsigned short *wp, w;
	wp = (unsigned short *) f;
	w = *wp;
	*wp = wp[1];
	wp[1] = w;
}

/* (2) Swap consecutive char fields */

static void FieldSwap(unsigned char *cp) {

	char c = *cp;
	*cp = cp[1];
	cp[1] = c;
}

#ifdef DEBUG
void FloatPrint(float f) {

  union {
    float f;
    unsigned char b[4];
  } dat;

  dat.f = f;

  printf("Float-bytes: 0x%02hx 0x%02hx 0x%02hx 0x%02hx\n",
	 dat.b[0],
	 dat.b[1],
	 dat.b[2],
	 dat.b[3]);
}
#endif

/* (3) Swap bytes in a float that is read back after writing */

static float FloatByteSwap(float f) {
  union {
    float f;
    unsigned char b[4];
  } dat1, dat2;

  dat1.f = f;
  dat2.b[0] = dat1.b[1];
  dat2.b[1] = dat1.b[0];
  dat2.b[2] = dat1.b[3];
  dat2.b[3] = dat1.b[2];

  return dat2.f;
}

/**
 * These are the serialization routines that convert the structures defined
 * in pow_messages.h for transmition over the MIL1553 cable to/from a power supply.
 * Reading back a control message reflects the way floats are stored on the
 * power supply and requires special handling.
 * In general floats are just word swapped. You don't need to call these
 * functions if you are not using the raw send/receive routines.
 */

void serialize_req_msg(req_msg  *req_p) {

	FieldSwap((unsigned char *) &req_p->type);
}

void serialize_read_ctrl_msg(ctrl_msg *ctrl_p) {

	FieldSwap((unsigned char *) &ctrl_p->ccsact_change);

	ctrl_p->ccv  = FloatByteSwap(ctrl_p->ccv);
	ctrl_p->ccv1 = FloatByteSwap(ctrl_p->ccv1);
	ctrl_p->ccv2 = FloatByteSwap(ctrl_p->ccv2);
	ctrl_p->ccv3 = FloatByteSwap(ctrl_p->ccv3);

	FieldSwap((unsigned char *) &ctrl_p->ccv_change);
	FieldSwap((unsigned char *) &ctrl_p->ccv2_change);
}

void serialize_write_ctrl_msg(ctrl_msg *ctrl_p) {

	FieldSwap((unsigned char *) &ctrl_p->ccsact_change);

	FloatWordSwap(&ctrl_p->ccv);
	FloatWordSwap(&ctrl_p->ccv1);
	FloatWordSwap(&ctrl_p->ccv2);
	FloatWordSwap(&ctrl_p->ccv3);

	FieldSwap((unsigned char *) &ctrl_p->ccv_change);
	FieldSwap((unsigned char *) &ctrl_p->ccv2_change);
}

void serialize_acq_msg(acq_msg *acq_p) {

	FieldSwap((unsigned char *) &acq_p->phys_status);
	FieldSwap((unsigned char *) &acq_p->ext_aspect);

	FloatWordSwap(&acq_p->aqn);
	FloatWordSwap(&acq_p->aqn1);
	FloatWordSwap(&acq_p->aqn2);
	FloatWordSwap(&acq_p->aqn3);
}

void serialize_conf_msg(conf_msg *conf_p) {

	FloatWordSwap(&conf_p->i_nominal);
	FloatWordSwap(&conf_p->resolution);
	FloatWordSwap(&conf_p->i_max);
	FloatWordSwap(&conf_p->i_min);
	FloatWordSwap(&conf_p->di_dt);
	FloatWordSwap(&conf_p->mode);
}

/**
 * Serialize power structures according to their type and transfer direction
 * The function is called by the send/recieve routines and subsequently the raw
 * methods are called.
 */

void serialize(struct quick_data_buffer *quick_pt, int rflag) {

	struct quick_data_buffer *qdp;

	qdp = quick_pt;
	while (qdp) {

		req_msg *req_p = (req_msg *) qdp->pkt;

		serialize_req_msg(req_p);

		switch (req_p->service) {

			case RS_REF:
				if (!rflag)
					serialize_write_ctrl_msg((ctrl_msg *) quick_pt->pkt);
				else
					serialize_acq_msg((acq_msg *) quick_pt->pkt);
			break;

			case RS_ECHO:
				serialize_read_ctrl_msg((ctrl_msg*)quick_pt->pkt);
			break;

			case RS_CONF:
				serialize_conf_msg((conf_msg*)quick_pt->pkt);
			break;

			default:
				/* Just send it the way it is, no conversion */
			break;
		}

		qdp = qdp->next;
	}
}

/**
  * @brief send a raw quick data buffer in network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Just use native structures as defined in pow_messages, no conversion
  * is needed.
  */

short mil1553_send_quick_data(int fn, struct quick_data_buffer *quick_pt) {

	serialize(quick_pt,0);
	return mil1553_send_raw_quick_data(fn,quick_pt);
}

/**
  * @brief get a raw quick data buffer from network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Just use native structures as defined in pow_messages, no conversion
  * is needed.
  */

short mil1553_get_quick_data(int fn, struct quick_data_buffer *quick_pt) {

	short cc;
	cc = mil1553_get_raw_quick_data(fn,quick_pt);
	serialize(quick_pt,1);
	return cc;
}

/* ===================================== */

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

			case EINPROGRESS:
				fprintf(stderr,"QDP Transaction partial failure");
			break;

			default:
				fprintf(stderr,"Driver/System error");
		}
		fprintf(stderr,"\n");
	}
}

/* ===================================== */

void mil1553_print_req_msg(req_msg *req_p) {

	printf("==> RequestMessage:\n");
	printf("   Family     :%02d",req_p->family);
	if (req_p->family == 3) printf(" => POWer-convertor");
	printf("\n");
	printf("   MessageType:%02d",req_p->type);
	if (req_p->type == 1) printf(" => OK");
	printf("\n");
	printf("   SubFamily  :%02d",req_p->sub_family);
	if (req_p->type == 0) printf(" => OK");
	printf("\n");
	printf("   MemberNum  :%02d",req_p->member);
	if (req_p->member > 0) printf(" => OK");
	printf("\n");
	printf("   Service    :%02d",req_p->service);
	if (req_p->service == 0) printf(" => RS_REF");
	if (req_p->service == 1) printf(" => RS_ECHO");
	if (req_p->service == 5) printf(" => RS_CONF");
	printf("\n");
	printf("   UTCSecond  :%u %s",req_p->protocol_date.sec,
				      ctime((time_t *) &req_p->protocol_date.sec));
	printf("   Usec       :%u\n",req_p->protocol_date.usec);
	printf("<==\n");
}

/* ===================================== */

void mil1553_print_ctrl_msg(ctrl_msg *ctrl_p) {

	printf("==> ControlMessage:\n");
	printf("   ccsact     :%02d change:%02d",ctrl_p->ccsact,ctrl_p->ccsact_change);
	if (ctrl_p->ccsact == 1) printf(" ==> Off");
	if (ctrl_p->ccsact == 2) printf(" ==> StandBy");
	if (ctrl_p->ccsact == 3) printf(" ==> On");
	if (ctrl_p->ccsact == 4) printf(" ==> Reset");
	printf("\n");
	printf("   ccv|[1-3]  :%f %f %f %f\n",ctrl_p->ccv,ctrl_p->ccv1,ctrl_p->ccv2,ctrl_p->ccv3);
	printf("<==\n");
}

/* ===================================== */

void mil1553_print_acq_msg(acq_msg *acq_p) {

	printf("==> AcquisitionMessage:\n");
	printf("   PhysicalStatus :%02d",acq_p->phys_status);
	if (acq_p->phys_status == 0) printf(" ==> NotDefined");
	if (acq_p->phys_status == 1) printf(" ==> Operational");
	if (acq_p->phys_status == 2) printf(" ==> PartialOperation");
	if (acq_p->phys_status == 3) printf(" ==> NotInOperation");
	if (acq_p->phys_status == 4) printf(" ==> NeedsComissioning");
	printf("\n");
	printf("   ConvertorStatus:%02d",acq_p->static_status);
	if (acq_p->static_status == 0 ) printf(" ==> Unconfigured");
	if (acq_p->static_status == 1 ) printf(" ==> Off");
	if (acq_p->static_status == 2 ) printf(" ==> Standby");
	if (acq_p->static_status == 3 ) printf(" ==> On");
	if (acq_p->static_status == 4 ) printf(" ==> AtPowerOn");
	if (acq_p->static_status == 5 ) printf(" ==> BeforeStandby");
	if (acq_p->static_status == 6 ) printf(" ==> BeforeOff");
	if (acq_p->static_status == 7 ) printf(" ==> OffToOnRetarded");
	if (acq_p->static_status == 8 ) printf(" ==> OffToStandbyRetarded");
	if (acq_p->static_status == 9 ) printf(" ==> WaitingCurrentZero_1");
	if (acq_p->static_status == 10) printf(" ==> WaitingCurrentZero_2");
	printf("\n");
	printf("   ExternalAspects:%02d",acq_p->ext_aspect);
	if (acq_p->ext_aspect == 0) printf(" ==> NotDefined");
	if (acq_p->ext_aspect == 1) printf(" ==> NotConnected");
	if (acq_p->ext_aspect == 2) printf(" ==> Local");
	if (acq_p->ext_aspect == 3) printf(" ==> Remote");
	if (acq_p->ext_aspect == 4) printf(" ==> VetoSecurity");
	if (acq_p->ext_aspect == 5) printf(" ==> BeamToDump");
	printf("\n");
	printf("   StatusQualifier:0x%02hx ==> ",acq_p->status_qualif);
	if (acq_p->status_qualif & 0x01) printf("InterlockFault:");
	if (acq_p->status_qualif & 0x02) printf("UnresetableFault:");
	if (acq_p->status_qualif & 0x04) printf("ResetableFault:");
	if (acq_p->status_qualif & 0x08) printf("Busy:");
	if (acq_p->status_qualif & 0x10) printf("Warning:");
	if (acq_p->status_qualif & 0x20) printf("CCVOutOfRange:");
	if (acq_p->status_qualif & 0x40) printf("ForewarningPulseMissing:");
	if (acq_p->status_qualif & 0x80) printf("VetoSecurity:");
	printf("\n");
	printf("   BusyTime       :%02d\n",acq_p->busytime);
	printf("   aqn|[1-3]      :%f %f %f %f\n",acq_p->aqn,acq_p->aqn1,acq_p->aqn2,acq_p->aqn3);
	printf("<==\n");
}

/* ===================================== */

void mil1553_print_conf_msg(conf_msg *conf_p) {

	printf("==> ConfigurationMessage:\n");
	printf("   i_nominal resolution :%f %f\n",conf_p->i_nominal,conf_p->resolution);
	printf("   i_max i_min di_dt    :%f %f %f\n",conf_p->i_max,conf_p->i_min,conf_p->di_dt);
	printf("   mode                 :%f\n",conf_p->mode);
	printf("<==\n");
}

/* ===================================== */

void mil1553_print_msg(struct quick_data_buffer *quick_pt, int rflag, int expect_service) {

	req_msg *req_p = (req_msg *) quick_pt->pkt;

	mil1553_print_req_msg(req_p);

	if ((expect_service >= 0) && (expect_service != req_p->service)) {
		printf("WARNING: Expected service:%d\n",expect_service);
		printf("What follows may be garbage\n");
	}

	switch (req_p->service) {

		case RS_REF:
			if (!rflag)
				mil1553_print_ctrl_msg((ctrl_msg *) quick_pt->pkt);
			else
				mil1553_print_acq_msg((acq_msg *) quick_pt->pkt);
		break;

		case RS_ECHO:
			mil1553_print_ctrl_msg((ctrl_msg*)quick_pt->pkt);
		break;

		case RS_CONF:
			mil1553_print_conf_msg((conf_msg*)quick_pt->pkt);
		break;

		default:
			printf("Not a recognized qdp message\n");
		break;
	}
}

/* ===================================== */

/**
 * @brief Read Config message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param conf_ptr - Points to destination message
 * @return zero if OK else error
 */

#define RETRIES 2

int mil1553_read_cfg_msg(int fn, int bc, int rti, conf_msg *conf_ptr) {

struct quick_data_buffer send_buf = { 0 },
			 *quickptr_req = &send_buf,
			 receive_buf = { 0 },
			 *quickptr_ctl = &receive_buf;
req_msg *req_ptr;
conf_msg *loc_conf_ptr;
short mbno = 1;
int cc, retries;
struct timeval cur_time = { 0 };

   retries = 0;

   /* Wait for the BC lock */

   milib_lock_bc(fn,bc);

retry:

   quickptr_req->bc     = bc;
   quickptr_req->rt     = rti;
   quickptr_req->stamp  = 0;
   quickptr_req->error  = 0;
   quickptr_req->pktcnt = sizeof(req_msg);
   quickptr_req->next   = NULL;

   /* Initialize request message */

   req_ptr = (req_msg*) &(quickptr_req->pkt);
   req_ptr->family     = POW_FAM;
   req_ptr->type       = TYPE;
   req_ptr->sub_family = SUB_FAMILY;
   req_ptr->member     = mbno;
   req_ptr->service    = RS_CONF;
   req_ptr->cycle.machine  = 0;
   req_ptr->cycle.pls_line = 0;
   req_ptr->specialist     = 0;

   /* will also set current time in RTI */

   gettimeofday(&cur_time, NULL);
   req_ptr->protocol_date.sec  = (int)cur_time.tv_sec;
   req_ptr->protocol_date.usec = (int)cur_time.tv_usec;

   /* Send request message to G64 */

   cc = mil1553_send_quick_data(fn,quickptr_req);
   if (cc) {
      milib_unlock_bc(fn,bc);
      return cc;
   }

   quickptr_ctl->bc     = bc;
   quickptr_ctl->rt     = rti;
   quickptr_ctl->next   = NULL;
   quickptr_ctl->pktcnt = sizeof(conf_msg);

   /* Wait for the reply message */

   cc = mil1553_get_quick_data(fn,quickptr_ctl);
   if (cc) {
      milib_unlock_bc(fn,bc);
      return cc;
   }

   /* Check for correct service, retry if its wrong */

   loc_conf_ptr = (conf_msg*) &(quickptr_ctl->pkt);
   if (loc_conf_ptr->service != RS_CONF) {
      if (retries++ < RETRIES) goto retry;
   }
   milib_unlock_bc(fn,bc);

   memcpy(conf_ptr,loc_conf_ptr,sizeof(conf_msg));

   return cc;
}

/* ============================= */                                                                                                                                 

/**
 * @brief Read Acquisition message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param acq_ptr  - Points to destination message
 * @return zero if OK else error
 */

int mil1553_read_acq_msg(int fn, int bc, int rti, acq_msg *acq_ptr) {

struct quick_data_buffer send_buf = { 0 },
			 *quickptr_req = &send_buf,
			 receive_buf = { 0 },
			 *quickptr_ctl = &receive_buf;
req_msg *req_ptr;
acq_msg *loc_acq_ptr;
short mbno = 1;
int cc, retries;
struct timeval cur_time = { 0 };

   retries = 0;

   /* Wait for the BC lock */

   milib_lock_bc(fn,bc);

retry:

   quickptr_req->bc     = bc;
   quickptr_req->rt     = rti;
   quickptr_req->stamp  = 0;
   quickptr_req->error  = 0;
   quickptr_req->pktcnt = sizeof(req_msg);
   quickptr_req->next   = NULL;

   /* Initialize request message */

   req_ptr = (req_msg*) &(quickptr_req->pkt);
   req_ptr->family         = POW_FAM;
   req_ptr->type           = TYPE;
   req_ptr->sub_family     = SUB_FAMILY;
   req_ptr->member         = mbno;
   req_ptr->service        = RS_REF;
   req_ptr->cycle.machine  = 0;
   req_ptr->cycle.pls_line = 0;
   req_ptr->specialist     = 0;

   /* will also set current time in RTI */

   gettimeofday(&cur_time, NULL);
   req_ptr->protocol_date.sec  = (int)cur_time.tv_sec;
   req_ptr->protocol_date.usec = (int)cur_time.tv_usec;

   /* Send request message to G64 */

   cc = mil1553_send_quick_data(fn,quickptr_req);
   if (cc) {
      milib_unlock_bc(fn,bc);
      return cc;
   }

   quickptr_ctl->bc = bc;
   quickptr_ctl->rt = rti;
   quickptr_ctl->next = NULL;

   /* expected answer: we'll wait for packet 44 bytes long (22 words) */

   quickptr_ctl->pktcnt = sizeof(acq_msg);

   cc = mil1553_get_quick_data(fn,quickptr_ctl);
   if (cc) {
      milib_unlock_bc(fn,bc);
      return cc;
   }

   /* Check for correct service, retry if its wrong */

   loc_acq_ptr = (acq_msg*) &(quickptr_ctl->pkt);
   if (loc_acq_ptr->service != RS_REF) {
      if (retries++ < RETRIES) goto retry;
   }
   milib_unlock_bc(fn,bc);

   memcpy(acq_ptr,loc_acq_ptr,sizeof(acq_msg));

   return cc;
}

/* ============================= */                                                                                                                                 

/**
 * @brief Read back Control message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param ctrl_ptr - Points to destination message
 * @return zero if OK else error
 */

int mil1553_read_ctrl_msg(int fn, int bc, int rti, ctrl_msg *ctrl_ptr) {

   struct quick_data_buffer send_buf = { 0 },
			    *quickptr_req = &send_buf,
			    receive_buf = { 0 },
			    *quickptr_ctl = &receive_buf;
   req_msg *req_ptr;
   ctrl_msg *loc_ctrl_ptr;
   short mbno = 1;
   int cc, retries;

   retries = 0;

   /* Wait for the BC lock */

   milib_lock_bc(fn,bc);

retry:

   quickptr_req->bc     = bc;
   quickptr_req->rt     = rti;
   quickptr_req->stamp  = 0;
   quickptr_req->error  = 0;
   quickptr_req->pktcnt = sizeof(req_msg);
   quickptr_req->next   = NULL;

   /* Initialize request message */

   req_ptr               = (req_msg*) &(quickptr_req->pkt);
   req_ptr->family       = POW_FAM;
   req_ptr->type         = TYPE;
   req_ptr->sub_family   = SUB_FAMILY;
   req_ptr->member       = mbno;
   req_ptr->service      = RS_ECHO;
   req_ptr->cycle.machine  = 0;
   req_ptr->cycle.pls_line = 0;
   req_ptr->specialist     = 0;

   cc = mil1553_send_quick_data(fn,quickptr_req);
   if (cc) {
      milib_unlock_bc(fn,bc);
      return cc;
   }

   quickptr_ctl->bc   = bc;
   quickptr_ctl->rt   = rti;
   quickptr_ctl->next = NULL;

   quickptr_ctl->pktcnt = sizeof(ctrl_msg);
   cc = mil1553_get_quick_data(fn,quickptr_ctl);
   if (cc) {
      milib_unlock_bc(fn,bc);
      return cc;
   }

   /* Check for correct service, retry if its wrong */

   loc_ctrl_ptr = (ctrl_msg*) &(quickptr_ctl->pkt);
   if (loc_ctrl_ptr->service != RS_ECHO) {
      if (retries++ < RETRIES) goto retry;
   }
   milib_unlock_bc(fn,bc);

   memcpy(ctrl_ptr,loc_ctrl_ptr,sizeof(ctrl_msg));

   return cc;
}

/* ============================= */                                                                                                                                 

/**
 * @brief Write Control message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param ctrl_ptr - Points to source message
 * @return zero if OK else error
 */

int mil1553_write_ctrl_msg(int fn, int bc, int rti, ctrl_msg *ctrl_ptr) {

   struct quick_data_buffer receive_buf = { 0 },
			   *quickptr_ctl = &receive_buf;
   int cc, retries, mbno = 1;
   ctrl_msg *loc_ctrl_ptr;
   struct timeval cur_time = { 0 };
   retries = 0;

   loc_ctrl_ptr = (ctrl_msg*) &(quickptr_ctl->pkt);
   memcpy(loc_ctrl_ptr,ctrl_ptr,sizeof(ctrl_msg));

   /* Wait for the BC lock */

   milib_lock_bc(fn,bc);

retry:

   quickptr_ctl->bc     = bc;
   quickptr_ctl->rt     = rti;
   quickptr_ctl->stamp  = 0;
   quickptr_ctl->error  = 0;
   quickptr_ctl->pktcnt = sizeof(ctrl_msg);
   quickptr_ctl->next   = NULL;

   /* Initialize request message */

   loc_ctrl_ptr->family         = POW_FAM;
   loc_ctrl_ptr->type           = TYPE;
   loc_ctrl_ptr->sub_family     = SUB_FAMILY;
   loc_ctrl_ptr->member         = mbno;
   loc_ctrl_ptr->service        = RS_REF;
   loc_ctrl_ptr->cycle.machine  = 0;
   loc_ctrl_ptr->cycle.pls_line = 0;
   loc_ctrl_ptr->specialist     = 0;

   /* send current time */

   gettimeofday(&cur_time, NULL); /* get current time */
   loc_ctrl_ptr->protocol_date.sec  = (int)cur_time.tv_sec;
   loc_ctrl_ptr->protocol_date.usec = (int)cur_time.tv_usec;

   /* Send ctrl_msg to G64 */

   cc = mil1553_send_quick_data(fn,quickptr_ctl);
   if (cc) {
      if (retries++ < RETRIES) goto retry;
      milib_unlock_bc(fn,bc);
      return cc;
   }

   milib_unlock_bc(fn,bc);
   return cc;
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

	struct msg_header_s *msh;
	struct quick_data_buffer *qptr;
	unsigned short *wptr;
	int i, j, cc, wc, occ;
	unsigned short txbuf[TX_BUF_SIZE];

	occ = 0;    /* Clear overall completion code */

	qptr = quick_pt;
	while (qptr) {

		msh  = build_message_header(qptr);
		wptr = (unsigned short *) msh;
		for (i=0; i<HEADER_SIZE; i++)
			txbuf[i] = wptr[i];
		free(msh);

		wc = (qptr->pktcnt + 1)/2;
		if (wc > MESS_SIZE)
			wc = MESS_SIZE;
		wc += HEADER_SIZE;

		wptr = (unsigned short *) qptr->pkt;
		swab(qptr->pkt,qptr->pkt,qptr->pktcnt);
		for (i=HEADER_SIZE, j=0; i<wc; i+=2,j+=2) {
			txbuf[i] = wptr[j];
		}
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
			occ = cc;  /* Overall cc error, continue with next */
			goto Next_qp;
		}
		str = rxbuf[0];
		if (str & STR_TIM) {
			qptr->error = ETIMEDOUT;
			occ = qptr->error;
			goto Next_qp;
		}
		if (str & STR_ME) {
			qptr->error = EPROTO;
			occ = qptr->error;
			goto Next_qp;
		}
		if (str & STR_BUY) {
			qptr->error = EBUSY;
			occ = qptr->error;
			goto Next_qp;
		}
		rti = (str & STR_RTI_MASK) >> STR_RTI_SHIFT;
		if (qptr->rt != rti) {
			qptr->error = ENODEV;
			occ = qptr->error;
			goto Next_qp;
		}
		msh = (struct msg_header_s *) &rxbuf[1];
		wptr = (unsigned short *) qptr->pkt;
		for (i=0,j=HEADER_SIZE+1; i<wc; i+=2,j+=2) {
			wptr[i] = rxbuf[j];
		}
		swab(qptr->pkt,qptr->pkt,qptr->pktcnt);
		qptr->error = 0;
Next_qp:        qptr = qptr->next;
	}
	return occ;
}
