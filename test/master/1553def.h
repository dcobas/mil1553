/* Definition des bits du CSR */

#ifndef DEFS1553
#define DEFS1553

#if 0
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
#endif
/* status register CSR for RTI */
#define TB_BIT    0x0001  /* 1 transf. buf. full  0 transf. buf. empty */
#define RB_BIT    0x0002  /* 1 recept. buf. full  0 recept. buf. empty */
#define INV_BIT   0x0004  /* 1 invalid message    0 valid message      */
#define RL_BIT    0x0008  /* 1 reset line                              */
#define INE_BIT   0x0010  /* 1 enabled interrupt  0 disabled interrupt */
#define INT_BIT   0x0020  /* 1 interrupt present  0 no interrupt       */
#define RTP_BIT   0x0040  /* 1 reset transmit pointer                  */
#define RRP_BIT   0x0080  /* 1 reset reception pointer                 */
#define BC_BIT    0x0100  /* 1 BC mode            0 RT mode            */
#define BCREQ_BIT 0x0200  /* 1 BC request                              */
#define LRREQ_BIT 0x0400  /* 1 local/remote request                    */
#define NEM_BIT   0x0800  /* 1 non expected message                    */
#define BRDIS_BIT 0x1000  /* 1 broadcast disable                       */
#define LM_BIT    0x2000  /* 1 local mode                              */
#define TM_BIT    0x4000  /* 1 test mode                               */
#define V5_BIT    0x8000  /* 1 5 Volt present     0 5 Volt not present */




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
