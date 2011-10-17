#include <errno.h>
#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, XILINX, status;
extern int go_on, erreur, t_err;
extern int rt_stat[];

int bit_test (void)
{
    return Set_csr (TM_BIT, "TEST");
}

#define mdr(tr,sa,wc) {if (mdrop(bc,30,tr,sa,wc,&status,(char *)data)==-1) { print (1, "system error in mdrop\n");erreur++;if (t_err) return(erreur);}}

int rt30 (void)
{
    short data[32], sa, wc;

    print (2, "mode code TR[0] SA[0] WC[16,31] vers RT[30]\r");
    for (wc = 16; wc < 32; wc++)
	if (wc != 18)
	    mdr (0, 0, wc);
    print (2, "mode code TR[0] SA[31] WC[16,31] vers RT[30]\r");
    for (wc = 16; wc < 32; wc++)
	if (wc != 18)
	    mdr (0, 31, wc);
    print (2, "mode code TR[1] SA[0] WC[0,31] vers RT[30]  \r");
    for (wc = 0; wc < 32; wc++)
	mdr (1, 0, wc);
    print (2, "mode code TR[1] SA[31] WC[0,31] vers RT[30]\r");
    for (wc = 0; wc < 32; wc++)
	mdr (1, 31, wc);
    print (2, "mode data TR[0] SA[2,30] WC[0,31] vers RT[30]\r");
    for (sa = 2; sa < 31; sa++)
	for (wc = 0; wc < 32; wc++)
	    mdr (0, sa, wc);
    print (2, "mode data TR[1] SA[1,30] WC[0,31] vers RT[30]\r");
    for (sa = 1; sa < 31; sa++)
	for (wc = 0; wc < 32; wc++)
	    mdr (1, sa, wc);
    return (erreur);
}

int signature (void)
{
    unsigned short buff[32];
    int i;

    for (i = 0; i < 31; i++) {
	print (2, "Test de la signature du RT[%d]\r", i);
	if (mdrop (bc, i, 1, 30, 1, &status, (char *) buff) != -1) {
	    if (rt_stat[i] == 0) {
		print (1, "RT [%d] mauvais. Il est ON au lieu d'etre OFF\n", i);
		erreur++;
	    }
	}
	else if (rt_stat[i] != 0) {
	    print (1, "RT [%d] mauvais. Il est OFF au lieu d'etre ON\n", i);
	    erreur++;
	}
    }
    return (erreur);
}

int bit_local (void)
{
    unsigned short csr;
    int delai;

    csr = 0xFFFF;
    print (2, "Clear du csr       \r");
    if (clr_csr (bc, rt, &csr, &status) == -1) {
	print (1, "System error [%d] in clr_csr", errno);
	erreur++;
	if (t_err)
	    return t_err;
    }
    print (2, "Envoi d'une interruption\r");
    csr = INT_BIT + INE_BIT;
    if (set_csr (bc, rt, &csr, &status) == -1) {
	print (1, "System error [%d] in set_csr", errno);
	erreur++;
	if (t_err)
	    return t_err;
    }
    csr = INE_BIT;
    if (clr_csr (bc, rt, &csr, &status) == -1) {
	print (1, "System error [%d] in clr_csr", errno);
	erreur++;
	if (t_err)
	    return t_err;
    }
    print (2, "Set des bits LRR et LOCAL\r");
    csr = LRREQ_BIT + LM_BIT;
    if (set_csr (bc, rt, &csr, &status) == -1) {
	print (1, "System error [%d] in set_csr", errno);
	erreur++;
	if (t_err)
	    return t_err;
    }
    delai = 60;
    print (1, "Le G64 a %d secondes pour demarrer son test LOCAL\n", delai);

    if (wait_for_csr (LRREQ_BIT + LM_BIT, LM_BIT, delai, 1) == 0)
	return (++erreur);
    print (1, "Le test LOCAL vient de demarrer sur le host.\n");
    print (1, "\tAttente de fin des tests sur le host.\n");
    if (wait_for_csr (LM_BIT, 0, 600, 1) == 0)
	return (++erreur);
    print (1, "Le test LOCAL vient de finir sur le host.");
    if (get_csr (bc, rt, &csr, &status) == -1) {
	print (1, "System error [%d] in get_csr", errno);
	erreur++;
	if (t_err)
	    return t_err;
    }
    if ((csr & INV_BIT) == INV_BIT) {
	print (0,
	       "Le G64 a detecte une ou plusieurs erreurs au cours du test local.\n");
	erreur++;
	csr = INV_BIT;
	if (clr_csr (bc, rt, &csr, &status) == -1) {
	    print (1, "System error [%d] in clr_csr", errno);
	    erreur++;
	}
	if (t_err)
	    return t_err;
    }
    return (erreur);
}
