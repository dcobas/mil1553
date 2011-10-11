#include <libmil1553.h>
#include <errno.h>

int milib_handle_open() {

	int cc;
	cc = open(DEV_PATH,O_RDWR,0);
	return cc;
}

int milib_set_timeout(int fn, int timeout_msec) {

	int cc;
	cc = ioctl(fn,MIL1553_SET_TIMEOUT_MSEC,&timeout_msec);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_timeout(int fn, int *timeout_msec) {

	int cc;
	cc = ioctl(fn,MIL1553_GET_TIMEOUT_MSEC,timeout_msec);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_set_debug_level(int fn, int debug_level) {

	int cc;
	cc = ioctl(fn,MIL1553_SET_DEBUG_LEVEL,&debug_level);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_debug_level(int fn, int *debug_level) {

	int cc;
	cc = ioctl(fn,MIL1553_GET_DEBUG_LEVEL,debug_level);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_drv_version(int fn, int *version) {

	int cc;
	cc = ioctl(fn,MIL1553_GET_DRV_VERSION,version);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_status(int fn, int bc, int *status) {

	int cc;
	*status = bc;
	cc = ioctl(fn,MIL1553_GET_STATUS,status);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_bcs_count(int fn, int *bcs_count) {

	int cc;
	cc = ioctl(fn,MIL1553_GET_BCS_COUNT,bcs_count);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_bc_info(int fn, struct mil1553_dev_info_s *dev_info) {

	int cc;
	cc = ioctl(fn,MIL1553_GET_BC_INFO,dev_info);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_raw_read(int fn, struct mil1553_riob_s *riob) {

	int cc;
	cc = ioctl(fn,MIL1553_RAW_READ,riob);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_raw_write(int fn, struct mil1553_riob_s *riob) {

	int cc;
	cc = ioctl(fn,MIL1553_RAW_WRITE,riob);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_up_rtis(int fn, int bc, int *up_rtis) {

	int cc;
	*up_rtis = bc;
	cc = ioctl(fn,MIL1553_GET_UP_RTIS,up_rtis);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_send(int fn, struct mil1553_send_s *send) {

	int cc;
	cc = ioctl(fn,MIL1553_SEND,send);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_recv(int fn, struct mil1553_recv_s *recv) {

	int cc;
	cc = ioctl(fn,MIL1553_RECV,recv);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_queue_size(int fn, int *size) {

	int cc;
	*size = 0;
	cc = ioctl(fn,MIL1553_QUEUE_SIZE,size);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_reset(int fn, int bc) {

	int cc;
	cc = ioctl(fn,MIL1553_RESET,&bc);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_write_reg(int fn, int bc, int reg_num, int reg_val) {

	struct mil1553_riob_s riob;
	riob.bc = bc;
	riob.reg_num = reg_num;
	riob.buffer = &reg_val;
	riob.regs = 1;
	return milib_raw_write(fn,&riob);
}

int milib_read_reg(int fn, int bc, int reg_num, int *reg_val) {

	struct mil1553_riob_s riob;
	riob.bc = bc;
	riob.reg_num = reg_num;
	riob.buffer = reg_val;
	riob.regs = 1;
	return milib_raw_read(fn,&riob);
}

char *stat_names[16] = { "CodeViol:",
			 "PartyErr:",
			 "Bit2:", "Bit3:", "Bit4:", "Bit5:", "Bit6:",
			 "Bit7:", "Bit8:", "Bit9:", "BitA:", "BitB:",
			 "RtiPollingOff:",
			 "BitD:",
			 "Busy:",
			 "Done:"};

char *milib_status_to_str(int stat) {

	int i;
	static char res[128];

	bzero((void *) res, 128);

	for (i=0; i<16; i++) {
		if ((1<<i) & stat)
		   strcat(res,stat_names[i]);
	}
	return res;
}

void milib_decode_txreg(unsigned int txreg, unsigned int *wc, unsigned int *sa, unsigned int *tr, unsigned int *rti) {

	if (wc)
		*wc  = (txreg & TXREG_WC_MASK)   >> TXREG_WC_SHIFT;
	if (sa)
		*sa  = (txreg & TXREG_SUBA_MASK) >> TXREG_SUBA_SHIFT;
	if (tr)
		*tr  = (txreg & TXREG_TR_MASK)   >> TXREG_TR_SHIFT;
	if (rti)
		*rti = (txreg & TXREG_RTI_MASK)  >> TXREG_RTI_SHIFT;
}

void milib_encode_txreg(unsigned int *txreg, unsigned int wc, unsigned int sa, unsigned int tr, unsigned int rti) {

	if (txreg)
		*txreg = ((wc  << TXREG_WC_SHIFT)   & TXREG_WC_MASK)
		       | ((sa  << TXREG_SUBA_SHIFT) & TXREG_SUBA_MASK)
		       | ((tr  << TXREG_TR_SHIFT)   & TXREG_TR_MASK)
		       | ((rti << TXREG_RTI_SHIFT)  & TXREG_RTI_MASK);
}

int milib_lock_bc(int fn, int bc) {

	int cc = 0;
	cc = ioctl(fn,MIL1553_LOCK_BC,&bc);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_unlock_bc(int fn, int bc) {

	int cc = 0;
	cc = ioctl(fn,MIL1553_UNLOCK_BC,&bc);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_set_bus_speed(int fn, int bc, int speed) {

	int cc = 0;
	struct mil1553_bus_speed_s bs;
	bs.speed = speed;
	bs.bc = bc;

	cc = ioctl(fn,MIL1553_SET_BUS_SPEED,&bs);

	if (cc < 0)
		return errno;
	return 0;
}

/**
 * @brief Provide a user level mdrop interface for legacy code
 */

static int mdrop_fn = 0;

short mdrop(short bc, short rti, short tr, short sa, short wc, short *status, char *buf) {

	int cc;
	struct mil1553_send_s send;
	struct mil1553_recv_s recv;
	struct mil1553_tx_item_s txitm;
	unsigned int txreg;


	if (!mdrop_fn) {
		cc = milib_handle_open();
		if (cc < 0)
			return cc;
		mdrop_fn = cc;
	}

	milib_encode_txreg(&txreg,wc,sa,tr,rti);

	txitm.bc = bc;
	txitm.rti_number = rti;
	txitm.txreg = txreg;
	txitm.no_reply = 0;
	bcopy(buf,txitm.txbuf,wc*2);

	send.item_count = 1;
	send.tx_item_array = &txitm;

	cc = milib_send(mdrop_fn, &send);
	if (cc)
		return -1;
	bzero((void *) &recv, sizeof(struct mil1553_recv_s));
	recv.pk_type = TX_ALL;
	recv.timeout = 100;
	cc = milib_recv(mdrop_fn, &recv);
	*status = recv.interrupt.rxbuf[0];
	if (cc)
		return -1;

	if ((recv.interrupt.bc != bc)
	||  (recv.interrupt.rti_number != rti))
		return -1;

	return 0;
}
