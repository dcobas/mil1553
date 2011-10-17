/* Test CSR */

#include "1553def.h"
#include "mdacc.h"

#define N_TB   0
#define N_RTP  6
#define N_RRP  7
#define N_BC   8
#define N_BCR  9
#define N_LOC 13

extern unsigned short bc, rt, XILINX, status;
extern int t_err, erreur, go_on, stop_after_err;

#define BAD(st) { print (1, st); erreur++; if (stop_after_err == 2) continue; }

int tst_csr (void)
{
    unsigned short old_csr, csr, patt;
    int bit;

    print (2, "Clear du csr\r");
    csr = 0xFFFF;
    if (clr_csr (bc, rt, &csr, &status) == -1) {
	print (1, "tst_csr:System error in clr_csr(0xFFFF)\n");
	erreur++;
    }

    bit = N_TB;
    do {
	if (bit == N_BC)
	    continue;
	patt = 1 << bit;
	print (2, "Set du bit[%d]\n", bit);
	if (get_csr (bc, rt, &old_csr, &status) == -1)
	    BAD ("\ntst_csr:System error in get_csr");
	do {
	    if (set_csr (bc, rt, &patt, &status) == -1)
		BAD ("\ntst_csr:System error in set_csr");
	    if (get_csr (bc, rt, &csr, &status) == -1)
		BAD ("\ntst_csr:System error in get_csr");

	    if (verifie_ST (status, csr) != 0) {
		print (1, "\ntst_csr:Erreur:Status [%X] not correspond to CSR [%X]\n", status, csr);
		erreur++;
		if (stop_after_err == 2)
		    break;
	    }
	    if ((bit == N_RTP) || (bit == N_RRP) || (XILINX && (bit == N_BCR))) {	/* le bit n'est pas modifie */
		if (csr != old_csr) {
		    print (1, "\ntst_csr:Error was [%x], after BIT_SET [%X]\n", csr, old_csr);
		    erreur++;
		    if (stop_after_err == 2)
			break;
		}
	    }
	    else if ((csr & patt) != patt) {
		print (1, "\ntst_csr:Erreur after BIT_SET [%X],- CSR [%X]\n", patt, csr);
		erreur++;
		if (stop_after_err == 2)
		    break;
	    }
	    if (go_on == 0)
		return (erreur);
	} while ((erreur != 0) && (stop_after_err == 0));

	print (2, "Clr du bit[%d]\n", bit);
	if (get_csr (bc, rt, &old_csr, &status) == -1)
	    BAD ("\ntst_csr:System error in get_csr");
	do {
	    if (clr_csr (bc, rt, &patt, &status) == -1)
		BAD ("\ntst_csr:System error in clr_csr");
	    if (get_csr (bc, rt, &csr, &status) == -1)
		BAD ("\ntst_csr:System error in get_csr");
	    if (verifie_ST (status, csr) != 0) {
		print (1, "\ntst_csr:Erreur:Status [%X] not correspond to CSR [%X]\n", status, csr);
		erreur++;
		if (stop_after_err == 2)
		    break;
	    }
	    if ((bit == N_RTP) || (bit == N_RRP) || (XILINX && (bit == N_BCR))) {	/* le bit n'est pas modifie */
		if (csr != old_csr) {
		    print (1, "\ntst_csr:Error was [%x], after BIT_CLEAR [%X]\n", csr, old_csr);
		    erreur++;
		    if (stop_after_err == 2)
			break;
		}
	    }
	    else if ((csr & patt) != 0) {
		print (1, "\ntst_csr:Erreur after BIT_CLEAR [%X],- CSR [%X]\n", patt, csr);
		erreur++;
		if (stop_after_err == 2)
		    break;
	    }
	    if (go_on == 0)
		return (erreur);
	} while ((erreur != 0) && (stop_after_err == 0));
    } while ((go_on) && (++bit < N_LOC));

    return (erreur);
}
