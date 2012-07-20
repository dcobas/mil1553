/**
 Julian Lewis March 28 2012 BE/CO/HT
 * Julian.Lewis@cern.ch
 *
 * This is a total rewrite of the CBMIA PCI driver to control MIL 1553
 * MIL 1553 bus controler CBMIA module
 *
 * This code relies on a new firmware version number 0x206 and later
 * In this version proper access to the TXREG uses a busy done bit.
 * Software polling has been implemented, hardware polling is removed.
 * The bus speed is fixed at 1Mbit.
 * Hardware test points and diagnostic/debug registers are added.
 *
 * Mil1553 driver
 * This driver is a complete rewrite of a previous version from Yuri
 * Version 1 was started by: BE/CO/HT Julian Lewis Tue 15th Feb 2011
 *
 * TODO: The tx-queues are not needed due to the rti and quick data protocols
 *       being implemented in user space. The queue mechanism adds complexity
 *       that it turns out is not needed. The queuing mechanisms therefore
 *       need revising. For the time being they are harmless and work fine so
 *       appart from more difficult code there are no other problems.
 */

#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>

#include "mil1553.h"
#include "mil1553P.h"

#ifndef MIL1553_DRIVER_VERSION
#define MIL1553_DRIVER_VERSION	"noversion"
#endif

#define PFX	"mil1553: "

char *mil1553_driver_version = MIL1553_DRIVER_VERSION;

static int   mil1553_major      = 0;
static char *mil1553_major_name = "mil1553";

MODULE_AUTHOR("Julian Lewis BE/CO/HT CERN");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MIL1553 Driver");
MODULE_SUPPORTED_DEVICE("CBMIA PCI module");

/**
 * Module parameters bcs=7,4,3,5 pci_bus=1,1,2,3 pci_slot=4,5,4,5
 */

static int bcs[MAX_DEVS];
static int pci_buses[MAX_DEVS];
static int pci_slots[MAX_DEVS];

static int bc_num;
static int pci_bus_num;
static int pci_slot_num;
static int debug_msg;

module_param_array(bcs,       int, &bc_num,       0);
module_param_array(pci_buses, int, &pci_bus_num,  0);
module_param_array(pci_slots, int, &pci_slot_num, 0);
module_param(debug_msg, int, 0);

MODULE_PARM_DESC(bcs,       "bus controller number 1..8");
MODULE_PARM_DESC(pci_buses, "pci bus number");
MODULE_PARM_DESC(pci_slots, "pci slot number");

static int dump_packet;			/* to dump corrupted packets */
module_param(dump_packet, int, 0);

#ifndef COMPILE_TIME
#define COMPILE_TIME 0
#endif

/**
 * Drivers static working area
 */

struct working_area_s wa;

/**
 * =========================================================
 * Debug ioctl calls
 */

#define NAME(x) [mil1553##x] = #x
static char *ioctl_names[] = {

	NAME(GET_DEBUG_LEVEL),
	NAME(SET_DEBUG_LEVEL),

	NAME(GET_TIMEOUT_MSEC),
	NAME(SET_TIMEOUT_MSEC),

	NAME(GET_DRV_VERSION),

	NAME(GET_STATUS),
	NAME(GET_TEMPERATURE),

	NAME(GET_BCS_COUNT),
	NAME(GET_BC_INFO),

	NAME(RAW_READ),
	NAME(RAW_WRITE),

	NAME(GET_UP_RTIS),
	NAME(SEND),
	NAME(RECV),

	NAME(LOCK_BC),
	NAME(UNLOCK_BC),

	NAME(QUEUE_SIZE),
	NAME(RESET),

	NAME(SET_POLLING),
	NAME(GET_POLLING),

	NAME(SET_TP),
	NAME(GET_TP),
};

/**
 * =========================================================
 * @brief Get device corresponding to a given BC
 * @param Given BC number of device
 * @return Pointer to device if exists, else NULL
 */

struct mil1553_device_s *get_dev(int bc)
{
	int i;
	struct mil1553_device_s *mdev;

	if (bc > 0) {
		for (i=0; i<wa.bcs; i++) {
			mdev = &(wa.mil1553_dev[i]);
			if (mdev->bc == bc)
				return mdev;
		}
	}
	return NULL;
}

/**
 * =========================================================
 * @brief Validate insmod args, can be empty
 * @return 1=OK 0=ERROR
 */

static int check_args(void)
{
       int i;

       if ((bc_num < 0) || (bc_num > MAX_DEVS)) {
	       printk("mill1553:bad BC count:%d, not installing\n",bc_num);
	       return 0;
       }
       if ((bc_num != pci_slot_num) || (bc_num != pci_bus_num)) {
	       printk("mill1553:bad parameter count\n");
	       return 0;
       }
       for (i=0; i<bc_num; i++) {
	       if ((bcs[i] <= 0) || (bcs[i] >= MAX_DEVS)) {
		       printk("mill1553:bad BC num:%d\n", bcs[i]);
		       return 0;
	       }
       }
       return 1;
}

