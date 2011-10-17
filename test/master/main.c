
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "1553def.h"
#include "mdacc.h"

/************************************************/
/* Definitions globales				*/
/************************************************/
unsigned short bc = 0, rt = 0, XILINX = 0;
int menu = 0;
int t_err = 1, iteration = 1, go_on = 1, erreur;
int rt_stat[32];
int log_print = 0, stop_after_err = 1;
unsigned short rt_comm = 7;
unsigned short tr_comm = 1;
unsigned short sa_comm = 1;
unsigned short wc_comm = 1;
int rep_comm = 1;
unsigned short data_comm[32];
unsigned short status = 0;
int Flag_of_TO_Req = 1;

/************************************************/
/* Definitions externes				*/
/************************************************/
extern int log_file, log_fd;
extern char log_filename[];


/************************************************/
/* Definitions locales				*/
/************************************************/
struct mdr_tests {
    int index;
    int (*fn) ();
    char txt[80];
};

static struct mdr_tests tbl[] = {
    {1, choix_rt, "choix d'un nouveau BC/RT"},
    {2, reset, "RT Reset\t"},
    {3, config, "configuration des parametres globaux"},
    {4, seq_tst, "elaboration sequence de test"},
    {5, exe_seq, "une sequence de test"},
    {6, mode_code, "test du mode code"},
    {7, mode_data, "test du mode donnee"},
    {8, broadcst, "test du broadcast"},
    {9, rt30, "test du rt30\t"},
    {13, comm_tst, "elaboration sequence de test"},
    {14, comm_exe, "une sequence de test"},

    {20, not_test, "\n"},

    {21, mode_code, "test du mode code"},
    {22, mode_data, "test du mode donnee"},
    {23, broadcst, "test du broadcast"},
    {24, tst_csr, "test du csr\t"},
    {25, rx_buf, "test du buffer de reception"},
    {26, tx_buf, "test du buffer de transmission"},
    {27, bit_test, "init du mode TEST"},
    {28, bit_local, "init du mode LOCAL"},
    {29, bounce, "test du remote bounce"},
    {30, collision, "test du remote collision:\n"},
    {31, signature, "test des signatures"},
    {32, rt30, "test du rt30\t"},
    {44, taccept, "TEST D'ACCEPTANCE"},
    {33, reset, "RT reset\t"},
    {34, rd_csr, "\n"},
    {35, wr_csr, "\n"},
    {36, cl_csr, "\n"},
    {38, seq_tst, "elaboration sequence de test"},
    {39, exe_seq, "une sequence de test"},

    {61, mode_code, "test du mode code"},
    {62, mode_data, "test du mode donnee"},
    {63, broadcst, "test du broadcast"},
    {64, tst_csr, "test du csr\t"},
    {65, rx_buf, "test du buffer de reception"},
    {66, tx_buf, "test du buffer de transmission"},
    {67, bit_test, "init du mode TEST"},
    {68, bit_local, "init du mode LOCAL"},
    {69, bounce, "test du remote bounce"},
    {70, collision, "test du remote collision\n"},
    {71, signature, "test des signatures"},
    {72, rt30, "test du rt30\t"},
    {84, accept_pz, "TEST D'ACCEPTANCE"},
    {73, reset, "RT reset\t"},
    {74, rd_csr, "\n"},
    {75, wr_csr, "\n"},
    {76, cl_csr, "\n"},
    {78, seq_tst, "elaboration sequence de test"},
    {79, exe_seq, "une sequence de test"},

    {101, mode_code, "test du mode code"},
    {102, mode_data, "test du mode donnee"},
    {103, broadcst, "test du broadcast"},
    {104, reset, "RT reset\t"},
    {105, signature, "test des signatures"},
    {106, rt30, "test du rt30\t"},
    {118, seq_tst, "elaboration sequence de test"},
    {119, exe_seq, "une sequence de test"},

