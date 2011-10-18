#include <unistd.h>
#include <errno.h>
#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, status;
extern int menu, log_print, go_on, errno;

void clr_line (void)
{
    print (2, "                                                                              \r");
}


int verifie_ST (unsigned short status, unsigned short csr)
{
    unsigned short temp_st = 0;

    if (csr & 0x6000)
	temp_st = 0x0008;
    if (csr & 0x0C00)
	temp_st += 0x0100;
    if (csr & 0x000C)
	temp_st += 0x0004;

    if ((status & temp_st) != temp_st)
	return (temp_st);
    return (0);
}

int valid_MC (unsigned short rt, unsigned short tr, unsigned short sa,
	      unsigned short wc, unsigned short data)
{
    unsigned short mc;

    mc = (rt << 11) | (tr << 10) | (sa << 5) | wc;
    if (mc == data)
	return (0);
    return (1);
}

int valid_ST (unsigned short rt, unsigned short data)
{
    if (menu == 0) {
	if ((rt << 11) == (data & 0x0fe7b))
	    return (0);
	return (1);
    }
    if ((rt << 11) == (data & 0x0fe7f))
	return (0);
    return (1);
}

int wait_for_csr (int pattern, int res, int delai, int flg)
{
    unsigned short csr;

    do {
	if (flg)
	    print (2, "Attente du CSR [%04X]=[%04X] :%.3d secondes    \r", pattern, csr, delai);
	if (get_csr (bc, rt, &csr, &status) == (-1)) {
	    print (1, "\nwait_for_csr:Sysytem error in the get_csr()");
	    return (0);
	}
	if ((csr & pattern) == res) {
	    if (flg)
		print (2, "Detection CSR OK \n");
	    return (1);
	}
	if (go_on == 0)
	    return (0);
	if (flg)
	    sleep (1);
    } while (--delai);
    return (0);
}

int Clr_csr (unsigned short pat, char *st)
{
    unsigned short csr = 0;

    print (2, "Mise a 0 du BIT %s\r", st);
    csr = pat;
    if (clr_csr (bc, rt, &csr, &status) == (-1)) {
	print (1, "Probleme systeme [%d] avec clr_csr", errno);
	return (1);
    }
    if (get_csr (bc, rt, &csr, &status) == (-1)) {
	print (1, "Probleme systeme [%d] avec get_csr", errno);
	return (1);
    }
    if (csr & pat) {
	print (1, " FAILED.\n");
	return (1);
    }
    return (0);
}

int Set_csr (unsigned short pat, char *st)
{
    unsigned short csr = 0;
    unsigned short str1[20], str2[20];

    if (log_print != 2)
	sleep (1);
    print (2, "Mise a 1 du BIT %s\r", st);
    csr = pat;
    if (set_csr (bc, rt, &csr, &status) == (-1)) {
	print (1, "Probleme systeme [%d] avec set_csr", errno);
	return (1);
    }
    if (get_csr (bc, rt, &csr, &status) == (-1)) {
	print (1, "Probleme systeme [%d] avec get_csr", errno);
	return (1);
    }
    if ((csr & pat) != pat) {
	convbin (pat, (char *) str1);
	convbin (csr, (char *) str2);
	print (1, " Set bit of CSR faild. Pattern: [%s] CSR: [%s]\n", (char *) str1, (char *) str2);
	return (1);
    }
    return (0);
}

int reset (void)
{
    char st[20];
    unsigned short csr[8];

    print (2, "\rReset (MC8) du bc[%d] rt[%d] ", bc, rt);
    if (mdrop (bc, rt, 1, 0, 8, &status, (char *) csr) != 0) {
	print (1, "\nReset:System error for mdrop mode_code:RESET");
	return (-1);
    }
    convbin (status, st);
    print (1, "    MC(8) - Status: %s\n", st);
    sleep (4);
    if (get_csr (bc, rt, csr, &status) == (-1)) {
	print (1, "\nReset:System error for get_csr");
	return (-1);
    }
    return (status);
}
