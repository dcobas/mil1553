#ifndef _LIBRTI_H
#define _LIBRTI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CSR_TB  0x0001
#define CSR_RB  0x0002
#define CSR_INV 0x0004
#define CSR_RL  0x0008
#define CSR_INE 0x0010
#define CSR_INT 0x0020
#define CSR_RTP 0x0040
#define CSR_RRP 0x0080
#define CSR_BC  0x0100
#define CSR_BCR 0x0200
#define CSR_LRR 0x0400
#define CSR_NEM 0x0800
#define CSR_BRD 0x1000
#define CSR_LOC 0x2000
#define CSR_TES 0x4000
#define CSR_OPE 0x8000


#define STR_TF  0x0001
#define STR_DBC 0x0002
#define STR_SF  0x0004
#define STR_BUY 0x0008
#define STR_BRO 0x0010
#define STR_TB  0x0020
#define STR_RB  0x0040
#define STR_TIM 0x0080
#define STR_SR  0x0100
#define STR_INS 0x0200
#define STR_ME  0x0400

#define STR_RTI_MASK 0xF800
#define STR_RTI_SHIFT 11

#define STR_RT0 0x0800
#define STR_RT1 0x1000
#define STR_RT2 0x2000
#define STR_RT3 0x4000
#define STR_RT4 0x8000

#define SA_CSR 1
#define SA_CLEAR_CSR 7
#define SA_SET_CSR 1
#define SA_RXBUF 2
#define SA_TXBUF 3
#define SA_SIGNATURE 30

#define SA_MODE 31
#define MODE_READ_STR 1
#define MODE_READ_LAST_STR 2
#define MODE_MASTER_RESET 8
#define MODE_READ_LAST_CMD 18

#define TR_READ 1
#define TR_WRITE 0

char *rtilib_csr_to_str(unsigned short csr);
char *rtilib_str_to_str(unsigned short str);
char *rtilib_sig_to_str(unsigned short sig);

#define NO_REPLY 1
#define REPLY 0

int rtilib_send_receive(int fn,
			int bc,
			int rti,
			int wc,
			int sa,
			int tr,
			int nreply,
			unsigned short *rxbuf,
			unsigned short *txbuf);


int rtilib_read_csr(int fn, int bc, int rti, unsigned short *csr, unsigned short *str);
int rtilib_clear_csr(int fn, int bc, int rti, unsigned short csr);
int rtilib_set_csr(int fn, int bc, int rti, unsigned short csr);
int rtilib_read_rxbuf(int fn, int bc, int rti, int wc, unsigned short *rxbuf);
int rtilib_write_rxbuf(int fn, int bc, int rti, int wc, unsigned short *txbuf);
int rtilib_read_txbuf(int fn, int bc, int rti, int wc, unsigned short *rxbuf);
int rtilib_write_txbuf(int fn, int bc, int rti, int wc, unsigned short *txbuf);
int rtilib_read_signature(int fn, int bc, int rti, unsigned short *sig);
int rtilib_read_str(int fn, int bc, int rti, unsigned short *str);
int rtilib_read_last_str(int fn, int bc, int rti, unsigned short *str);
int rtilib_master_reset(int fn, int bc, int rti);
int rtilib_read_last_cmd(int fn, int bc, int rti, unsigned short *cmd);
int rtilib_send_eqp(int fn, int bc, int rti, int wc, unsigned short *txbuf);
int rtilib_recv_eqp(int fn, int bc, int rti, int wc, unsigned short *rxbuf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LIBRTI_H */