    {124, accept_cr, "TEST D'ACCEPTANCE"},
    {-1, 0, ""},
};

#include <termio.h>
static int f_in;
static struct termio old, new, iniold;

static struct {
    int indx, iteration;
} seq[99];
static int bouclage_seq;
static int accept_flg = 0;
static int trsig = 777;

/************************************************/
/* fonctions locales diverses			*/
/************************************************/
void ter_exit (void)
{
    if ((f_in = open (ttyname (0), 2)) < 0) {
	perror ("open");
	exit (2);
    }
    if (ioctl (f_in, TCSETA, &iniold) < 0) {
	perror ("ioctl");
	exit (2);
    }
    printf ("\n Exit by Your request(^c)\n");
    exit (0);
}


void inter (int sig)
{				/* signal handling */
    trsig = sig;
    switch (sig) {
    case 3:
	ter_exit ();
	break;
    case 2:
	go_on = 0;
	break;
    default:
	printf ("\n\tsig = %d\n", sig);
	break;
    }
}

void prologue (void)
/* on change les caracteristiques du input standard */
/* pour prendre les caracteres 'au vol' 	    */
{
    char *ttyname ();

    if ((f_in = open (ttyname (0), 2)) < 0) {
	perror ("open");
	exit (2);
    }
    if (ioctl (f_in, TCGETA, &old) < 0) {
	perror ("ioctl");
	exit (2);
    }
    new.c_iflag = old.c_iflag;
    new.c_oflag = old.c_oflag;
    new.c_cflag = old.c_cflag;
    new.c_lflag = old.c_lflag - ICANON - ECHO;
    new.c_line = old.c_line;
    new.c_cc[VMIN] = 0;
    new.c_cc[VTIME] = 20;
    new.c_cc[VINTR] = 5;
    new.c_cc[VQUIT] = 3;
    new.c_cc[VKILL] = 4;

    if (ioctl (f_in, TCSETA, &new) < 0) {
	perror ("ioctlset");
	exit (2);
    }
}

void epilogue (void)
{
    old.c_cc[VINTR] = 5;
    old.c_cc[VQUIT] = 3;
    old.c_cc[VKILL] = 4;
    if (ioctl (f_in, TCSETA, &old) < 0) {
	perror ("ioctlset");
	exit (2);
    }
    close (f_in);
}

/************************************************/
/* fonctions locales de configuration du test	*/
/************************************************/

int config (void)
{
    char dir, choix[32];
    int flag;

    fgets (choix, sizeof (choix), stdin);
    printf (" Configuration parameters:  (^E - return to menu)\n");
    flag = 0;
    while (flag == 0) {
	dir = 8;
	printf (" Stop/Loop/Continue after error (1/0/2)      - [%d]%c%c", stop_after_err, dir, dir);
	if (fgets (choix, sizeof (choix), stdin) != NULL)
	    if (strlen (choix) != 0)
		stop_after_err = atoi (choix);
	if (stop_after_err == 0) {
	    flag = 1;
	    printf ("Loop untill ^E\n");
	}
	else if (stop_after_err == 1) {
	    printf ("Stop\n");
	    flag = 1;
	}
	else if (stop_after_err == 2) {
	    printf ("Continue\n");
	    flag = 1;
	}
    }
    flag = 0;
    while (flag == 0) {
	printf (" Trace/No Trace (OK-style) (1/0)             - [%d]%c%c", log_print, dir, dir);
	if (fgets (choix, sizeof (choix), stdin) != NULL)
	    if (strlen (choix) != 0)
		log_print = atoi (choix);
	if (log_print == 1) {
	    flag = 1;
	    printf ("Detailed Trace\n");
	}
	else if (log_print == 0) {
	    printf ("Without trace (OK-style)\n");
	    flag = 1;
	}
    }
    flag = 0;
    while (flag == 0) {
	printf (" Print/No Print mess. about error (1/0)      - [%d]%c%c", t_err, dir, dir);
	if (fgets (choix, sizeof (choix), stdin) != NULL)
	    if (strlen (choix) != 0)
		t_err = atoi (choix);
	if (t_err == 1) {
	    flag = 1;
	    printf ("Print\n");
	}
	else if (t_err == 0) {
	    printf ("No Print\n");
	    flag = 1;
	}
    }
    printf (" Value of repetition of the each test;\n");
    printf ("\t0 - unlimited loop (untill ^e)\t     - [%d]%c%c", iteration, dir, dir);
    if (fgets (choix, sizeof (choix), stdin) != NULL)
	if (strlen (choix) != 0)
	    iteration = atoi (choix);
    return (0);
}

