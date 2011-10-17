/* Test Modes donnees */
#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, status;
extern int erreur, go_on, t_err, stop_after_err;
extern int Flag_of_TO_Req;

int mode_data (void)
{
    unsigned short TR, SA, WC;
    unsigned short data[32], old_status;
    int err;

    WC = (-1);
    SA = 2;
    TR = 0;
    err = 0;
/*	Flag_of_TO_Req = 0;	*/
    print (2, "Execution du Mode data(TR[%d] SA[%d] WC[0-31])\n", TR, SA);
    do {
	if (++WC == 32) {	/* mdrop de WC=0,31 pour SA=[2,30] et on boucle en read et write (TR=[0,1]) */
	    WC = 0;
	    if (SA < 30)
		SA++;
	    else if (TR == 0) {
		TR = 1;
		SA = 2;
	    }
	    else
		break;		/* On a tout fait */
	    print (2, "Execution du Mode data(TR[%d] SA[%d] WC[0-31])\n", TR, SA);
	}
	do {
	    if (mdrop (bc, rt, TR, SA, WC, &status, (char *) data) == (-1)) {
		print (1, "System error in mdrop(TR[%d] SA[%d] WC[%d])\n", TR, SA, WC);
		err++;
	    }
	    if (valid_ST (rt, status) != 0) {
		print (1, "Erreur:mdrop(TR[%d] SA[%d] WC[%d]) donne un mauvais status[%X]\n", TR, SA, WC, status);
		err++;
	    }
	    if (go_on == 0)
		return (err);
	} while ((stop_after_err == 0) && (err != 0));
	erreur += err;
	err = 0;

	/* Execution d'un MC 18 (Read last command) */
	do {
	    if (mdrop (bc, rt, 1, 31, 18, &status, (char *) data) == (-1)) {
		print (1, "System error in mdrop(TR[1] SA[%d] WC[18])\n", SA);
		err++;
	    }
	    if (valid_MC (rt, TR, SA, WC, data[0]) != 0) {
		print (1, "Erreur:mdrop(TR[1] SA[%d] WC[%d]) donne la mauvaise commande[%X]\n", SA, WC, data[0]);
		err++;
	    }
	    if (go_on == 0)
		return (err);
	} while ((stop_after_err == 0) && (err != 0));
	erreur += err;
	err = 0;

	/* Execution d'un MC 2 (Read last status) */
	old_status = status & 0xFF7F;
	do {
	    if (mdrop (bc, rt, 1, 31, 2, &status, (char *) data) == (-1)) {
		print (1, "System error in mdrop(TR[1] SA[%d] WC[2])\n", SA);
		err++;
	    }
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
    } while (go_on);
    return (erreur);
}
