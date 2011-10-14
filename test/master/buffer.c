#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>        /* Error numbers */
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>
#include <termios.h>
#include <string.h>

#include <mil1553.h>
#include <libmil1553.h>

#include <1553def.h>

extern unsigned short bc,rt,XILINX,status;
extern int erreur,go_on,t_err;

#define LOC 0x2000
#define RRP 0x0080
#define RTP 0x0040

#define BAD(str) {print(1,str);erreur++;if (t_err) continue;}
#define BAD1(str) {print(1,str);erreur++;if (t_err) return;}

int convbin (number, string)
short number;
char *string;
{

int i;
    sprintf (string,"%s","b");
    for (i = 0; i < 16; i++) {
	if (number < 0)
	    strcat(string, "1");
	    else strcat(string, "0");
	number = number <<= 1;
	}
    string [i+1] = 0;   
    return (0);
    }


int tst_rst_pointer(flg,fun_wr,fun_rd,fun_rst)
short flg;		/* bit du csr (RRP ou RTP) */
int (*fun_wr)();	/* fonction pour ecrire dans le buffer (set_tx_buf ou set_rx_buf) */
int (*fun_rd)();	/* fonction pour lire dans le buffer (get_tx_buf ou get_rx_buf) */
int (*fun_rst)();	/* fonction pour faire reset pointer (set_csr ou clr_csr) */
{
	unsigned short buf[4096],*pt,csr;
	unsigned short i,seuil;
	char str1[20], str2[20];
	int Fl_buf;

	seuil=128;	/* size in 16-bits words */
	if (flg & 0x80) Fl_buf = 0;
	/* Clear buffer */
/*	print(2,"Clear r/t buffer of the RT-card\n");*/	
	for (pt=buf, i=0; i<seuil; i++) *pt++=0;
	if (fun_wr(bc,rt,seuil*2,buf,&status)==-1) {
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in write_T_buf\n")
		else BAD1("\rtst_rst_pointer:System error in write_R_buf\n");
	    }
	/* reset pointer */
/*	print(2,"Reset pointer\n");*/	
	csr=flg;
	if (fun_rst(bc,rt,&csr,&status)==-1) {
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in reset_T_pointer\n")
		else BAD1("\rtst_rst_pointer:System error in reset_R_pointer\n");
	    }
	/* write 32 words */
/*	print(2,"Write 64 bytes\n");*/	
	for (pt=buf,i=0;i<32;i++) *pt++=i;
	if (fun_wr(bc,rt,64,buf,&status)==-1) { 
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in write_T_buf\n")
		else BAD1("\rtst_rst_pointer:System error in write_R_buf\n");
	    }
	/* reset pointer */
/*	print(2,"Reset pointer\n");*/	
	csr=flg;
	if (fun_rst(bc,rt,&csr,&status)==-1) { 
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in reset_T_pointer\n")
		else BAD1("\rtst_rst_pointer:System error in reset_R_pointer\n");
	    }
	/* read 16 words and compare */
/*	print(2,"Read 32 bytes\n");*/	
	for (pt=buf,i=0;i<16;i++) *pt++=0;
	if (fun_rd(bc,rt,32,buf,&status)==-1) { 
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in read_T_buf\n")
		else BAD1("\rtst_rst_pointer:System error in read_R_buf\n");
	    }
	for (pt=buf,i=0;i<16;i++,pt++) {
	    if (*pt!=i) {
		convbin(*pt, str1);
		convbin(i, str2);
		print(1,"\nDATA error in tst_rst_pointer:\n read [%s] write [%s]\n", str1, str2);
		erreur++;
		if (t_err) return(erreur);
		}
	    }
	/* Set local bit and play with the pointer */
/*	print(2,"Set local bit\n");*/	
	csr=LOC;
	if (set_csr(bc,rt,&csr,&status)==-1) 
	    BAD1("\rtst_rst_pointer:System error in set LOCAL bit\n");
	/* reset pointer */
	csr=flg;
	if (fun_rst(bc,rt,&csr,&status)==-1) { 
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in reset_T_pointer\n")
		else BAD1("\rtst_rst_pointer:System error in reset_R_pointer\n");
	    }
	/* write 8 words */
/*	print(2,"Write 16 bytes\n");*/	
	for (pt=buf,i=0;i<8;i++) *pt++=(i*4)+i;	/* other pattern */
	if (fun_wr(bc,rt,16,buf,&status)==-1) { 
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in write_T_buf\n")
		else BAD1("\rtst_rst_pointer:System error in write_R_buf\n");
	    }
	/* read 8 words and compare. They should be 0xFFFF */
/*	print(2,"Read 16 bytes\n");*/	
	for (pt=buf,i=0;i<8;i++) *pt++=0;
	if (fun_rd(bc,rt,16,buf,&status)==-1) { 
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in read_T_buf\n")
		else BAD1("\rtst_rst_pointer:System error in read_R_buf\n");
	    }
	for (pt=buf,i=0;i<8;i++,pt++) {
	    if (*pt!=0xFFFF) {
		convbin (*pt,str1);
		convbin (0xffff,str2);
		print(1,"\nDATA error in tst_rst_pointer:\n read [%s] write [%s]\n", str1, str2);
		erreur++;
		if (t_err) return(erreur);
		}
	    }
	/* Reset local bit and read again the buffer */
/*	print(2,"Reset local bit\n");*/	
	csr=LOC;
	if (clr_csr(bc,rt,&csr,&status)==-1) 
	    BAD1("\rtst_rst_pointer:System error in reset LOCAL bit\n");
	/* read 16 words and compare */
/*	print(2,"Read 32 bytes\n");*/	
	for (pt=buf,i=0;i<16;i++) *pt++=0;
	if (fun_rd(bc,rt,32,buf,&status)==-1) {
	    if (Fl_buf == 0) 
		BAD1("\rtst_rst_pointer:System error in read_T_buf\n")
		else BAD1("\rtst_rst_pointer:System error in read_R_buf\n");
	    }
	for (pt=buf,i=16;i<32;i++,pt++) {
	    if (*pt!=i) {
		convbin(*pt, str1);
		convbin(i, str2);
		print(1,"\nDATA error in tst_rst_pointer:\n read [%s] write [%s]\n", str1, str2);
		erreur++;
		if (t_err) return(erreur);
		}
	    }
}
	