void logfile (void)
{
    log_file = 0;
    return;
}
void menu_3 (void)
{
    printf ("\n\n\t\tTEST command_responce\tbc[%d] rt[%d]\n\n", bc, rt);
    printf ("\tTest MODE CODE ..............................a\n");
    printf ("\tTest MODE DONNEE ............................b\n");
    printf ("\ttest BROADCAST ..............................c\n");
    printf ("\tReset du RT courant .........................d\n");
    printf ("\tTest des signatures .........................e\n");
    printf ("\tTest du RT30 ................................f\n");
    printf ("\tElaboration d'une sequence de programme .....r\n");
    printf ("\tExecution d'une sequence de programme .......s\n");
    printf ("\tTest d'acceptance ...........................x\n");
    printf ("\tMenu Principal ..............................z\n\n");
}


void menu_2 (void)
{
    printf ("\n\n\t\tTEST Pizza\tbc[%d] rt[%d]\n\n", bc, rt);
    printf ("\tTest MODE CODE ..............................a\n");
    printf ("\tTest MODE DONNEE ............................b\n");
    printf ("\ttest BROADCAST ..............................c\n");
    printf ("\tTest CSR ....................................d\n");
    printf ("\tTest RECEIVE BUFFER .........................e\n");
    printf ("\tTest TRANSMIT BUFFER ........................f\n");
    printf ("\tINIT Mode TEST ..............................g\n");
    printf ("\tINIT Mode LOCAL .............................h\n");
    printf ("\tTest des signatures .........................k\n");
    printf ("\tTest du RT30 ................................l\n");
    printf ("\tReset du RT courant .........................m\n");
    printf ("\tLecture du CSR ..............................n\n");
    printf ("\tEcriture du CSR .............................o\n");
    printf ("\tClear du CSR ................................p\n");
    printf ("\tElaboration d'une sequence de programme .....r\n");
    printf ("\tExecution d'une sequence de programme .......s\n");
    printf ("\tTest d'acceptance ...........................x\n");
    printf ("\tMenu Principal ..............................z\n\n");
}

void menu_1 (void)
{
    printf ("\n\n\t\tTEST du RTI\tbc[%d] rt[%d]\n\n", bc, rt);
    printf ("\tTest MODE CODE ..............................a\n");
    printf ("\tTest MODE DONNEE ............................b\n");
    printf ("\ttest BROADCAST ..............................c\n");
    printf ("\tTest CSR ....................................d\n");
    printf ("\tTest RECEIVE BUFFER .........................e\n");
    printf ("\tTest TRANSMIT BUFFER ........................f\n");
    printf ("\tINIT Mode TEST ..............................g\n");
    printf ("\tINIT Mode LOCAL .............................h\n");
    printf ("\tINIT REMOTE bounce ..........................i\n");
    printf ("\tINIT REMOTE collision .......................j\n");
    printf ("\tTest des signatures .........................k\n");
    printf ("\tTest du RT30 ................................l\n");
    printf ("\tReset du RT courant .........................m\n");
    printf ("\tLecture du CSR ..............................n\n");
    printf ("\tEcriture du CSR .............................o\n");
    printf ("\tClear du CSR ................................p\n");
    printf ("\tElaboration d'une sequence de programme .....r\n");
    printf ("\tExecution d'une sequence de programme .......s\n");
    printf ("\tTest d'acceptance ...........................x\n");
    printf ("\tMenu Principal ..............................z\n\n");
}

