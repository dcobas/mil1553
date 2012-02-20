/**************************************************************************/
/* Command line stuff                                                     */
/**************************************************************************/

#ifndef CMDS_H
#define CMDS_H

int Illegal();              /* llegal command */

int Quit();                 /* Quit test program  */
int Help();                 /* Help on commands   */
int News();                 /* Show GMT test news */
int History();              /* History            */
int Shell();                /* Shell command      */
int Sleep();                /* Sleep seconds      */
int Pause();                /* Pause keyboard     */
int Atoms();                /* Atom list commands */

int ChangeEditor();
int ChangeDirectory();
int Batch();

int EditDebug();     /* Edit LUN debug level */
int GetVersion();    /* Get driver version */
int ReadStatus();    /* Read status */
int GetSetBC();      /* Select bus controler module */
int GetBcInfo();     /* Get BC info */
int EditTimeout();   /* Edit timeout */
int WaitInterrupt(); /* Wait interrupt */
int RawRead();       /* Read from device to memory */
int RawWrite();      /* Write from memory to device */
int EditMem();       /* Edit memory */
int EditRegs();      /* Edit registers */
int GetUpRtis();     /* Get up RTIs */
int SendPackets();   /* Send packets */
int EditPackets();   /* Edit packets */
int ResetBc();       /* Reset bus controller */

int GetSetRti();     /* Get set current RTI */
int EditCsr();       /* Edit RTI csr */
int ReadStr();       /* Read RTI Status and last status */

int ReadSig();       /* Read RTI signature */
int MasterReset();   /* Master reset RTI */
int ReadRxBuf();     /* Read RTI rxbuf */
int ReadTxBuf();     /* Read RTI txbuf */
int WriteRxBuf();    /* Write RTI rxbuf from memory */
int WriteTxBuf();    /* Write RTI txbuf from memory */
int GetSetSpeed();   /* Select bus controler speed */

int GetQueueSize();  /* Get the client queue size */

int read_cfg_msg();  /* Read power supply config */
int read_acq_msg();  /* Read acquisition */
int read_ctl_msg();  /* Read pow control settings */
int write_ctl_msg(); /* Write pow control settings */
int edit_ctl_msg();  /* Edit pow control settings */

int test_rti();      /* RTI test */

int SetPolling();    /* Set RTI polling On/Off */

/* Jtag backend to public code */

FILE *inp;

typedef enum {

   CmdNOCM,    /* llegal command */

   CmdQUIT,    /* Quit test program  */
   CmdHELP,    /* Help on commands   */
   CmdNEWS,    /* Show GMT test news */
   CmdHIST,    /* History            */
   CmdSHELL,   /* Shell command      */
   CmdSLEEP,   /* Sleep seconds      */
   CmdPAUSE,   /* Pause keyboard     */
   CmdATOMS,   /* Atom list commands */

   CmdCHNGE,   /* Change editor to next program */
   CmdCD,      /* Change working directory */
   CmdBATCH,   /* Set batch mode on off */

   CmdDEBUG,   /* Edit LUN debug level */
   CmdDRVER,   /* Get driver version */
   CmdRST,     /* Read status */
   CmdBC,      /* Select bus controler module */
   CmdINFO,    /* Get BC info */
   CmdTMOUT,   /* Edit timeout */
   CmdWAIT,    /* Wait interrupt */
   CmdRMEM,    /* Read from device to memory */
   CmdWMEM,    /* Write from memory to device */
   CmdEMEM,    /* Edit memory */
   CmdEREG,    /* Edit registers */
   CmdRTIS,    /* Get up RTIs */
   CmdSPK,     /* Send packets */
   CmdEPK,     /* Edit packets */
   CmdRESET,   /* Reset bus controller */

   CmdRTI,     /* Get set current RTI */
   CmdCSR,     /* Edit CSR register */
   CmdSTR,     /* Read current and last status */
   CmdSIG,     /* Read RTI signature */
   CmdMRS,     /* RTI Master reset */
   CmdRRX,     /* RTI read rxbuf */
   CmdRTX,     /* RTI read txbuf */
   CmdWRX,     /* RTI write rxbuf */
   CmdWTX,     /* RTI write txbuf */
   CmdSPEED,   /* Get set the bus speed */
   CmdPOLL,    /* Set RTI polling */

   CmdQSZE,    /* Get clients queue size */

   CmdRCNF,    /* Read power supply config */
   CMDRACQ,    /* Read power supply acquisition */
   CmdRCTL,    /* Read power control values */
   CmdWCTL,    /* Write power control values */
   CmdECTL,    /* Edit power control values */

   CmdRTIT,    /* Perform RTI test */

   CmdCMDS } CmdId;

