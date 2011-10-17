#include <stdio.h>
#include "1553def.h"
#include "mdacc.h"

extern unsigned short bc, rt, status, stop_after_err;
extern int erreur, go_on, t_err;
char str1[20];

int rd_csr (void)
{
    unsigned short csr;

    if (get_csr (bc, rt, &csr, &status) == -1) {
	print (1, "rd_csr:System error with get_csr\n");
	return (++erreur);
    }
    if (status & 0x80) {
	printf ("  I beg Your pardon, but....(after get_csr())");
	return (0);
    }
    convbin (csr, str1);
    printf ("CSR: %s\n", str1);
    return (0);
}

int wr_csr (void)
{
    unsigned short csr;
    int val;

    printf ("Please, data for writing to the CSR (hexadecimal format) :");
    scanf ("%x", &val);
    csr = (unsigned short) val;
    if (set_csr (bc, rt, &csr, &status) == -1) {
	print (1, "wr_csr:System error with set_csr\n");
	return (++erreur);
    }
    if (status & 0x80) {
	printf ("  I beg Your pardon, but....(after set_csr())");
	return (0);
    }
    if (get_csr (bc, rt, &csr, &status) == -1) {
	print (1, "wr_csr:System error with get_csr\n");
	return (++erreur);
    }
    if (status & 0x80) {
	printf ("  I beg Your pardon, but....(after get_csr())");
	return (0);
    }
    convbin (csr, str1);
    printf ("CSR: %s\n", str1);
    return (0);
}

int cl_csr (void)
{
    unsigned short csr;
    int val;

    printf ("Please, data for clearing in the CSR (hexadecimal format) :");
    scanf ("%x", &val);
    csr = (unsigned short) val;
    if (clr_csr (bc, rt, &csr, &status) == -1) {
	print (1, "cl_csr:System error with clr_csr\n");
	return (++erreur);
    }
    if (status & 0x80) {
	printf ("  I beg Your pardon, but....(after clr_csr())");
	return (0);
    }
    if (get_csr (bc, rt, &csr, &status) == -1) {
	print (1, "cl_csr:System error with get_csr\n");
	return (++erreur);
    }
    if (status & 0x80) {
	printf ("  I beg Your pardon, but....(after get_csr())");
	return (0);
    }
    convbin (csr, str1);
    printf ("CSR: %s\n", str1);
    return (0);
}
