#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, XILINX, status;
extern int rt_stat[], erreur;

#define BAD(st) { print (1, st); erreur++; }

int choix_rt (void)
{
    unsigned short buff[32];
    int n, i;
    long mask;

    print (0, "Recognition are ...\n");
    /***************** choise of BC ************************************/
    if (get_connected_bc (&mask)) {
	BAD ("Probleme avec get_connected_bc");
	return (0);
    }
    printf ("BC(s) Connectes :  (Mask ->%04lX)\n", mask);
    n = 0;
    for (i = 0; i < 32; i++)
	if (mask & (1 << i)) {
	    printf ("  BC number: %d.\n", i);
	    if (n == 0)
		bc = i;
	    n++;
	}
    if (n == 0) {
	print (0, "\nPity, but any BC not connects to VME-crate.");
	exit (0);
    }
    if (n > 1)
	do {
	    printf ("Choose what You like, please. BC number:");
	    scanf ("%hd", &bc);
	    if (bc < 1 || bc > 31) {
		printf ("bc [%d] illegal, read documentation, please!\n", bc);
		continue;
	    }
	    if (mask & (1 << bc))
		break;
	    printf ("bc [%d] not connected\n", bc);
	} while (1);
    print (0, "OK for bc[%d]\n", bc);
    /****************** choise of RT ***********************************/
    n = 0;
    for (i = 0; i < 31; i++) {
	rt_stat[i] = 0;
	if (mdrop (bc, i, 1, 30, 1, &status, (char *) buff) != -1) {
	    rt = i;
	    printf ("   RT nr:%d ->", i);
	    n++;
	    if (status & 0x80) {
		printf ("\n Attention!!!  Time_Out_Bit!!!\tStatus is: %x\n", status);
		return (0);
	    }

	    printf ("\tSignature :%X  ", buff[0]);
	    switch (buff[0] & 0xffff) {
	    case 0xffff:
		printf ("->Ancienne carte RTI. /CMIG/CMIV\n");
		rt_stat[i] = 1;	/* pas XILINX */
		break;
	    case 0xfffe:
		printf ("->Carte ICM.\n");
		rt_stat[i] = 2;	/* XILINX */
		break;
	    case 0xfffd:
		printf ("->Nouvelle carte RTI G64. /CMIG\n");
		rt_stat[i] = 2;	/* XILINX */
		break;
	    case 0xfffc:
		printf ("->Carte RTI G96. /CMIG\n");
		rt_stat[i] = 2;	/* XILINX */
		break;
	    case 0xfffb:
		printf ("->Nouvelle carte RTI VME. CMIV\n");
		rt_stat[i] = 2;	/* XILINX */
		break;
	    case 0xfff8:
		printf ("->Carte RS232. CMXA_(2 ports)\n");
		rt_stat[i] = 3;	/* XILINX */
		break;
	    case 0xfff7:
		printf ("->Carte RS232. CMXA_(8 ports)\n");
		rt_stat[i] = 3;	/* XILINX */
		break;
	    case 0xffa0:
	    case 0xffa1:
	    case 0xffa2:
	    case 0xffa3:
	    case 0xffa4:
	    case 0xffa5:
	    case 0xffa6:
	    case 0xffa7:
		printf ("->Carte MPX CMMX.\n");
		rt_stat[i] = 2;	/* XILINX */
		break;
	    case 0xffa8:
	    case 0xffa9:
	    case 0xffaa:
	    case 0xffab:
	    case 0xffac:
	    case 0xffad:
	    case 0xffae:
	    case 0xffaf:
		printf ("->Carte MPX CMMT.\n");
		rt_stat[i] = 2;	/* XILINX */
		break;
	    case 0xffd0:
	    case 0xffd1:
	    case 0xffd2:
	    case 0xffd3:
	    case 0xffd4:
	    case 0xffd5:
	    case 0xffd6:
	    case 0xffd7:
	    case 0xffd8:
	    case 0xffd9:
	    case 0xffda:
	    case 0xffdb:
	    case 0xffdc:
	    case 0xffdd:
	    case 0xffde:
	    case 0xffdf:
		printf ("->Carte CR1553. CMCG\n");
		rt_stat[i] = 4;	/* XILINX */
		break;;
	    default:
		printf ("Cette signature est inconnue.\n");
		rt_stat[i] = 0;
	    }
	}
    }
    if (n == 0) {
	print (0, "\nPity, but this BC [%d] has't any RT.\n", bc);
	exit (0);
    }
    if (n > 1)
	do {
	    printf ("Choose what You like, please. RT number:");
	    scanf ("%hd", &rt);
	    if (rt < 1 || rt > 31) {
		printf ("rt [%d] illegal, read documentation, please!\n", rt);
		continue;
	    }
	    if (rt_stat[rt])
		break;
	    printf ("rt [%d] not connected\n", rt);
	} while (1);
    print (0, "OK for rt[%d]\n", rt);
    XILINX = ((rt_stat[rt] == 2) || (rt_stat[rt] == 3));
    if (XILINX)
	print (0, "It's card XILINX type\n");
    else
	print (0, "It's card not_XILINX type\n");
    return (0);
}