void menu_0 (void)
{
    printf ("\n\n\t\tMenu Principal\tbc[%d] rt[%d]\n\n", bc, rt);
    printf ("\tChoix d'un nouveau BC/RT ....................a\n");
    printf ("\tReset du RT courant .........................b\n");
    printf ("\tConfiguration globale .......................c\n");
    printf ("\tElaboration d'une sequence de programme .....d\n");
    printf ("\tExecution d'une sequence de programme .......e\n");
    printf ("\tTest MODE CODE ..............................f\n");
    printf ("\tTest MODE DONNEE ............................g\n");
    printf ("\ttest BROADCAST ..............................h\n");
    printf ("\tTest du RT30 ................................i\n");
    printf ("\tElaboration d'une commande ..................m\n");
    printf ("\tExecution d'une commande ....................n\n");
    printf ("\tTest command_responce ./CMCG/................v\n");
    printf ("\tTest Pizza ............/CMXA/................w\n");
    printf ("\tTest RTI ............../CMIG/CMIV............x\n");
    printf ("\tFIN .........................................z\n\n");
}

/******************************************************/
/* fonctions d'elaboration de d'execution commande    */
/******************************************************/
int comm_tst (void)
{
    char choix[5];
    int dir = 8;
    int i = 0, Dt, er = 0;
    int Br_c = 0;

    fgets (choix, sizeof (choix), stdin);
    printf ("\n\tExcuse me, but ... Any Questions:\n");
    printf (" Broadcast command (yes/no)(1/0)\t\t\t:[%d]%c%c", Br_c, dir, dir);
    if (fgets (choix, sizeof (choix), stdin) != NULL)
	if (strlen (choix) != 0)
	    Br_c = atoi (choix);
    if (Br_c == 0) {
	rt_comm = rt;
	printf ("\n\tOK: You are working without Broadcast mode; Your RT = %d\n\n", rt_comm);
    }
    else {
	rt_comm = 0x1f;
	printf ("\n\tOK: You are wish Broadcast mode; Your RT = %d\n\n", rt_comm);
    }
    printf (" T/R (1 - Transmit(Read from), 0 - Receive(Write to)): - ");
    if (fgets (choix, sizeof (choix), stdin) != NULL)
	if (strlen (choix) != 0)
	    tr_comm = atoi (choix);
    tr_comm &= 1;
    printf (" SA  (0 or 31 - Mode Code,\t 1-30 - Mode Data):    - ");
    if (fgets (choix, sizeof (choix), stdin) != NULL)
	if (strlen (choix) != 0)
	    sa_comm = atoi (choix);
    sa_comm &= 31;
    printf (" WC  (0-31 - if Mode Code,\t 1-32 - if Mode Data): - ");
    if (fgets (choix, sizeof (choix), stdin) != NULL)
	if (strlen (choix) != 0)
	    wc_comm = atoi (choix);
    wc_comm &= 31;
    if ((sa_comm != 0) && (sa_comm != 31)) {
#if 0
	wc_comm -= 1;
#endif
	if (tr_comm == 0) {
	    printf (" Data for writing (Hexadecimal format, please):\n");
	    for (i = 0; i < wc_comm; i++) {
		printf ("buffer[%d] = ", i);
		scanf ("%x", &Dt);
		data_comm[i] = Dt;
	    }
	}
    }
    else {
	if (tr_comm == 0) {
	    printf (" Data for writing (Hexadecimal format, please): ");
	    scanf ("%x", &Dt);
	    data_comm[0] = Dt;
	}
    }

    printf (" Number of repetitions (0 - infini mode; exit - ^e):   - ");
    scanf ("%d", &rep_comm);
    return (er);
}

