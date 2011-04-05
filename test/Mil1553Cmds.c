/* ****************************************************************************** */
/* MIL1553 test program, calls driver                                             */
/* Julian Lewis AB/CO/HT Julian.Lewis@cern.ch FEB 2011                            */
/* ****************************************************************************** */

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>   /* ctime */
#include <sys/time.h>
#include <stdlib.h>

#include <Cmds.h>
#include <libmil1553.h>
#include <librti.h>
#include <libquick.h>
#include "pow_messages.h"

#ifndef COMPILE_TIME
#define COMPILE_TIME 0
#endif

float strtof(const char *nptr, char **endptr);

int rti = 1;                                                                                                                                                        
static char *editor = "e";

/* ============================= */

char *defaultconfigpath = "./mil1553test.config";

char *configpath = NULL;
char localconfigpath[128];  /* After a CD */

static int  reg     =  0;
static int  debug   =  0;
static int  tmo     =  1000;

static unsigned int memlen  = MAX_REGS * sizeof(unsigned int);
static unsigned int mem[MAX_REGS];

/* ============================= */

struct regdesc {
   char name[32];
   char flags[4];
   int  offset;
   int  size;
   int  window;
   int  depth;
   struct regdesc *next;
};

static struct regdesc *regs = NULL;
int reg_cnt = 0;

/* ============================= */

static char path[128];

char *GetFile(char *name) {
FILE *gpath = NULL;
char txt[128];
int i, j;

   if (configpath) {
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
      }
   }

   if (configpath == NULL) {
      configpath = "./mil1553.config";
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = "/dsc/local/data/mil1553.config";
	 gpath = fopen(configpath,"r");
	 if (gpath == NULL) {
	    configpath = defaultconfigpath;
	    gpath = fopen(configpath,"r");
	    if (gpath == NULL) {
	       configpath = NULL;
	       sprintf(path,"./%s",name);
	       return path;
	    }
	 }
      }
   }

   bzero((void *) path,128);

   while (1) {
      if (fgets(txt,128,gpath) == NULL) break;
      if (strncmp(name,txt,strlen(name)) == 0) {
	 for (i=strlen(name); i<strlen(txt); i++) {
	    if (txt[i] != ' ') break;
	 }
	 j= 0;
	 while ((txt[i] != ' ') && (txt[i] != 0)) {
	    path[j] = txt[i];
	    j++; i++;
	 }
	 strcat(path,name);
	 fclose(gpath);
	 return path;
      }
   }

   fclose(gpath);
   sprintf(path,"./%s",name);
   return path;
}

/* ============================= */

struct regdesc *new_reg() {

struct regdesc *reg;

   reg = malloc(sizeof(struct regdesc));
   if (reg == NULL) return NULL;

   reg->next = regs;
   regs = reg;
   return regs;
}

/* ============================= */

struct regdesc *get_reg_by_offset(struct regdesc *start, int offset) {

struct regdesc *reg;

   reg = start;
   while (reg) {
      if (reg->offset == offset)
	 return reg;
      reg = reg->next;
   }
   return NULL;
}

/* ============================= */

struct regdesc *get_reg_by_name(struct regdesc *start, char *name) {

struct regdesc *reg;

   reg = start;
   while (reg) {
      if (strcmp(reg->name,name) == 0)
	 return reg;
      reg = reg->next;
   }
   return NULL;
}

/* ============================= */

void print_regs() {
struct regdesc *reg;

   reg = regs;
   while (reg) {
      if (reg->depth != 1) {
	 printf("%16s - %2s Wn:%d Of:0x%04X Sz:%d Dp:%d\n",
		reg->name,
		reg->flags,
		reg->window,
		reg->offset,
		reg->size,
		reg->depth);
      } else {
	 printf("%16s - %2s Wn:%d Of:0x%04X Sz:%d\n",
		reg->name,
		reg->flags,
		reg->window,
		reg->offset,
		reg->size);
      }
      reg = reg->next;
   }
}

/* ============================= */

void read_regs() {

char txt[128];
FILE *nf = NULL;
struct regdesc *reg;

   if (regs) return;

   nf = fopen(GetFile(reg_file),"r");
   if (nf == NULL) {
      printf("Can't open:%s\n",reg_file);
      return;
   }

   while (1) {
      if (fgets(txt,128,nf) == NULL) break;
      if (strncmp("$", txt, 1) == 0) {
	 reg = new_reg();

	 sscanf(txt,"$ %s %s 0x%X %d %d %d",
		reg->name,
		reg->flags,
		&reg->offset,
		&reg->size,
		&reg->window,
		&reg->depth);
	 reg_cnt++;
      }
   }
   print_regs();
   printf("Device:%s %d:reg descriptions read\n",DEV_NAME, reg_cnt);
}

/* ============================= */

static char *TimeToStr(int t) {

static char tbuf[128];

char tmp[128];
char *yr, *ti, *md, *mn, *dy;

   bzero((void *) tbuf, 128);
   bzero((void *) tmp, 128);

   if (t) {
      ctime_r ((time_t *) &t, tmp);  /* Day Mon DD HH:MM:SS YYYY\n\0 */

      tmp[3] = 0;
      dy = &(tmp[0]);
      tmp[7] = 0;
      mn = &(tmp[4]);
      tmp[10] = 0;
      md = &(tmp[8]);
      if (md[0] == ' ') md[0] = '0';
      tmp[19] = 0;
      ti = &(tmp[11]);
      tmp[24] = 0;
      yr = &(tmp[20]);

      sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);
    } else
      sprintf (tbuf, "--- Zero ---");
    return (tbuf);
}

/* ============================= */
/* News                          */

int News(int arg) {

char sys[128], npt[128];

   arg++;

   if (GetFile("mil1553_news")) {
      strcpy(npt,path);
      sprintf(sys,"%s %s",GetFile(editor),npt);
      printf("\n%s\n",sys);
      system(sys);
      printf("\n");
   }
   return(arg);
}

/* ============================= */
/* Batch mode controls YesNo     */

static int yesno=1;
int Batch(int arg) {

ArgVal   *v;
AtomType  at;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Numeric) {
      yesno = v->Number;
      arg++;
   }
   return arg;
}

