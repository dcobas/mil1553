#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <strings.h>
#include <string.h>

#include <mil1553.h>

#define DEV_NAME "mil1553"
#define DEV_PATH "/dev/mil1553"

int milib_handle_open();
int milib_set_timeout(int fn, int timeout_msec);
int milib_get_timeout(int fn, int *timeout_msec);
int milib_set_debug_level(int fn, int debug_level);
int milib_get_debug_level(int fn, int *debug_level);
int milib_get_drv_version(int fn, int *version);
int milib_get_status(int fn, int bc, int *status);
int milib_get_bcs_count(int fn, int *bcs_count);
int milib_get_bc_info(int fn, struct mil1553_dev_info_s *dev_info);
int milib_raw_read(int fn, struct mil1553_riob_s *riob);
int milib_raw_write(int fn, struct mil1553_riob_s *riob);
int milib_get_up_rtis(int fn, int bc, int *up_rtis);
int milib_send(int fn, struct mil1553_send_s *send);
int milib_recv(int fn, struct mil1553_recv_s *recv);
int milib_write_reg(int fn, int bc, int reg_num, int reg_val);
int milib_read_reg(int fn, int bc, int reg_num, int *reg_val);
char *milib_status_to_str(int stat);
void milib_decode_txreg(unsigned int txreg, unsigned int *wc, unsigned int *sa, unsigned int *tr, unsigned int *rti);
void milib_encode_txreg(unsigned int *txreg, unsigned int wc, unsigned int sa, unsigned int tr, unsigned int rti);
int milib_lock_bc(int fn, int bc);
int milib_unlock_bc(int fn, int bc);