/**
 * =========================================================
 * @brief       Hunt for a BC number
 * @param bus   The PCI bus
 * @param slot  The PCI slot number
 * @return      The bc number or zero if not found
 */

int hunt_bc(int bus, int slot)
{
	int i;
	for (i=0; i<bc_num; i++)
		if ((pci_slots[i] == slot)
		&&  (pci_buses[i] == bus))
			return bcs[i];
	return 0;
}

/**
 * =========================================================
 * @brief Print debug information
 * @param debug_level  debug level 0..7
 * @param ionr         command number decoded
 * @param iosz         size of arg in bytes
 * @param iodr         io direction
 * @param kmem         points to kernel memory where arg copied to/from
 * @param flag         Before or after logic flag
 *
 * For people developing mil1553 code this debug routine is useful, especially
 * if they are attempting to use raw IO. There are 7 levels of debug implemented
 * throughout the driver code that controls just how much information gets printed
 * out. This will help in maintanence, debugging modifications etc.
 */

#define MAX_PCOUNT 16
#define BEFORE 1
#define AFTER 2

static void debug_ioctl(int   debug_level,
			int   ionr,
			int   iosz,
			int   iodr,
			void *kmem,
			int   flag)
{
	int cindx, *values, pcount, i;

	if (flag == BEFORE) {

		if ((ionr <= mil1553FIRST) || (ionr >= mil1553LAST)) {
			printk("mil1553:Illegal ioctl:ionr:%d iosz:%d iodr:%d\n",ionr,iosz,iodr);
			return;
		}

		if (!debug_level)
			return;

		cindx = ionr - mil1553FIRST -1;

		printk("\n=> mil1553:ioctl:ionr:%d[%s] iosz:%d iodr:%d[",ionr,ioctl_names[cindx],iosz,iodr);
		if (iodr & _IOC_WRITE)
			printk("W");
		if (iodr & _IOC_READ)
			printk("R");
		printk("]\n");
	}

	if (debug_level > 1) {
		values = kmem;
		pcount = iosz / sizeof(int);
		if (pcount > MAX_PCOUNT)
			pcount = MAX_PCOUNT;

		if ((flag == BEFORE) && (iodr & _IOC_WRITE)) {
			printk("IO Buffer BEFORE logic:");
			for (i=0; i<pcount; i++) {
				if (!(i % 4))
					printk("\nkmem:%02d:",i);
				printk("0x%08X ",values[i]);
			}
		}
		if ((flag == AFTER) && (iodr & _IOC_READ)) {
			printk("IO Buffer AFTER logic:");
			for (i=0; i<pcount; i++) {
				if (!(i % 4))
					printk("\nkmem:%02d:",i);
				printk("0x%08X ",values[i]);
			}
		}
		printk("\n");
	}
}

/**
 * =========================================================
 * @brief           Read U32 integers from mapped address space
 * @param mdev      Mill1553 device
 * @param riob      IO buffer descriptor
 * @param buf       Buffer to hold data read
 * @return          Number of bytes read
 */

static int _raw_read(struct mil1553_device_s *mdev,
		     struct mil1553_riob_s *riob,
		     void *buf)
{

	int i;
	uint32_t *uip, *hip;

	uip = buf;
	if ((!mdev) || (!mdev->memory_map))
		return 0;

	hip = (uint32_t *) mdev->memory_map + riob->reg_num;

	for (i=0; i<riob->regs; i++) {
		uip[i] = ioread32be(&hip[i]);
	}

	/*
	 * Remember that i will be greater than length
	 * when the loop terminates.
	 */

	return i*sizeof(int);
}

/**
 * @brief Just calls _raw_read with spin lock protection
 */

static int raw_read(struct mil1553_device_s *mdev,
		    struct mil1553_riob_s *riob,
		    void *buf)
{
	int res;
	spin_lock(&mdev->lock);
	res = _raw_read(mdev, riob, buf);
	spin_unlock(&mdev->lock);
	return res;
}

/**
 * =========================================================
 * @brief           Write U32 integers to mapped address space
 * @param mdev      Mill1553 device
 * @param riob      IO buffer descriptor
 * @param buf       Buffer to holding data to write
 * @return          Number of bytes written
 */

static int _raw_write(struct mil1553_device_s *mdev,
		      struct mil1553_riob_s *riob,
		      void *buf)
{

	int i;
	uint32_t *uip, *hip;

	uip = buf;
	if ((!mdev) || (!mdev->memory_map))
		return 0;

	hip = (uint32_t *) mdev->memory_map + riob->reg_num;

	for (i=0; i<riob->regs; i++) {
		iowrite32be(uip[i],&hip[i]);
	}

	/*
	 * Remember that i will be greater than length
	 * when the loop terminates.
	 */

	return i*sizeof(int);
}

