#ifndef _LIBQUICK_H
#define _LIBQUICK_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <errno.h>
#include <librti.h>

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


void mil1553_print_error(int cc);


/**
  * @brief send a raw quick data buffer network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_send_raw_quick_data_net(int fn, struct quick_data_buffer *quick_pt);

/**
  * @brief get a raw quick data buffer network order
  * @param file handle returned from the init routine
  * @param pointer to data buffer
  * @return 0 success, else standard system error
  *
  * Using this call on a power supply requires underatanding how data structures
  * need to be serialized. EXPERTS ONLY
  */

short mil1553_get_raw_quick_data_net(int fn, struct quick_data_buffer *quick_pt);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LIBQUICK_H */