int comm_exe (void)
{
    int i, tr, nl;
    unsigned short TR = 0, SA = 0, WC = 0;

    i = rep_comm;
    do {
	if (tr_comm == 1)
	    for (nl = 0; nl <= 31; nl++)
		data_comm[nl] = 0;
	if (mdrop (bc, rt_comm, tr_comm, sa_comm, wc_comm, &status, (char *) data_comm) == -1) {
	    print (1, "System error in mdrop(TR[%d] SA[%d] WC[%d])\n", TR, SA, WC);
	    erreur++;
	    if (t_err)
		continue;
	}
	print (1, "\n Command: RT[%d] TR[%d] SA[%d] WC[%d]. . . Status is: %x ", rt_comm, tr_comm, sa_comm, wc_comm, status);
	if (rt_comm != 31) {
	    if ((wc_comm == 18) || (wc_comm == 2)) {
#if 0
		Flag_of_TO_Req = 0;
#endif
	    }
	    else {
		Flag_of_TO_Req = 1;
		if (status & 0x80) {
		    print (1, "\nError!Time_Out bit is seting after this command...(Illegal Host Access \n");
		}
	    }
	}
	if (tr_comm == 0)
	    print (2, "\n");
	else {
	    tr = 0;
	    nl = 0;
	    print (1, "\nData are:\n");
	    do {
		print (1, "\t%4x", data_comm[tr++]);
		if (++nl == 4) {
		    print (1, "\n");
		    nl = 0;
		}
		if ((sa_comm == 0) || (sa_comm == 31))
		    break;
	    } while (tr < wc_comm);
	}
    } while ((--i) && (go_on != 0));
    return (erreur);
}

/******************************************************/
/* fonctions d'elaboration de d'execution de sequence */
/******************************************************/
int seq_tst (void)
{
    char choix[5];
    int i = 0;

    printf ("\n\tEntree de la sequence des tests a effectuer.\n");
    do {
	if (menu == 0)
	    printf ("(b)Reset (f)MODE CODE (g)MODE DATA (h)BROADCAST (i)duRT30\n");
	else if (menu == 1) {
	    printf ("(a)MODE CODE       (b)MODE DATA (c)BROADCAST (d)CSR       (e)RECEIVE BUFFER\n");
	    printf ("(f)TRANSMIT BUFFER (g)bit_Test  (h)bit_Local (i)BOUNCE    (j)Collision\n");
	    printf ("(k)Signatures      (l)du RT30   (m)Reset\n");
	}
	else if (menu == 2) {
	    printf ("(a)MODE CODE       (b)MODE DATA (c)BROADCAST (d)CSR        (e)RECEIVE BUFFER\n");
	    printf ("(f)TRANSMIT BUFFER (g)bit_Test  (h)bit_Local (k)Signatures (l)du RT30\n");
	    printf ("(m)Reset\n");

	}
	else if (menu == 3) {
	    printf ("(a)MODE CODE       (b)MODE DATA (c)BROADCAST (e)Signatures (f)du RT30\n");
	    printf ("(d)Reset\n");
	}
	printf ("\tCharacter du test a effectuer (0 = EXIT) :");
	scanf ("%s", choix);
	if (choix[0] == '0') {
	    seq[i].indx = 0;
	    break;
	}
	if (menu != 0)
	    seq[i].indx = (choix[0] - 'a') + 1 + 20 * (2 * menu - 1);
	else
	    seq[i].indx = (choix[0] - 'a') + 1;
	printf ("\tNumber of repetitions  (0=infinie; exit - ^e) :");
	scanf ("%d", &seq[i++].iteration);
    } while (1);

    printf ("\n");
    return (0);
}