/**
 * @brief Just calls _raw_write with spin lock protection
 */

static int raw_write(struct mil1553_device_s *mdev,
		     struct mil1553_riob_s *riob,
		     void *buf)
{
	int res;
	spin_lock(&mdev->lock);
	res = _raw_write(mdev, riob, buf);
	spin_unlock(&mdev->lock);
	return res;
}

#define BETWEEN_TRIES_MS 1
#define TX_TRIES 100
#define TX_RETRIES 5
#define TX_WAIT_US 10
#define CBMIA_INT_TIMEOUT 2
#define INT_MISSING_TIMEOUT 4

static int int_timeout = CBMIA_INT_TIMEOUT;
static int busy_timeout = INT_MISSING_TIMEOUT;
static int clear_missed_int = 0;

static int do_start_tx(struct mil1553_device_s *mdev, uint32_t txreg)
{
	struct memory_map_s *memory_map = mdev->memory_map;
	int i, icnt, timeleft;
	int retries = TX_RETRIES;

retries:
	icnt = mdev->icnt;
	timeleft = wait_event_interruptible_timeout(mdev->int_complete,
		!atomic_read(&mdev->int_busy), msecs_to_jiffies(busy_timeout));
	if (timeleft <= 0) {
		printk(KERN_ERR PFX 
			"attempt to Tx on busy BC %d, timed out after "
			" %u ms\n", mdev->bc, busy_timeout);
	}
	atomic_set(&mdev->int_busy, 1);
	for (i = 0; i < TX_TRIES; i++) {
		if ((ioread32be(&memory_map->hstat) & HSTAT_BUSY_BIT) == 0) {
			iowrite32be(txreg, &memory_map->txreg);
			mdev->tx_count++;
			break;
		}
		printk(KERN_ERR PFX "HSTAT_BUSY_BIT != 0 in do_start_tx; "
				"tx_count %d, ms %u on pid %d\n", mdev->tx_count,
					jiffies_to_msecs(jiffies), current->pid);
		udelay(TX_WAIT_US);
	}
	udelay(8*TX_WAIT_US);
	timeleft = wait_event_interruptible_timeout(mdev->int_complete,
		!atomic_read(&mdev->int_busy),
		msecs_to_jiffies(int_timeout));
	if (timeleft <= 0) {
		printk(KERN_ERR PFX "interrupt pending"
				" after %d msecs in bc %d, "
				"timeleft = %d, pid = %d\n",
				int_timeout,
				mdev->bc, timeleft, current->pid);
		if (--retries > 0)
			goto retries;
		else
			printk(KERN_ERR PFX "could not TX to "
				"bc %d after %d retries, leaving\n",
				mdev->bc, TX_RETRIES);
			return -EBUSY;
	} else
		return 0;
}

static void ping_rtis(struct mil1553_device_s *mdev)
{
	int rti;
	uint32_t txreg;
	struct memory_map_s *memory_map;

	memory_map = mdev->memory_map;
	if (mdev->busy_done == BC_DONE) {       /** Make sure no transaction in progress */
		for (rti=1; rti<=30; rti++) {   /** Next RTI to poll */
			txreg = ((1  << TXREG_WC_SHIFT)   & TXREG_WC_MASK)
			      | ((30 << TXREG_SUBA_SHIFT) & TXREG_SUBA_MASK)
			      | ((1  << TXREG_TR_SHIFT)   & TXREG_TR_MASK)
			      | ((rti<< TXREG_RTI_SHIFT)  & TXREG_RTI_MASK);
			do_start_tx(mdev, txreg);
			msleep(BETWEEN_TRIES_MS);               /** Wait between pollings */
		}
	}
}

/**
 * =========================================================
 * @brief Get the word count from a txreg
 * @param txreg
 * @return the word count
 *
 * After much trial and error the behaviour of wc
 * values on the cbmia seems to be as follows...
 * On reading data the status is always prefixed and
 * this plays no part in the word count interpretation.
 * A value of zero represents a word count of 32 in
 * the appropriate modes.
 */

unsigned int get_wc(unsigned int txreg)
{
	unsigned int wc;

	wc = (txreg & TXREG_WC_MASK) >> TXREG_WC_SHIFT;
	if (wc == 0)
		wc = 32;
	return wc;
}

