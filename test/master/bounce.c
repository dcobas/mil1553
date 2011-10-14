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

extern unsigned short bc,rt,XILINX,status;
extern int erreur,go_on,t_err;
extern convbin();

#define BAD1(st) {print(1,st);erreur++;if (t_err) return(erreur);}
#define BAD(st) {print(1,st);erreur++;if (t_err) continue;}

int bounce()
{
	unsigned short csr;
	unsigned short buff[4096],data[4096],*pts,*ptd;
	int i,l,flag=0,count=0;
	int seuil=0;
	unsigned char str1[20], str2[20];
	char dir;

	seuil = 128;
	dir = 8;
	/* tell G64 to start bounce with LRR and NEM set */
	csr=LRR+NEM;
	if (set_csr(bc,rt,&csr,&status)==-1) BAD1("system error in set LRR+NEM");

	/* reset bit TB et RB*/
	csr=TB+RB;
	if (clr_csr(bc,rt,&csr,&status)==-1) BAD1("system error in clr RB+TB");

	/* fill data */
	for(i=0;i<seuil;i++)
		data[i]=(i*(0xffff/seuil))+((0xffff/seuil)*l+l);

	l=1;
	while (go_on) {
		print (1,"%c%c",'|'+(l&0xfffe), dir);
		print(2,"Test avec %4d mots\n",l);

		/* reset receive pointer */
		csr=RRP;
		if (set_csr(bc,rt,&csr,&status)==-1) BAD("system error in set RRP");

		/* fill buffer */
		if(set_rx_buf(bc,rt,l*2,data,&status) != 0) BAD("system error in set_rc_buf");

		/* set bits RB */
		csr=RB;
		if (set_csr(bc,rt,&csr,&status)==-1) BAD("system error in set RB");

		/* wait for reponse */
		if (wait_for_csr(TB,TB,20000,0)==0) {
			print(1,"TIME_OUT a l'attente du Transmit Buffer plein.\n");
			erreur++;
			if (t_err) continue;
		}

		/* reset transmit pointer */
		csr=RTP;
		if (set_csr(bc,rt,&csr,&status)==-1) BAD("system error in set RTP");

		/* read reponse */
		for (i=0;i<l;i++) buff[i]=0;
		if (get_tx_buf(bc,rt,l*2,buff,&status) != 0) BAD("system error in get_tx_buf");

		/* reset bit TB */
		csr=TB;
		if (clr_csr(bc,rt,&csr,&status)==-1) BAD("system error in clr TB");

		/* compare */
		flag=0;i=l;
		pts=data;ptd=buff;
		do {
		    if (*pts!=*ptd) {
			flag++; erreur++;
			convbin(*pts, str1);
			convbin(*ptd, str2);
			print(1,"DATA error in test_BOUNCE:\tPointer %d\n  read [%s]\n write [%s]\n",i, str2, str1);
			break;
			}
		    pts++;ptd++;
		    } while (--i);
		if (++l > seuil) break;
	}
	/* clear collision pattern LRR and NEM set */
	csr=LRR+NEM;
	if (clr_csr(bc,rt,&csr,&status)==-1) BAD1("system error in clr LRR+NEM");

	if (get_csr(bc,rt,&csr,&status)==-1) BAD1("system error in get_csr");
	if ((csr & INV) == INV) {
		print(0,"Le G64 a detecte une ou plusieurs erreurs au cours du test collision\n");
		erreur++;
		csr=INV;
		if (clr_csr(bc,rt,&csr,&status)==-1) BAD1("system error in clr INV");
		if (t_err) return;
	}
	return(erreur);
}

int collision()
{
	unsigned short csr,sta;

	/* tell G64 to start bounce with LRR and BRD set */
	print (2,"\rTest Remote collision:\n");
	csr=LRR+BRD;
	if (set_csr(bc,rt,&csr,&status)==-1) BAD1("system error in set LRR+BRD");
	print(2,"LRR + BRD set in RT\r");

	/* test first the receive buffer */
	csr=RB;
	if (set_csr(bc,rt,&csr,&status)==-1) BAD1("system error in set RB");
	if (wait_for_csr(TB,TB,60,1)==0) {
		print(1,"Erreur de synchro. Le RT n'a pas mits son bit TB a 1\n");
		erreur++;
		return(1);
	}
	print(1,"Test du receive buffer");

	rx_buf(); /* test receive buffer */
	rx_buf(); /* test receive buffer */
	rx_buf(); /* test receive buffer */
	print (1,"\n");
	csr=RB;
	if (clr_csr(bc,rt,&csr,&status)==-1) BAD1("system error in clr RB");

	/* Wait for the end of the G64 */
	if (wait_for_csr(TB,0,60,1)==0) {
		print(1,"Erreur de synchro. Le RT n'a pas mis son bit TB a 0\n");
		erreur++;
		return(1);
	}

	/* Then test the transmit buffer */
	csr=TB;
	if (set_csr(bc,rt,&csr,&status)==-1) BAD1("system error in set TB");
	if (wait_for_csr(RB,RB,60,1)==0) {
		print(1,"Erreur de synchro. Le RT n'a pas mits son bit RB a 1\n");
		erreur++;
		return(1);
	}
	print(1,"Test du transmit buffer");
	tx_buf(); /* test transmit buffer */
	tx_buf(); /* test transmit buffer */
	tx_buf(); /* test transmit buffer */
	print (1,"\n");
	csr=TB;
	if (clr_csr(bc,rt,&csr,&sta)==-1) BAD1("system error in clr TB");

	/* Wait for the end of the G64 */
	if (wait_for_csr(RB,0,60,1)==0) {
		print(1,"Erreur de synchro. Le RT n'a pas mis son bit RB a 0\n");
		return(1);
	}

	/* clear collision pattern LRR and BRD set */
	csr=LRR+BRD;
	if (clr_csr(bc,rt,&csr,&status)==-1) BAD1("system error in clr LRR+BRD");

	if (get_csr(bc,rt,&csr,&status)==-1) BAD1("system error in get_csr");
	if ((csr & INV) == INV) {
		print(0,"Le G64 a detecte une ou plusieurs erreurs au cours du test collision\n");
		erreur++;
		csr=INV;
		if (clr_csr(bc,rt,&csr,&status)==-1) BAD1("system error in clr INV");
		if (t_err) return;
	}
	return(erreur);
}
