/* main.c */
extern void ter_exit(void);
extern void inter(int sig);
extern void prologue(void);
extern void epilogue(void);
extern int config(void);
extern void logfile(void);
extern void menu_3(void);
extern void menu_2(void);
extern void menu_1(void);
extern void menu_0(void);
extern int comm_tst(void);
extern int comm_exe(void);
extern int seq_tst(void);
extern int exe_seq(void);
extern int accept_cr(void);
extern int accept_pz(void);
extern int taccept(void);
extern int not_test(void);
extern int exec(int indx, int tr);
extern void init(void);
extern int main(int argc, char **argv);
/* bounce.c */
extern int bounce(void);
extern int collision(void);
/* broadcst.c */
extern int broadcst(void);
/* buffer.c */
extern int convbin(short number, char *string);
extern int tst_rst_pointer(short flg, int (*fun_wr)(short, short, short, char *, unsigned short *), int (*fun_rd)(short, short, short, char *, unsigned short *), int (*fun_rst)(short, short, unsigned short *, unsigned short *));
extern int tst_buffer(short flg, int (*fun_wr)(short, short, short, char *, unsigned short *), int (*fun_rd)(short, short, short, char *, unsigned short *), char *str);
extern void full_buff(int data_code, unsigned short *buf1);
extern int tst_buff(short flg, int (*fun_wr)(short, short, short, char *, unsigned short *), int (*fun_rd)(short, short, short, char *, unsigned short *), char *str);
extern int rx_buf(void);
extern int tx_buf(void);
/* tst_csr.c */
extern int rd_csr(void);
extern int wr_csr(void);
extern int cl_csr(void);
/* csr.c */
extern int tst_csr(void);
/* mode_code.c */
extern int mode_code(void);
/* mode_data.c */
extern int mode_data(void);
/* procedures.c */
extern void clr_line(void);
extern int verifie_ST(unsigned short status, unsigned short csr);
extern int valid_MC(unsigned short rt, unsigned short tr, unsigned short sa, unsigned short wc, unsigned short data);
extern int valid_ST(unsigned short rt, unsigned short data);
extern int wait_for_csr(int pattern, int res, int delai, int flg);
extern int Clr_csr(unsigned short pat, char *st);
extern int Set_csr(unsigned short pat, char *st);
extern int reset(void);
/* choix_rt.c */
extern int choix_rt(void);
/* signat.c */
extern int bit_test(void);
extern int rt30(void);
extern int signature(void);
extern int bit_local(void);
/* print.c */
extern void print (int flg, char *str, ...)  __attribute__ ((__format__ (__printf__, 2, 3)));
