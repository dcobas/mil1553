/* Test Modes codes */
#include <stdio.h>
#include <errno.h>
#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, XILINX, status;
extern int erreur, t_err, go_on, stop_after_err;
extern int Flag_of_TO_Req;

int mode_code (void)
{
    unsigned short TR, SA;
    unsigned short old_status;
    unsigned short data[32];
    short mc;
    int err;

    mc = (-1);
    SA = 0;
    TR = 0;
    err = 0;
    Flag_of_TO_Req = 0;
    print (2, "Execution du Mode code(TR[%d] SA[%d] WC[0-31])\n", TR, SA);
    do {
	if (++mc == 32) {	/* on passe les mc de 0 a 31 pour TR=0 et TR=1 et on boucle le tout pour SA=0 et SA=31 */
	    mc = 0;		/* ca fait donc 4 tours complets de mc [0,31]   */
	    if (TR == 0) {
		TR = 1;
	    }
	    else if (SA == 0) {
		SA = 31;
		TR = 0;
	    }
	    else
		break;		/* C'est bon, on a tout fait... */
	}
	if (mc == 18)
	    continue;		/* pas de MC18 dans le test */
	if ((TR == 1) && ((mc == 2) || (mc == 8)))
	    continue;		/* pas de MC2 ou 8 en read */
#if 0
	if ((XILINX == 0) && (TR == 0) && (mc < 16))
	    continue;		/* pas de MC0-15 en write pour les anciennes cartes */
#endif
	if ((TR == 0) && (mc < 16))
	    continue;
	/* Execution du Mode Code */
	do {
	    if (mdrop (bc, rt, TR, SA, mc, &status, (char *) data) == (-1)) {
		print (1, "Mode_Code:System error in mdrop(TR[%d] SA[%d] WC[%d])\n", TR, SA, mc);
		err++;
		if ((status & 0x80) && ((TR == 1) && (mc < 16)))
		    print (1, "\n Attension!!!  Time_Out_Bit!!!\tStatus is: %x\n", status);
#if 0
		if ((TR == 1) || (mc > 15)) {
		    print (1, "System error in mdrop(TR[%d] SA[%d] WC[%d])\n", TR, SA, mc);
		    err++;
		}
		if ((TR == 0) && (mc < 16)) {	/* c'est pas normal. Il DOIT y avoir -1 car ce mc ne doit pas etre accepte */
		    print (1, "Erreur:le mdrop(TR[%d] SA[%d] WC[%d]) a ete accepte\n", TR, SA, mc);
		    err++;
		}
#endif
	    }

	    /* Test du status 1553 */
	    if (valid_ST (rt, status) != 0) {
		print (1, "Erreur:mdrop(TR[%d] SA[%d] WC[%d]) donne un mauvais status[%X]\n", TR, SA, mc, status);
		err++;
	    }
	    if (go_on == 0)
		return (err);
	} while ((stop_after_err == 0) && (err != 0));
	erreur += err;
	err = 0;

	/* Execution d'un MC 18 (Read last command) */
	print (2, "Execution MC 18 (Read last command)(TR[%d] SA[%d] WC[0-31])\n", TR, SA);
	old_status = status & 0xFF7F;
	do {
	    if (mdrop (bc, rt, 1, 31, 18, &status, (char *) data) == (-1)) {
		print (1, "System error in mdrop(TR[1] SA[%d] WC[18])\n", SA);
		err++;
	    }
	    if (valid_MC (rt, TR, SA, mc, data[0]) != 0) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[18]) donne la mauvaise commande[%X]\n", SA, data[0]);
		err++;
	    }
	    /* Le MC 18 ne doit pas avoir modifie le status 1553 */
	    status &= 0xFF7F;
	    if (status != old_status) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[18]) a modifie le status: lu[%X] au lieu de[%X]\n", SA, status, old_status);
		err++;
	    }
	    if (go_on == 0)
		return (err);
	} while ((stop_after_err == 0) && (err != 0));
	erreur += err;
	err = 0;
	do {
	    /* Execution d'un autre MC 18  a la sous-adresse 0 pour changer... */
	    if (mdrop (bc, rt, 1, 0, 18, &status, (char *) data) != 0) {
		print (1, "System error in mdrop(TR[1] SA[%d] WC[18])\n", SA);
		err++;
	    }
	    /* le premier MC 18 ne doit pas avoir modifie le LAST COMMAND */
	    if (valid_MC (rt, TR, SA, mc, data[0]) != 0) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[18]) a modifie la commande:[%X]\n", SA, data[0]);
		err++;
	    }
	    /* Test du status 1553 */
	    if (valid_ST (rt, status) != 0) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[18]) donne un mauvais status[%X]\n", SA, status);
		err++;
	    }
	    if (go_on == 0)
		return (err);
	} while ((stop_after_err == 0) && (err != 0));
	erreur += err;
	err = 0;

	/* Execution d'un MC 2 (Read last status) */
	print (2, "Execution MC 2 (Read last status)(TR[%d] SA[%d] WC[0-31])\n", TR, SA);
	old_status = status & 0xFF7F;
	do {
	    if (mdrop (bc, rt, 1, 31, 2, &status, (char *) data) != 0) {
		print (1, "System error in mdrop(TR[1] SA[%d] WC[2])\n", SA);
		err++;
	    }
	    /* Le MC 2 ne doit pas avoir modifie le LAST STATUS */
	    status &= 0xFF7F;
	    if (status != old_status) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[2]) a modifie le status: lu[%X] au lieu de[%X]\n", SA, status, old_status);
		err++;
	    }
	    if (go_on == 0)
		return (err);
	} while ((stop_after_err == 0) && (err != 0));
	erreur += err;
	err = 0;

	/* les anciennes cartes ont leur LAST COMMAND  modifie par un MC 2, pas les nouvelles */
	if (XILINX == 0)
	    continue;
	do {
	    if (mdrop (bc, rt, 1, 31, 18, &status, (char *) data) != 0) {
		print (1, "System error in mdrop(TR[1] SA[%d] WC[18])\n", SA);
		err++;
	    }
	    if (valid_MC (rt, TR, SA, mc, data[0]) != 0) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[2]) a modifie la commande:[%X]\n", SA, data[0]);
		err++;
	    }
	    if (valid_ST (rt, status) != 0) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[18]) donne un mauvais status[%X]\n", SA, status);
		err++;
	    }
	    if (go_on == 0)
		return (err);
	} while ((stop_after_err == 0) && (err != 0));
	erreur += err;
	err = 0;
    } while (go_on);
    return (erreur);
}