/* ============================= */

static int YesNo(char *question, char *name) {
int yn, c;

   if (yesno == 0) return 1;

   printf("%s: %s\n",question,name);
   printf("Continue (Y/N):"); yn = getchar(); c = getchar();
   if ((yn != (int) 'y') && (yn != 'Y')) return 0;
   return 1;
}

/* ============================= */

int ChangeEditor(int arg) {
static int eflg = 0;

   arg++;
   if (eflg++ > 4) eflg = 1;

   if      (eflg == 1) editor = "e";
   else if (eflg == 2) editor = "emacs";
   else if (eflg == 3) editor = "nedit";
   else if (eflg == 4) editor = "vi";

   printf("Editor: %s\n",GetFile(editor));
   return arg;
}

/* ============================= */

static int DefaultFile = 0;

int GetPath(int arg,char *defnam) {
ArgVal   *v;
AtomType  at;

char fname[128], *cp;
int n, earg;

   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;
   if ((v->Type == Close)
   ||  (v->Type == Terminator)) {

      DefaultFile = 1;
      GetFile(defnam);

   } else {

      DefaultFile = 0;
      bzero((void *) fname, 128);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));
      strcpy(path,fname);
   }
   return earg;
}

/* ============================= */

int ChangeDirectory(int arg) {
ArgVal   *v;
AtomType  at;
char txt[128], fname[128], *cp;
int n, earg;

   arg++;
   for (earg=arg; earg<pcnt; earg++) {
      v = &(vals[earg]);
      if ((v->Type == Close)
      ||  (v->Type == Terminator)) break;
   }

   v = &(vals[arg]);
   at = v->Type;
   if ((v->Type != Close)
   &&  (v->Type != Terminator)) {

      bzero((void *) fname, 128);

      n = 0;
      cp = &(cmdbuf[v->Pos]);
      do {
	 at = atomhash[(int) (*cp)];
	 if ((at != Seperator)
	 &&  (at != Close)
	 &&  (at != Terminator))
	    fname[n++] = *cp;
	 fname[n] = 0;
	 cp++;
      } while ((at != Close) && (at != Terminator));

      strcpy(localconfigpath,fname);
      strcat(localconfigpath,"/mil1553.config");
      if (YesNo("Change mil1553 config to:",localconfigpath))
	 configpath = localconfigpath;
      else
	 configpath = NULL;
   }

   cp = GetFile(editor);
   sprintf(txt,"%s %s",cp,configpath);
   printf("\n%s\n",txt);
   system(txt);
   printf("\n");
   return(arg);
}

/* ============================= */

unsigned long get_mem(int address) {

int i;
unsigned long data = 0;

   i = address/sizeof(int);
   data = ((int *) mem)[i];
   return data;
}

/* ============================= */

void set_mem(int address, unsigned long data) {

int i;

   i = address/sizeof(int);
   ((int *) mem)[i] = (int) data;
   return;
}

/* ============================= */

char *get_name(int offset) {

struct regdesc *reg;
static char ntxt[128];
char atxt[64];
int rnum;

   bzero((void *) ntxt, 128);

   rnum = offset/sizeof(int);

   reg = get_reg_by_offset(regs,offset);
   while (reg) {
      if (reg->depth != 1) sprintf(atxt,"%s:%02d:%s:[0x%X]:",reg->name,rnum,reg->flags,reg->depth);
      else                 sprintf(atxt,"%s:%02d:%s:",reg->name,rnum,reg->flags);
      strcat(ntxt,atxt);
      reg = get_reg_by_offset(reg->next,offset);
   }
   return(ntxt);
}

/* ============================= */

int get_offset(char *name) {

struct regdesc *reg;

   reg = get_reg_by_name(regs,name);
   if (reg) return reg->offset;
   return 0;
}

/* ============================= */

void EditBuffer(int address) {

unsigned long addr, data;
char c, *cp, str[128];
int n, radix, nadr;

   printf("EditMemory: [?]Help [/]Open [CR]Next [.]Exit [x]Hex [d]Dec\n\n");

   addr = address;
   radix = 16;
   c = '\n';

   while (1) {

      if (radix == 16) {
	 if (c=='\n') printf("%24s 0x%04x: 0x%08x ", get_name(addr), (int) addr, (int) get_mem(addr));
      } else {
	 if (c=='\n') printf("%24s %04d: %5d ",      get_name(addr), (int) addr, (int) get_mem(addr));
      }

      c = (char) getchar();

      if (c == '/') {
	 bzero((void *) str, 128); n = 0;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 nadr = strtoul(str,&cp,radix);
	 if (cp != str) addr = nadr;
      }

      else if (c == 'x')  { radix = 16; if (addr) addr--; continue; }
      else if (c == 'd')  { radix = 10; if (addr) addr--; continue; }
      else if (c == '.')  { c = getchar(); printf("\n"); break; }
      else if (c == 'q')  { c = getchar(); printf("\n"); break; }
      else if (c == '\n') { addr += sizeof(int); if (addr >= memlen) {addr = 0; printf("\n----\n");} }
      else if (c == '?')  { printf("[?]Help [/]Open [CR]Next [.]Exit [x]Hex [d]Dec\n"); }

      else {
	 bzero((void *) str, 128); n = 0;
	 str[n++] = c;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 data = strtoul(str,&cp,radix);
	 if (cp != str) set_mem(addr,data);
      }
   }
}

/* ============================= */

int EditMem(int arg) { /* Address */
ArgVal   *v;
AtomType  at;
int address;

   arg++;
   address = 0;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      address = v->Number;
      v = &(vals[arg]);
      at = v->Type;
      if (address >= memlen) address = 0;
   }

   EditBuffer(address);
   return arg;
}

/* ============================= */

int EditRegs(int arg) { /* Register number */
ArgVal   *v;
AtomType  at;

int reg_val;
char *cp;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      reg = v->Number;
      cp = get_name(reg*sizeof(int));
   } else if (at == Alpha) {
     cp = v->Text;
     reg = get_offset(cp)/sizeof(int);
   } else goto rdrg;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      reg_val = v->Number;
      if (milib_write_reg(milf,bc,reg,reg_val) < 0) {
	 printf("Can't write reg:%d %s\n",reg,cp);
	 return arg;
      }
   }

