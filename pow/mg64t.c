/*
 -------------------------------------------------------------
|   Module : Test Program for DSC-G64 Communication           |
|            for MIL-1553 to the G64-Power Converters.        |
|                                                             |
|   History : It tests the QuickData access                   |
|             calls provided in the libquick.a Library.       |
 -------------------------------------------------------------
*/

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h> /* Defines EXIT_SUCCESS and EXIT_FAILURE */
#include <unistd.h> /* Misc. POSIX 1003.4 defines */
#include <time.h>   /* Definitions for time structures. */
#include <dir.h> 
#include <string.h> 
#include <errno.h> 
#include <ctype.h>

#include <drvrutil/mil1553quicklib.h>
#include "pow_messages.h"

#define TYPE    1
#define SUB_FAMILY 0
#define POW_FAM 3

#define MAX_NB_RT     8
#define NIL (struct quick_data_buffer *)0

#define LSB(w)  ((w) & 0xFF)
#define MSB(w)  (((w) >> 8) & 0xFF)

extern  int get_rt_csr();
extern  int get_tx_buffer();
extern  int get_rx_buffer();
extern  int get_bc_tx_buffer();
extern  int get_bc_rx_buffer();
extern  int set_rt_csr();
extern  int clr_rt_csr();


static long    bc_mask, rt_mask;

static char *program;
char * act_val[] = {"ILLEGAL","OFF","STBY","ON","RESET"};
char * pstat_val[]={"ILLEGAL","OPERATIONAL","2","3","4"};
char * aspec_val[]={"ILLEGAL","NOTCON","LOCAL","REMOT"};

static  void p_stat();
static  void Print_Buffer();
static void read_ctl_msg();
static void read_cfg_msg();
static void read_acq_msg();
static void write_ctl_msg();

void Menu()
{
  printf("\n");
  printf("   Available Commands at your disposal:\n");
  printf(" 0 : Quit, exit from program\n");
  printf(" 1 : Overview of Connected RTs       ");
  printf(" 2 : Read RT's CSR Register\n");
  printf(" 3 : Read RT's Transmit Buffer       ");
  printf(" 4 : Read RT's Receive Buffer\n");
  printf(" 5 : Read BC's Transmit Buffer       ");
  printf(" 6 : Read BC's Receive Buffer\n");
  printf(" 7 : Read back control msg of a Pow  ");
  printf(" 8 : Read back config msg of a Pow\n");
  printf(" 9 : Read acquisition msg of a Pow   ");
  printf(" 10: Change Iref of a  Pow\n");
  printf(" 11: Reset Remote Terminal(RT)\n");

}
void call_failed()
{
  if (errno == 6) printf("BC number out of range");
  if (errno == 61) printf("RT Number out of range");
  printf("..call failed [%d]\n",errno);
}

int print_quickerr(err)
int err;
{
    if (err != 0) {
	switch (err) {
	    case BC_not_connected: 
		printf("Error: BC not connected\n");
		return(err);
	    case RT_not_connected: 
		printf("Error: RT not connected\n");
		return(err);
	    case TB_not_set: 
		printf("Error: TB not set\n");
		return(err);
	    case M1553_error: 
		printf("Error Mil1553\n");
		return(err);
	    case Bad_buffer: 
		printf("Error: Bad buffer");
		return(err);
	    default: 
		printf("Error quickdata\n");
		}
	}
    return(err);
}


/* ----- Start of main ---------------------------*/

