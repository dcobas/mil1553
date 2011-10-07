/**
 * =========================================================================================
 * Basic driver API for raw PCI IO
 */

#ifndef MIL1553
#define MIL1553

#define INTERRUPT 0
#define INTEN 1
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

#define TIMEOUT   1000
#define TX_START  0x01
#define TX_END    0x02
#define TX_ALL    0x04

#define MAX_REGS 43

/**
 * Usefull masks and shifits for working on registers
 */

#define ISRC_RTI_SHIFT 27
#define ISRC_RTI_MASK  (0x1F << ISRC_RTI_SHIFT)
#define ISRC_WC_SHIFT  22
#define ISRC_WC_MASK   (0x1F << ISRC_WC_SHIFT)
#define ISRC_IRQ       1

#define INTEN_INF      1

#define HSTAT_VER_SHIFT       0
#define HSTAT_VER_MASK        (0xFFFF << HSTAT_VER_SHIFT)
#define HSTAT_STAT_SHIFT      16
#define HSTAT_STAT_MASK       (0xFFFF << HSTAT_STAT_SHIFT)

#define HSTAT_CODE_VIOL_SHIFT 16
#define HSTAT_CODE_VIOL_MASK  (1 << HSTAT_CODE_VIOL_SHIFT)
#define HSTAT_PAR_ERR_SHIFT   17
#define HSTAT_PAR_ERR_MASK    (1 << HSTAT_PAR_ERR_SHIFT)
#define HSTAT_POLL_OFF_SHIFT  28
#define HSTAT_POLL_OFF_MASK   (1 << HSTAT_POLL_OFF_SHIFT)
#define HSTAT_BUSY_DONE_SHIFT 30
#define HSTAT_BUSY_DONE_MASK  (3 << HSTAT_BUSY_DONE_SHIFT)
#define HSTAT_BUSY            1
#define HSTAT_DONE            2

#define CMD_POLL_OFF_SHIFT 1
#define CMD_POLL_OFF_MASK  (1 << CMD_POLL_OFF_SHIFT)
#define CMD_POLL_OFF       1
#define CMD_SPEED_SHIFT    30
#define CMD_SPEED_MASK     (3 << CMD_SPEED_SHIFT)
#define CMD_SPEED_1M       0
#define CMD_SPEED_500K     1
#define CMD_SPEED_250K     2
#define CMD_SPEED_125K     3

#define TXREG_WC_SHIFT   0
#define TXREG_WC_MASK    (0x1F << TXREG_WC_SHIFT)
#define TXREG_SUBA_SHIFT 5
#define TXREG_SUBA_MASK  (0x1F << TXREG_SUBA_SHIFT)
#define TXREG_TR_SHIFT   10
#define TXREG_TR_MASK    (1 << TXREG_TR_SHIFT)
#define TXREG_RTI_SHIFT  11
#define TXREG_RTI_MASK   (0x1F << TXREG_RTI_SHIFT)

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
	unsigned short rxbuf[RX_BUF_SIZE];    /** Receive  buffer */
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
	unsigned int speed;                   /** Current bus speed */
	unsigned int icnt;                    /** Total interrupt count */
	unsigned int isrdebug;                /** Debug information from isr */
};

struct mil1553_bus_speed_s {
	unsigned int bc;                      /** The BC you want to set */
	unsigned int speed;                   /** The bus speed 0..3 */
};

#define STATUS_NOT_BUSY   0x8000
#define STATUS_BUSY       0x4000
#define STATUS_POLL_OFF   0x1000
#define STATUS_PARITY_ERR 0x0002
#define STATUS_CODE_VIOL  0x0001

#define BUS_SPEED_1MEGBIT 0
#define BUS_SPEED_500KBIT 1
#define BUS_SPEED_250KBIT 2
#define BUS_SPEED_125KBIT 3

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
	mil1553SET_BUS_SPEED,     /** Set the bus speed */

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

	mil1553LAST               /** For range checking (LAST - FIRST) */

} mil1553_ioctl_function_t;

#define mil1553IOCTL_FUNCTIONS (mil1553LAST - mil1553FIRST - 1)

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
#define MIL1553_SET_BUS_SPEED    PIOWR(mil1553SET_BUS_SPEED,   struct mil1553_bus_speed_s)
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

#endif