int exe_seq (void)
{
    int i = 0, er = 0;

    do {
	i = 0;
	print (1, "\n");
	while (seq[i].indx) {
	    er += exec (seq[i].indx, seq[i].iteration);
	    if (go_on == 0)
		return (er);	/* ^C stops the current loop */
	    if ((stop_after_err == 1) && (er != 0))
		return (er);    /* we stop at the first error in accept */
	    i++;		/* next step in the sequence */
	}
	if (t_err && er)
	    return (er);	/* On s'arrette quand il y a des erreurs */
    } while (!(stop_after_err));
    return (er);
}

int accept_cr (void)
{				/* test standard d'acceptance d'une carte */
    int er, cer;
    char Tx_CR[] = "Cherche volontaire pour ....\n";

    bouclage_seq = 0;
    seq[0].indx = 104;
    seq[0].iteration = 1;	/* reset du bc/rt       */
    seq[1].indx = 101;
    seq[1].iteration = 1;	/* test mode code       */
    seq[2].indx = 102;
    seq[2].iteration = 1;	/* test mode donnee     */
    seq[3].indx = 103;
    seq[3].iteration = 1;	/* test broadcast       */
    seq[4].indx = 105;
    seq[4].iteration = 1;	/* test des signatures  */
    seq[5].indx = 106;
    seq[5].iteration = 1;	/* test du rt30         */
    seq[6].indx = 104;
    seq[6].iteration = 1;	/* reset du bc/rt       */
    seq[7].indx = 20;
    seq[7].iteration = 1;	/* new_line if needs    */

    seq[8].indx = 0;
    accept_flg = 1;
    er = exe_seq ();
    accept_flg = 0;
    for (cer = 0; cer <= strlen (Tx_CR); cer++) {
	print (1, "%c", Tx_CR[cer]);
	sleep (1);
    }
    return (er);
}

int accept_pz (void)
{				/* test standard d'acceptance d'une carte */
    int er;

    bouclage_seq = 0;
    seq[0].indx = 71;
    seq[0].iteration = 1;	/* test des signatures  */
    seq[1].indx = 72;
    seq[1].iteration = 1;	/* test du rt30         */
    seq[2].indx = 73;
    seq[2].iteration = 1;	/* reset du bc/rt       */
    seq[3].indx = 61;
    seq[3].iteration = 1;	/* test mode code       */
    seq[4].indx = 62;
    seq[4].iteration = 1;	/* test mode donnee     */
    seq[5].indx = 63;
    seq[5].iteration = 1;	/* test broadcast       */
    seq[6].indx = 67;
    seq[6].iteration = 1;	/* init du mode TEST    */
    seq[7].indx = 64;
    seq[7].iteration = 1;	/* test du csr          */
    seq[8].indx = 65;
    seq[8].iteration = 1;	/* test du RxBuffer     */
    seq[9].indx = 66;
    seq[9].iteration = 1;	/* test du TxBuffer     */
    seq[10].indx = 20;
    seq[10].iteration = 1;	/* new_line if needs    */

    seq[11].indx = 0;
    accept_flg = 1;
    er = exe_seq ();
    accept_flg = 0;
    return (er);
}

int taccept (void)
{				/* test standard d'acceptance d'une carte */
    int er;

    bouclage_seq = 0;
    seq[0].indx = 31;
    seq[0].iteration = 1;	/* test des signatures  */
    seq[1].indx = 32;
    seq[1].iteration = 1;	/* test du rt30         */
    seq[2].indx = 2;
    seq[2].iteration = 1;	/* reset du bc/rt       */
    seq[3].indx = 21;
    seq[3].iteration = 1;	/* test mode code       */
    seq[4].indx = 22;
    seq[4].iteration = 1;	/* test mode donnee     */
    seq[5].indx = 23;
    seq[5].iteration = 1;	/* test broadcast       */
    seq[6].indx = 27;
    seq[6].iteration = 1;	/* init du mode TEST    */
    seq[7].indx = 24;
    seq[7].iteration = 1;	/* test du csr          */
    seq[8].indx = 25;
    seq[8].iteration = 1;	/* test du RxBuffer     */
    seq[9].indx = 26;
    seq[9].iteration = 1;	/* test du TxBuffer     */
    seq[10].indx = 2;
    seq[10].iteration = 1;	/* reset du bc/rt       */
    seq[11].indx = 28;
    seq[11].iteration = 1;	/* test LOCAL           */
    seq[12].indx = 29;
    seq[12].iteration = 1;	/* test remote bounce   */
    seq[13].indx = 30;
    seq[13].iteration = 1;	/* test remote collision */
    seq[14].indx = 0;
    accept_flg = 1;
    er = exe_seq ();
    accept_flg = 0;
    return (er);
}
int not_test (void)
{
    return (0);
}