rdrg:
   cp = get_name(reg*sizeof(int));

   if (milib_read_reg(milf,bc,reg,&reg_val) < 0) {
      printf("Can't read reg:%d %s\n",reg,cp);
      return arg;
   }
   printf("reg:%d %s = 0x%X (%d) OK\n",reg,cp,reg_val,reg_val);
   return arg;
}

/* ============================= */

int EditDebug(int arg) {
ArgVal   *v;
AtomType  at;

int tdebug, cc;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (milib_set_debug_level(milf,v->Number) >= 0) debug = v->Number;
      else printf("Error from milib_set_debug_level, level:%d\n",v->Number);
   }
   cc = milib_get_debug_level(milf,&tdebug);
   if (!cc) debug = tdebug;
   else {
      printf("Error:milib_get_debug_level\n");
      milib_print_error(cc);
   }
   printf("debug:level:%d:",debug);
   if (debug) printf("ON\n");
   else       printf("OFF\n");
   return arg;
}

/* ============================= */

int EditTimeout(int arg) {
ArgVal   *v;
AtomType  at;

int ttmo, cc;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      cc = milib_set_timeout(milf,v->Number);
      if (!cc) tmo = v->Number;
      else {
	 printf("Error:milib_set_timeout:level:%d\n",v->Number);
	 milib_print_error(cc);
      }
   }
   cc = milib_get_timeout(milf,&ttmo);
   if (!cc) tmo = ttmo;
   else {
      printf("Error:milib_get_timeout\n");
      milib_print_error(cc);
   }
   printf("timeout:milliseconds:%d:",tmo);
   if (tmo) printf("SET\n");
   else     printf("NOT_SET, wait forever\n");
   return arg;
}

/* ============================= */

static struct mil1553_recv_s recv_buf;
struct mil1553_rti_interrupt_s *isrp;

int WaitInterrupt(int arg) {

ArgVal   *v;
AtomType  at;
int pk_type;
short *words;
int i, cc;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   pk_type = recv_buf.pk_type;
   if (at == Numeric) {
      arg++;
      pk_type = v->Number;
   }
   recv_buf.pk_type = pk_type;
   recv_buf.timeout = tmo;

   cc = milib_recv(milf,&recv_buf);
   if (cc) {
      printf("Error:milib_recv:%d\n",cc);
      milib_print_error(cc);
      return arg;
   }

   isrp = &recv_buf.interrupt;
   words = (short *) isrp->rxbuf;

   printf("Interrupt:Count:%d:pktyp:%02d:bc:%d:rti:%02d:wc:%02d\n",
	  recv_buf.icnt,
	  recv_buf.pk_type,
	  isrp->bc,
	  isrp->rti_number,
	  isrp->wc);

   if (isrp->wc) {
      for (i=0; i<RX_BUF_SIZE; i++) {
	 if (i > isrp->wc) break;
	 if (!(i % 4)) printf("\n%02d: ",i);
	 printf("0x%04hX ",words[i]);
      }
      printf("\n");
   }
   return arg;
}

/* ============================= */

int RawRead(int arg) {  /* Start, Item count */

ArgVal   *v;
AtomType  at;
int regs, start, cc;
struct mil1553_riob_s riob;

   arg++;

   start = 0;
   regs = MAX_REGS;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      start = v->Number;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 regs = v->Number;

	 arg++;
      }
   }

   if ((start + regs) > MAX_REGS) {
      printf("Bad Address:Start:%d + Count:%d = Address:%d\n",start,regs,start+regs);
      return arg;
   }

   printf("Read registers from:%02d to:%02d = count:%02d\n",start,regs,start+regs);

   riob.bc = bc;
   riob.reg_num = start;
   riob.buffer = mem;
   riob.regs = regs;
   cc = milib_raw_read(milf,&riob);
   if (cc) {
      printf("milib_raw_read:Error:%d\n",cc);
      milib_print_error(cc);
   }
   return arg;
}

/* ============================= */

int RawWrite(int arg) {
ArgVal   *v;
AtomType  at;
int regs, start;
struct mil1553_riob_s riob;
int cc;

   arg++;

   start = 0;
   regs = 1;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      start = v->Number;

      arg++;
      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 regs = v->Number;

	 arg++;
      }
   }

   if ((start + regs) > MAX_REGS) {
      printf("Bad Address:Start:%d + Count:%d = Address:%d\n",start,regs,start+regs);
      return arg;
   }

   printf("Write registers from:%02d to:%02d = count:%02d\n",start,regs,start+regs);

   riob.bc = bc;
   riob.reg_num = start;
   riob.buffer = mem;
   riob.regs = regs;
   cc = milib_raw_write(milf,&riob);
   if (cc) {
      printf("milib_raw_write:Error:%d\n",cc);
      milib_print_error(cc);
   }
   return arg;
}

/* ============================= */

int GetVersion(int arg) {
int ver, cc;

   cc = milib_get_drv_version(milf,&ver);
   if (cc) {
      printf("milib_get_drv_version:Error:%d\n",cc);
      milib_print_error(cc);
   }
      printf("Driver:  %s - %d\n",TimeToStr(ver),ver);

   arg++;
   return arg;
}

/* ============================= */

int ReadStatus(int arg) {    /* Read status */

int cc;
int stat;

   arg++;
   cc = milib_get_status(milf, bc, &stat);
   if (cc) {
      printf("milib_get_status:Error:%d\n",cc);
      milib_print_error(cc);
   }
   printf("Status:0x%4X %s\n",stat,milib_status_to_str(stat));
   return arg;
}

/* ============================= */

int GetSetBC(int arg) {      /* Select bus controler module */

ArgVal   *v;
AtomType  at;
int tbc = 0;
int bcs_count = 0;
int cc;

   arg++;

   cc = milib_get_bcs_count(milf, &bcs_count);
   if (cc) {
      printf("milib_get_bcs_count:Error:%d\n",cc);
      milib_print_error(cc);
   }

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      tbc = v->Number;
      if (tbc < bcs_count) bc = tbc;
      arg++;
   }

   printf("Installed Bus Controllers:%d Current Bus Controller:%d\n",bcs_count,bc);

   return arg;
}