int main ( int argc, char **argv) {


  u_short  buffer[128];
  int      cmd, i, j;
  short    bc, rt;
  u_short  csr;

	/*  */
 
  program = basename(argv[0]);
  
  if (init_quickdriver()) {    /* initialize M1553b driver */
	printf("%s: no init possible [%d]\n", program, errno);
	exit(errno);
	}

  for (;;) {
	     Menu();
	     cmd = 0;
	     printf("\nEnter a choice (0,1,2,3,4,5,6,7,8,9,10,11) ? > ");
	     scanf("%d",&cmd);
	     switch (cmd) {
		     case 0:
			  printf("\nLeaving the Test Program\n");
			  exit(0);
		     case 1:
			  if (get_connected_bc(&bc_mask)) {
			      printf("get_connected_bc failed [%d]\n",errno);
			      break;
			  }
			  for (i=1; i<32; i++) {
			      if (bc_mask & (1<<i)) {
				 if (get_connected_rt(i, &rt_mask)) {
				    printf("get_connected_rt failed [%d]\n",
					      errno);
				    break;
				 }
				 else {
				     printf("\nBC online is: %d \n", i);
				     for (j=0; j<31; j++)
					 if (rt_mask & (1<<j))
					     printf("RT on line is :%d\n", j);
				     printf("\n");
				 }
			      }
			  }
			  break;
		     case 2:	/*read CSR*/
			  printf("\nBC (1-31) and RT (0-30) : ");
			  scanf("%hd%hd",&bc,&rt);
			  if (get_rt_csr(bc, rt, &csr)) {
			     call_failed();
			  }
			 else {
			    printf("\nOn bc %d the rt %d CSR Status = %04X \n", bc,rt,csr);
			    p_stat(csr);
			  }
			  break;
		     case 3:	/* read TX buffer */
			  printf("\nBC (1-31) and RT (0-30) : ");
			  scanf("%hd%hd",&bc,&rt);
			  if (get_tx_buffer(bc, rt, &buffer[0])) {
			     call_failed();
			      break;
			  }
			  printf("rt transmit buffer: \n\n");
			  Print_Buffer(buffer);
			  break;
		     case 4:
			  printf("\nBC (1-31) and RT (0-30) : ");
			  scanf("%hd%hd",&bc,&rt);
			  if (get_rx_buffer(bc, rt, &buffer[0])) {
			     call_failed();
			      break;
			  }
			  printf("rt receive buffer: \n\n");
			  Print_Buffer(buffer);
			  break;
		     case 5:
			  printf("\nBC (1-31)  ");
			  scanf("%hd",&bc);
			  if (get_bc_tx_buffer(bc, &buffer[0])) {
			     call_failed();
			      break;
			  }
			  printf("bc transmit buffer: \n\n");
			  Print_Buffer(buffer);
			  break;
		     case 6:
			  printf("\nBC (1-31)  ");
			  scanf("%hd",&bc);
			  if (get_bc_rx_buffer(bc, &buffer[0])) {
			     call_failed();
			      break;
			  }
			  printf("bc receive buffer: \n\n");
			  Print_Buffer(buffer);
			  break;
		     case 7:
			  printf("\nBC (1-31), RT (0-30)");
			  scanf("%hd%hd",&bc,&rt);
			  read_ctl_msg(bc,rt);
			  break;
		     case 8:
			  printf("\nBC (1-31), RT (0-30)");
			  scanf("%hd%hd",&bc,&rt);
			  read_cfg_msg(bc,rt);
			  break;
	             case 9:
			  printf("\nBC (1-31), RT (0-30)");
			  scanf("%hd%hd",&bc,&rt);
			  read_acq_msg(bc,rt);
			  break;
		     case 10:
			  printf("\nBC (1-31), RT (0-30)");
			  scanf("%hd%hd",&bc,&rt);
			  write_ctl_msg(bc,rt);
			  break;	  
		     case 11:
			  printf("\nBC (1-31), RT (1-30) : ");
			  scanf("%hd%hd",&bc,&rt);
			  if (reset_rt(bc, rt)) {
			     call_failed();
			  }
			  else
			   printf("from bc %d rt %d is reset \n", bc,rt);
			  break;
		     default:
			  printf("Sorry, unknown choice, try again !\n");
			  break;
	     } /* End Switch */
	} /* End for */
	return (0);
} /* End of Main */

/* ------ Procedures definitions ---- */

void p_stat(stat)
	    u_short stat;
{
int i;
char *st;
   printf(" _______________________________________________________________\n");
   printf("| 5V|TES|LOC|BRD|NEM|LRR|BCR| BC|RRP|RTP|INT|INE| RL|INV| RB| TB|\n");
   printf("!---------------------------------------------------------------|\n");
   for (i = 15; i >= 0; i--) {
       if ( (1<<i) & stat) st = "| 1 ";
       else st = "| 0 ";
       printf(st);
   }
   printf("|\n");
   printf("|---------------------------------------------------------------|\n");
   printf("\n");
}




void Print_Buffer(buf)
u_short *buf;
{
int     i;

for (i=0; i < 127; i+=4)
	printf("buff[%d]=%4x  buff[%d]=%4x  buff[%d]=%4x  buff[%d]=%4x\n",
		i,buf[i],i+1,buf[i+1],i+2,buf[i+2],i+3,buf[i+3]);
}

/*--------------------------------------------------------------------------------*/
void read_ctl_msg(bc,rt)
short bc,rt;