/********************************************************/
/* main: menu + input + exec a boucler.			*/
/********************************************************/
int exec (int indx, int tr)
{				/* index a executer + nombre de tours a effectuer */
    struct mdr_tests *tb_pt = tbl;
    int er, tt = 0;
    char st[30];

    while (tb_pt->index != -1) {
	if (tb_pt->index == indx) {	/* J'ai trouve la fonction */
	    er = 0;
	    do {
		if (indx == 5 || indx == 44 || indx == 84 || indx == 124)
		    print (1, "\rExecution de %s", tb_pt->txt);
		else
		    print (1, "\n\t%s", tb_pt->txt);
		erreur = 0;
		Flag_of_TO_Req = 1;
		erreur = tb_pt->fn ();
		if (indx == 2 || indx == 33 || indx == 73 || indx == 104) {
		    if (erreur == -1)
			erreur = 1;
		    else {
			convbin (erreur, (char *) st);
			erreur = 0;
			print (1, "\t\t\t    MC(0) - Status: %s", st);
		    }
		}
		er += erreur;
		if (indx == 1)
		    break;	/* choix_rt ne doit pas boucler! */
		if (indx == 3)
		    break;	/* config ne doit pas boucler! */
		if ((indx == 4) || (indx == 13) || (indx == 38)
		    || (indx == 78) || (indx == 118))
		    break;	/* seq_tst ne doit pas boucler! */
		clr_line ();
		if ((tt) || (tr > 1) || (tr == 0)) {
		    if ((indx == 14) || (indx == 39) || (indx == 79)
			|| (indx == 119))
			sprintf (st, "    TOUR [%d]", ++tt);
		    else
			sprintf (st, "\tTOUR [%d]", ++tt);
		}
		else
		    sprintf (st, "\t");
		if (indx != 14) {
		    if (erreur)
			print (0, "%s%s  %d errors in\n", st, tb_pt->txt, erreur);
		    else {
			if (indx != 30 && indx != 70 && indx != 25
			    && indx != 26 && indx != 65 && indx != 66
			    && indx != 33 && indx != 73 && indx != 2
			    && indx != 104 && indx != 34 && indx != 35
			    && indx != 36 && indx != 74 && indx != 75
			    && indx != 76)
			    print (1, "%s OK", st);
			else if (indx == 25 || indx == 26 || indx == 65
				 || indx != 66)
			    print (2, "\n");
		    }
		}
		if ((status & 0x80) && (Flag_of_TO_Req)) {
		    printf ("\n Attension!!!  Time_Out_Bit!!!\tStatus is: %x\n", status);
		}
		if (go_on == 0) {
		    print (0, " Interrupted by your request !(^e)\n");
		    break;
		}		/* ^C arrete l'execution */
		if (tt && (indx == 5 || indx == 44))
		    print (0, "\n");	/* c'est plus clair */
		if ((stop_after_err == 1) && (er != 0))
		    break;
	    } while ((tr == 0) || (--tr > 0));
	    return (er);
	}
	tb_pt++;
    }
    print (0, "No-o-o, this is bad choice. See Menu, please!\n");
    return (1);
}

