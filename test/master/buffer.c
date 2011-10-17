#include <stdio.h>
#include <string.h>
#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, XILINX, status;
extern int erreur, go_on, t_err;

#define BAD(str)  { print (1, str); erreur++; if (t_err) continue; }
#define BAD1(str) { print (1, str); erreur++; if (t_err) return t_err; }


int convbin (short number, char *string)
{
    int i;

    sprintf (string, "%s", "b");
    for (i = 0; i < 16; i++) {
	if (number < 0)
	    strcat (string, "1");
	else
	    strcat (string, "0");
	number = (number << 1);
    }
    string[i + 1] = 0;
    return (0);
}


int tst_rst_pointer (short flg,	/* bit du csr (RRP ou RTP) */
		     int (*fun_wr) (short, short, short, char *, unsigned short *),	/* ecrire dans le buffer (set_tx_buf ou set_rx_buf) */
		     int (*fun_rd) (short, short, short, char *, unsigned short *),	/* lire dans le buffer (get_tx_buf ou get_rx_buf) */
		     int (*fun_rst) (short, short, unsigned short *,
				     unsigned short *))
{				/* faire reset pointer (set_csr ou clr_csr) */
    unsigned short buf[4096], *pt, csr;
    unsigned short i, seuil;
    char str1[20], str2[20];
    int Fl_buf = 0;

    seuil = 128;		/* size in 16-bits words */
    if (flg & 0x80)
	Fl_buf = 0;
    /* Clear buffer */
    for (pt = buf, i = 0; i < seuil; i++)
	*pt++ = 0;
    if (fun_wr (bc, rt, seuil * 2, (char *) buf, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in write_T_buf\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in write_R_buf\n");
    }
    /* reset pointer */
    csr = flg;
    if (fun_rst (bc, rt, &csr, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in reset_T_pointer\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in reset_R_pointer\n");
    }
    /* write 32 words */
    for (pt = buf, i = 0; i < 32; i++)
	*pt++ = i;
    if (fun_wr (bc, rt, 64, (char *) buf, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in write_T_buf\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in write_R_buf\n");
    }
    /* reset pointer */
    csr = flg;
    if (fun_rst (bc, rt, &csr, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in reset_T_pointer\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in reset_R_pointer\n");
    }
    /* read 16 words and compare */
    for (pt = buf, i = 0; i < 16; i++)
	*pt++ = 0;
    if (fun_rd (bc, rt, 32, (char *) buf, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in read_T_buf\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in read_R_buf\n");
    }
    for (pt = buf, i = 0; i < 16; i++, pt++) {
	if (*pt != i) {
	    convbin (*pt, str1);
	    convbin (i, str2);
	    print (1, "\nDATA error in tst_rst_pointer:\n read [%s] write [%s]\n", str1, str2);
	    erreur++;
	    if (t_err)
		return (erreur);
	}
    }
    /* Set local bit and play with the pointer */
    csr = LM_BIT;
    if (set_csr (bc, rt, &csr, &status) == (-1))
	BAD1 ("\rtst_rst_pointer:System error in set LOCAL bit\n");
    /* reset pointer */
    csr = flg;
    if (fun_rst (bc, rt, &csr, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in reset_T_pointer\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in reset_R_pointer\n");
    }
    /* write 8 words */
    for (pt = buf, i = 0; i < 8; i++)
	*pt++ = (i * 4) + i;	/* other pattern */
    if (fun_wr (bc, rt, 16, (char *) buf, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in write_T_buf\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in write_R_buf\n");
    }
    /* read 8 words and compare. They should be 0xFFFF */
    for (pt = buf, i = 0; i < 8; i++)
	*pt++ = 0;
    if (fun_rd (bc, rt, 16, (char *) buf, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in read_T_buf\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in read_R_buf\n");
    }
    for (pt = buf, i = 0; i < 8; i++, pt++) {
	if (*pt != 0xFFFF) {
	    convbin (*pt, str1);
	    convbin (0xffff, str2);
	    print (1, "\nDATA error in tst_rst_pointer:\n read [%s] write [%s]\n", str1, str2);
	    erreur++;
	    if (t_err)
		return (erreur);
	}
    }
    /* Reset local bit and read again the buffer */
    csr = LM_BIT;
    if (clr_csr (bc, rt, &csr, &status) == (-1))
	BAD1 ("\rtst_rst_pointer:System error in reset LOCAL bit\n");
    /* read 16 words and compare */
    for (pt = buf, i = 0; i < 16; i++)
	*pt++ = 0;
    if (fun_rd (bc, rt, 32, (char *) buf, &status) == (-1)) {
	if (Fl_buf == 0)
	    BAD1 ("\rtst_rst_pointer:System error in read_T_buf\n")
	else
	    BAD1 ("\rtst_rst_pointer:System error in read_R_buf\n");
    }
    for (pt = buf, i = 16; i < 32; i++, pt++) {
	if (*pt != i) {
	    convbin (*pt, str1);
	    convbin (i, str2);
	    print (1, "\nDATA error in tst_rst_pointer:\n read [%s] write [%s]\n", str1, str2);
	    erreur++;
	    if (t_err)
		return (erreur);
	}
    }
    return 0;
}

int tst_buffer (short flg,	/* bit du csr (RRP ou RTP) */
		int (*fun_wr) (short, short, short, char *, unsigned short *),	/* fonction pour ecrire dans le buffer (set_tx_buf ou set_rx_buf) */
		int (*fun_rd) (short, short, short, char *, unsigned short *),	/* fonction pour lire dans le buffer (get_tx_buf ou get_rx_buf) */
		char *str)
{

    unsigned short buf1[4096], buf2[4096], *pt, *pt1, csr;
    short i;
    char str1[20], str2[20];
    int data_code, sycle_pass, lim, size_of_portion, Fl_buf = 0;

    sycle_pass = 0;
    data_code = 1;
    if (flg & 0x80)
	Fl_buf = 0;
    do {
	full_buff (data_code, buf1);
	for (size_of_portion = 2; size_of_portion <= 256;
	     size_of_portion += 2) {

	    /* clear buffer */
	    print (2, "clear buffer\n");
	    for (pt = buf2, i = 0; i < 128; i++)
		*pt++ = 0;	/* clear buffer */
	    if (fun_wr (bc, rt, size_of_portion, (char *) buf2, &status) ==
		(-1)) {
		if (Fl_buf == 0)
		    BAD1 ("\rtst_rst_pointer:System error in write_T_buf\n")
		else
		    BAD1 ("\rtst_rst_pointer:System error in write_R_buf\n");
	    }
	    /* reset pointer */
	    csr = flg;
	    print (2, "reset pointer; ");
	    if (set_csr (bc, rt, &csr, &status) == (-1)) {
		if (Fl_buf == 0)
		    BAD1 ("\rtst_buffer:System error in reset_T_pointer\n")
		else
		    BAD1 ("\rtst_buffer:System error in reset_R_pointer\n");
	    }
	    /* Send data in one chunk */
	    print (2, "writing %d words to the buffer\n", size_of_portion);
	    if (fun_wr (bc, rt, size_of_portion, (char *) buf1, &status) ==
		(-1)) {
		if (Fl_buf == 0)
		    BAD1 ("\rtst_buffer:System error in write_T_buf\n")
		else
		    BAD1 ("\rtst_buffer:System error in write_R_buf\n");
	    }
	    /* reset pointer */
	    csr = flg;
	    print (2, "reset pointer; ");
	    if (set_csr (bc, rt, &csr, &status) == (-1)) {
		if (Fl_buf == 0)
		    BAD1 ("\rtst_buffer:System error in reset_T_pointer\n")
		else
		    BAD1 ("\rtst_buffer:System error in reset_R_pointer\n");
	    }
	    /* Read data in one chunk */
	    print (2, "reading %d words to the buffer\n", size_of_portion);
	    if (fun_rd (bc, rt, size_of_portion, (char *) buf2, &status) ==
		(-1)) {
		if (Fl_buf == 0)
		    BAD1 ("\rtst_buffer:System error in read_T_buf\n")
		else
		    BAD1 ("\rtst_buffer:System error in read_R_buf\n");
	    }
	    /* compare data */
	    lim = size_of_portion / 2;
	    print (2, "compare data\n");
	    for (pt = buf1, pt1 = buf2, i = 0; i < lim; i++, pt++, pt1++)
		if (*pt != *pt1) {
		    convbin (*pt, str1);
		    convbin (*pt1, str2);
		    print (1, "\rDATA error in tst_buffer:\tPointer %d\n  read [%s]\n write [%s]\n", i, str1, str2);
		    erreur++;
		}
	    if (data_code == 6)
		size_of_portion += 8;
	    if (go_on == 0)
		break;
	}
	if (data_code == 6) {
	    data_code--;
	    if (++sycle_pass == 16)
		data_code++;
	}
    } while ((++data_code < 7) && (go_on != 0));
    return (erreur);
}

void full_buff (int data_code, unsigned short *buf1)
{
    int i, k;
    char temp;

    switch (data_code) {
    case 1:
	for (i = 0; i < 128; i++)
	    *buf1++ = 0xaaaa;
	break;
    case 2:
	for (i = 0; i < 128; i++)
	    *buf1++ = 0x5555;
	break;
    case 3:
	for (i = 0; i < 128; i++)
	    *buf1++ = 0xffff;
	break;
    case 4:
	for (i = 0; i < 128; i++) {
	    *buf1++ = 0xff00;
	}
	break;
    case 5:
	for (i = 0; i < 128; i++) {
	    *buf1++ = 0x00ff;
	}
	break;
    case 6:
	for (i = 0; i <= 9; i++) {
	    *buf1 = 0xfffe;
	    for (k = 0; k < 16; k++) {
		temp = *buf1++;
		*buf1 = (temp << 1) + 1;
	    }
	}
	break;
    default:
	break;
    }
}

int tst_buff (short flg,	/* bit du csr (RRP ou RTP) */
	      int (*fun_wr) (short, short, short, char *, unsigned short *),	/* fonction pour ecrire dans le buffer (set_tx_buf ou set_rx_buf) */
	      int (*fun_rd) (short, short, short, char *, unsigned short *),	/* fonction pour lire dans le buffer (get_tx_buf ou get_rx_buf) */
	      char *str)
{
    int nloops;

    nloops = 0;
    do {
	if (tst_buffer (flg, fun_wr, fun_rd, str) != 0) {
#if 0
	    printf ("N = %d",nloops);
#endif
	    break;
	}
    } while (nloops-- > 0);
    if (erreur == 0)
	print (1, " OK");
    return (erreur);
}


int rx_buf (void)
{
    erreur = 0;
    tst_rst_pointer (RRP_BIT, set_rx_buf, get_rx_buf, set_csr);
    if (XILINX)
	tst_rst_pointer (RRP_BIT, set_rx_buf, get_rx_buf, clr_csr);

    erreur = 0;
    go_on = 1;
    tst_buff (RRP_BIT, set_rx_buf, get_rx_buf, "receive");
    return (erreur);
}

int tx_buf (void)
{
    erreur = 0;
    tst_rst_pointer (RTP_BIT, set_tx_buf, get_tx_buf, set_csr);
    if (XILINX)
	tst_rst_pointer (RTP_BIT, set_tx_buf, get_tx_buf, clr_csr);

    erreur = 0;
    go_on = 1;
    tst_buff (RTP_BIT, set_tx_buf, get_tx_buf, "transmit");
    return (erreur);
}