static irqreturn_t mil1553_isr(int irq, void *arg)
{
	struct mil1553_device_s *mdev = arg;
	struct rti_interrupt_s *rti_interrupt = &mdev->rti_interrupt;
	struct memory_map_s *memory_map = mdev->memory_map;
	uint32_t isrc;
	int rtin, pk_ok, timeout;

	isrc = ioread32be(&memory_map->isrc);   /** Read and clear the interrupt */
	if ((isrc & ISRC) == 0)
		return IRQ_NONE;

	mdev->icnt++;
	wa.icnt++;
	
	rti_interrupt->bc	  = mdev->bc;		/* redundant */
	rti_interrupt->rti_number = rtin = (isrc & ISRC_RTI_MASK) >> ISRC_RTI_SHIFT;
	rti_interrupt->wc	  = (isrc & ISRC_WC_MASK) >> ISRC_WC_SHIFT;
	rti_interrupt->timeout	  = timeout = (isrc & ISRC_TIME_OUT);
	rti_interrupt->packet_ok  = pk_ok = (ISRC_GOOD_BITS & isrc) &&
					    ((ISRC_BAD_BITS & isrc) == 0);
	if (!timeout)
		mdev->up_rtis |= 1 << rtin;
	if (!pk_ok || timeout) {
		mdev->up_rtis &= ~(1 << rtin);
		wa.isrdebug = isrc;
		mdev->busy_done = BC_DONE;
	}

	if (!atomic_xchg(&mdev->int_busy, 0)) {
		printk(KERN_ERR PFX "spurious int on idle bc %d\n", mdev->bc);
	}
	wake_up_interruptible(&mdev->int_complete);
	return IRQ_HANDLED;
}

/**
 * =========================================================
 * @brief           Add the next mil1553 pci device
 * @param  pcur     Previous added device or NULL
 * @param  mdev     Device context
 * @return          Pointer to pci_device structure or NULL
 */

#define BAR2 2
#define PCICR 1
#define MEM_SPACE_ACCESS 2

struct pci_dev *add_next_dev(struct pci_dev *pcur,
			     struct mil1553_device_s *mdev)
{
	struct pci_dev *pprev = pcur;
	int cc, len;
	char bar_name[32];

	pcur = pci_get_device(VID_CERN, DID_MIL1553, pprev);
	if (!pcur)
		return NULL;

	mdev->pci_bus_num = pcur->bus->number;
	mdev->pci_slt_num = PCI_SLOT(pcur->devfn);
	cc = pci_enable_device(pcur);
	printk("mil1553:VID:0x%X DID:0x%X BUS:%d SLOT:%d",
	       VID_CERN,
	       DID_MIL1553,
	       mdev->pci_bus_num,
	       mdev->pci_slt_num);
	if (cc) {
		printk(" pci_enable:ERROR:%d",cc);
		return NULL;
	} else
		printk(" Enabled:OK\n");

	/*
	 * Map BAR2 the CBMIA FPGA (Its BIG endian !!)
	 */

	sprintf(bar_name,"mil1553.bc.%d.bar.%d",mdev->bc,BAR2);
	len = pci_resource_len(pcur, BAR2);
	cc = pci_request_region(pcur, BAR2, bar_name);
	if (cc) {
		pci_disable_device(pcur);
		printk("mil1553:pci_request_region:len:0x%x:%s:ERROR:%d\n",len,bar_name,cc);
		return NULL;

	}
	mdev->memory_map = (struct memory_map_s *) pci_iomap(pcur,BAR2,len);

	/*
	 * Configure interrupt handler
	 */

	cc = request_irq(pcur->irq, mil1553_isr, IRQF_SHARED, "MIL1553", mdev);
	if (cc) {
		pci_disable_device(pcur);
		printk("mil1553:request_irq:ERROR%d\n",cc);
		return NULL;
	}

	printk("mil1553:Device Bus:%d Slot:%d INSTALLED:OK\n",
	       mdev->pci_bus_num,
	       mdev->pci_slt_num);
	return pcur;
}

/**
 * =========================================================
 * @brief           Release a PCI device
 * @param dev       PCI device handle
 */

void release_device(struct mil1553_device_s *mdev)
{

	if (mdev->pdev) {
		pci_iounmap(mdev->pdev, (void *) mdev->memory_map);
		pci_release_region(mdev->pdev, BAR2);
		pci_disable_device(mdev->pdev);
		pci_dev_put(mdev->pdev);
		free_irq(mdev->pdev->irq,mdev);
		printk("mil1553:BC:%d RELEASED DEVICE:OK\n",mdev->bc);
	}
}

/**
 * Initialize device hardware:
 * Clear interrupt.
 * Enable interrupt.
 * Read 64-bit serial number.
 * Set transaction done
 */

static void init_device(struct mil1553_device_s *mdev)
{

	struct memory_map_s *memory_map = mdev->memory_map;

	ioread32be(&memory_map->isrc);
	iowrite32be(INTEN,&memory_map->inten);

	mdev->snum_h = ioread32be(&memory_map->snum_h);
	mdev->snum_l = ioread32be(&memory_map->snum_l);

	mdev->busy_done = BC_DONE; /** End transaction */
}

/**
 * =========================================================
 * @brief           Get an unused BC number
 * @return          A lun number or zero if none available
 */

static int used_bcs = 1;

