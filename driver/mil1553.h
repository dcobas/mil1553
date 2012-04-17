/**
 * Julian Lewis March 28 2012 BE/CO/HT
 * Julian.Lewis@cern.ch
 *
 * MIL 1553 bus controler CBMIA module library include file.
 *
 * This code relies on a new firmware version number 204 and later
 * In this version proper access to the TXREG uses a busy done bit.
 * Software polling has been implemented, hardware polling is removed.
 * The bus speed is fixed at 1Mbit.
 * Hardware test points and diagnostic/debug registers are added.
 */

#ifndef MIL1553
#define MIL1553

#define INTERRUPT 0
#define INTEREN 1
#define RTIPRESENT 2
#define STATUS 3
#define COMMAND 4
#define SNUMU 6
#define SNUML 7
#define TXREG 8
#define RXREG 9
#define RXBUF 10
#define TXBUF 27

#define TX_BUF_SIZE 32
#define RX_BUF_SIZE (TX_BUF_SIZE +1)

#define TX_START    0x01
#define TX_END      0x02
#define TX_ALL      0x04

#define MAX_REGS 53

/**
 * Useful masks and shifts for working on registers
 */

#define ISRC_RTI_SHIFT 27
#define ISRC_RTI_MASK  (0x1F << ISRC_RTI_SHIFT)
#define ISRC_WC_SHIFT  21
#define ISRC_WC_MASK   (0x3F << ISRC_WC_SHIFT)

#define ISRC_END_TRANSACTION  0x000001
#define ISRC_RECEPTION_ERROR  0x008000
#define ISRC_TIME_OUT         0x010000
#define ISRC_BAD_WC           0x020000
#define ISRC_MANCHESTER_ERROR 0x040000
#define ISRC_PARITY_ERROR     0x080000
#define ISRC_TR_BIT           0x100000

#define ISRC_BAD_BITS  (ISRC_TIME_OUT | ISRC_BAD_WC | ISRC_MANCHESTER_ERROR | \
				ISRC_PARITY_ERROR | ISRC_RECEPTION_ERROR )
#define ISRC_GOOD_BITS (ISRC_END_TRANSACTION)

#define ISRC_IRQ       1
#define ISRC           (ISRC_IRQ)

#define INTEN_INF      1
#define INTEN          (INTEN_INF)

#define HSTAT_VER_SHIFT    0
#define HSTAT_VER_MASK     (0xFFFF << HSTAT_VER_SHIFT)
#define HSTAT_STAT_SHIFT   16
#define HSTAT_STAT_MASK    (0xFFFF << HSTAT_STAT_SHIFT)

#define HSTAT_BUSY_SHIFT   31
#define HSTAT_BUSY_MASK    (1 << HSTAT_BUSY_SHIFT)
#define HSTAT_BUSY         1
#define HSTAT_BUSY_BIT     0x80000000

#define CMD_RESET_SHIFT    0
#define CMD_RESET_MASK     (1 << CMD_RESET_SHIFT)
#define CMD_RESET          1

#define CMD_TP0_SHIFT      16
#define CMD_TP0_MASK       (0xF << CMD_TP0_SHIFT)
#define CMD_TP1_SHIFT      20
#define CMD_TP1_MASK       (0xF << CMD_TP1_SHIFT)
#define CMD_TP2_SHIFT      24
#define CMD_TP2_MASK       (0xF << CMD_TP2_SHIFT)
#define CMD_TP3_SHIFT      28
#define CMD_TP3_MASK       (0xF << CMD_TP3_SHIFT)
#define CMD_TPS_MASK       (0xFFFF0000)

#define CMD_TP_TRANSACTION_IN_PROGRESS 0
#define CMD_TP_TX_ENABLE               1
#define CMD_TP_RX_IN_PROGRESS          2
#define CMD_TP_RXD                     3
#define CMD_TP_TX_DONE                 4
#define CMD_TP_RX_DONE                 5
#define CMD_TP_MANCHESTER_ERROR        6
#define CMD_TP_PARITY_ERROR            7
#define CMD_TP_WC_ERROR                8
#define CMD_TP_TIMEOUT                 9
#define CMD_TP_TX_CLASH                10
#define CMD_TP_SEND_FRAME              11
#define CMD_TP_SEND_FRAME_REQUEST      12
#define CMD_TP_TXD                     13
#define CMD_TP_TRANSACTION_END         14
#define CMD_TP_RX_ERROR                15

