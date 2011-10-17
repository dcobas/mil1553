/* Test broadcast */
#include <errno.h>
#include <stdio.h>

#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, XILINX, status;
extern int go_on, t_err, erreur;

#define BAD(st) { print (1,"%s errno[%d]\n",st,errno); erreur++; if (t_err) continue; }

int broadcst (void)
{
    unsigned short TR, SA, WC;
    unsigned short data[32];
    char m_str[132];

    /*print(0,"Broadcast data\n"); */
    TR = 0;
    SA = 2;
    WC = (-1);
    print (2, "Mdrop(TR[%d] SA[%d] WC[0-31])\r", TR, SA);
    do {
	if (++WC == 32) {	/* On fait WC[0,31] avec SA[2,31] pour TR=write */
	    WC = 0;
	    if (++SA == 32)
		break;		/* on a fini */
	    print (2, "Mdrop(TR[%d] SA[%d] WC[0-31])\n", TR, SA);
	}
	if (mdrop (bc, 31, TR, SA, WC, &status, (char *) data) == (-1)) {
	    sprintf (m_str, "system error in mdrop(RT[31]TR[%d]SA[%d]WC[%d])", TR, SA, WC);
	    BAD (m_str);
	}
	/* execution d'un MC2 (Read last status) */
	if (mdrop (bc, rt, 1, 31, 2, &status, (char *) data) == (-1)) {
	    sprintf (m_str, "system error in mdrop(RT[%d]TR[1]SA[31]WC[2])", rt);
	    BAD (m_str);
	}
	if ((status & 0x0010) != 0x0010) {
	    print (1, "Erreur de status du bit Broadcast du RT [%X]\n", status);
	    erreur++;
	    if (t_err)
		continue;
	}
	if (XILINX == 0)
	    continue;		/* pas de last command pour les vieilles cartes */
	/* execution d'un MC18 (Read last command) */
	if (mdrop (bc, rt, 1, 31, 18, &status, (char *) data) == (-1)) {
	    sprintf (m_str, "system error in mdrop(RT[%d]TR[1]SA[31]WC[18])", rt);
	    BAD (m_str);
	}
	if (valid_MC (31, TR, SA, WC, data[0]) != 0) {
	    print (1, "Erreur: mc18 donne la mauvaise commande[%X]\n", data[0]);
	    erreur++;
	    if (t_err)
		continue;
	}
    } while (go_on);

    clr_line ();
    if (erreur)
	print (0, "Broadcast data a donne %d erreurs\n", erreur);

    erreur = 0;
    go_on = 1;
    WC = (-1);
    TR = 1;
    /*print(0,"Broadcast mode code\n"); */
    do {
	if (++WC == 16)
	    TR = 0;		/* Les mode codes 1-15 sont en read et 16-31 en write */
	if (WC == 32)
	    break;		/* C'est tout pour aujourd'hui */
	if ((WC == 2) || (WC == 8) || (WC == 18))
	    continue;		/* mode codes speciaux */
	print (2, "Broadcast Mode code(TR[%d] SA[0] WC[%d])\n", TR, WC);
	if (mdrop (bc, 31, TR, 0, WC, &status, (char *) data) == (-1)) {
	    sprintf (m_str, "system error in mdrop(RT[31]TR[%d]SA[0]WC[%d])", TR, WC);
	    BAD (m_str);
	}
	if ((status & 0x0010) != 0x0010 && t_err == 1) {
	    print (1, "Erreur de status du bit Broadcast [%X] pour le MC[%d]\n", status, WC);
	    erreur++;
	    if (t_err)
		continue;
	}
	if (XILINX == 0)
	    continue;		/* pas de last command pour les vieilles cartes */
	if (mdrop (bc, rt, 1, 31, 18, &status, (char *) data) == (-1)) {
	    sprintf (m_str, "system error in mdrop(RT[%d]TR[1]SA[31]WC[18])", rt);
	    BAD (m_str);
	}
	if (valid_MC (31, TR, 0, WC, data[0]) != 0) {
	    print (1, "Erreur: mc18 donne la mauvaise commande[%X]\n", data[0]);
	    erreur++;
	    if (t_err)
		continue;
	}
    } while (go_on);

    clr_line ();
    if (erreur)
	print (0, "Broadcast mode code a donne %d erreurs\n", erreur);
    return (erreur);
}