#define RTI_WAIT_us 100
static void encode_txreg(unsigned int *txreg,
	unsigned int wc, unsigned int sa, unsigned int tr, unsigned int rti)
{
	if (wc >= 32)
		wc = 0;
	if (txreg)
		*txreg = ((wc  << TXREG_WC_SHIFT)   & TXREG_WC_MASK)
		       | ((sa  << TXREG_SUBA_SHIFT) & TXREG_SUBA_MASK)
		       | ((tr  << TXREG_TR_SHIFT)   & TXREG_TR_MASK)
		       | ((rti << TXREG_RTI_SHIFT)  & TXREG_RTI_MASK);
}

static void dump_buf(unsigned short *buf, int wc)
{
	int i;

	printk("wc = %d\n", wc);
	for (i = 0; i < wc; i++) {
		if (i % 2 == 0)
			printk("%04x: ", i);
		printk("%04x ", buf[i]);
		if (i % 2 == 1)
			printk("\n");
	}
	printk("\n");
}

static int send_receive(struct mil1553_device_s *mdev,
			int rti, int sent_wc, int sa, int tr,
			int wants_reply,
			unsigned short *rxbuf,
			unsigned short *txbuf,
			int *received_wc)
{
	uint32_t		txreg;
	uint32_t		*regp, reg;
	int			i, cc;
	struct rti_interrupt_s	*rti_interrupt = &mdev->rti_interrupt;
	struct memory_map_s	*memory_map = mdev->memory_map;

	if (debug_msg)
	printk(KERN_ERR PFX "calling send_receive "
		"%d:%d wc:%d sa:%d tr:%d %s\n",
		mdev->bc, rti, sent_wc, sa, tr,
		wants_reply? "reply" : "noreply");
	mutex_lock_interruptible(&mdev->bcdev);
	encode_txreg(&txreg, sent_wc, sa, tr, rti);
	if (sent_wc > TX_BUF_SIZE)
		sent_wc = TX_BUF_SIZE;
	regp = (uint32_t *) memory_map->txbuf;
	for (i=0; i < (sent_wc + 1) / 2; i++) {
		reg  = txbuf[i*2 + 1] << 16;
		reg |= txbuf[i*2 + 0] & 0xFFFF;
		iowrite32be(reg, &regp[i]);
	}
	if (debug_msg) {
		printk(KERN_ERR PFX "sending txbuf\n");
		dump_buf(txbuf, sent_wc);
	}
	cc = do_start_tx(mdev, txreg);
	if (cc)
		goto exit;

	if (!wants_reply)
		goto exit;

	memset(rxbuf, 0, sizeof(rxbuf));
	if (rti_interrupt->rti_number == 0) {
		cc = -ETIME;
		goto exit;
	} else if (rti_interrupt->rti_number != rti) {
		printk(KERN_ERR PFX "wrong rti expected %d, got %d replied\n",
		rti, rti_interrupt->rti_number);
	}

	/* Remember rxbuf is accessed as u32 but wc is the u16 count */
	/* Word order is little endian */
	*received_wc = rti_interrupt->wc;
	regp = (uint32_t *) memory_map->rxbuf;
	if (debug_msg) {
		printk(KERN_ERR PFX "copying wc = %d\n", rti_interrupt->wc);
		if (rti_interrupt->wc > 29) {
			for (i = 0; i < RX_BUF_SIZE; i++) {
				reg = ioread32be(&regp[i]);
				printk(KERN_ERR "%04x:  %08x\n", i, reg);
			}
		}
	}
	for (i = 0; i < (rti_interrupt->wc + 1) / 2 ; i++) {
	       reg  = ioread32be(&regp[i]);
	       rxbuf[i*2 + 1] = reg >> 16;
	       rxbuf[i*2 + 0] = reg & 0xFFFF;
	}
	if (debug_msg) {
		printk(KERN_ERR PFX "received rxbuf\n");
		dump_buf(rxbuf, rti_interrupt->wc);
	}
exit:
	mutex_unlock(&mdev->bcdev);
	return cc;
}

int get_unused_bc(void)
{

	int i, bc, bit;

	for (i=0; i<bc_num; i++) {
		bc = bcs[i];
		bit = 1 << bc;
		used_bcs |= bit;
	}

	for (i=1; i<MAX_DEVS; i++) {
		bit = 1 << i;
		if (used_bcs & bit)
			continue;
		used_bcs |= bit;
		return i;
	}
	return 0;
}

/**
 * =========================================================
 * Open
 * Allocate a client context and initialize it
 * Place pointer to client in the file private data pointer
 */

int mil1553_open(struct inode *inode, struct file *filp)
{

	struct client_s *client;

	client = kmalloc(sizeof(struct client_s),GFP_KERNEL);
	if (client == NULL)
		return -ENOMEM;

	memset(client,0,sizeof(struct client_s));

	init_waitqueue_head(&client->wait_queue);
	client->timeout = msecs_to_jiffies(RTI_TIMEOUT);
	spin_lock_init(&client->rx_queue.lock);

	filp->private_data = client;
	return 0;
}