/* ============================= */

#define SPEEDS 4

char *speed_names[SPEEDS] = { "1Mbit",
			      "500Kbit",
			      "250Kbit",
			      "125Kbit" };

char *speed_to_str(int speed) {

static char *res = "???";

   if ((speed >=0) && (speed < SPEEDS))
      res = speed_names[speed];

   return res;
}

int GetBcInfo(int arg) {     /* Get BC info */

struct mil1553_dev_info_s dev_info;
int cc;

   arg++;

   dev_info.bc = bc;
   cc = milib_get_bc_info(milf,&dev_info);
   if (cc) {
      printf("milib_get_bc_info:Error:%d\n",cc);
      milib_print_error(cc);
   }

   printf("Bus Controler    :%d => %02x:%02x CERN/ECP/EDU Device 0301 CBMIA\n",
	  dev_info.bc,
	  dev_info.pci_bus_num,
	  dev_info.pci_slt_num);

   printf("PCI Bus Number   :%d\n",dev_info.pci_bus_num);
   printf("PCI Slot Number  :%d\n",dev_info.pci_slt_num);
   printf("Serial Number    :0x%08x:%08x\n",dev_info.snum_h,dev_info.snum_l);
   printf("Hardware Version :0x%08x\n",dev_info.hardware_ver_num);
   printf("Bus Speed        :%d => %s\n",dev_info.speed,speed_to_str(dev_info.speed));
   printf("Interrupt count  :%d\n",dev_info.icnt);

   /* Meaning of isrdebug bits, for me to know whats going on */

   /* 0x01 CBMIA interrupt */
   /* 0x02 Queue empty */
   /* 0x04 Client icnt incremented */
   /* 0x08 Data copied to client queue */
   /* 0x10 Client wake up called */
   /* 0x20 Busy BC set */
   /* 0x40 Done BC set */

   printf("IsrTrace         :0x%08x\n",dev_info.isrdebug);

   return arg;
}

/* ============================= */

int GetUpRtis(int arg) {     /* Get up RTIs */

unsigned short sig;
int up_rtis, i, count, cc;

   arg++;
   up_rtis = 0;
   count = 0;

   cc = milib_get_up_rtis(milf, bc, &up_rtis);
   if (cc) {
      printf("milib_get_up_rtis:Error:%d\n",cc);
      milib_print_error(cc);
   }

   printf("up_rtis:0x%08X",up_rtis);
   for (i=1; i<31; i++) {
      if ((1<<i) & up_rtis) {
	 printf("|");
	 count++;
      } else {
	 printf(".");
      }
   }
   printf("\n");

   printf("numbers:");
   for (i=1; i<31; i++) {
      if ((1<<i) & up_rtis)
	 printf("%02d:",i);
   }
   printf("\n");

   for (i=1; i<31; i++) {
      if ((1<<i) & up_rtis) {
	 cc = rtilib_read_signature(milf, bc, i, &sig);
	 if (cc) {
	    printf("rtiib_read_signature:Error:%d\n",cc);
	    rtilib_print_error(cc);
	 }
	 printf("Rti:%02d:Signature:0x%04hX %s",i,sig,rtilib_sig_to_str(sig));
	 if (i == rti) printf(" <===");
	 printf("\n");
      }
   }

   printf("ative  :%d Total\n",count);
   return arg;
}

/* ============================= */

#define MAX_ITEMS 32

static struct mil1553_send_s send_head = {0, NULL};
static struct mil1553_tx_item_s tx_items[MAX_ITEMS];
static int item_count = 0;

int SendPackets(int arg) {   /* Send packets */

ArgVal   *v;
AtomType  at;
int cc, num;

   arg++;

   num = 1;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      num = v->Number;
      if (num > item_count) {
	 printf("Not enough items in buffer\n");
	 return arg;
      }
   }
   send_head.item_count = num;
   send_head.tx_item_array = tx_items;
   cc = milib_send(milf, &send_head);
   if (cc) {
      printf("milib_send:Error:%d\n",cc);
      milib_print_error(cc);
   }
   return arg;
}

/* ============================= */

char *item_help =

"<CrLf>         Next item\n"
"/<Item Number> Open item for edit\n"
"?              Print this help text\n"
". q            Exit from the editor\n"
"\n"
"s<Sub address>          0..31\n"
"w<Word Count>           0..31\n"
"t<Transmit Receive bit> 0=Write 1=Read\n"
"b<Bus controler>        0..N\n"
"r<Rti number>           0..31\n"
"v<Word,Value>           0..31 Value\n"
"d                       Display words\n"
"i<Item count>           Items to send\n"
"c                       Copy from memory\n";

#define TEXTLEN 128

