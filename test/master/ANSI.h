/* tst_csr.c */
int rd_csr (void)
int wr_csr (void)
int cl_csr (void)
/* csr.c */
int tst_csr (void)
/* print.c */
static void PANIC (char *, int)
static void Serv_Log (char *)
void print (int, char *, void *, void *, void *, void *, void *)
/* bounce.c */
int bounce (void)
int collision (void)
/* broadcst.c */
int broadcst (void)
/* buffer.c */
int convbin (int, char *)
int tst_rst_pointer (int, int  (*) (void), int  (*) (void), int  (*) (void))
int tst_buffer (int, int  (*) (void), int  (*) (void), char *)
int full_buff (int, unsigned short *)
int tst_buff (int, int  (*) (void), int  (*) (void), char *)
int rx_buf (void)
int tx_buf (void)
/* mode_code.c */
int mode_code (void)
/* mode_data.c */
int mode_data (void)
/* procedures.c */
void clr_line (void)
int verifie_ST (int, int)
int valid_MC (int, int, int, int, int)
int valid_ST (int, int)
int wait_for_csr (int, int, int, int)
int Clr_csr (int, char *)
int Set_csr (int, char *)
int reset (void)
/* choix_rt.c */
int choix_rt (void)
/* signat.c */
void bit_test (void)
int rt30 (void)
int signature (void)
int bit_local (void)
/* 1553def.c */
int lib_init (void)
int stop_poll (void)
short mdrop (short, short, short, short, short, short *, char *)
int get_connected_bc (long *)
int clr_csr (unsigned short, unsigned short, unsigned short *, unsigned short *)
int set_csr (unsigned short, unsigned short, unsigned short *, unsigned short *)
int get_csr (unsigned short, unsigned short, unsigned short *, unsigned short *)
int get_tx_buf (unsigned short, unsigned short, unsigned short, unsigned short *, unsigned short *)
int set_tx_buf (unsigned short, unsigned short, unsigned short, unsigned short *, unsigned short *)
int get_rx_buf (unsigned short, unsigned short, unsigned short, unsigned short *, unsigned short *)
int set_rx_buf (unsigned short, unsigned short, unsigned short, unsigned short *, unsigned short *)