/**
 * =========================================================
 * Close
 */

int mil1553_close(struct inode *inode, struct file *filp)
{

	struct client_s         *client;
	struct mil1553_device_s *mdev;
	int                      bc;

	client = (struct client_s *) filp->private_data;
	if (client) {
		bc = client->bc_locked;
		if (bc) {
			mdev = get_dev(bc);
			if (mdev)
				mutex_unlock(&mdev->bc_lock);
		}
		kfree(client);
		filp->private_data = NULL;
	}
	return 0;
}

/**
 * =========================================================
 * Ioctl
 */

int mil1553_ioctl(struct inode *inode, struct file *filp,
		  unsigned int cmd, unsigned long arg)
{

	void *mem, *buf; /* Io memory */
	int iodr;        /* Io Direction */
	int iosz;        /* Io Size in bytes */
	int ionr;        /* Io Number */

	struct memory_map_s *memory_map;

	int bc, cc = 0;
	unsigned int cnt, blen;

	uint32_t reg, tp;

	unsigned long *ularg;

	struct mil1553_riob_s       *riob;
	struct mil1553_device_s     *mdev;
	struct mil1553_dev_info_s   *dev_info;
	struct mil1553_send_recv_s  *sr;

	struct client_s   *client = (struct client_s *) filp->private_data;

	ionr = _IOC_NR(cmd);
	iodr = _IOC_DIR(cmd);
	iosz = _IOC_SIZE(cmd);

	if ((ionr >= mil1553LAST) || (ionr <= mil1553FIRST))
		return -ENOTTY;

	if ((mem = kmalloc(iosz,GFP_KERNEL)) == NULL)
		return -ENOMEM;

	if (iodr & _IOC_WRITE) {
		cc = copy_from_user(mem, (char *) arg, iosz);
		if (cc)
			goto error_exit;
	}

	debug_ioctl(client->debug_level,ionr,iosz,iodr,mem,BEFORE);

	ularg = mem;

	switch (ionr) {

		case mil1553SET_POLLING:

			wa.nopol = *ularg;
		break;

		case mil1553GET_POLLING:

			*ularg = wa.nopol;
		break;

		case mil1553GET_DEBUG_LEVEL:   /** Get the debug level 0..7 */

			*ularg = client->debug_level;
		break;

		case mil1553SET_DEBUG_LEVEL:   /** Set the debug level 0..7 */

			client->debug_level = *ularg;
		break;

		case mil1553GET_TIMEOUT_MSEC:  /** Get the client timeout in milliseconds */

			*ularg = jiffies_to_msecs(client->timeout);
		break;

		case mil1553SET_TIMEOUT_MSEC:  /** Set the client timeout in milliseconds */

			client->timeout = msecs_to_jiffies(*ularg);
		break;

		case mil1553GET_DRV_VERSION:   /** Get the UTC driver compilation data */

			*ularg = COMPILE_TIME;
		break;

		case mil1553GET_STATUS:        /** Reads the status register */

			bc = *ularg;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			memory_map = mdev->memory_map;
			reg = ioread32be(&memory_map->hstat);
			*ularg = (reg & HSTAT_STAT_MASK) >> HSTAT_STAT_SHIFT;
		break;

		case mil1553RESET:             /** Reads the status register */

			bc = *ularg;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			memory_map = mdev->memory_map;
			iowrite32be(CMD_RESET,&memory_map->cmd);
			init_device(mdev);
			mdev->up_rtis = 0;
			wa.isrdebug = 0;
		break;

		case mil1553GET_TEMPERATURE:

			bc = *ularg;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			memory_map = mdev->memory_map;
			*ularg = ioread32be(&memory_map->temp);
		break;

		case mil1553SET_TP:
			bc = *ularg & MAX_DEVS_MASK;
			tp = *ularg & CMD_TPS_MASK;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			memory_map = mdev->memory_map;
			iowrite32be(tp,&memory_map->cmd);
		break;

		case mil1553GET_TP:
			bc = *ularg & MAX_DEVS_MASK;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			memory_map = mdev->memory_map;
			*ularg = ioread32be(&memory_map->cmd) & (CMD_TPS_MASK | bc);
		break;

		case mil1553GET_BCS_COUNT:     /** Get the Bus Controllers count */

			*ularg = wa.bcs;
		break;

		case mil1553GET_BC_INFO:       /** Get information aboult a Bus Controller */

			dev_info = mem;
			bc = dev_info->bc;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			memory_map = mdev->memory_map;
			dev_info->pci_bus_num = mdev->pci_bus_num;
			dev_info->pci_slt_num = mdev->pci_slt_num;
			dev_info->snum_h = mdev->snum_h;
			dev_info->snum_l = mdev->snum_l;

			reg = ioread32be(&memory_map->hstat);
			dev_info->hardware_ver_num = (reg & HSTAT_VER_MASK) >> HSTAT_VER_SHIFT;

			dev_info->tx_frames         = ioread32be(&memory_map->tx_frames);
			dev_info->rx_frames         = ioread32be(&memory_map->rx_frames);
			dev_info->rx_errors         = ioread32be(&memory_map->rx_errors);
			dev_info->timeouts          = ioread32be(&memory_map->timeouts);
			dev_info->parity_errors     = ioread32be(&memory_map->parity_errors);
			dev_info->manchester_errors = ioread32be(&memory_map->manchester_errors);
			dev_info->wc_errors         = ioread32be(&memory_map->wc_errors);
			dev_info->tx_clash_errors   = ioread32be(&memory_map->tx_clash_errors);
			dev_info->nb_wds            = ioread32be(&memory_map->nb_wds);
			dev_info->rti_timeouts      = ioread32be(&memory_map->rti_timeouts);

			dev_info->icnt = mdev->icnt;
			dev_info->tx_count = mdev->tx_count;
			dev_info->isrdebug = wa.isrdebug;

			dev_info->quick_owned = atomic_read(&mdev->quick_owned);
			dev_info->quick_owner = mdev->quick_owner;
		break;

		case mil1553RAW_READ:          /** Raw read PCI registers */

			riob = mem;
			if (riob->regs > MAX_REGS) {
				cc = -EADDRNOTAVAIL;
				goto error_exit;
			}
			blen = riob->regs*sizeof(int);
			buf = kmalloc(blen,GFP_KERNEL);
			if (!buf) {
				kfree(mem);
				return -ENOMEM;
			}
			cnt = 0;
			bc = riob->bc;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				kfree(buf);
				goto error_exit;
			}
			cnt = raw_read(mdev, riob, buf);
			if (cnt)
				cc = copy_to_user(riob->buffer,
						  buf, blen);
			kfree(buf);
			if (!cnt || cc)
				goto error_exit;
		break;

		case mil1553RAW_WRITE:         /** Raw write PCI registers */

			riob = mem;
			if (riob->regs > MAX_REGS) {
				cc = -EADDRNOTAVAIL;
				goto error_exit;
			}
			blen = riob->regs*sizeof(int);
			buf = kmalloc(blen,GFP_KERNEL);
			if (!buf) {
				kfree(mem);
				return -ENOMEM;
			}
			cc = copy_from_user(buf, riob->buffer, blen);
			cnt = 0;
			bc = riob->bc;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				kfree(buf);
				goto error_exit;
			}
			cnt = raw_write(mdev, riob, buf);
			kfree(buf);
			if (!cnt || cc)
				goto error_exit;
		break;

		case mil1553GET_UP_RTIS:
			bc = *ularg;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			ping_rtis(mdev);
			*ularg = mdev->up_rtis;
		break;

		case mil1553SEND_RECEIVE:
			sr = mem;
			if ((mdev = get_dev(sr->bc)) == NULL) {
				cc = -EFAULT;
				goto error_exit;
			}
			cc = send_receive(mdev,
				sr->rti, sr->wc, sr->sa, sr->tr,
				sr->wants_reply,
				sr->rxbuf, sr->txbuf,
				&sr->received_wc);
		break;

		case mil1553LOCK_BC:
		        cc = 0;
			goto error_exit;
			bc = *ularg;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			if (client->bc_locked != 0) {
				cc = -EDEADLK;
				goto error_exit;
			}
			while (atomic_xchg(&mdev->quick_owned, 1)) {
				cc = wait_event_interruptible_timeout(mdev->quick_wq,
							atomic_read(&mdev->quick_owned) == 0,
							msecs_to_jiffies(5000));
				if (cc == 0)
					printk(KERN_ERR PFX "could not get lock of BC %d owned by pid %d\n",
						mdev->bc, mdev->quick_owner);
				if (signal_pending(current))
					return -ERESTARTSYS;
				return -EDEADLK;

			}
			mdev->quick_owner = current->pid;
			client->bc_locked = bc;
			return 0;
		break;

		case mil1553UNLOCK_BC:
		        cc = 0;
			goto error_exit;
			bc = *ularg;
			mdev = get_dev(bc);
			if (!mdev) {
				cc = -EFAULT;
				goto error_exit;
			}
			if (client->bc_locked != bc) {
				cc = -ENOLCK;
				goto error_exit;
			}
			client->bc_locked = 0;
			mdev->quick_owner = 0;
			BUG_ON(atomic_xchg(&mdev->quick_owned, 0) == 0);
			wake_up(&mdev->quick_wq);
			return 0;
		break;

		default:
			goto error_exit;
	}

	debug_ioctl(client->debug_level,ionr,iosz,iodr,mem,AFTER);

	if (iodr & _IOC_READ) {
		cc = copy_to_user((char *) arg, mem, iosz);
		if (cc)
			goto error_exit;
	}

	kfree(mem);
	return 0;