void edit_items(int item_num) {

static struct mil1553_tx_item_s *tx_item;
unsigned int lbc, lrti, ltxreg;
unsigned int wc, sa, tr;
unsigned short wv, *ltxbuf, *mtxbuf;

char c, *cp, *ep, txt[TEXTLEN];
int i, n, wbk;

   printf("%s\n",item_help);

   do {
      tx_item = &tx_items[item_num];
      lbc     = tx_item->bc;
      lrti    = tx_item->rti_number;
      ltxreg  = tx_item->txreg;
      ltxbuf  = tx_item->txbuf;

      milib_decode_txreg(ltxreg,&wc,&sa,&tr,NULL);

      printf("[Item:%02d] Bc:%1d Rti:%d Wc:%d Sa:%d TR:%d Icnt:%d : ",
	     item_num,lbc,lrti,wc,sa,tr,item_count);

      fflush(stdout);

      bzero((void *) txt, TEXTLEN); n = 0; c = 0; wbk = 0;
      cp = ep = txt;
      while ((c != '\n') && (n < TEXTLEN)) c = txt[n++] = (char) getchar();

      while (*cp != 0) {
	 switch (*cp++) {

	    case '\n':
	       if (wbk) {
		  milib_encode_txreg(&ltxreg,wc,sa,tr,lrti);
		  tx_item->bc = lbc;
		  tx_item->rti_number = lrti;
		  tx_item->txreg = ltxreg;
		  break;
	       }
	       item_num++;
	       if (item_num >= MAX_ITEMS) {
		  item_num = 0;
		  printf("\n");
	       }
	    break;

	    case '/':
	       item_num = strtoul(cp,&ep,0);
	       if (cp != ep) cp = ep;
	       if (item_num >= MAX_ITEMS) item_num = 0;
	       wbk = 1;
	    break;

	    case '?':
	       printf("%s\n",item_help);
	       wbk = 1;
	    break;

	    case 'q':
	    case '.': return;

	    case 's':
	       sa = strtoul(cp,&ep,0); cp = ep;
	       wbk = 1;
	    break;

	    case 'i':
	       item_count = strtoul(cp,&ep,0); cp = ep;
	       wbk = 1;
	    break;

	    case 'w':
	       wc = strtoul(cp,&ep,0); cp = ep;
	       wbk = 1;
	    break;

	    case 't':
	       tr = strtoul(cp,&ep,0); cp = ep;
	       wbk = 1;
	    break;

	    case 'b':
	       lbc = strtoul(cp,&ep,0); cp = ep;
	       wbk = 1;
	    break;

	    case 'r':
	       lrti = strtoul(cp,&ep,0); cp = ep;
	       wbk = 1;
	    break;

	    case 'v':
	       i = strtoul(cp,&ep,0); cp = ep;
	       wv = (short) strtoul(cp,&ep,0); cp = ep;
	       ltxbuf[i] = wv;

	    case 'd':
	       for (i=0; i<wc; i++) {
		  if ((i % 4) == 0) printf("\n");
		  printf("[%02d:0x%04X %05d]",i,ltxbuf[i],ltxbuf[i]);
	       }
	       printf("\n");
	       wbk = 1;
	    break;

	    case 'c':
	       mtxbuf = (unsigned short *) &mem[TXBUF];
	       for (i=0; i<TX_BUF_SIZE; i++) {
		  ltxbuf[i] = mtxbuf[i];
		  if ((i % 4) == 0) printf("\n");
		  printf("[%02d:0x%04X %05d]",i,ltxbuf[i],ltxbuf[i]);
	       }
	       printf("\n\n");
	    break;

	    default: break;
	 }
      }
   } while (1);
   return;
}

/* ============================= */

int EditPackets(int arg) {   /* Edit packets */
ArgVal   *v;
AtomType  at;
int item_num = 0;

   arg++;

   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      item_num = v->Number;
      if (item_num >= MAX_ITEMS) item_num = 0;
   }
   edit_items(item_num);
   return arg;
}

/* ============================= */                                                                                                                                 
                                                                                                                                                                    
int GetSetRti(int arg) {                                                                                                                                            
                                                                                                                                                                    
ArgVal   *v;                                                                                                                                                        
AtomType  at;                                                                                                                                                       
short trti;                                                                                                                                                         
int cc, up_rtis;                                                                                                                                                    
                                                                                                                                                                    
   trti = 0;                                                                                                                                                        
                                                                                                                                                                    
   arg++;                                                                                                                                                           
   v = &(vals[arg]);                                                                                                                                                
   at = v->Type;                                                                                                                                                    
   if (at == Numeric) {                                                                                                                                             
      arg++;                                                                                                                                                        
      trti = (short) (v->Number & 0xFFFF);                                                                                                                          
   }                                                                                                                                                                
                                                                                                                                                                    
   cc = milib_get_up_rtis(milf, bc, &up_rtis);                                                                                                                      
   if (cc) {
      printf("millib_get_up_rtis:Error:%d\n",cc);
      milib_print_error(cc);
   }
                                                                                                                                                                    
   if (trti && ((1 << trti) & up_rtis))                                                                                                                             
      rti = trti;                                                                                                                                                   
                                                                                                                                                                    
   printf("Current RTI:%02d\n",rti);                                                                                                                                
   return arg;                                                                                                                                                      
}                                                                                                                                                                   
                                                                                                                                                                    
/* ============================= */                                                                                                                                 
                                                                                                                                                                    
int EditCsr(int arg) {                                                                                                                                              
                                                                                                                                                                    
ArgVal   *v;                                                                                                                                                        
AtomType  at;                                                                                                                                                       
unsigned short tcsr, csr, str, setb, clrb, msk;
int i, cc;
                                                                                                                                                                    
   arg++;                                                                                                                                                           
                                                                                                                                                                    
   tcsr = 0;                                                                                                                                                        

   printf("\ncsr [<set> [<clear>]]\n");
   for (i=0; i<16; i++) {
      msk = 1 << i;
      if (!(i %4)) printf("\nCSR Bit:%02d ",i);
      printf("[0x%04hX %4s]",msk,rtilib_csr_to_str(msk));
   }
   printf("\n\n");
                                                                                                                                                                    
   v = &(vals[arg]);                                                                                                                                                
   at = v->Type;                                                                                                                                                    
   if (at == Numeric) {                                                                                                                                             
      setb = (short) (v->Number & 0xFFFF);
      if (setb) {
	 cc = rtilib_set_csr(milf, bc, rti, setb);
	 if (cc) {
	    printf("rtiib_set_csr:Error:%d\n",cc);
	    rtilib_print_error(cc);
	 }
      }
      arg++;                                                                                                                                                        
      v = &(vals[arg]);
      at = v->Type;
      if (at == Numeric) {
	 clrb = (short) (v->Number & 0xFFFF);
	 if (clrb) {
	    cc = rtilib_clear_csr(milf, bc, rti, clrb);
	    if (cc) {
	       printf("rtiib_clear_csr:Error:%d\n",cc);
	       rtilib_print_error(cc);
	    }
	 }
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;
      }
   }
                                                                                                                                                                    
   cc = rtilib_read_csr(milf, bc, rti, &csr, &str);
   if (cc) {
      printf("rtilib_read_csr:Error:%d\n",cc);
      rtilib_print_error(cc);
   }

   printf("Current CSR:0x%04hX %s\n", csr, rtilib_csr_to_str(csr));
   printf("Current STR:0x%04hX %s\n", str, rtilib_str_to_str(str));

   return arg;
}

/* ============================= */                                                                                                                                 
                                                                                                                                                                    