int tst_buffer(flg,fun_wr,fun_rd,str)
short flg;		/* bit du csr (RRP ou RTP) */
int (*fun_wr)();	/* fonction pour ecrire dans le buffer (set_tx_buf ou set_rx_buf) */
int (*fun_rd)();	/* fonction pour lire dans le buffer (get_tx_buf ou get_rx_buf) */
char *str;
{

	unsigned short buf1[4096],buf2[4096],*pt,*pt1,csr;
	unsigned char *cpt, *wpt;
	short i;
	char str1[20], str2[20];
	int data_code, sycle_pass, lim, size_of_portion,Fl_buf;
	long l=0;

	sycle_pass = 0;
	data_code = 1;
	if (flg & 0x80) Fl_buf = 0;
	do  {
	    full_buff(data_code,buf1);
	    for (size_of_portion = 2; size_of_portion <=256; size_of_portion += 2) {

		/* clear buffer */
		print (2,"clear buffer\n");
		for (pt=buf2,i=0;i<128;i++) *pt++=0;	/* clear buffer */
		if (fun_wr(bc,rt,size_of_portion,buf2,&status)==-1) 
		    if (Fl_buf == 0) 
			BAD1("\rtst_rst_pointer:System error in write_T_buf\n")
			else BAD1("\rtst_rst_pointer:System error in write_R_buf\n");

		/* reset pointer */
		csr=flg;
		print (2,"reset pointer; ");
		if (set_csr(bc,rt,&csr,&status)==-1) 
		    if (Fl_buf == 0) 
			BAD1("\rtst_buffer:System error in reset_T_pointer\n")
			else BAD1("\rtst_buffer:System error in reset_R_pointer\n");

		/* Send data in one chunk */
		print (2,"writing %d words to the buffer\n",size_of_portion);
		if (fun_wr(bc,rt,size_of_portion,buf1,&status)==-1) 
		    if (Fl_buf == 0) 
			BAD1("\rtst_buffer:System error in write_T_buf\n")
			else BAD1("\rtst_buffer:System error in write_R_buf\n");

		/* reset pointer */
		csr=flg;
		print (2,"reset pointer; ");
		if (set_csr(bc,rt,&csr,&status)==-1) 
		    if (Fl_buf == 0) 
			BAD1("\rtst_buffer:System error in reset_T_pointer\n")
			else BAD1("\rtst_buffer:System error in reset_R_pointer\n");

		/* Read data in one chunk */
		print (2,"reading %d words to the buffer\n",size_of_portion);
		if (fun_rd(bc,rt,size_of_portion,buf2,&status)==-1) 
		    if (Fl_buf == 0) 
			BAD1("\rtst_buffer:System error in read_T_buf\n")
			else BAD1("\rtst_buffer:System error in read_R_buf\n");

		/* compare data */
		lim = size_of_portion/2;
		print (2,"compare data\n");
		for (pt=buf1,pt1=buf2,i=0;i<lim;i++,pt++,pt1++)
		    if (*pt!=*pt1) {
			convbin(*pt, str1);
			convbin(*pt1, str2);
			print(1,"\rDATA error in tst_buffer:\tPointer %d\n  read [%s]\n write [%s]\n",i, str1, str2);
			erreur++;
			}
		if (data_code == 6) 
		    size_of_portion += 8;
		if (go_on == 0) break;
		}
	    if (data_code == 6) {
		data_code--;
		if (++sycle_pass == 16)
		    data_code++;
		} 
	    } while ((++data_code < 7) && (go_on != 0));
	return(erreur);
}

int full_buff(data_code,buf1)			
int data_code;
unsigned short *buf1;
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

int tst_buff(flg,fun_wr,fun_rd,str)
short flg;		/* bit du csr (RRP ou RTP) */
int (*fun_wr)();	/* fonction pour ecrire dans le buffer (set_tx_buf ou set_rx_buf) */
int (*fun_rd)();	/* fonction pour lire dans le buffer (get_tx_buf ou get_rx_buf) */
char *str;
{
int nloops;

	nloops = 0;
	do  {
	    if (tst_buffer(flg,fun_wr,fun_rd,str) != 0) {
/*		printf ("N = %d",nloops); 		*/
		break;							
		}
	    } while (nloops-- > 0);
	if (erreur == 0)
	    print (1," OK"); 
	return(erreur);
}


int rx_buf()
{
	erreur=0;
	tst_rst_pointer(RRP,set_rx_buf,get_rx_buf,set_csr);
	if (XILINX) tst_rst_pointer(RRP,set_rx_buf,get_rx_buf,clr_csr);

	erreur=0;
	go_on=1;
	tst_buff(RRP,set_rx_buf,get_rx_buf,"receive");
	return(erreur);
}

int tx_buf()
{
	erreur=0;
	tst_rst_pointer(RTP,set_tx_buf,get_tx_buf,set_csr);
	if (XILINX) tst_rst_pointer(RTP,set_tx_buf,get_tx_buf,clr_csr);

	erreur=0;
	go_on=1;
	tst_buff(RTP,set_tx_buf,get_tx_buf,"transmit");
	return(erreur);
}