error_exit:
	kfree(mem);

	if ((client) && (client->debug_level > 4))
		printk("mil1553:Ioctl:%d:ErrorExit:%d\n",ionr,cc);

	if (cc < 0)
		return cc;
	return -EACCES;
}

/**
 * =========================================================
 */

long mil1553_ioctl_ulck(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int res;

	res = mil1553_ioctl(filp->f_dentry->d_inode, filp, cmd, arg);
	return res;
}

/**
 * =========================================================
 */

int mil1553_ioctl_lck(struct inode *inode, struct file *filp,
		      unsigned int cmd, unsigned long arg)
{
	int res;

	res = mil1553_ioctl(inode, filp, cmd, arg);
	return res;
}

/**
 * =========================================================
 */
struct file_operations mil1553_fops = {
	.owner          = THIS_MODULE,
	.ioctl          = mil1553_ioctl_lck,
	.unlocked_ioctl = mil1553_ioctl_ulck,
	.open           = mil1553_open,
	.release        = mil1553_close,
};

/**
 * =========================================================
 * Installer, hunt down modules and install them
 */

static struct dentry *dir;
static struct dentry *dbg_int_timeout;
static struct dentry *dbg_busy_timeout;
static struct dentry *dbg_clear_missed_int;

static void create_debugfs_flags(void)
{
	printk("creating debugfs entries\n");
	dir = debugfs_create_dir("cbmia", NULL);
	dbg_int_timeout = debugfs_create_u32("int_timeout", 0644, dir, &int_timeout);
	dbg_busy_timeout = debugfs_create_u32("busy_timeout", 0644, dir, &busy_timeout);
	dbg_clear_missed_int = debugfs_create_u32("clear_missed_int", 0644, dir, &clear_missed_int);
	printk("creating debugfs entries: %p %p %p %p\n", dir,
		dbg_int_timeout, dbg_busy_timeout, dbg_clear_missed_int);
}