typedef struct {
   CmdId  Id;
   char  *Name;
   char  *Help;
   char  *Optns;
   int  (*Proc)(); } Cmd;

static Cmd cmds[CmdCMDS] = {

   { CmdNOCM,   "???",    "Illegal command"          ,""                   ,Illegal },

   { CmdQUIT,    "q" ,    "Quit test program"        ,""                   ,Quit  },
   { CmdHELP,    "h" ,    "Help on commands"         ,""                   ,Help  },
   { CmdNEWS,    "news",  "Show GMT test news"       ,""                   ,News  },
   { CmdHIST,    "his",   "History"                  ,""                   ,History},
   { CmdSHELL,   "sh",    "Shell command"            ,"UnixCmd"            ,Shell },
   { CmdSLEEP,   "s" ,    "Sleep seconds"            ,"Seconds"            ,Sleep },
   { CmdPAUSE,   "z" ,    "Pause keyboard"           ,""                   ,Pause },
   { CmdATOMS,   "a" ,    "Atom list commands"       ,""                   ,Atoms },

   { CmdCHNGE,   "ce",    "Change text editor used"  ,""                   ,ChangeEditor },
   { CmdCD,      "cd",    "Change working directory" ,""                   ,ChangeDirectory },
   { CmdBATCH,   "bat",   "Set batch mode on off"    ,"1|0"                ,Batch   },

   { CmdDEBUG,   "deb",   "Edit LUN debug level"     ,"level"              ,EditDebug         },
   { CmdDRVER,   "dvr",   "Get driver version"       ,""                   ,GetVersion        },
   { CmdRST,     "rst",   "Read status"              ,""                   ,ReadStatus        },
   { CmdBC,      "bc",    "Select bus controler"     ,"bc"                 ,GetSetBC          },
   { CmdINFO,    "binf",  "Get BC info"              ,""                   ,GetBcInfo         },
   { CmdTMOUT,   "tmo",   "Edit timeout"             ,"milliseconds"       ,EditTimeout       },
   { CmdWAIT,    "wi",    "Wait interrupt"           ,""                   ,WaitInterrupt     },
   { CmdRMEM,    "rmem",  "Read from dev to memory"  ,"start,count"        ,RawRead           },
   { CmdWMEM,    "wmem",  "Write from memory to dev" ,"start,count"        ,RawWrite          },
   { CmdEMEM,    "emem",  "Set Edit memory"          ,"start,[count],value",EditMem           },
   { CmdEREG,    "ereg",  "Edit registers"           ,"reg|name,val"       ,EditRegs          },
   { CmdRTIS,    "rtis",  "Get up RTIs"              ,"bc(resets)"         ,GetUpRtis         },
   { CmdSPK,     "spk",   "Send packets"             ,"start,count"        ,SendPackets       },
   { CmdEPK,     "epk",   "Edit packets"             ,"start,count"        ,EditPackets       },
   { CmdRESET,   "reset", "Reset bus controller"     ,""                   ,ResetBc           },

   { CmdRTI,     "rti",   "Get set current RTI"      ,"rti number"         ,GetSetRti         },
   { CmdCSR,     "csr",   "Edit CSR register"        ,"set,clear bits"     ,EditCsr           },
   { CmdSTR,     "str",   "Read current last status" ,""                   ,ReadStr           },
   { CmdSIG,     "sig",   "Read RTI signature"       ,""                   ,ReadSig           },
   { CmdMRS,     "mrs",   "RTI Master reset"         ,"1=All"              ,MasterReset       },
   { CmdRRX,     "rrx",   "RTI read rxbuf"           ,"wc"                 ,ReadRxBuf         },
   { CmdRTX,     "rtx",   "RTI read txbuf (receive)" ,"wc"                 ,ReadTxBuf         },
   { CmdWRX,     "wrx",   "RTI write rxbuf (send)"   ,"wc"                 ,WriteRxBuf        },
   { CmdWTX,     "wtx",   "RTI write txbuf"          ,"wc"                 ,WriteTxBuf        },
   { CmdSPEED,   "bspd",  "Get set BC bus speed"     ,"0..3"               ,GetSetSpeed       },
   { CmdPOLL,    "nopol", "Set RTI no-polling flag"  ,"0..1"               ,SetPolling        },

   { CmdQSZE,    "qsz",   "Get Queue Size"           ,""                   ,GetQueueSize      },

   { CmdRCNF,    "rcnf",  "POW read config"          ,""                   ,read_cfg_msg      },
   { CMDRACQ,    "racq",  "POW read acquisition"     ,""                   ,read_acq_msg      },
   { CmdRCTL,    "rctl",  "POW read control values"  ,"0|1 new/old"        ,read_ctl_msg      },
   { CmdWCTL,    "wctl",  "POW write control values" ,""                   ,write_ctl_msg     },
   { CmdECTL,    "ectl",  "POW edit control values"  ,""                   ,edit_ctl_msg      },

   { CmdRTIT,    "trti",  "Perform RTI test"         ,""                   ,test_rti          }

   };

