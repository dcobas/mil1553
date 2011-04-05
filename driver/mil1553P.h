/**
 * Julian Lewis Mon 14th Feb 2011 BE/CO/HT
 * Julian.Lewis@cern.ch
 *
 * This is a total rewrite of the CBMIA PCI driver to control MIL 1553
 *
 * MIL 1553 bus controler CBMIA module
 * Private driver definitions
 */

#ifndef MIL1553_P
#define MIL1553_P

/* PCI Vendor and Device IDs */

#define VID_CERN 0x10DC
#define DID_MIL1553 0x301

/**
 * Memory layout as described in
 * LHC-V0.7 User Manual
 * Programming interface for the PCI 1553 CBMIA card
 */

#define MAX_DEVS 8

struct memory_map_s {
	uint32_t isrc;       /** R/W INTERRUPTREG  Interrupt source    */
	uint32_t inten;      /** R/W INTENBREG     Interrupt enable    */
	uint32_t up_rtis;    /** R   RTIPRESENTREG Mask of online RTIs */
	uint32_t hstat;      /** R   SOURCEREG     Hardware status     */
	uint32_t cmd;        /** R/W COMMANDREG    Command register    */
	uint32_t not_used_1;
	uint32_t snum_h;     /** R   MIDP          High 32 bits of serial number */
	uint32_t snum_l;     /** R   LIDP          Low  32 bits of serial number */
	uint32_t txreg;      /** R/W TXREG         Transmit control register */
	uint32_t rxreg;      /** R   RXREG         Receiver register ?? */

	uint16_t rxbuf[RX_BUF_SIZE]; /** R   RXBUFREG Receive  buffer */
	uint16_t not_used_2;
	uint16_t txbuf[TX_BUF_SIZE]; /** R/W TXBUFREG Transmit buffer */
};

#define MAX_RTI_BUFFERS 30
#define QSZ MAX_RTI_BUFFERS

/**
 * About what follows: Parrallel and sequential transfers.
 * A client is allocated in the "open" call and a pointer is stored in filp->private_data,
 * this pointer is used to identify the client that initiates writes and receives results.
 * Clients may write to multiple BCs and RTIs in a single driver call.
 * The data to be written is stored in tx_queues for each BC along with the initiating client.
 * When the RTI responds the ISR gets called.
 * There can be only one RTI transaction at a time for a given BC.
 * The ISR acquires the data from the RTI, puts it on the clients queue, and wakes it up.
 * The client process gets woken up multiple times until all writes are complete or timed out.
 * The ISR initiates the next write until the tx_queues are empty.
 */

/**
 * RTI Interrupt item, these are sent to clients on completion of a command
 */

struct rti_interrupt_s {
	uint32_t bc;                      /** Bus controller */
	uint32_t rti_number;              /** Rti that interrupted */
	uint32_t wc;                      /** Buffer word count */
	uint32_t rxbuf_rti_stat;          /** RTI status in RX buffer */
	uint32_t rxbuf[RX_BUF_SIZE];      /** Receive  buffer */
};

/**
 * RX queue for a client to read results of commands
 */

struct rx_queue_s {
	spinlock_t lock;                           /** To lock the queue */
	uint32_t rp;                               /** Read pointer */
	uint32_t wp;                               /** Write pointer */
	struct rti_interrupt_s rti_interrupt[QSZ]; /** Clients queue */
};

/**
 * Each client has it unique ID, which is just the address this structure in the
 * filp->private_data. Each client has a queue.
 */

struct client_s {
	uint32_t pk_type;               /** Interrupt mask for START, END, ALL */
	uint32_t icnt;                  /** Number of interrupts for this client */
	uint32_t debug_level;           /** Debug level for this client */
	uint32_t timeout;               /** Timeout in jiffels */
	wait_queue_head_t wait_queue;   /** Client waits on this for available data */
	struct rx_queue_s rx_queue;     /** Results of commands */
};

/**
 * A Transmit item corresponds to a client writing to an RTI
 */

struct tx_item_s {
	uint32_t no_reply;              /** Set when no reply wanted */
	uint32_t pk_type;               /** Packet type START, MIDDLE or END */
	struct client_s *client;        /** The client who issued the command */
	uint32_t bc;                    /** Bus controller number */
	uint32_t rti_number;            /** RTI number */
	uint32_t txreg;                 /** Transmit register wc, sa, t/r bit, rti */
	uint32_t txbuf[TX_BUF_SIZE];    /** Buffer */
};

/**
 * Transmit queue, its a pool of items waiting to be written to the RTIs
 * For each BC there can be only one item pending transmission, successive
 * writes to RTIs on one BC are queued and executed sequentially under
 * interrupt control.
 */

struct tx_queue_s {
	spinlock_t lock;                /** To lock the queue */
	uint32_t rp;                    /** Read pointer */
	uint32_t wp;                    /** Write pointer */
	struct tx_item_s tx_item[QSZ];  /** Buffer of items to be transmitted */
};

/**
 * A mapped CBMIA PCI module is a mil1553 device
 * I decided to keep the tx_queue seperatly as its not strictly part of a device
 */

#define BC_DONE 0
#define BC_BUSY 1

struct mil1553_device_s {
	spinlock_t           lock;        /** To lock the queue */
	uint32_t             bc;          /** Bus controller */
	uint32_t             pci_bus_num; /** PCI bus number */
	uint32_t             pci_slt_num; /** PCI slot number */
	uint32_t             snum_h;      /** High 32 bits of serial number */
	uint32_t             snum_l;      /** Low  32 bits of serial number */
	struct pci_dev      *pdev;        /** Pci device handle */
	struct memory_map_s *memory_map;  /** Mapped BAR2 device memory */
	uint32_t             busy_done;   /** Bus controller busy/done status */
	uint32_t             up_rtis;     /** Last known up rtis mask */
	struct tx_queue_s   *tx_queue;    /** Transmit Queue pointer */
};

/**
 * The driver working area.
 */

struct working_area_s {
	uint32_t bcs;                                  /** The number of BCs installed */
	struct mil1553_device_s mil1553_dev[MAX_DEVS]; /** The BC device descriptions */
	struct tx_queue_s tx_queue[MAX_DEVS];          /** Data and commands waiting to be transmitted */
	uint32_t icnt;                                 /** Total interrupt count */
	uint32_t isrdebug;                             /** Trace ISR */
};

#endif