void init (void)
{
    char *ttyname ();

/*	int sig;	*/

    seq[0].indx = 0;
    if (lib_init () != 0) {
	printf ("Il y a un probleme dans la liaison systeme du 1553. J'arrette\n");
	exit (1);
    }
    if (stop_poll () != 0) {
	printf ("Impossible de stopper le polling. J'arrette,\n");
	exit (1);
    }
    if ((f_in = open (ttyname (0), 2)) < 0) {
	perror ("open");
	exit (2);
    }
    if (ioctl (f_in, TCGETA, &iniold) < 0) {
	perror ("ioctl");
	exit (2);
    }
    iniold.c_cc[VINTR] = 5;
    iniold.c_cc[VQUIT] = 3;
    iniold.c_cc[VKILL] = 4;
    if (ioctl (f_in, TCSETA, &iniold) < 0) {
	perror ("ioctlset");
	exit (2);
    }
    close (f_in);
#if 0
    signal (SIGTERM, inter);
    signal (SIGTSTP, inter);
    signal (SIGQUIT, inter);
    signal (SIGINT, inter);
#endif
    iteration = 1;
    t_err = 1;			/*log_print=2; */
    setbuf (stdout, 0);
}

int main (int argc, char **argv)
{
    int indx;
    char c;

    init ();
    printf ("\n\n\n   WELCOME TO THE BANC OF THE TEST 1553\n\n");
    logfile ();
    print (0, " TEST 1553:  I'M   R E A D Y\n");
    choix_rt ();
    do {
	trsig = -1;
	prologue ();
	if (menu == 1)
	    menu_1 ();
	else if (menu == 2)
	    menu_2 ();
	else if (menu == 3)
	    menu_3 ();
	else
	    menu_0 ();
	printf ("choix :");
	while (read (f_in, &c, 1) == 0);
	epilogue ();
#if 0
	if (mdrop (bc, rt, 1, 31, 2, &status, data) == -1) {
	    printf ("System error in mdrop(RT[%d] TR[1] SA[31] WC[2])\n", rt);
	    printf (" - Reading Last Status (check Time_Out_Bit)\n");
	    printf ("Excuse me, but I'm wrong, - not Ready (or You, or Today is bad day, or ...\n");
	    if ((f_in = open (ttyname (0), 2)) < 0) {
		perror ("open");
		exit (2);
	    }
	    if (ioctl (f_in, TCSETA, &iniold) < 0) {
		perror ("ioctl");
		exit (2);
	    }
	    printf ("\n It's never too later to mind! See You later.\n");
	    exit (0);
	}
#endif
	if (c == 'z') {
	    if ((menu == 1) || (menu == 2) || (menu == 3)) {	/* pass au menu principal */
		menu = 0;
		continue;
	    }
	    else
		break;		/* fin du test */
	}
	if (menu == 0) {
	    if ((c == 'x') && ((rt_stat[rt] == 2) || (rt_stat[rt] == 1))) {	/* pass au menu de t1553 */
		menu = 1;
		continue;
	    }
	    if ((c == 'w') && (rt_stat[rt] == 3)) {	/* pass au menu de t232 */
		menu = 2;
		continue;
	    }
	    if ((c == 'v') && (rt_stat[rt] == 4)) {	/* pass au menu de t232 */
		menu = 3;
		continue;
	    }

	    if ((c == 'x') || (c == 'w') || (c == 'v')) {
		printf ("\nIt's wrong choice: (%c)\n", c);
		printf ("You are able to check this card rt[%d] by another Test (see Menu)\n", rt);
		continue;
	    }
	}
	if (menu != 0)
	    indx = (c - 'a') + 1 + 20 * (2 * menu - 1);
	else
	    indx = (c - 'a') + 1;
	exec (indx, iteration);
	go_on = 1;
	prologue ();
	printf ("\nPress any key, please :");
	while (read (f_in, &c, 1) == 0);
	epilogue ();
    } while (trsig != 3);
    print (0, "A tchao bonsoir....\n");
    if (log_file == 1)
	close (log_fd);
    return 0;
}