{
    struct quick_data_buffer    send_buf,
                               *quickptr_req = &send_buf,
                                receive_buf,
                               *quickptr_ctl = &receive_buf;
    req_msg * req_ptr;
    ctrl_msg * ctrl_ptr;
    clock_t clk;
    short mbno=1, act, i;

    quickptr_req -> bc = bc;
    /* BC number */
    quickptr_req -> rt = rt;
    /* RT number */
    quickptr_req -> stamp = 0;
    quickptr_req -> error = 0;
    quickptr_req -> pktcnt = sizeof (req_msg);
    quickptr_req -> next = NIL;

/* Initialize request message */
    req_ptr = (req_msg *) & (quickptr_req -> pkt);
    req_ptr -> family = POW_FAM;
    req_ptr -> type = TYPE;
    req_ptr -> sub_family = SUB_FAMILY;
    req_ptr -> member = mbno;  /*member-no.......*/
    req_ptr -> service = 1;	/* requested service to read back ctrl msg 
				*/
    req_ptr -> cycle.machine = 0;
    req_ptr -> cycle.pls_line = 0;
    req_ptr -> specialist = 0;

/* Send request message to G64 */
    if (send_quick_data (quickptr_req) != 0) {
	printf("Send Quickdata error\n");
	return;
    }
    quickptr_ctl -> bc = bc;
    /* BC number */
    quickptr_ctl -> rt = rt;
    /* RT number */
    quickptr_ctl -> next = NIL;
    clk = clock () + 100000;	/* timeout 100ms */

/* Wait for the reply message */

    get_quick_data (quickptr_ctl);
    while ((clock () < clk) && (quickptr_ctl -> error != 0)) {
	get_quick_data (quickptr_ctl);
    }
    if (clock () >= clk) {
	printf("timeout\n");
	return;
    }
    if (print_quickerr(quickptr_ctl -> error) != 0) return;


/* print received data */
    ctrl_ptr = (ctrl_msg *) & (quickptr_ctl -> pkt);

/* check if wanted service was delivered */
    if (ctrl_ptr -> service != 1)
	printf("service Error\n");
    act = ctrl_ptr->ccsact;
    i=act; if (i<1 || i>4) i=0;
    printf("family:%d type:%d subfamily:%d\n",
            ctrl_ptr->family, ctrl_ptr->type, ctrl_ptr->sub_family);
    printf("member:%d, service:%d, mach:%d, pls:%d\n",
            ctrl_ptr->member,ctrl_ptr->service,ctrl_ptr->cycle.machine,ctrl_ptr->cycle.pls_line);
    printf("sec: %d, usec:%d, specialist:%d\n",
            ctrl_ptr->protocol_date.sec,ctrl_ptr->protocol_date.usec,ctrl_ptr->specialist);
    printf("actu-change-flag:%d, actuation: %s[%d],\n",
            ctrl_ptr->ccsact_change, act_val[i],act);
    printf("ccv:%f, ccv1:%f, ccv2:%f, ccv3:%f\n",
            ctrl_ptr->ccv, ctrl_ptr->ccv1, ctrl_ptr->ccv2, ctrl_ptr->ccv3);
    printf("ccv-chg:%d, ccv1-chg:%d,ccv2-chg:%d,ccv3-chg:%d\n",
            ctrl_ptr->ccv_change, ctrl_ptr->ccv1_change,ctrl_ptr->ccv2_change,
	    ctrl_ptr->ccv3_change);
}
/*--------------------------------------------------------------------------------*/
void read_cfg_msg(bc,rt)
short bc,rt;

