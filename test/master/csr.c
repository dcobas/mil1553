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

/* Test CSR */

#define TB_I   0
#define RB_I   1
#define INV_I  2
#define RL_I   3
#define INE_I  4
#define INT_I  5
#define RTP_I  6
#define RRP_I  7
#define BC_I   8
#define BC_IR  9
#define LRR_I 10
#define NEM_I 11
#define BRD_I 12
#define LOC_I 13
#define TES_I 14
#define V5_I  15

extern unsigned short bc,rt,XILINX,status;
extern int t_err,erreur,go_on,stop_after_err;
#define BAD(st) {print(1,st);erreur++;if (stop_after_err == 2) continue;}

int tst_csr()
{
	unsigned short old_csr,csr,patt;
	int bit;

	print(2,"Clear du csr\r");
	csr=0xFFFF;
	if (clr_csr(bc,rt,&csr,&status)==-1) {
	    print(1,"tst_csr:System error in clr_csr(0xFFFF)\n");
	    erreur++;
	    }

	bit=TB_I;
	do {
	    if (bit==BC_I) continue;
	    patt=1<<bit;
	    print(2,"Set du bit[%d]\n",bit);
	    if (get_csr(bc,rt,&old_csr,&status)==-1) 
		BAD("\ntst_csr:System error in get_csr");
	    do  {
		if (set_csr(bc,rt,&patt,&status)==-1) 
		    BAD("\ntst_csr:System error in set_csr");
		if (get_csr(bc,rt,&csr,&status)==-1) 
		    BAD("\ntst_csr:System error in get_csr");

		if (verifie_ST(status,csr)!=0) {
		    print(1,"\ntst_csr:Erreur:Status [%X] not correspond to CSR [%X]\n",status,csr);
		    erreur++;
		    if (stop_after_err == 2) break;
		    }
		if ((bit==RTP_I) || (bit==RRP_I) || (XILINX && (bit==BC_IR))) { /* le bit n'est pas modifie */
		    if (csr != old_csr) {
			print(1,"\ntst_csr:Error was [%x], after BIT_SET [%X]\n",csr,old_csr);
			erreur++;
			if (stop_after_err == 2) break;
			}
		    } else if ((csr&patt) != patt) {
			print(1,"\ntst_csr:Erreur after BIT_SET [%X],- CSR [%X]\n",patt,csr);
			erreur++;
			if (stop_after_err == 2) break;
			}
		if (go_on == 0) return (erreur);
		} while ((erreur != 0) && (stop_after_err == 0));

	    print(2,"Clr du bit[%d]\n",bit);
	    if (get_csr(bc,rt,&old_csr,&status)==-1) 
		BAD("\ntst_csr:System error in get_csr");
	    do  {
		if (clr_csr(bc,rt,&patt,&status)==-1) 
		    BAD("\ntst_csr:System error in clr_csr");
		if (get_csr(bc,rt,&csr,&status)==-1) 
		    BAD("\ntst_csr:System error in get_csr");
		if (verifie_ST(status,csr)!=0) {
		    print(1,"\ntst_csr:Erreur:Status [%X] not correspond to CSR [%X]\n",status,csr);
		    erreur++;
		    if (stop_after_err == 2) break;
		    }
		if ((bit==RTP_I) || (bit==RRP_I) || (XILINX && (bit==BC_IR))) { /* le bit n'est pas modifie */
		    if (csr != old_csr) {
			print(1,"\ntst_csr:Error was [%x], after BIT_CLEAR [%X]\n",csr,old_csr);
			erreur++;
			if (stop_after_err == 2) break;
			}
		    } else if ((csr&patt) != 0) {
			print(1,"\ntst_csr:Erreur after BIT_CLEAR [%X],- CSR [%X]\n",patt,csr);
			erreur++;
			if (stop_after_err == 2) break;
			}
		if (go_on == 0) return(erreur);
		}while ((erreur != 0) && (stop_after_err == 0)); 
	    } while ((go_on) && (++bit<LOC_I));

	return(erreur);
}
