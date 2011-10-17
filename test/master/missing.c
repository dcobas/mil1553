/* Routines currently missing on L865 from libquick.c in /user/pedror/DRIVERS/m1553/lib */

#include <errno.h>
#include "1553def.h"


int set_rx_buf (short bc, short rt, short bcnt, char *buf, unsigned short *sta)
{
    int status;
    int wc = 0;
    int i = 0;

    if (bcnt % 2) {
	errno = EINVAL;
	return (-1);
    }				/* This driver can only read 16 bit words.. */
    while (bcnt > 0) {
	wc = bcnt / 2;
	bcnt = (wc >= 32) ? bcnt - 64 : bcnt - (wc * 2);
	status = mdrop (bc, rt, W_MODE, SA_W_REC_BUF, ((wc >= 32) ? 0 : wc), sta, buf + (64 * i));
	i++;
	if (status != 0)
	    return (-1);
    }
    return (0);
}				/* end routine set_rx_buf */

/************************************************************************/
int get_rx_buf (short bc, short rt, short bcnt, char *buf, unsigned short *sta)
{
    int status;
    int wc = 0;
    int i = 0;

    if (bcnt % 2) {
	errno = EINVAL;
	return (-1);
    }				/* This driver can only read 16 bit words.. */

    while (bcnt > 0) {
	wc = bcnt / 2;
	bcnt = (wc >= 32) ? bcnt - 64 : bcnt - (wc * 2);
	status = mdrop (bc, rt, R_MODE, SA_R_REC_BUF, (wc >= 32) ? 0 : wc, sta, buf + (64 * i));
	i++;
	if (status != 0)
	    return (-1);
    }
    return (0);
}				/* end routine get_rx_buf */

/************************************************************************/
int set_tx_buf (short bc, short rt, short bcnt, char *buf, unsigned short *sta)
{
    int status;
    int wc = 0;
    int i = 0;

    if (bcnt % 2) {
	errno = EINVAL;
	return (-1);
    }				/* This driver can only read 16 bit words.. */

    while (bcnt > 0) {
	wc = bcnt / 2;
	bcnt = (wc >= 32) ? bcnt - 64 : bcnt - (wc * 2);
	status = mdrop (bc, rt, W_MODE, SA_W_TR_BUF, (wc >= 32) ? 0 : wc, sta, buf + (64 * i));
	i++;
	if (status != 0)
	    return (-1);
    }
    return (0);
}				/* end routine set_tx_buf */

/************************************************************************/
int get_tx_buf (short bc, short rt, short bcnt, char *buf, unsigned short *sta)
{
    int status;
    int wc = 0;
    int i = 0;

    if (bcnt % 2) {
	errno = EINVAL;
	return (-1);
    }				/* This driver can only read 16 bit words.. */

    while (bcnt > 0) {
	wc = bcnt / 2;
	bcnt = (wc >= 32) ? bcnt - 64 : bcnt - (wc * 2);
	status = mdrop (bc, rt, R_MODE, SA_R_TR_BUF, (wc >= 32) ? 0 : wc, sta, buf + (64 * i));
	i++;
	if (status != 0)
	    return (-1);
    }
    return (0);

}				/* end routine get_tx_buf */


/************************************************************************/
int get_csr (short bc, short rt, unsigned short *csr, unsigned short *sta)
{
    int status;

    if ((status = mdrop (bc, rt, R_MODE, SA_R_CSR, 1, sta, (char *) csr)) != 0)
	return (-1);
    return (0);

}				/* end routine get_csr */


/************************************************************************/
int set_csr (short bc, short rt, unsigned short *csr, unsigned short *sta)
{
    int status;

    if ((status = mdrop (bc, rt, W_MODE, SA_S_CSR, 1, sta, (char *) csr)) != 0)
	return (-1);
    return (0);
}				/* end routine set_rt_csr */


/************************************************************************/
int clr_csr (short bc, short rt, unsigned short *csr, unsigned short *sta)
{
    int status;

    if ((status = mdrop (bc, rt, W_MODE, SA_C_CSR, 1, sta, (char *) csr)) != 0)
	return (-1);
    return (0);

}				/* end routine clr_csr */


/************************************************************************/
int stop_poll (void)
{
  /** polling is always stopped in QuickData driver **/
    return (0);
}
