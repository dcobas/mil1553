/* Definition des bits du CSR */

#ifndef DEFS1553
#define DEFS1553

#define TB  0x0001
#define RB  0x0002
#define INV 0x0004
#define RL  0x0008
#define INE 0x0010
#define INT 0x0020
#define RTP 0x0040
#define RRP 0x0080
#define BC  0x0100
#define BCR 0x0200
#define LRR 0x0400
#define NEM 0x0800
#define BRD 0x1000
#define LOC 0x2000
#define TES 0x4000
#define V5  0x8000

int lib_init();

int stop_poll();

int get_connected_bc(long mask);

int clr_csr(unsigned short bc,
	    unsigned short rt,
	    unsigned short *csr,
	    unsigned short *status);

int set_csr(unsigned short bc,
	    unsigned short rt,
	    unsigned short *csr,
	    unsigned short *status);

int get_csr(unsigned short bc,
	    unsigned short rt,
	    unsigned short *csr,
	    unsigned short *status);

int get_tx_buf(unsigned short bc,
	       unsigned short rt,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status);

int set_tx_buf(unsigned short bc,
	       unsigned short rt,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status);

int get_rx_buf(unsigned short bc,
	       unsigned short rt,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status);

int set_rx_buf(unsigned short bc,
	       unsigned short rt,
	       unsigned short wc,
	       unsigned short *buf,
	       unsigned short *status);


#endif
