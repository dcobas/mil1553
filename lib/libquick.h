#ifndef LIBQUICK
#define LIBQUICK

#include <linux/errno.h>

#include <mil1553.h>
#include <libmil1553.h>
#include <librti.h>
#include <pow_messages.h>

/**
 * Implement the quick data interface for legacy code.
 *
 * Julian Lewis BE/CO/HT 17th March 2011
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

/**
 * Old error codes
 * This library just uses standard error numbers
 */

#define welldone         0            /* no error */
#define BC_not_connected EFAULT       /* destination BC is not known */
#define RT_not_connected ENODEV       /* destination RT is not known */
#define TB_not_set       ETIMEDOUT    /* destination RT has nothing to send */
#define RB_set           EBUSY        /* destination RT has a busy receive buffer */
#define Bad_buffer       EPROTO       /* the buffer contains bad information */
				      /* i.e., rt not in [0,31]; bc not in [1,31]; ... */

#define QDP_USZ 240 /** Quick Packet user data size in bytes (120 words MAX) */

/**
 * @brief Definition of the Quick Data buffer chain
 *
 * For the driver as well as for application programs using MIL1553B
 * @b QuickData packet type
 */
struct quick_data_buffer {
   char   bc;           /** BC number of the destination (1..31) */
   char   rt;           /** RTI number of the destination (1..30) */
   short  stamp;        /** 16 bits giving a user number */
   short  error;        /** field with an error by the driver */
   short  pktcnt;       /** length in BYTES of the data (max QDP_USZ). Should be multiple of word (i.e. 2 bytes long) */
   char   pkt[QDP_USZ]; /** data */

   struct quick_data_buffer *next; /** for chained buffers */
};

/**
 * @brief Open the mil1553 driver and initialize library
 * @return File handle greater than zero if successful, or zero on error
 *
 * The returned file handle is used in all subsequent calls to send and get.
 * Every thread should obtain its own file handle, there is no limit to the
 * number of concurrent threads that can use the library.
 * This is now thread safe, sorry I changed the API.
 */

int mil1553_init_quickdriver(void);

/**
  * @brief send a raw quick data buffer in host byte order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else negative standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_send_raw_quick_data(int fn, struct quick_data_buffer *quick_pt);

/**
  * @brief get a raw quick data buffer in host byte order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else negative standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_get_raw_quick_data(int fn, struct quick_data_buffer *quick_pt);

/**
  * @brief send a raw quick data buffer in network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else negative standard system error
  */

short mil1553_send_quick_data(int fn, struct quick_data_buffer *quick_pt);

/**
  * @brief get a raw quick data buffer from network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else negative standard system error
  */

short mil1553_get_quick_data(int fn, struct quick_data_buffer *quick_pt);

/**
 * These are the serialization routines that convert the structures defined
 * in pow_messages.h for transmition over the MIL1553 cable to/from a power supply.
 * Reading back a control message reflects the way floats are stored on the
 * power supply and requires special handling (serialize_read_ctrl_msg).
 * In general floats are just word swapped. You don't need to call these
 * functions if you are not using the raw send/receive routines.
 */

void serialize_req_msg(req_msg  *req_p);
void serialize_read_ctrl_msg(ctrl_msg *ctrl_p);
void serialize_write_ctrl_msg(ctrl_msg *ctrl_p);
void serialize_acq_msg(acq_msg *acq_p);
void serialize_conf_msg(conf_msg *conf_p);

void mil1553_print_error(int cc);
void mil1553_print_msg(struct quick_data_buffer *quick_pt, int rflag, int expect_service);
void mil1553_print_req_msg(req_msg *req_p);
void mil1553_print_ctrl_msg(ctrl_msg *ctrl_p);
void mil1553_print_acq_msg(acq_msg *acq_p);
void mil1553_print_conf_msg(conf_msg *conf_p);

/**
 * @brief Read Config message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param conf_ptr - Points to destination message
 * @return zero if OK else error
 */

int mil1553_read_cfg_msg(int fn, int bc, int rti, conf_msg *conf_ptr);

/**
 * @brief Read Acquisition message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param acq_ptr  - Points to destination message
 * @return zero if OK else error
 */

int mil1553_read_acq_msg(int fn, int bc, int rti, acq_msg *acq_ptr);

/**
 * @brief Read back Control message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param ctrl_ptr - Points to destination message
 * @return zero if OK else error
 */

int mil1553_read_ctrl_msg(int fn, int bc, int rti, ctrl_msg *ctrl_ptr);

/**
 * @brief Write Control message with locking and retry
 * @param fn       - File handle returned from the init routine
 * @param bc       - Bus controller 1..NB
 * @param rti      - RTI number 1..NR
 * @param ctrl_ptr - Points to source message
 * @return zero if OK else error
 */

int mil1553_write_ctrl_msg(int fn, int bc, int rti, ctrl_msg *ctrl_ptr);

#endif