{
    struct quick_data_buffer    send_buf,
                               *quickptr_req = &send_buf,
                                receive_buf,
                               *quickptr_ctl = &receive_buf;
    req_msg * req_ptr;
    conf_msg * conf_ptr;
    clock_t clk;
    short mbno=1;

    quickptr_req -> bc = bc;
    /* BC number */
    quickptr_req -> rt = rt;
    /* RT number */
    quickptr_req -> stamp = 0;
    quickptr_req -> error = 0;
    quickptr_req -> pktcnt = 22;
    quickptr_req -> next = NIL;

/* Initialize request message */
    req_ptr = (req_msg *) & (quickptr_req -> pkt);
    req_ptr -> family = 3;
    req_ptr -> type = TYPE;
    req_ptr -> sub_family = SUB_FAMILY;
    req_ptr -> member = mbno;
    req_ptr -> service = 5;	/* requested service to read back config msg 
				*/
    req_ptr -> cycle.machine = 0;
    req_ptr -> cycle.pls_line = 0;
    req_ptr -> specialist = 0;

/* Send request message to G64 */
    if (send_quick_data (quickptr_req) != 0) {
	printf("Send Quickdata error\n");
	return;
    }
    quickptr_ctl -> bc = bc;
    /* BC number */
    quickptr_ctl -> rt = rt;
    /* RT number */
    quickptr_ctl -> next = NIL;
    clk = clock () + 100000;	/* timeout 100ms */

/* Wait for the reply message */

    get_quick_data (quickptr_ctl);
    while ((clock () < clk) && (quickptr_ctl -> error != 0)) {
	get_quick_data (quickptr_ctl);
    }
    if (clock () >= clk) {
	printf("timeout\n");
	return;
    }
    if (print_quickerr(quickptr_ctl -> error) != 0) return;

/* print received data */
    conf_ptr = (conf_msg *) & (quickptr_ctl -> pkt);

/* check if wanted service was delivered */
    if (conf_ptr -> service != 5)
	printf("service Error\n");

    printf("family:%d typ:%d subfam:%d\n",
            conf_ptr->family, conf_ptr->type, conf_ptr->sub_family);
    printf("member:%d, service:%d, mach:%d, pls:%d\n",
            conf_ptr->member,conf_ptr->service,conf_ptr->cycle.machine,conf_ptr->cycle.pls_line);
    printf("sec: %d, usec:%d, specialist:%d\n",
            conf_ptr->protocol_date.sec,conf_ptr->protocol_date.usec,conf_ptr->specialist);
    printf("i_nominal:%f, resolution:%f,\n",
            conf_ptr->i_nominal, conf_ptr->resolution);
    printf("imax:%f, imin:%f, di-dt:%f, mode:%f\n",
            conf_ptr->i_max, conf_ptr->i_min, conf_ptr->di_dt,
	    conf_ptr->mode);

}
/*--------------------------------------------------------------------------------*/
void read_acq_msg(bc,rt)
short bc,rt;

{
    struct quick_data_buffer    send_buf,
                               *quickptr_req = &send_buf,
                                receive_buf,
                               *quickptr_ctl = &receive_buf;
    req_msg * req_ptr;
    acq_msg * ctrl_ptr;
    clock_t clk;
    short mbno=1, act,asp,i,j;

    quickptr_req -> bc = bc;
    /* BC number */
    quickptr_req -> rt = rt;
    /* RT number */
    quickptr_req -> stamp = 0;
    quickptr_req -> error = 0;
    quickptr_req -> pktcnt = sizeof (req_msg);
    quickptr_req -> next = NIL;

/* Initialize request message */
    req_ptr = (req_msg *) & (quickptr_req -> pkt);
    req_ptr -> family = POW_FAM;
    req_ptr -> type = TYPE;
    req_ptr -> sub_family = SUB_FAMILY;
    req_ptr -> member = mbno;  /*member-no.......*/
    req_ptr -> service = 0;	/* requested service to read acq msg 
				*/
    req_ptr -> cycle.machine = 0;
    req_ptr -> cycle.pls_line = 0;
    req_ptr -> specialist = 0;

/* Send request message to G64 */
    if (send_quick_data (quickptr_req) != 0) {
	printf("Send Quickdata error\n");
	return;
    }
    quickptr_ctl -> bc = bc;
    /* BC number */
    quickptr_ctl -> rt = rt;
    /* RT number */
    quickptr_ctl -> next = NIL;
    clk = clock () + 100000;	/* timeout 100ms */

/* Wait for the reply message */

    get_quick_data (quickptr_ctl);
    while ((clock () < clk) && (quickptr_ctl -> error != 0)) {
	get_quick_data (quickptr_ctl);
    }
    if (clock () >= clk) {
	printf("timeout\n");
	return;
    }
    if (print_quickerr(quickptr_ctl -> error) != 0) return;

/* print received data */
    ctrl_ptr = (acq_msg *) & (quickptr_ctl -> pkt);

/* check if wanted service was delivered */
    if (ctrl_ptr -> service != 0)
	printf("service Error\n");
    act=ctrl_ptr->static_status; i=act; if (i<1 || i>3) i=0;
    asp=ctrl_ptr->ext_aspect; j=asp; if (j<1 || j>3) j=0;
    printf("family:%d typ:%d subfam:%d\n",
            ctrl_ptr->family, ctrl_ptr->type, ctrl_ptr->sub_family);
    printf("member:%d, service:%d, mach:%d, pls:%d\n",
            ctrl_ptr->member,ctrl_ptr->service,ctrl_ptr->cycle.machine,ctrl_ptr->cycle.pls_line);
    printf("sec: %d, usec:%d, specialist:%d\n",
            ctrl_ptr->protocol_date.sec,ctrl_ptr->protocol_date.usec,ctrl_ptr->specialist);
    printf("phys_status:%d, static_status:%s[%d],\n",
            ctrl_ptr->phys_status, act_val[i],act);
    printf("ext_aspect:%s[%d], status_qualif:%d, busy_time:%d\n",
            aspec_val[j], asp, ctrl_ptr->status_qualif, ctrl_ptr->busytime);
    printf("aqn:%f, aqn1:%f,aqn2:%f,aqn3:%f\n",
            ctrl_ptr->aqn, ctrl_ptr->aqn1,ctrl_ptr->aqn2, ctrl_ptr->aqn3);
}
/*--------------------------------------------------------------------------------*/
void write_ctl_msg(bc,rt)
short bc,rt;