int ReadStr(int arg) {
                                                                                                                                                                    
unsigned short str;
int i, msk, cc;
                                                                                                                                                                    
   arg++;                                                                                                                                                           

   for (i=0; i<STR_RTI_SHIFT; i++) {
      msk = 1 << i;
      if (!(i %4)) printf("\nSTR Bit:%02d ",i);
      printf("[0x%04hX %4s]",msk,rtilib_str_to_str(msk));
   }
   printf("\n\n");
                                                                                                                                                                    
   cc = rtilib_read_str(milf, bc, rti, &str);
   if (cc) {
      printf("rtilib_read_str:Error:%d\n",cc);
      rtilib_print_error(cc);
   }
   printf("Current STR:0x%04hX %s\n", str, rtilib_str_to_str(str));

   cc = rtilib_read_last_str(milf, bc, rti, &str);
   if (cc) {
      printf("rtilib_read_last_str:Error:%d\n",cc);
      rtilib_print_error(cc);
   }
   printf("Last    STR:0x%04hX %s\n", str, rtilib_str_to_str(str));

   return arg;
}

/* ============================= */                                                                                                                                 
                                                                                                                                                                    
int ReadSig(int arg) {
                                                                                                                                                                    
unsigned short sig;
int cc;
                                                                                                                                                                    
   arg++;                                                                                                                                                           
                                                                                                                                                                    
   cc = rtilib_read_signature(milf, bc, rti, &sig);
   if (cc) {
      printf("rtilib_read_signature:Error:%d\n",cc);
      rtilib_print_error(cc);
   }
   printf("Signature:0x%04hX %s\n",sig,rtilib_sig_to_str(sig));

   return arg;
}

/* ============================= */                                                                                                                                 

int MasterReset(int arg) {
                                                                                                                                                                    
int cc;
                                                                                                                                                                    
   arg++;                                                                                                                                                           
                                                                                                                                                                    
   cc = rtilib_master_reset(milf, bc, rti);
   if (cc) {
      printf("rtilib_master_reset:Error:%d\n",cc);
      rtilib_print_error(cc);
   }
   printf("ResetDone\n");

   return arg;
}

/* ============================= */                                                                                                                                 

int ReadRxBuf(int arg) {
                                                                                                                                                                    
ArgVal   *v;                                                                                                                                                        
AtomType  at;                                                                                                                                                       
int i, wc, cc;
unsigned short rxbuf[RX_BUF_SIZE];
                                                                                                                                                                    
   arg++;                                                                                                                                                           

   wc = TX_BUF_SIZE -1;
   v = &(vals[arg]);                                                                                                                                                
   at = v->Type;                                                                                                                                                    
   if (at == Numeric) {                                                                                                                                             
      wc = v->Number;
      if (wc >= TX_BUF_SIZE) wc = TX_BUF_SIZE -1;
      arg++;
   }

   bzero((void *) rxbuf, RX_BUF_SIZE * sizeof(unsigned short));
                                                                                                                                                                    
   cc = rtilib_read_rxbuf(milf, bc, rti, wc, rxbuf);
   if (cc) {
      printf("rtilib_read_rxbuf:Error:%d\n",cc);
      rtilib_print_error(cc);
   }


   for (i=0; i<wc+1; i++) {
      if (!(i % 4)) printf("\nWord:%02d ",i);
      printf("0x%04hX ",rxbuf[i]);
   }
   printf("\n");
   return arg;
}

/* ============================= */                                                                                                                                 

int ReadTxBuf(int arg) {
                                                                                                                                                                    
ArgVal   *v;                                                                                                                                                        
AtomType  at;                                                                                                                                                       
int i, wc, cc;
unsigned short txbuf[TX_BUF_SIZE];
                                                                                                                                                                    
   arg++;                                                                                                                                                           

   wc = TX_BUF_SIZE -1;
   v = &(vals[arg]);                                                                                                                                                
   at = v->Type;                                                                                                                                                    
   if (at == Numeric) {                                                                                                                                             
      wc = v->Number;
      if (wc >= TX_BUF_SIZE) wc = TX_BUF_SIZE -1;
      arg++;
   }

   bzero((void *) txbuf, TX_BUF_SIZE * sizeof(unsigned short));
                                                                                                                                                                    
   cc = rtilib_read_txbuf(milf, bc, rti, wc, txbuf);
   if (cc) {
      printf("rtilib_read_txbuf:Error:%d\n",cc);
      rtilib_print_error(cc);
   }

   for (i=0; i<wc+1; i++) {
      if (!(i % 4)) printf("\nWord:%02d ",i);
      printf("0x%04hX ",txbuf[i]);
   }
   printf("\n");
   return arg;
}

/* ============================= */                                                                                                                                 

int WriteRxBuf(int arg) {
                                                                                                                                                                    
ArgVal   *v;                                                                                                                                                        
AtomType  at;                                                                                                                                                       
int wc, cc;
unsigned short *rxbuf;
                                                                                                                                                                    
   arg++;                                                                                                                                                           

   wc = TX_BUF_SIZE -1;
   v = &(vals[arg]);                                                                                                                                                
   at = v->Type;                                                                                                                                                    
   if (at == Numeric) {                                                                                                                                             
      wc = v->Number;
      if (wc >= TX_BUF_SIZE) wc = TX_BUF_SIZE -1;
      arg++;
   }

   rxbuf = (unsigned short *) &mem[RXBUF];

   cc = rtilib_write_rxbuf(milf, bc, rti, wc, (unsigned short *) &rxbuf[0]);
   if (cc) {
      printf("rtilib_write_rxbuf:Error:%d\n",cc);
      rtilib_print_error(cc);
   }
   printf("WriteRxBuf done\n");

   return arg;
}

/* ============================= */                                                                                                                                 

int WriteTxBuf(int arg) {
                                                                                                                                                                    
ArgVal   *v;                                                                                                                                                        
AtomType  at;                                                                                                                                                       
int wc, cc;
unsigned short *txbuf;

   arg++;                                                                                                                                                           

   wc = TX_BUF_SIZE -1;
   v = &(vals[arg]);                                                                                                                                                
   at = v->Type;                                                                                                                                                    
   if (at == Numeric) {                                                                                                                                             
      wc = v->Number;
      if (wc >= TX_BUF_SIZE) wc = TX_BUF_SIZE -1;
      arg++;
   }

   txbuf = (unsigned short *) &mem[TXBUF];
                                                                                                                                                                    
   cc = rtilib_write_txbuf(milf, bc, rti, wc, (unsigned short *) &txbuf[0]);
   if (cc) {
      printf("rtilib_write_txbuf:Error:%d\n",cc);
      rtilib_print_error(cc);
   }
   printf("WriteTxBufDone\n");
   return arg;
}