static void remove_debugfs_flags(void)
{
	debugfs_remove(dbg_int_timeout);
	debugfs_remove(dbg_busy_timeout);
	debugfs_remove(dbg_clear_missed_int);
	debugfs_remove(dir);
}

int mil1553_install(void)
{
	int cc, i, bc = 0;
	struct pci_dev *pdev = NULL;
	struct mil1553_device_s *mdev;

	memset(&wa, 0, sizeof(struct working_area_s));
	create_debugfs_flags();

	if (check_args()) {

		cc = register_chrdev(mil1553_major, mil1553_major_name, &mil1553_fops);
		if (cc < 0)
			return cc;
		if (mil1553_major == 0)
			mil1553_major = cc; /* dynamic */

		for (i=0; i<MAX_DEVS; i++) {
			mdev = &wa.mil1553_dev[i];
			spin_lock_init(&mdev->lock);
			mdev->tx_queue = &wa.tx_queue[i];
			spin_lock_init(&mdev->tx_queue->lock);
			mutex_init(&mdev->bc_lock);

			mdev->pdev = add_next_dev(pdev,mdev);
			if (!mdev->pdev)
				break;

			bc = hunt_bc(mdev->pci_bus_num,mdev->pci_slt_num);
			printk("mil1553:Hunt:Bus:%d Slot:%d => ",
			       mdev->pci_bus_num,
			       mdev->pci_slt_num);

			if (bc) {
				printk("Found declared BC:%d\n",bc);
			} else {
				bc = get_unused_bc();
				printk("Assigned unused BC:%d\n",bc);
			}

			mdev->bc = bc;
			iowrite32be(CMD_RESET, &mdev->memory_map->cmd);
			init_device(mdev);
			init_waitqueue_head(&mdev->int_complete);
			init_waitqueue_head(&mdev->quick_wq);
			atomic_set(&mdev->int_busy, 0);
			atomic_set(&mdev->quick_owned, 0);
			mdev->quick_owner = 0;
			mutex_init(&mdev->mutex);
			mutex_init(&mdev->bcdev);
			ping_rtis(mdev);
			printk("BC:%d SerialNumber:0x%08X%08X\n",
				bc,mdev->snum_h,mdev->snum_l);
			pdev = mdev->pdev;
			wa.bcs++;
		}
	}
	printk("mil1553:Installed:%d Bus controllers\n",wa.bcs);
	return 0;
}

void mil1553_uninstall(void)
{
	int i;
	struct mil1553_device_s *mdev;

	remove_debugfs_flags();
	for (i=0; i<wa.bcs; i++) {
		mdev = &wa.mil1553_dev[i];
		release_device(mdev);
	}
	unregister_chrdev(mil1553_major,mil1553_major_name);
	printk("mil1553:Driver uninstalled\n");
}

module_init(mil1553_install);
module_exit(mil1553_uninstall);