#define TXREG_WC_SHIFT   0
#define TXREG_WC_MASK    (0x1F << TXREG_WC_SHIFT)
#define TXREG_SUBA_SHIFT 5
#define TXREG_SUBA_MASK  (0x1F << TXREG_SUBA_SHIFT)
#define TXREG_TR_SHIFT   10
#define TXREG_TR_MASK    (1 << TXREG_TR_SHIFT)
#define TXREG_RTI_SHIFT  11
#define TXREG_RTI_MASK   (0x1F << TXREG_RTI_SHIFT)

#define NB_WD_TX_SHIFT  0
#define NB_WD_TX_MASK   (0x3F << NB_WD_TX_SHIFT)
#define NB_WD_RX_SHIFT  6
#define NB_WD_RX_MASK   (0x3F << NB_WD_RX_SHIFT)
#define NB_WD_EXP_SHIFT 12
#define NB_WD_EXP_MASK  (0x3F << NB_WD_EXP_SHIFT)

#define NB_WD_RX_ERROR         0x04000000
#define NB_WD_TIME_OUT         0x08000000
#define NB_WD_WC_DIFFER        0x10000000
#define NB_WD_MANCHESTER_ERROR 0x20000000
#define NB_WD_PARITY_ERROR     0x40000000
#define NB_WD_TR_FLAG          0x80000000

/**
 * Beware, on a 64-bit machine the size of these structures will change.
 * This issue must be addressed in the future when the driver is ported
 * on to future 64-bit machines.
 */

/*
 * Parameter for raw IO
 */

struct mil1553_riob_s {
	unsigned int   bc;        /** Thats which bus controller */
	unsigned long  reg_num;   /** Register number */
	void          *buffer;    /** Pointer to data area */
	unsigned int   regs;      /** Register count */
};

struct mil1553_tx_item_s {
	unsigned int   bc;                    /** Bus controller number */
	unsigned int   rti_number;            /** RTI number */
	unsigned int   txreg;                 /** Transmit register wc, sa, t/r bit, rti */
	unsigned int   no_reply;              /** Don't send back a reply to client */
	unsigned short txbuf[TX_BUF_SIZE];    /** Buffer */
};

struct mil1553_send_s {
	unsigned int item_count;
	struct mil1553_tx_item_s *tx_item_array;
};

struct mil1553_rti_interrupt_s {
	unsigned int bc;                      /** Bus controller */
	unsigned int rti_number;              /** Rti that interrupted */
	unsigned int wc;                      /** Buffer word count */
	unsigned short rxbuf[RX_BUF_SIZE+1];  /** Receive buffer (32-bit access) */
};

struct mil1553_recv_s {
	unsigned int pk_type;                 /** Type to wait for START, END, ALL */
	unsigned int timeout;                 /** Timeout msec or zero */
	unsigned int icnt;                    /** Clients interrupt count */
	struct mil1553_rti_interrupt_s interrupt;
};

struct mil1553_dev_info_s {
	unsigned int bc;                      /** The BC you want to get info about */
	unsigned int pci_bus_num;             /** PCI bus number */
	unsigned int pci_slt_num;             /** PCI slot number */
	unsigned int snum_h;                  /** High 32 bits of serial number */
	unsigned int snum_l;                  /** Low  32 bits of serial number */
	unsigned int hardware_ver_num;        /** Hardware version number */
	unsigned int temperature;             /** Temperature */
	unsigned int icnt;                    /** Total interrupt count */
	unsigned int isrdebug;                /** Debug information from isr */
	unsigned int tx_frames;               /** Number of tx frames */
	unsigned int rx_frames;               /** Number of rx frames */
	unsigned int rx_errors;               /** Rx errors counter value */
	unsigned int timeouts;                /** Timeouts counter */
	unsigned int parity_errors;           /** Number of parity errors */
	unsigned int manchester_errors;       /** Number of manchester code errors */
	unsigned int wc_errors;               /** Number of word count errors */
	unsigned int tx_clash_errors;         /** Number of tx clash errors */
	unsigned int nb_wds;                  /** Word count expected/received + tr bit */
	unsigned int rti_timeouts;            /** Target RTI didn't respond counter */
	unsigned int tx_count;	              /** Number of tx initiated */
};

/*
 * Enumerate IOCTL functions
 */

#define PRANDOM 0

