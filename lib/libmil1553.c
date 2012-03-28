/**
 * Julian Lewis March 28 2012 BE/CO/HT
 * Julian.Lewis@cern.ch
 *
 * MIL 1553 library for CBMIA FW version 204 and later
 *
 * This code relies on a new firmware version number 204 and later
 * In this version proper access to the TXREG uses a busy done bit.
 * Software polling has been implemented, hardware polling is removed.
 * The bus speed is fixed at 1Mbit.
 * Hardware test points and diagnostic/debug registers are added.
 */

#include <libmil1553.h>
#include <errno.h>

int milib_handle_open() {

	int cc;
	cc = open(DEV_PATH,O_RDWR,0);
	return cc;
}

int milib_set_polling(int fn, int flag) {

	int cc;
	unsigned long reg;
	if (flag)
		reg = 1;
	else
		reg = 0;
	cc = ioctl(fn,MIL1553_SET_POLLING,&reg);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_polling(int fn, int *flag) {

	int cc;
	unsigned long reg;
	cc = ioctl(fn,MIL1553_GET_POLLING,&reg);
	if (cc < 0)
		return errno;
	if (reg)
		*flag = 0;
	else
		*flag = 1;
	return 0;
}

int milib_set_test_point(int fn, int bc, int tp) {

	int cc;
	unsigned long reg = (tp << 16) | bc;
	cc = ioctl(fn,MIL1553_SET_TP,&reg);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_test_point(int fn, int bc, int *tp) {

	int cc;
	unsigned long reg = bc;
	cc = ioctl(fn,MIL1553_GET_TP,&reg);
	if (cc < 0)
		return errno;
	*tp = reg >> 16;
	return 0;
}

int milib_set_timeout(int fn, int timeout_msec) {

	int cc;
	unsigned long reg = timeout_msec;
	cc = ioctl(fn,MIL1553_SET_TIMEOUT_MSEC,&reg);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_timeout(int fn, int *timeout_msec) {

	int cc;
	unsigned long reg = 0;
	cc = ioctl(fn,MIL1553_GET_TIMEOUT_MSEC,&reg);
	if (cc < 0)
		return errno;
	*timeout_msec = reg;
	return 0;
}

int milib_set_debug_level(int fn, int debug_level) {

	int cc;
	unsigned long reg = debug_level;
	cc = ioctl(fn,MIL1553_SET_DEBUG_LEVEL,&reg);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_debug_level(int fn, int *debug_level) {

	int cc;
	unsigned long reg = 0;
	cc = ioctl(fn,MIL1553_GET_DEBUG_LEVEL,&reg);
	if (cc < 0)
		return errno;
	*debug_level = reg;
	return 0;
}

int milib_get_drv_version(int fn, int *version) {

	int cc;
	unsigned long reg = 0;
	cc = ioctl(fn,MIL1553_GET_DRV_VERSION,&reg);
	if (cc < 0)
		return errno;
	*version = reg;
	return 0;
}

int milib_get_status(int fn, int bc, int *status) {

	int cc;
	unsigned long reg = bc;
	cc = ioctl(fn,MIL1553_GET_STATUS,&reg);
	if (cc < 0)
		return errno;
	*status = reg;
	return 0;
}

int milib_get_bcs_count(int fn, int *bcs_count) {

	int cc;
	unsigned long reg = 0;
	cc = ioctl(fn,MIL1553_GET_BCS_COUNT,&reg);
	if (cc < 0)
		return errno;
	*bcs_count = reg;
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
	unsigned long reg = bc;
	cc = ioctl(fn,MIL1553_GET_UP_RTIS,&reg);
	if (cc < 0)
		return errno;
	*up_rtis = reg;
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
	unsigned long reg = 0;
	cc = ioctl(fn,MIL1553_QUEUE_SIZE,&reg);
	if (cc < 0)
		return errno;
	*size = reg;
	return 0;
}

int milib_reset(int fn, int bc) {

	int cc;
	unsigned long reg = bc;
	cc = ioctl(fn,MIL1553_RESET,&reg);
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

char *stat_names[16] = { "Bit0:", "Bit1:", "Bit2:", "Bit3:",
			 "Bit4:", "Bit5:", "Bit6:", "Bit7:",
			 "Bit8:", "Bit9:", "BitA:", "BitB:",
			 "BitC:", "BitD:", "BitE:", "Busy:" };

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

	if (wc) {
		*wc  = (txreg & TXREG_WC_MASK)   >> TXREG_WC_SHIFT;
		if (*wc == 0)
			*wc = 32;
	}
	if (sa)
		*sa  = (txreg & TXREG_SUBA_MASK) >> TXREG_SUBA_SHIFT;
	if (tr)
		*tr  = (txreg & TXREG_TR_MASK)   >> TXREG_TR_SHIFT;
	if (rti)
		*rti = (txreg & TXREG_RTI_MASK)  >> TXREG_RTI_SHIFT;
}

void milib_encode_txreg(unsigned int *txreg, unsigned int wc, unsigned int sa, unsigned int tr, unsigned int rti) {

	if (wc >= 32)
		wc = 0;
	if (txreg)
		*txreg = ((wc  << TXREG_WC_SHIFT)   & TXREG_WC_MASK)
		       | ((sa  << TXREG_SUBA_SHIFT) & TXREG_SUBA_MASK)
		       | ((tr  << TXREG_TR_SHIFT)   & TXREG_TR_MASK)
		       | ((rti << TXREG_RTI_SHIFT)  & TXREG_RTI_MASK);
}

int milib_lock_bc(int fn, int bc) {

	int cc = 0;
	unsigned long reg = bc;
	cc = ioctl(fn,MIL1553_LOCK_BC,&reg);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_unlock_bc(int fn, int bc) {

	int cc = 0;
	unsigned long reg = bc;
	cc = ioctl(fn,MIL1553_UNLOCK_BC,&reg);
	if (cc < 0)
		return errno;
	return 0;
}

int milib_get_temperature(int fn, int bc, float *temp) {

	int ip, fp, cc = 0;
	unsigned long reg = bc;
	cc = ioctl(fn,MIL1553_GET_TEMPERATURE,&reg);
	if (cc < 0)
		return errno;

	ip = (reg & 0x7F0) >> 4;
	if (reg & 0xF800) ip = -ip;
	fp = reg & 0xF;

	*temp = (float) ip;
	if (fp)
		*temp += 1.0 / (float) fp;
	return 0;
}
