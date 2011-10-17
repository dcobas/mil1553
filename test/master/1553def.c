/**
 * This is needed to make banctest work
 * Julian
 */

#include <libmil1553.h>
#include <librti.h>

/**
 * ======================================
 */

static int mfn = 0;

int lib_init()
{
	if (mfn == 0) {
		mfn = milib_handle_open();
		if (mfn <= 0) {
			mfn = 0;
			return -1;
		}
	}
	return 0;
}

/**
 * ======================================
 * The driver handels polling its none of
 * the test programs buisness
 */

int stop_poll()
{
	return 0;
}

/**
 * ======================================
 * Provide a user level mdrop interface
 */

short mdrop(short bc, short rti, short tr, short sa, short wc, short *status, char *buf)
{
	int cc;
	struct mil1553_send_s send;
	struct mil1553_recv_s recv;
	struct mil1553_tx_item_s txitm;
	unsigned int txreg;
	bc++;

	milib_encode_txreg(&txreg,wc,sa,tr,rti);

	txitm.bc = bc;
	txitm.rti_number = rti;
	txitm.txreg = txreg;
	txitm.no_reply = 0;
	bcopy(buf,txitm.txbuf,wc*2);

	send.item_count = 1;
	send.tx_item_array = &txitm;

	bzero((void *) &recv, sizeof(struct mil1553_recv_s));
	recv.pk_type = TX_ALL;
	recv.timeout = 1;
	cc = milib_recv(mfn, &recv);

	cc = milib_send(mfn, &send);
	if (cc)
		{
// fprintf(stderr, "mdrop: error in milib_send for mdrop\n");
		milib_reset(mfn, bc);
		return -1;
		}
	bzero((void *) &recv, sizeof(struct mil1553_recv_s));
	recv.pk_type = TX_ALL;
	recv.timeout = 1000;
	cc = milib_recv(mfn, &recv);
	*status = recv.interrupt.rxbuf[0];
	if (cc)
		{ fprintf(stderr, "mdrop: error in milib_recv for mdrop\n");
		milib_reset(mfn, bc);
		return -1;
		}

	if ((recv.interrupt.bc != bc) ||  (recv.interrupt.rti_number != rti))
		{ fprintf(stderr, "mdrop: error in milib_recv for mdrop bad bc/rt %d/%d\n", (int)recv.interrupt.bc-1, (int)recv.interrupt.rti_number);
		milib_reset(mfn, bc);
		return -1;
		}

	bcopy(&recv.interrupt.rxbuf[tr],buf,TX_BUF_SIZE);

	return 0;
}

/**
 * ======================================
 * One bit set per installed BC
 */

int get_connected_bc(long *mask)
{
	int i, bcs_count;

	if (milib_get_bcs_count(mfn, &bcs_count))
		return -1;

	*mask = 0;
	for (i=0; i<bcs_count; i++)
	     *mask |= (1 << i);

	return 0;
}

/**
 * ======================================
 * Clear bits in the csr
 */

int clr_csr(unsigned short bc,
	    unsigned short rti,
	    unsigned short *csr,
	    unsigned short *status)
{
	bc++;
	if (rtilib_clear_csr(mfn, (int) bc, (int) rti, *csr))
		return -1;

	return 0;
}

/**
 * ======================================
 * Set bits in the csr
 */

int set_csr(unsigned short bc,
	    unsigned short rti,
	    unsigned short *csr,
	    unsigned short *status)
{
	bc++;
	if (rtilib_set_csr(mfn, (int) bc, (int) rti, *csr))
		return -1;

	return 0;
}

/**
 * ======================================
 */

int get_csr(unsigned short bc,
	    unsigned short rti,
	    unsigned short *csr,
	    unsigned short *status)
{
	bc++;
	if (rtilib_read_csr(mfn, (int) bc, (int) rti, csr, status))
		return -1;

	return 0;
}

/**
 * ======================================
 */

int get_tx_buf(unsigned short bc,
	       unsigned short rti,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status)
{
	bc++;
	if (rtilib_read_txbuf(mfn, (int) bc, (int) rti, (int) wc, buf))
		return -1;

	if (rtilib_read_str(mfn, (int) bc, (int) rti, status))
		return -1;

	return 0;
}

/**
 * ======================================
 */

int set_tx_buf(unsigned short bc,
	       unsigned short rti,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status)
{
	bc++;
	if (rtilib_write_txbuf(mfn, (int) bc, (int) rti, (int) wc, buf))
		return -1;

	if (rtilib_read_str(mfn, (int) bc, (int) rti, status))
		return -1;

	return 0;
}

/**
 * ======================================
 */

int get_rx_buf(unsigned short bc,
	       unsigned short rti,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status)
{
	bc++;
	if (rtilib_read_rxbuf(mfn, (int) bc, (int) rti, (int) wc, buf))
		return -1;

	if (rtilib_read_str(mfn, (int) bc, (int) rti, status))
		return -1;

	return 0;
}

/**
 * ======================================
 */

int set_rx_buf(unsigned short bc,
	       unsigned short rti,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status)
{
	bc++;
	if (rtilib_write_rxbuf(mfn, (int) bc, (int) rti, (int) wc, buf))
		return -1;

	if (rtilib_read_str(mfn, (int) bc, (int) rti, status))
		return -1;

	return 0;
}