/* ============================= */                                                                                                                                 
/* Import code from mg64t.c to deal with power supplies */

int read_cfg_msg(int arg) {

struct quick_data_buffer send_buf = { 0 },
			 *quickptr_req = &send_buf,
			 receive_buf = { 0 },
			 *quickptr_ctl = &receive_buf;
req_msg  *req_ptr;
short mbno = 1;
int cc;
struct timeval cur_time = { 0 };

   arg++;

   quickptr_req->bc     = bc;
   quickptr_req->rt     = rti;
   quickptr_req->stamp  = 0;
   quickptr_req->error  = 0;
   quickptr_req->pktcnt = sizeof(req_msg);
   quickptr_req->next   = NULL;

   /* Initialize request message */

   req_ptr = (req_msg*) &(quickptr_req->pkt);
   req_ptr->family     = POW_FAM;
   req_ptr->type       = TYPE;
   req_ptr->sub_family = SUB_FAMILY;
   req_ptr->member     = mbno;
   req_ptr->service    = RS_CONF;
   req_ptr->cycle.machine  = 0;
   req_ptr->cycle.pls_line = 0;
   req_ptr->specialist     = 0;

   /* will also set current time in RTI */

   gettimeofday(&cur_time, NULL);
   req_ptr->protocol_date.sec  = (int)cur_time.tv_sec;
   req_ptr->protocol_date.usec = (int)cur_time.tv_usec;

   /* Send request message to G64 */

   cc = mil1553_send_quick_data(milf,quickptr_req);
   if (cc) {
      printf("mil1553_send_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   quickptr_ctl->bc   = bc;
   quickptr_ctl->rt   = rti;
   quickptr_ctl->next = NULL;

   /* expected answer: we'll wait for packet 48 bytes long (24 words) */

   quickptr_ctl->pktcnt = sizeof(conf_msg);

   /* Wait for the reply message */

   cc = mil1553_get_quick_data(milf,quickptr_ctl);
   if (cc) {
      printf("mil1553_get_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   mil1553_print_msg(quickptr_ctl,1,RS_CONF);

   return arg;
}

/* ============================= */                                                                                                                                 

int read_acq_msg(int arg) {

struct quick_data_buffer send_buf = { 0 },
			 *quickptr_req = &send_buf,
			 receive_buf = { 0 },
			 *quickptr_ctl = &receive_buf;
req_msg *req_ptr;
short mbno = 1;
int cc;
struct timeval cur_time = { 0 };

   arg++;

   quickptr_req->bc     = bc;
   quickptr_req->rt     = rti;
   quickptr_req->stamp  = 0;
   quickptr_req->error  = 0;
   quickptr_req->pktcnt = sizeof(req_msg);
   quickptr_req->next   = NULL;

   /* Initialize request message */

   req_ptr = (req_msg*) &(quickptr_req->pkt);
   req_ptr->family         = POW_FAM;
   req_ptr->type           = TYPE;
   req_ptr->sub_family     = SUB_FAMILY;
   req_ptr->member         = mbno;
   req_ptr->service        = RS_REF;
   req_ptr->cycle.machine  = 0;
   req_ptr->cycle.pls_line = 0;
   req_ptr->specialist     = 0;

   /* will also set current time in RTI */

   gettimeofday(&cur_time, NULL);
   req_ptr->protocol_date.sec  = (int)cur_time.tv_sec;
   req_ptr->protocol_date.usec = (int)cur_time.tv_usec;

   /* Send request message to G64 */

   cc = mil1553_send_quick_data(milf,quickptr_req);
   if (cc) {
      printf("mil1553_send_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   quickptr_ctl->bc = bc;
   quickptr_ctl->rt = rti;
   quickptr_ctl->next = NULL;

   /* expected answer: we'll wait for packet 44 bytes long (22 words) */

   quickptr_ctl->pktcnt = sizeof(acq_msg);

   cc = mil1553_get_quick_data(milf,quickptr_ctl);
   if (cc) {
      printf("mil1553_get_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
      printf("Missing trigger timing, no acquisition data available\n");
      return arg;
   }

   mil1553_print_msg(quickptr_ctl,1,RS_REF);

   return arg;
}

/* ============================= */                                                                                                                                 

int EditCCV(ctrl_msg *ctrl_ptr) {

#define VALUES 5
#define ACVALS 4

   unsigned char ccsact;
   char c, *cp, str[128];
   float ccv, ccv1, ccv2, ccv3;
   int res, n, p, np;
   char *vnms[VALUES] = { "ccsact", "ccv", "ccv1", "ccv2", "ccv3" };
   char *anms[ACVALS] = { "Off","StandBy","On","Reset" };
   float *ptrs[VALUES -1];

   res = 0;

   ccsact = ctrl_ptr->ccsact;
   ccv    = ctrl_ptr->ccv;
   ccv1   = ctrl_ptr->ccv1;
   ccv2   = ctrl_ptr->ccv2;
   ccv3   = ctrl_ptr->ccv3;

   ptrs[0] = &ctrl_ptr->ccv;
   ptrs[1] = &ctrl_ptr->ccv1;
   ptrs[2] = &ctrl_ptr->ccv2;
   ptrs[3] = &ctrl_ptr->ccv3;

   printf("EditCtrl: [?]Help [/]Open [CR]Next [.][q]Exit \n");
   printf("ccsact:0=Off, 1=Standby, 2=On, 3=Reset\n");

   p = 0;
   c = '\n';

   while (1) {

      printf("%02d:%6s:",p,vnms[p]);
      if (p) printf("%f:",*ptrs[p-1]);
      else   printf("%02d:%6s:",ctrl_ptr->ccsact,anms[(int) ctrl_ptr->ccsact]);

      fflush(stdout);
      c = (char) getchar();

      if (c == '/') {
	 bzero((void *) str, 128); n = 0;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 np = strtoul(str,&cp,0);
	 if (cp != str) p = np;
	 if (p >= VALUES) p = 0;
      }

      else if (c == '.')  { c = getchar(); printf("\n"); break; }
      else if (c == 'q')  { c = getchar(); printf("\n"); break; }
      else if (c == '\n') { p++; if (p >= VALUES) { p = 0; printf("\n----\n"); } }
      else if (c == '?')  { printf("[?]Help [/]Open [CR]Next [.][q]Exit\n");
			    printf("ccsact:0=Off, 1=Standby, 2=On, 3=Reset\n"); }
      else {
	 bzero((void *) str, 128); n = 0;
	 str[n++] = c;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 if (cp != str) {
	    if (p) *ptrs[p-1] = strtof(str,&cp);
	    else   ctrl_ptr->ccsact = strtoul(str,&cp,0);
	 }
      }
   }

   if (ccsact != ctrl_ptr->ccsact) { ctrl_ptr->ccsact_change =1; res=1; } else ctrl_ptr->ccsact_change =0;
   if (ccv    != ctrl_ptr->ccv   ) { ctrl_ptr->ccv_change    =1; res=1; } else ctrl_ptr->ccv_change    =0;
   if (ccv1   != ctrl_ptr->ccv1  ) { ctrl_ptr->ccv1_change   =1; res=1; } else ctrl_ptr->ccv1_change   =0;
   if (ccv2   != ctrl_ptr->ccv2  ) { ctrl_ptr->ccv2_change   =1; res=1; } else ctrl_ptr->ccv2_change   =0;
   if (ccv3   != ctrl_ptr->ccv3  ) { ctrl_ptr->ccv3_change   =1; res=1; } else ctrl_ptr->ccv3_change   =0;

   return res;
}

/* ============================= */                                                                                                                                 

int write_ctl_msg(int arg) {

  struct quick_data_buffer send_buf = { 0 },
			   *quickptr_req = &send_buf,
			   receive_buf = { 0 },
			   *quickptr_ctl = &receive_buf;
   req_msg *req_ptr;
   ctrl_msg *ctrl_ptr;
   short mbno = 1;
   int cc;

   struct timeval cur_time = { 0 };

   arg++;

   quickptr_req->bc     = bc;
   quickptr_req->rt     = rti;
   quickptr_req->stamp  = 0;
   quickptr_req->error  = 0;
   quickptr_req->pktcnt = sizeof(req_msg);
   quickptr_req->next   = NULL;

   /* Initialize request message */

   req_ptr                 = (req_msg *) &quickptr_req->pkt;
   req_ptr->family         = POW_FAM;
   req_ptr->type           = TYPE;
   req_ptr->sub_family     = SUB_FAMILY;
   req_ptr->member         = mbno;
   req_ptr->service        = RS_ECHO;
   req_ptr->cycle.machine  = 0;
   req_ptr->cycle.pls_line = 0;
   req_ptr->specialist     = 0;

   /* Send request message to G64 */

   cc = mil1553_send_quick_data(milf,quickptr_req);
   if (cc) {
      printf("mil1553_send_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   quickptr_ctl->bc     = bc;
   quickptr_ctl->rt     = rti;
   quickptr_ctl->next   = NULL;
   quickptr_ctl->pktcnt = sizeof(ctrl_msg);

   cc = mil1553_get_quick_data(milf,quickptr_ctl);
   if (cc) {
      printf("mil1553_get_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   /* reuse received data */

   ctrl_ptr = (ctrl_msg *) &(quickptr_ctl->pkt);

   /* check if wanted service was delivered */

   if (ctrl_ptr->service != RS_ECHO)
      printf("Send Quickdata error. "
	     "Expected service %d (RS_ECHO) received %d\n",
	     RS_ECHO, ctrl_ptr->service);

   if (!EditCCV(ctrl_ptr)) printf("No changes - Sending anyway\n");

   ctrl_ptr->service = RS_REF;

   /* send current time */

   gettimeofday(&cur_time, NULL); /* get current time */
   printf("Send current time %s", ctime(&cur_time.tv_sec));
   ctrl_ptr->protocol_date.sec  = (int)cur_time.tv_sec;
   ctrl_ptr->protocol_date.usec = (int)cur_time.tv_usec;

   /* Send ctrl_msg to G64 */

   cc = mil1553_send_quick_data(milf,quickptr_ctl);
   if (cc) {
      printf("mil1553_send_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   return arg;
}

/* ============================= */                                                                                                                                 

int read_ctl_msg(int arg) {

   struct quick_data_buffer send_buf = { 0 },
			    *quickptr_req = &send_buf,
			    receive_buf = { 0 },
			    *quickptr_ctl = &receive_buf;
   req_msg *req_ptr;
   short mbno = 1;
   int cc;

   arg++;

   quickptr_req->bc     = bc;
   quickptr_req->rt     = rti;
   quickptr_req->stamp  = 0;
   quickptr_req->error  = 0;
   quickptr_req->pktcnt = sizeof(req_msg);
   quickptr_req->next   = NULL;

   /* Initialize request message */

   req_ptr               = (req_msg*) &(quickptr_req->pkt);
   req_ptr->family       = POW_FAM;
   req_ptr->type         = TYPE;
   req_ptr->sub_family   = SUB_FAMILY;
   req_ptr->member       = mbno;
   req_ptr->service      = RS_ECHO;
   req_ptr->cycle.machine  = 0;
   req_ptr->cycle.pls_line = 0;
   req_ptr->specialist     = 0;

   cc = mil1553_send_quick_data(milf,quickptr_req);
   if (cc) {
      printf("mil1553_send_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   quickptr_ctl->bc   = bc;
   quickptr_ctl->rt   = rti;
   quickptr_ctl->next = NULL;

   quickptr_ctl->pktcnt = sizeof(ctrl_msg);
   cc = mil1553_get_quick_data(milf,quickptr_ctl);
   if (cc) {
      printf("mil1553_get_quick_data:Error:%d\n",cc);
      mil1553_print_error(cc);
   }

   mil1553_print_msg(quickptr_ctl,0,RS_ECHO);

   return arg;
}