typedef enum {

	mil1553FIRST = PRANDOM,

	mil1553GET_DEBUG_LEVEL,   /** Get the debug level 0..7 */
	mil1553SET_DEBUG_LEVEL,   /** Set the debug level 0..7 */

	mil1553GET_TIMEOUT_MSEC,  /** Get the client timeout in milliseconds */
	mil1553SET_TIMEOUT_MSEC,  /** Set the client timeout in milliseconds */

	mil1553GET_DRV_VERSION,   /** Get the UTC driver compilation data */

	mil1553GET_STATUS,        /** Reads the status register */
	mil1553GET_TEMPERATURE,   /** Get the temperature */

	mil1553GET_BCS_COUNT,     /** Get the Bus Controllers count */
	mil1553GET_BC_INFO,       /** Get information aboult a Bus Controller */

	mil1553RAW_READ,          /** Raw read PCI registers */
	mil1553RAW_WRITE,         /** Raw write PCI registers */

	mil1553GET_UP_RTIS,       /** Get the up RTIs mask for a given BC */
	mil1553SEND,              /** Send data to RTIs */
	mil1553RECV,              /** Wait for and read results back from RTIs */

	mil1553LOCK_BC,           /** Lock a BC for transaction by a client */
	mil1553UNLOCK_BC,         /** Unlock a BC after transaction complete */

	mil1553QUEUE_SIZE,        /** Returns the number of items on clients queue */
	mil1553RESET,             /** Resets a bus controller */

	mil1553SET_POLLING,       /** Set software polling */
	mil1553GET_POLLING,       /** Get software polling */

	mil1553SET_TP,            /** Set up test points */
	mil1553GET_TP,            /** Get test points */

	mil1553LAST               /** For range checking (LAST - FIRST) */

} mil1553_ioctl_function_t;

/*
 * Set up the IOCTL numbers
 */

#define MAGIC 'P'

#define PIO(nr)      _IO(MAGIC,nr)
#define PIOR(nr,sz)  _IOR(MAGIC,nr,sz)
#define PIOW(nr,sz)  _IOW(MAGIC,nr,sz)
#define PIOWR(nr,sz) _IOWR(MAGIC,nr,sz)

#define MIL1553_GET_DEBUG_LEVEL  PIOR(mil1553GET_DEBUG_LEVEL,  unsigned long)
#define MIL1553_SET_DEBUG_LEVEL  PIOW(mil1553SET_DEBUG_LEVEL,  unsigned long)
#define MIL1553_GET_TIMEOUT_MSEC PIOR(mil1553GET_TIMEOUT_MSEC, unsigned long)
#define MIL1553_SET_TIMEOUT_MSEC PIOW(mil1553SET_TIMEOUT_MSEC, unsigned long)
#define MIL1553_GET_DRV_VERSION  PIOR(mil1553GET_DRV_VERSION,  unsigned long)
#define MIL1553_GET_STATUS       PIOWR(mil1553GET_STATUS,      unsigned long)
#define MIL1553_GET_TEMPERATURE  PIOWR(mil1553GET_TEMPERATURE, unsigned long)
#define MIL1553_GET_BCS_COUNT    PIOR(mil1553GET_BCS_COUNT,    unsigned long)
#define MIL1553_GET_BC_INFO      PIOWR(mil1553GET_BC_INFO,     struct mil1553_dev_info_s)
#define MIL1553_RAW_READ         PIOWR(mil1553RAW_READ,        struct mil1553_riob_s)
#define MIL1553_RAW_WRITE        PIOWR(mil1553RAW_WRITE,       struct mil1553_riob_s)
#define MIL1553_GET_UP_RTIS      PIOWR(mil1553GET_UP_RTIS,     unsigned long)
#define MIL1553_SEND             PIOW(mil1553SEND,             struct mil1553_send_s)
#define MIL1553_RECV             PIOWR(mil1553RECV,            struct mil1553_recv_s)
#define MIL1553_LOCK_BC          PIOW(mil1553LOCK_BC,          unsigned long)
#define MIL1553_UNLOCK_BC        PIOW(mil1553UNLOCK_BC,        unsigned long)
#define MIL1553_QUEUE_SIZE       PIOR(mil1553QUEUE_SIZE,       unsigned long)
#define MIL1553_RESET            PIOW(mil1553RESET,            unsigned long)
#define MIL1553_SET_POLLING      PIOW(mil1553SET_POLLING,      unsigned long)
#define MIL1553_GET_POLLING      PIOR(mil1553GET_POLLING,      unsigned long)
#define MIL1553_SET_TP           PIOWR(mil1553SET_TP,          unsigned long)
#define MIL1553_GET_TP           PIOWR(mil1553GET_TP,          unsigned long)

#endif