typedef enum {

   OprNOOP,

   OprNE,  OprEQ,  OprGT,  OprGE,  OprLT , OprLE,   OprAS,
   OprPL,  OprMI,  OprTI,  OprDI,  OprAND, OprOR,   OprXOR,
   OprNOT, OprNEG, OprLSH, OprRSH, OprINC, OprDECR, OprPOP,
   OprSTM,

   OprOPRS } OprId;

typedef struct {
   OprId  Id;
   char  *Name;
   char  *Help; } Opr;

static Opr oprs[OprOPRS] = {
   { OprNOOP, "?"  ,"???     Not an operator"       },
   { OprNE,   "#"  ,"Test:   Not equal"             },
   { OprEQ,   "="  ,"Test:   Equal"                 },
   { OprGT,   ">"  ,"Test:   Greater than"          },
   { OprGE,   ">=" ,"Test:   Greater than or equal" },
   { OprLT,   "<"  ,"Test:   Less than"             },
   { OprLE,   "<=" ,"Test:   Less than or equal"    },
   { OprAS,   ":=" ,"Assign: Becomes equal"         },
   { OprPL,   "+"  ,"Arith:  Add"                   },
   { OprMI,   "-"  ,"Arith:  Subtract"              },
   { OprTI,   "*"  ,"Arith:  Multiply"              },
   { OprDI,   "/"  ,"Arith:  Divide"                },
   { OprAND,  "&"  ,"Bits:   AND"                   },
   { OprOR,   "!"  ,"Bits:   OR"                    },
   { OprXOR,  "!!" ,"Bits:   XOR"                   },
   { OprNOT,  "##" ,"Bits:   Ones Compliment"       },
   { OprNEG,  "#-" ,"Arith:  Twos compliment"       },
   { OprLSH,  "<<" ,"Bits:   Left shift"            },
   { OprRSH,  ">>" ,"Bits:   Right shift"           },
   { OprINC,  "++" ,"Arith:  Increment"             },
   { OprDECR, "--" ,"Arith:  Decrement"             },
   { OprPOP,  ";"  ,"Stack:  POP"                   },
   { OprSTM,  "->" ,"Stack:  PUSH"                  } };

static char atomhash[256] = {
  10,9,9,9,9,9,9,9,9,0,0,9,9,0,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  0 ,1,9,1,9,4,1,9,2,3,1,1,0,1,11,1,5,5,5,5,5,5,5,5,5,5,1,1,1,1,1,1,
  10,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,7,9,8,9,6,
  9 ,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 };

typedef enum {
   Seperator=0,Operator=1,Open=2,Close=3,Comment=4,Numeric=5,Alpha=6,
   Open_index=7,Close_index=8,Illegal_char=9,Terminator=10,Bit=11,
 } AtomType;

#define MAX_ARG_LENGTH  32
#define MAX_ARG_COUNT   16
#define MAX_ARG_HISTORY 16

typedef struct {
   int      Pos;
   int      Number;
   AtomType Type;
   char     Text[MAX_ARG_LENGTH];
   CmdId    CId;
   OprId    OId;
} ArgVal;

static int pcnt = 0;
static ArgVal val_bufs[MAX_ARG_HISTORY][MAX_ARG_COUNT];
static ArgVal *vals = val_bufs[0];

#ifndef True
#define True 1
#define False 0
#endif

#endif
