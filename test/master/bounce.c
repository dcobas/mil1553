#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, XILINX, status;
extern int erreur, go_on, t_err;

#define BAD1(st) {print(1,st);erreur++;if (t_err) return(erreur);}
#define BAD(st) {print(1,st);erreur++;if (t_err) continue;}

int bounce (void)
{
    unsigned short csr;
    unsigned short buff[4096], data[4096], *pts, *ptd;
    int i, l = 0, flag = 0;
    int seuil = 0;
    unsigned char str1[20], str2[20];
    char dir;

    seuil = 128;
    dir = 8;
    /* tell G64 to start bounce with LRR and NEM set */
    csr = LRREQ_BIT + NEM_BIT;
    if (set_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in set LRR+NEM");

    /* reset bit TB et RB */
    csr = TB_BIT + RB_BIT;
    if (clr_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in clr RB+TB");

    /* fill data */
    for (i = 0; i < seuil; i++)
	data[i] = (i * (0xffff / seuil)) + ((0xffff / seuil) * l + l);

    l = 1;
    while (go_on) {
	print (1, "%c%c", '|' + (l & 0xfffe), dir);
	print (2, "Test avec %4d mots\n", l);

	/* reset receive pointer */
	csr = RRP_BIT;
	if (set_csr (bc, rt, &csr, &status) == -1)
	    BAD ("system error in set RRP");

	/* fill buffer */
	if (set_rx_buf (bc, rt, l * 2, (char *) data, &status) != 0)
	    BAD ("system error in set_rc_buf");

	/* set bits RB */
	csr = RB_BIT;
	if (set_csr (bc, rt, &csr, &status) == -1)
	    BAD ("system error in set RB");

	/* wait for reponse */
	if (wait_for_csr (TB_BIT, TB_BIT, 20000, 0) == 0) {
	    print (1, "TIME_OUT a l'attente du Transmit Buffer plein.\n");
	    erreur++;
	    if (t_err)
		continue;
	}

	/* reset transmit pointer */
	csr = RTP_BIT;
	if (set_csr (bc, rt, &csr, &status) == -1)
	    BAD ("system error in set RTP");

	/* read reponse */
	for (i = 0; i < l; i++)
	    buff[i] = 0;
	if (get_tx_buf (bc, rt, l * 2, (char *) buff, &status) != 0)
	    BAD ("system error in get_tx_buf");

	/* reset bit TB */
	csr = TB_BIT;
	if (clr_csr (bc, rt, &csr, &status) == -1)
	    BAD ("system error in clr TB");

	/* compare */
	flag = 0;
	i = l;
	pts = data;
	ptd = buff;
	do {
	    if (*pts != *ptd) {
		flag++;
		erreur++;
		convbin (*pts, (char *) str1);
		convbin (*ptd, (char *) str2);
		print (1, "DATA error in test_BOUNCE:\tPointer %d\n  read [%s]\n write [%s]\n", i, str2, str1);
		break;
	    }
	    pts++;
	    ptd++;
	} while (--i);
	if (++l > seuil)
	    break;
    }
    /* clear collision pattern LRR and NEM set */
    csr = LRREQ_BIT + NEM_BIT;
    if (clr_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in clr LRR+NEM");

    if (get_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in get_csr");
    if ((csr & INV_BIT) == INV_BIT) {
	print (0, "Le G64 a detecte une ou plusieurs erreurs au cours du test collision\n");
	erreur++;
	csr = INV_BIT;
	if (clr_csr (bc, rt, &csr, &status) == -1)
	    BAD1 ("system error in clr INV");
	if (t_err)
	    return t_err;
    }
    return (erreur);
}

int collision (void)
{
    unsigned short csr, sta;

    /* tell G64 to start bounce with LRR and BRD set */
    print (2, "\rTest Remote collision:\n");
    csr = LRREQ_BIT + BRDIS_BIT;
    if (set_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in set LRR+BRD");
    print (2, "LRR + BRD set in RT\r");

    /* test first the receive buffer */
    csr = RB_BIT;
    if (set_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in set RB");
    if (wait_for_csr (TB_BIT, TB_BIT, 60, 1) == 0) {
	print (1, "Erreur de synchro. Le RT n'a pas mits son bit TB a 1\n");
	erreur++;
	return (1);
    }
    print (1, "Test du receive buffer");

    rx_buf ();			/* test receive buffer */
    rx_buf ();			/* test receive buffer */
    rx_buf ();			/* test receive buffer */
    print (1, "\n");
    csr = RB_BIT;
    if (clr_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in clr RB");

    /* Wait for the end of the G64 */
    if (wait_for_csr (TB_BIT, 0, 60, 1) == 0) {
	print (1, "Erreur de synchro. Le RT n'a pas mis son bit TB a 0\n");
	erreur++;
	return (1);
    }

    /* Then test the transmit buffer */
    csr = TB_BIT;
    if (set_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in set TB");
    if (wait_for_csr (RB_BIT, RB_BIT, 60, 1) == 0) {
	print (1, "Erreur de synchro. Le RT n'a pas mits son bit RB a 1\n");
	erreur++;
	return (1);
    }
    print (1, "Test du transmit buffer");
    tx_buf ();			/* test transmit buffer */
    tx_buf ();			/* test transmit buffer */
    tx_buf ();			/* test transmit buffer */
    print (1, "\n");
    csr = TB_BIT;
    if (clr_csr (bc, rt, &csr, &sta) == -1)
	BAD1 ("system error in clr TB");

    /* Wait for the end of the G64 */
    if (wait_for_csr (RB_BIT, 0, 60, 1) == 0) {
	print (1, "Erreur de synchro. Le RT n'a pas mis son bit RB a 0\n");
	return (1);
    }

    /* clear collision pattern LRR and BRD set */
    csr = LRREQ_BIT + BRDIS_BIT;
    if (clr_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in clr LRR+BRD");

    if (get_csr (bc, rt, &csr, &status) == -1)
	BAD1 ("system error in get_csr");
    if ((csr & INV_BIT) == INV_BIT) {
	print (0, "Le G64 a detecte une ou plusieurs erreurs au cours du test collision\n");
	erreur++;
	csr = INV_BIT;
	if (clr_csr (bc, rt, &csr, &status) == -1)
	    BAD1 ("system error in clr INV");
	if (t_err)
	    return t_err;
    }
    return (erreur);
}