{
    struct quick_data_buffer    send_buf,
                               *quickptr_req = &send_buf,
                                receive_buf,
                               *quickptr_ctl = &receive_buf;
    req_msg * req_ptr;
    ctrl_msg * ctrl_ptr;
    clock_t clk;
    short mbno=1;
    float iref;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>from powrt:

    gettimeofday(&da, NULL);

	memcpy(&pc->pkt[0], &cact->v2[i*6], 24);  non-ppm ctrl. values (actuations) 
	memcpy(&pc->pkt[24], &cact->v3[i*5], 20); ppm ctrl. values 
	memcpy(&pc->pkt[12], &da, 8);             set TOD field 
        pc->pkt[8] = 0;
	pc->pkt[9] = plstb;                      machine type: CPS, PSB, CTF 
        pc->pkt[10] = 0;
        pc->pkt[11] = (tgm) ? next_grp_val : pres_grp_val; cycle 
        pc->error = 0;
        pc->pktcnt = 44;                         size of control message  


    Send messages over MIL-1553: cact->nb messages 
    if (send_quick_data(cact->ctl) != 0) { MIL-1553 error (encoded in errno) 
        for (i = 0; i <= cact->nb; i++) cact->er[i] = EQP_QCKDATERR; log errors 
        if (trace_ctl_flg) fprintf(stderr, "%s: DoControl: send_quick_data ioctl error = %d\n", program, errno);
    }
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

    quickptr_req -> bc = bc;
    /* BC number */
    quickptr_req -> rt = rt;
    /* RT number */
    quickptr_req -> stamp = 0;
    quickptr_req -> error = 0;
    quickptr_req -> pktcnt = sizeof (req_msg);
    quickptr_req -> next = NIL;

/* Initialize request message */
    req_ptr = (req_msg *) & (quickptr_req -> pkt);
    req_ptr -> family = POW_FAM;
    req_ptr -> type = TYPE;
    req_ptr -> sub_family = SUB_FAMILY;
    req_ptr -> member = mbno;  /*member-no.......*/
    req_ptr -> service = 1;	/* requested service to read back ctrl msg 
				*/
    req_ptr -> cycle.machine = 0;
    req_ptr -> cycle.pls_line = 0;
    req_ptr -> specialist = 0;

/* Send request message to G64 */
    if (send_quick_data (quickptr_req) != 0) {
	printf("Send Quickdata error\n");
	return;
    }
    quickptr_ctl -> bc = bc;
    /* BC number */
    quickptr_ctl -> rt = rt;
    /* RT number */
    quickptr_ctl -> next = NIL;
    clk = clock () + 100000;	/* timeout 100ms */

/* Wait for the reply message */

    get_quick_data (quickptr_ctl);
    while ((clock () < clk) && (quickptr_ctl -> error != 0)) {
	get_quick_data (quickptr_ctl);
    }
    if (clock () >= clk) {
	printf("get_quick_data timeout after 100ms\n");
	return;
    }
    if (print_quickerr(quickptr_ctl -> error) != 0) return;


/* reuse received data */
    ctrl_ptr = (ctrl_msg *) & (quickptr_ctl -> pkt);

/* check if wanted service was delivered */
    if (ctrl_ptr -> service != 1) {
	printf("Service Error\n");
	return;
	}

    printf("Present Iref:%f\n",ctrl_ptr->ccv);
    printf("\nEnter New Iref:");
    scanf("%f",&iref);

    ctrl_ptr->ccv = iref;
    ctrl_ptr->ccv_change = 1; /*request to change*/
    ctrl_ptr -> service =0;    
	    
/* Send request message to G64 */
    if (send_quick_data (quickptr_ctl) != 0) {
	printf("Send Quickdata error sending ctl msg\n");
	return;
    }
}



