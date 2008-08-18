/*======================================================================

    Aironet driver for 4500 and 4800 series cards

    The contents of this file are subject to the Mozilla Public
    License Version 1.0 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    This code was developed by Benjamin Reed <breed@almaden.ibm.com>
    including portions of which come from the Aironet PC4500
    Developer's Reference Manual and used with permission.  Copyright
    (C) 1999 Benjamin Reed.  All Rights Reserved.  Permission to use
    code in the Developer's manual was granted for this driver by
    Aironet.
    
======================================================================*/

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/autoconf.h>
#ifdef CONFIG_MODVERSIONS
#define MODVERSIONS
#include <linux/modversions.h>
#endif                 

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/bitops.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/ioport.h>
#include <linux/config.h>
#ifdef CONFIG_PCI
#  include <linux/pci.h>

static struct {
	unsigned short vendor;
	unsigned short id;
} card_ids[] = { { 0x14b9, 1 },
		 { 0x14b9, 0x4500 },
		 { 0x14b9, 0x4800 },
		 { 0, 0 } 
};
#endif

/* As you can see this list is HUGH!
   I really don't know what a lot of these counts are about, but they
   are all here for completeness.  If the IGNLABEL macro is put in
   infront of the label, that statistic will not be included in the list
   of statistics in the /proc filesystem */

#define IGNLABEL 0&(int)
char *statsLabels[] = {
	"RxOverrun",
	IGNLABEL "RxPlcpCrcErr",
	IGNLABEL "RxPlcpFormatErr",
	IGNLABEL "RxPlcpLengthErr",
	"RxMacCrcErr",
	"RxMacCrcOk",
	"RxWepErr",
	"RxWepOk",
	"RetryLong",
	"RetryShort",
	"MaxRetries",
	"NoAck",
	"NoCts",
	"RxAck",
	"RxCts",
	"TxAck",
	"TxRts",
	"TxCts",
	"TxMc",
	"TxBc",
	"TxUcFrags",
	"TxUcPackets",
	"TxBeacon",
	"RxBeacon",
	"TxSinColl",
	"TxMulColl",
	"DefersNo",
	"DefersProt",
	"DefersEngy",
	"DupFram",
	"RxFragDisc",
	"TxAged",
	"RxAged",
	"LostSync-MaxRetry",
	"LostSync-MissedBeacons",
	"LostSync-ArlExceeded",
	"LostSync-Deauth",
	"LostSync-Disassoced",
	"LostSync-TsfTiming",
	"HostTxMc",
	"HostTxBc",
	"HostTxUc",
	"HostTxFail",
	"HostRxMc",
	"HostRxBc",
	"HostRxUc",
	"HostRxDiscard",
	IGNLABEL "HmacTxMc",
	IGNLABEL "HmacTxBc",
	IGNLABEL "HmacTxUc",
	IGNLABEL "HmacTxFail",
	IGNLABEL "HmacRxMc",
	IGNLABEL "HmacRxBc",
	IGNLABEL "HmacRxUc",
	IGNLABEL "HmacRxDiscard",
	IGNLABEL "HmacRxAccepted",
	"SsidMismatch",
	"ApMismatch",
	"RatesMismatch",
	"AuthReject",
	"AuthTimeout",
	"AssocReject",
	"AssocTimeout",
	IGNLABEL "ReasonOutsideTable",
	IGNLABEL "ReasonStatus1",
	IGNLABEL "ReasonStatus2",
	IGNLABEL "ReasonStatus3",
	IGNLABEL "ReasonStatus4",
	IGNLABEL "ReasonStatus5",
	IGNLABEL "ReasonStatus6",
	IGNLABEL "ReasonStatus7",
	IGNLABEL "ReasonStatus8",
	IGNLABEL "ReasonStatus9",
	IGNLABEL "ReasonStatus10",
	IGNLABEL "ReasonStatus11",
	IGNLABEL "ReasonStatus12",
	IGNLABEL "ReasonStatus13",
	IGNLABEL "ReasonStatus14",
	IGNLABEL "ReasonStatus15",
	IGNLABEL "ReasonStatus16",
	IGNLABEL "ReasonStatus17",
	IGNLABEL "ReasonStatus18",
	IGNLABEL "ReasonStatus19",
	"RxMan",
	"TxMan",
	"RxRefresh",
	"TxRefresh",
	"RxPoll",
	"TxPoll",
	"HostRetries",
	"LostSync-HostReq",
	"HostTxBytes",
	"HostRxBytes",
	"ElapsedUsec",
	"ElapsedSec",
	"LostSyncBetterAP",
	(char*)-1 };
#ifndef RUN_AT
#define RUN_AT(x) (jiffies+(x))
#endif


/* These two variables are for insmod, since it seems that the rates
   can only be set in setup_card.  Rates should be a comma separated
   (no spaces) list of rates (up to 8). */

int rates[8] = {0,0,0,0,0,0,0,0};
int basic_rate = 0;
char *ssids[3] = {0,0,0};

int io[4]={ 0,};
int irq[4]={ 0,};

int auto_wep = 0; /* If set, it tries to figure out the wep mode */


#if (LINUX_VERSION_CODE > 0x20155)
/* new kernel */
MODULE_PARM(io,"1-4i");
MODULE_PARM(irq,"1-4i");
MODULE_PARM(basic_rate,"i");
MODULE_PARM(rates,"1-8i");
MODULE_PARM(ssids,"1-3s");
MODULE_PARM(auto_wep,"i");
/*
 *  Im pretty lazy about the locking.  In theory having just one lock for the
 *  whole driver should be okay, but I should actually have one driver for
 *  each interface.  Since there will probably be only one interface this isnt
 *  a big deal.  Once I know it works, I will put the lock in the airo_priv
 *  structure.
 */
#include <asm/uaccess.h>
#define DO_SPIN_LOCK
// This line will generate a warning on uniprocessor machines, just ignore
static spinlock_t xxx_lock = SPIN_LOCK_UNLOCKED;

#define KFREE_SKB(a,b)  dev_kfree_skb(a)
#define PROC_REGISTER(a,b) proc_register(a,b)
#else 
/* old kernel */
#define le32_to_cpu(x) (x)
#define cpu_to_le32(x) (x)
#define le16_to_cpu(x) (x)
#define cpu_to_le16(x) (x)
#include <linux/bios32.h>
#define net_device_stats enet_statistics
#define KFREE_SKB(a,b)  dev_kfree_skb(a,b)
#define PROC_REGISTER(a,b) proc_register_dynamic(a,b)
#endif 

#define min(x,y) ((x<y)?x:y)

#define isalnum(x) ((x>='a'&&x<='z')||(x>='A'&&x<='Z')||(x>='0'&&x<='9'))

/* This is a kind of sloppy hack to get this information to OUT4500 and
   IN4500.  I would be extremely interested in the situation where this
   doesnt work though!!! */
static int do8bitIO = 0;

/* Return codes */
#define SUCCESS 0
#define ERROR -1
#define NO_PACKET -2

/* Commands */
#define NOP 0x0010
#define MAC_ENABLE 0x0001
#define MAC_DISABLE 0x0002
#define CMD_ACCESS 0x0021
#define CMD_ALLOCATETX 0x000a
#define CMD_TRANSMIT 0x000b
#define HOSTSLEEP 0x85
#define CMD_SETMODE 0x0009

/* Registers */
#define COMMAND 0x00
#define PARAM0 0x02
#define PARAM1 0x04
#define PARAM2 0x06
#define STATUS 0x08
#define RESP0 0x0a
#define RESP1 0x0c
#define RESP2 0x0e
#define LINKSTAT 0x10
#define SELECT0 0x18
#define OFFSET0 0x1c
#define RXFID 0x20
#define TXALLOCFID 0x22
#define TXCOMPLFID 0x24
#define DATA0 0x36
#define EVSTAT 0x30
#define EVINTEN 0x32
#define EVACK 0x34
#define TXCOMPLFID 0x24

/* BAP selectors */
#define BAP0 0
#define BAP1 2

/* Flags */
#define COMMAND_BUSY 0x8000

#define BAP_BUSY 0x8000
#define BAP_ERR 0x4000
#define BAP_DONE 0x2000

#define PROMISC 0xffff

#define EV_CMD 0x10
#define EV_CLEARCOMMANDBUSY 0x4000
#define EV_RX 0x01
#define EV_TX 0x02
#define EV_TXEXC 0x04
#define EV_ALLOC 0x08
#define EV_LINK 0x80
#define EV_AWAKE 0x100
#define STATUS_INTS ( EV_AWAKE | EV_LINK | EV_TXEXC | EV_TX | EV_RX )

/* The RIDs */
#define RID_WEP_PERM 0xFF16
#define RID_WEP_TEMP 0xFF15
#define RID_SSID     0xFF11
#define RID_CONFIG   0xFF10
#define RID_ACTUALCONFIG 0xFF20 /*readonly*/
#define RID_MODULATION 0xFF17
#define RID_STATS 0xFF68
#define RID_STATSDELTA 0xFF69
#define RID_STATSDELTACLEAR 0xFF6A

/*
 * Rids and endian-ness:  The Rids will always be little endian, since
 * this is what the card wants.  So any assignments to are from the
 * rids need to be converted to the correct endian.
 */

/* This structure came from an email sent to me from an engineer at
   aironet for inclusion into this driver */
typedef struct {
	u16 len;
	u16 kindex;
	u8 mac[6];
	u16 klen;
	u8 key[5];  /* 40-bit keys */
} WepKeyRid;

/* These structures are from the Aironet's PC4500 Developers Manual */
typedef struct {
	u16 len;
	u8 ssid[32];
} Ssid;

typedef struct {
	u16 len;
	Ssid ssids[3];
} SsidRid;

typedef struct {
        u16 len;
        u16 modulation;
#define MOD_DEFAULT 0
#define MOD_CCK 1
#define MOD_MOK 2
} ModulationRid;

typedef struct {
	u16 cmd;
	u16 parm0;
	u16 parm1;
	u16 parm2;
} Cmd;

typedef struct {
	u16 status;
	u16 rsp0;
	u16 rsp1;
	u16 rsp2;
} Resp;

typedef struct {
	u16 len; /* sizeof(ConfigRid) */
	u16 opmode; /* operating mode */
#define MODE_STA_IBSS 0
#define MODE_STA_ESS 1
#define MODE_AP 2
#define MODE_AP_RPTR 3
#define MODE_ETHERNET_HOST (0<<8) /* rx payloads converted */
#define MODE_LLC_HOST (1<<8) /* rx payloads left as is */
#define MODE_AIRONET_EXTEND (1<<9) /* enable Aironet extenstions */
#define MODE_AP_INTERFACE (1<<10) /* enable ap interface extensions */
	u16 rmode; /* receive mode */
#define RXMODE_BC_MC_ADDR 0
#define RXMODE_BC_ADDR 1 /* ignore multicasts */
#define RXMODE_ADDR 2 /* ignore multicast and broadcast */
#define RXMODE_RFMON 3 /* wireless monitor mode */
#define RXMODE_RFMON_ANYBSS 4
#define RXMODE_LANMON 5 /* lan style monitor -- data packets only */
#define RXMODE_DISABLE_802_3_HEADER (1<<8) /* disables 802.3 header on rx */
	u16 fragThresh;
	u16 rtsThres;
	u8 macAddr[6];
	u8 rates[8];
	u16 shortRetryLimit;
	u16 longRetryLimit;
	u16 txLifetime; /* in kusec */
	u16 rxLifetime; /* in kusec */
	u16 stationary;
	u16 ordering;
	u16 u16deviceType; /* for overriding device type */
	u16 _reserved1[5];
	/*---------- Scanning/Associating ----------*/
	u16 scanMode;
#define SCANMODE_ACTIVE 0
#define SCANMODE_PASSIVE 1
#define SCANMODE_AIROSCAN 2
	u16 probeDelay; /* in kusec */
	u16 probeEnergyTimeout; /* in kusec */
        u16 probeResponseTimeout;
	u16 beaconListenTimeout;
	u16 joinNetTimeout;
	u16 authTimeout;
	u16 authType;
#define AUTH_OPEN 0x1
#define AUTH_ENCRYPT 0x101
#define AUTH_SHAREDKEY 0x102
	u16 associationTimeout;
	u16 specifiedApTimeout;
	u16 offlineScanInterval;
	u16 offlineScanDuration;
	u16 linkLossDelay;
	u16 maxBeaconLostTime;
	u16 refreshInterval;
#define DISABLE_REFRESH 0xFFFF
	u16 _reserved1a[1];
	/*---------- Power save operation ----------*/
	u16 powerSaveMode;
#define POWERSAVE_CAM 0
#define POWERSAVE_PSP 1
#define POWERSAVE_PSPCAM 2
	u16 sleepForDtims;
	u16 listenInterval;
	u16 fastListenInterval;
	u16 listenDecay;
	u16 fastListenDelay;
	u16 _reserved2[2];
	/*---------- Ap/Ibss config items ----------*/
	u16 beaconPeriod;
	u16 atimDuration;
	u16 hopPeriod;
	u16 channelSet;
	u16 channel;
	u16 dtimPeriod;
	u16 _reserved3[2];
	/*---------- Radio configuration ----------*/
	u16 radioType;
#define RADIOTYPE_DEFAULT 0
#define RADIOTYPE_802_11 1
#define RADIOTYPE_LEGACY 2
	u8 rxDiversity;
	u8 txDiversity;
	u16 txPower;
#define TXPOWER_DEFAULT 0
	u16 rssiThreshold;
#define RSSI_DEFAULT 0
        u16 modulation;
	u16 radioSpecific[3];
	/*---------- Aironet Extensions ----------*/
	u8 nodeName[16];
	u16 arlThreshold;
	u16 arlDecay;
	u16 arlDelay;
	u16 _reserved4[1];
	/*---------- Aironet Extensions ----------*/
	u16 magicAction;
#define MAGIC_ACTION_STSCHG 1
#define MACIC_ACTION_RESUME 2
#define MAGIC_IGNORE_MCAST (1<<8)
#define MAGIC_IGNORE_BCAST (1<<9)
#define MAGIC_SWITCH_TO_PSP (0<<10)
#define MAGIC_STAY_IN_CAM (1<<10)
	u16 filler;
} ConfigRid;

typedef struct {
	u16 len;
	u8 mac[6];
	u16 mode;
	u16 errorCode;
	u16 sigQuality;
	u16 SSIDlen;
	char SSID[32];
	char apName[16];
	char bssid[4][6];
	u16 beaconPeriod;
	u16 dimPeriod;
	u16 atimDuration;
	u16 hopPeriod;
	u16 channelSet;
	u16 channel;
	u16 hopsToBackbone;
	u16 apTotalLoad;
	u16 generatedLoad;
} StatusRid;

typedef struct {
	u16 len;
	char oui[3];
	u16 prodNum;
	char manName[32];
	char prodName[16];
	char prodVer[8];
	char factoryAddr[6];
	char aironetAddr[6];
	u16 radioType;
	u16 country;
	char callid[6];
	char supportedRates[8];
	char rxDiversity;
	char txDiversity;
	u16 txPowerLevels[8];
	u16 hardVer;
	u16 hardCap;
	u16 tempRange;
	u16 softVer;
	u16 softSubVer;
	u16 interfaceVer;
	u16 softCap;
	u16 bootBlockVer;
} CapabilityRid;

#define TXCTL_TXOK (1<<1) /* report if tx is ok */
#define TXCTL_TXEX (1<<2) /* report if tx fails */
#define TXCTL_802_3 (0<<3) /* 802.3 packet */
#define TXCTL_802_11 (1<<3) /* 802.11 mac packet */
#define TXCTL_ETHERNET (0<<4) /* payload has ethertype */
#define TXCTL_LLC (1<<4) /* payload is llc */
#define TXCTL_RELEASE (0<<5) /* release after completion */
#define TXCTL_NORELEASE (1<<5) /* on completion returns to host */

#define BUSY_FID 0x10000

static char *version =
"airo.c 0.98so 1900(:^)/01/09 03:26:30 (Benjamin Reed)";

struct airo_info;

static int get_dec_u16( char *buffer, int *start, int limit );
static void OUT4500( struct airo_info *, u16 register, u16 value );
static unsigned short IN4500( struct airo_info *, u16 register );
static u16 setup_card(struct airo_info*, u8 *mac, ConfigRid *);
static void enable_interrupts(struct airo_info*);
static void disable_interrupts(struct airo_info*);
static u16 issuecommand(struct airo_info*, Cmd *pCmd, Resp *pRsp);
static int bap_setup(struct airo_info*, u16 rid, u16 offset, int whichbap);
static int bap_read(struct airo_info*, u16 *pu16Dst, int bytelen, 
		    int whichbap);
static int bap_write(struct airo_info*, const u16 *pu16Src, int bytelen,
		     int whichbap);
static int PC4500_accessrid(struct airo_info*, u16 rid, u16 accmd);
static int PC4500_readrid(struct airo_info*, u16 rid, void *pBuf, int len);
static int PC4500_writerid(struct airo_info*, u16 rid, const void
			   *pBuf, int len);
static int do_writerid( struct airo_info*, u16 rid, const void *rid_data, 
			int len );
static u16 transmit_allocate(struct airo_info*, int lenPayload);
static int transmit_802_3_packet(struct airo_info*, u16 TxFid, char
				 *pPacket, int len);

static void airo_interrupt( int irq, void* dev_id, struct pt_regs *regs);

struct airo_info {
	struct net_device_stats	stats;
	int open;
	char name[8];
	struct device                 *dev;
	/* Note, we can have MAX_FIDS outstanding.  FIDs are 16-bits, so we
	   use the high bit to mark wether it is in use. */
#define MAX_FIDS 6
	int                           fids[MAX_FIDS];
	int registered;
	ConfigRid config;
	u16 authtype;
	struct timer_list timer;
	u16 rateFreq;
	struct proc_dir_entry proc_entry;
	struct proc_dir_entry proc_statsdelta_entry;
	struct proc_dir_entry proc_stats_entry;
	struct proc_dir_entry proc_status_entry;
	struct proc_dir_entry proc_config_entry;
	struct proc_dir_entry proc_SSID_entry;
	struct proc_dir_entry proc_wepkey_entry;
	struct airo_info *next;
        int flags;
#define FLAG_PROMISC 0x01
#define FLAG_RADIO_OFF 0x02
};

static int setup_proc_entry( struct device *dev,
			     struct airo_info *apriv );
static int takedown_proc_entry( struct device *dev,
				struct airo_info *apriv );


static int airo_init( struct device *dev ) {
	return 0;
}

static int airo_open(struct device *dev) {
	struct airo_info *info = dev->priv;

	MOD_INC_USE_COUNT;
	if ( info->open == 0 ) {
		enable_interrupts(info);
	}
	info->open++;
	
	dev->interrupt = 0; dev->tbusy = 0; dev->start = 1;
	
	return 0;
}

static int airo_start_xmit(struct sk_buff *skb, struct device *dev) {
	s16 len;
	s16 retval = 0;
	u16 status;
	u32 flags;
	s8 *buffer;
	int i;
	struct airo_info *priv = (struct airo_info*)dev->priv;
	u32 *fids = priv->fids;
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	
	if ( dev->tbusy ) {
		retval = -EBUSY;
		goto tx_done;
	}
	
	/* Find a vacant FID */
	for( i = 0; i < MAX_FIDS; i++ ) {
		if ( !( fids[i] & 0xffff0000 ) ) break;
	}
	
	if ( i == MAX_FIDS ) {
		dev->tbusy = 1;
		retval = -EBUSY;
		goto tx_done;
	}
	
	if ( skb == NULL ) {
		printk( KERN_INFO "skb == NULL!!!\n" );
		return 0;
	}
	
	len = ETH_ZLEN < skb->len ? skb->len : ETH_ZLEN; /* check min length*/
	buffer = skb->data;
	
	
	
	status = transmit_802_3_packet( priv, 
					fids[i],
					skb->data, len );
	
	if ( status == SUCCESS ) {
                /* Mark fid as used & save length for later */
		fids[i] |= (len << 16); 
		dev->trans_start = jiffies;
	} else {
		((struct airo_info*)dev->priv)->stats.tx_errors++;
	}
 tx_done:
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
	KFREE_SKB( skb, FREE_WRITE );
	return 0;
}

static struct net_device_stats *airo_get_stats(struct device *dev) {
	return &(((struct airo_info*)dev->priv)->stats);
}

static void airo_set_multicast_list(struct device *dev) {
        struct airo_info *ai = (struct airo_info*)dev->priv;

        ai->flags &= ~FLAG_PROMISC;
	if (dev->flags&IFF_PROMISC) {
	        ai->flags |= FLAG_PROMISC;
	} else if ((dev->flags&IFF_ALLMULTI)||dev->mc_count>0) {
		/* Turn on multicast.  (Should be already setup...) */
	}
}

static int private_ioctl(struct device *dev, struct ifreq *rq, 
			 int cmd) {
	return 0;
}

static int airo_close(struct device *dev) { 
	struct airo_info *ai = (struct airo_info*)dev->priv;
	ai->open--;
	if ( !ai->open ) {
		dev->start = 0;
		disable_interrupts( ai );
	}
	MOD_DEC_USE_COUNT;
	return 0;
}

static void del_airo_dev( struct device *dev );

void stop_airo_card( struct device *dev ) 
{
	struct airo_info *ai = (struct airo_info*)dev->priv;
	takedown_proc_entry( dev, ai );
	if (ai->registered) {
		unregister_netdev( dev );
		ai->registered = 0;
	}
	disable_interrupts(ai);
	release_region( dev->base_addr, 64 );
	free_irq( dev->irq, dev );
	del_airo_dev( dev );
	if (dev->priv)
		kfree(dev->priv);
	kfree( dev );
	if (auto_wep) del_timer(&(ai)->timer);
}

static void add_airo_dev( struct device *dev ); 

struct device *init_airo_card( unsigned short irq, int port )
{
	struct device *dev;
	struct airo_info *ai;
	int i;
	
	/* Create the network device object. */
	dev = kmalloc(sizeof(struct device), GFP_KERNEL);
	memset(dev, 0, sizeof(struct device));
	
	/* Space for the info structure. */
	dev->priv = kmalloc(sizeof(struct airo_info), GFP_KERNEL);
	memset(dev->priv, 0, sizeof(struct airo_info));
	ai = (struct airo_info *)dev->priv;
        ai->dev = dev;
	add_airo_dev( dev ); 
	
	/* The Airo-specific entries in the device structure. */
	dev->hard_start_xmit = &airo_start_xmit;
	dev->get_stats = &airo_get_stats;
	dev->set_multicast_list = &airo_set_multicast_list;
	dev->do_ioctl = &private_ioctl;
	dev->name = ((struct airo_info *)dev->priv)->name;
	ether_setup(dev);
	dev->init = &airo_init;
	dev->open = &airo_open;
	dev->stop = &airo_close;
	dev->tbusy = 1;
	
	dev->irq = irq;
	dev->base_addr = port;
	
	if ( request_irq( dev->irq, airo_interrupt, 
			  SA_SHIRQ | SA_INTERRUPT, dev->name, dev ) ) {
		printk(KERN_ERR "airo_common: register interrupt %d failed\n", irq );
	}
	request_region( dev->base_addr, 64, dev->name );
	
	memset( &((struct airo_info*)dev->priv)->config, 0,
		sizeof(((struct airo_info*)dev->priv)->config));
	
	if ( setup_card( ai, dev->dev_addr, 
			 &((struct airo_info*)dev->priv)->config) 
	     != SUCCESS ) {
		printk( KERN_INFO "Aironet MAC could not be enabled\n" );
		goto init_undo;
	} else {
		printk( KERN_INFO "Aironet MAC enabled %s %x:%x:%x:%x:%x:%x\n",
			dev->name,
			dev->dev_addr[0],
			dev->dev_addr[1],
			dev->dev_addr[2],
			dev->dev_addr[3],
			dev->dev_addr[4],
			dev->dev_addr[5]
			);
		/* Allocate the transmit buffers */
		for( i = 0; i < MAX_FIDS; i++ ) {
			ai->fids[i] = transmit_allocate( ai, 2000 );
		}
	}
	
	if (register_netdev(dev) != 0) {
		printk(KERN_ERR "airo_common: register_netdev() failed\n");
		goto init_undo;
	}
	((struct airo_info*)dev->priv)->registered = 1;
	
	setup_proc_entry( dev, (struct airo_info*)dev->priv );
	
	dev->tbusy = 0;
	return dev;
 init_undo:
	stop_airo_card( dev );
	return dev;
}

int reset_airo_card( struct device *dev ) {
	int i;
	struct airo_info *ai = (struct airo_info*)dev->priv;

	if ( setup_card(ai, dev->dev_addr,
			&(ai)->config) != SUCCESS ) {
		printk( KERN_INFO "Aironet MAC could not be enabled\n" );
		return -1;
	} else {
		printk( KERN_INFO "Aironet MAC enabled %s %x:%x:%x:%x:%x:%x\n",
			dev->name,
			dev->dev_addr[0],
			dev->dev_addr[1],
			dev->dev_addr[2],
			dev->dev_addr[3],
			dev->dev_addr[4],
			dev->dev_addr[5]
			);
		/* Allocate the transmit buffers */
		for( i = 0; i < MAX_FIDS; i++ ) {
			((struct airo_info*)dev->priv)->fids[i] =
				transmit_allocate( ai, 2000 );
		}
	}
	enable_interrupts( ai );
	dev->interrupt = 0; dev->tbusy = 0; dev->start = 1;
	return 0;
}

static void airo_interrupt ( int irq, void* dev_id, struct pt_regs *regs) {
	struct device *dev = (struct device *)dev_id;
	u16 len;
	u16 status;
	u16 fid;
	struct airo_info *apriv = (struct airo_info *)dev->priv;
	u16 savedInterrupts;
	

	if ((dev == NULL) || !dev->start )
		return;
	
	if (dev->interrupt) {
		printk( KERN_INFO 
			"%s: re-entering the interrupt handler.\n", dev->name);
		return;
	}
	dev->interrupt = 1;
	
	status = IN4500( apriv, EVSTAT );
	if ( !status ) return;
	
	if ( status & EV_AWAKE ) {
		OUT4500( apriv, EVACK, EV_AWAKE );
		OUT4500( apriv, EVACK, EV_AWAKE );
	}
	
	savedInterrupts = IN4500( apriv, EVINTEN );
	OUT4500( apriv, EVINTEN, 0 );
	
	if ( status & EV_LINK ) {
		/* The link status has changed, if you want to put a
		   monitor hook in, do it here.  (Remember that
		   interrupts are still disabled!)
		*/
		/*u16 newStatus = */IN4500(apriv, LINKSTAT);
		/* Here is what newStatus means: */
#define NOBEACON 0x8000 /* Loss of sync - missed beacons */
#define MAXRETRIES 0x8001 /* Loss of sync - max retries */
#define MAXARL 0x8002 /* Loss of sync - average retry level exceeded*/
#define FORCELOSS 0x8003 /* Loss of sync - host request */
#define TSFSYNC 0x8004 /* Loss of sync - TSF synchronization */
#define DEAUTH 0x8100 /* Deauthentication (low byte is reason code) */
#define DISASS 0x8200 /* Disassociation (low byte is reason code) */
#define ASSFAIL 0x8400 /* Association failure (low byte is reason
			  code) */
#define AUTHFAIL 0x0300 /* Authentication failure (low byte is reason
			   code) */
#define ASSOCIATED 0x0400 /* Assocatied */
#define RC_RESERVED 0 /* Reserved return code */
#define RC_NOREASON 1 /* Unspecified reason */
#define RC_AUTHINV 2 /* Previous authentication invalid */
#define RC_DEAUTH 3 /* Deauthenticated because sending station is
		       leaving */
#define RC_NOACT 4 /* Disassociated due to inactivity */
#define RC_MAXLOAD 5 /* Disassociated because AP is unable to handle
			all currently associated stations */
#define RC_BADCLASS2 6 /* Class 2 frame received from
			  non-Authenticated station */
#define RC_BADCLASS3 7 /* Class 3 frame received from
			  non-Associated station */
#define RC_STATLEAVE 8 /* Disassociated because sending station is
			  leaving BSS */
#define RC_NOAUTH 9 /* Station requesting (Re)Association is not
		       Authenticated with the responding station */
	}
	
	/* Check to see if there is something to recieve */
	if ( status & EV_RX  ) {
		struct sk_buff *skb;
		fid = IN4500( apriv, RXFID );

		/* Get the bit rate */
		bap_setup( apriv, fid, 0x0a, BAP0 );
		bap_read( apriv, &apriv->rateFreq, 
			  sizeof(apriv->rateFreq), BAP0 );
		apriv->rateFreq = le16_to_cpu(apriv->rateFreq);
	      
		/* Get the packet length */
		bap_setup( apriv, fid, 0x36, BAP0 );
		bap_read( apriv, &len, sizeof(len), BAP0 );

		len = le16_to_cpu(len);
                /* The length only counts the payload
		   not the hw addresses */
		len += 12; 
		if ( len < 12 || len > 2048 ) {
#if LINUX_VERSION_CODE > 0x20127
			apriv->stats.rx_length_errors++;
#endif
			apriv->stats.rx_errors++;
			printk( KERN_INFO 
				"Bad size %d\n", len );
		} else {
			skb = dev_alloc_skb( len + 2 );
			if ( !skb ) {
				apriv->stats.rx_dropped++;
			} else {
				char *buffer;
				buffer = skb_put( skb, len );
				bap_read( apriv, (u16*)buffer, len, BAP0 );
				apriv->stats.rx_packets++;
#if LINUX_VERSION_CODE > 0x20127
				apriv->stats.rx_bytes += len;
#endif
				skb->dev = dev;
				skb->ip_summed = CHECKSUM_NONE;
				skb->protocol = eth_type_trans( skb, dev );
				netif_rx( skb );
			}
		}
	}

	/* Check to see if a packet has been transmitted */
	if (  status & ( EV_TX|EV_TXEXC ) ) {
		int i;
		int len = 0;
		
		fid = IN4500(apriv, TXCOMPLFID);
		
		for( i = 0; i < MAX_FIDS; i++ ) {
			if ( ( apriv->fids[i] & 0xffff ) == fid ) {
				len = apriv->fids[i] >> 16;
				/* Set up to be used again */
				apriv->fids[i] &= 0xffff; 
				dev->tbusy = 0;
				break;
			}
		}
		if ( i == MAX_FIDS ) {
			printk( KERN_INFO 
				"Unallocated FID was used to xmit\n" );
		}
		if ( status & EV_TX ) {
			apriv->stats.tx_packets++;
#if LINUX_VERSION_CODE > 0x20127
			if( i != MAX_FIDS) 
				apriv->stats.tx_bytes += len;
#endif
		} else {
			apriv->stats.tx_errors++;
		}
		mark_bh( NET_BH );
	}
	if ( status & ~STATUS_INTS ) 
		printk( KERN_INFO 
			"Got weird status %x\n", 
			status & ~STATUS_INTS );
	OUT4500( apriv, EVACK, status & STATUS_INTS );
	OUT4500( apriv, EVINTEN, savedInterrupts );
	
	/* done.. */
	dev->interrupt = 0;
	return;     
}

/*
 *  Routines to talk to the card
 */

/*
 *  This was originally written for the 4500, hence the name
 */
static void OUT4500( struct airo_info *ai, u16 reg, u16 val ) {
	val = cpu_to_le16(val);
	if ( !do8bitIO )
		outw( val, ai->dev->base_addr + reg );
	else {
		outb( val & 0xff, ai->dev->base_addr + reg );
		outb( val >> 8, ai->dev->base_addr + reg + 1 );
	}
}

static u16 IN4500( struct airo_info *ai, u16 reg ) {
	unsigned short rc;
	
	if ( !do8bitIO )
		rc = inw( ai->dev->base_addr + reg );
	else {
		rc = inb( ai->dev->base_addr + reg );
		rc += ((int)inb( ai->dev->base_addr + reg + 1 )) << 8;
	}
	return le16_to_cpu(rc);
}

static int enable_MAC( struct airo_info *ai, Resp *rsp ) {
        Cmd cmd;
	int status;

        if (ai->flags&FLAG_RADIO_OFF) return SUCCESS;
	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = MAC_ENABLE; // disable in case already enabled
	status = issuecommand(ai, &cmd, rsp);
	if (status == SUCCESS && ai->flags|FLAG_PROMISC) {
	        Cmd cmd;
		Resp rsp;
		memset(&cmd, 0, sizeof(cmd));
		cmd.cmd=CMD_SETMODE;
		cmd.parm1=PROMISC;
		issuecommand(ai, &cmd, &rsp);
	}
	return status;
}

static void disable_MAC( struct airo_info *ai ) {
        Cmd cmd;
	Resp rsp;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = MAC_DISABLE; // disable in case already enabled
	issuecommand(ai, &cmd, &rsp);
}

static void enable_interrupts( struct airo_info *ai ) {
	/* Reset the status register */
	u16 status = IN4500( ai, EVSTAT );
	OUT4500( ai, EVACK, status );
	/* Enable the interrupts */
	OUT4500( ai, EVINTEN, STATUS_INTS );
	/* Note there is a race condition between the last two lines that
	   I dont know how to get rid of right now... */
}

static void disable_interrupts( struct airo_info *ai ) {
	OUT4500( ai, EVINTEN, 0 );
}

static u16 setup_card(struct airo_info *ai, u8 *mac, 
		      ConfigRid *config)
{
	Cmd cmd; 
	Resp rsp;
	ConfigRid cfg;
	int status;
	int i;
	SsidRid mySsid;

	memset( &mySsid, 0, sizeof( mySsid ) );

	/* The NOP is the first step in getting the card going */
	cmd.cmd = NOP;
	cmd.parm0 = cmd.parm1 = cmd.parm2 = 0;
	if ( issuecommand( ai, &cmd, &rsp ) != SUCCESS ) {
		return ERROR;
	}
	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = MAC_DISABLE; // disable in case already enabled
	if ( issuecommand( ai, &cmd, &rsp ) != SUCCESS ) {
		return ERROR;
	}
	
	if ( config->len ) {
		cfg = *config;
	} else {
		// general configuration (read/modify/write)
		status = PC4500_readrid(ai, RID_CONFIG, &cfg, sizeof(cfg));
		if ( status != SUCCESS ) return ERROR;
		cfg.opmode = MODE_STA_ESS; // station in ESS mode
    
		/* Save off the MAC */
		for( i = 0; i < 6; i++ ) {
			mac[i] = cfg.macAddr[i];
		}

		/* Check to see if there are any insmod configured 
		   rates to add */
		if ( rates ) {
			int i = 0;
			if ( rates[0] ) memset(cfg.rates,0,sizeof(cfg.rates));
			for( i = 0; i < 8 && rates[i]; i++ ) {
				cfg.rates[i] = rates[i];
			}    
		}
		if ( basic_rate > 0 ) {
			int i;
			for( i = 0; i < 8; i++ ) {
				if ( cfg.rates[i] == basic_rate ||
				     !cfg.rates ) {
					cfg.rates[i] = basic_rate | 0x80;
					break;
				}
			}
		}
		*config = cfg;
	}
	
	/* Setup the SSIDs if present */
	if ( ssids[0] ) {
		int i = 0;
		for( i = 0; i < 3 && ssids[i]; i++ ) {
			mySsid.ssids[i].len = strlen(ssids[i]);
			if ( mySsid.ssids[i].len > 32 ) 
				mySsid.ssids[i].len = 32;
			memcpy(mySsid.ssids[i].ssid, ssids[i],
			       mySsid.ssids[i].len);
			mySsid.ssids[i].len = cpu_to_le16(mySsid.ssids[i].len);
		}
	}
	
	status = PC4500_writerid( ai, RID_CONFIG, &cfg, sizeof(cfg));
	if ( status != SUCCESS ) return ERROR;
	
	/* Set up the SSID list */
	status = PC4500_writerid(ai, RID_SSID, &mySsid, sizeof(mySsid));
	if ( status != SUCCESS ) return ERROR;
	
	status = enable_MAC(ai, &rsp);
	if (status != SUCCESS || (rsp.status & 0xFF00) != 0) {
		int reason = rsp.rsp0;
		int badRidNumber = rsp.rsp1;
		int badRidOffset = rsp.rsp2;
		printk( KERN_INFO 
			"Bad MAC enable reason = %x, rid = %x, offset = %d\n",
			reason,
			badRidNumber,
			badRidOffset );
		return ERROR;
	}
	return SUCCESS;
}

static u16 issuecommand(struct airo_info *ai, Cmd *pCmd, Resp *pRsp) {
        // Im really paranoid about letting it run forever!
	int max_tries = 600000;  

	OUT4500(ai, PARAM0, pCmd->parm0);
	OUT4500(ai, PARAM1, pCmd->parm1);
	OUT4500(ai, PARAM2, pCmd->parm2);
	OUT4500(ai, COMMAND, pCmd->cmd);
	while ( max_tries-- &&
		(IN4500(ai, EVSTAT) & EV_CMD) == 0) {
		if ( IN4500(ai, COMMAND) == pCmd->cmd) { 
			// PC4500 didn't notice command, try again
			OUT4500(ai, COMMAND, pCmd->cmd);
		}
	}
	if ( max_tries == -1 ) {
		printk( KERN_INFO 
			"Max tries exceeded when issueing command\n" );
		return ERROR;
	}
	// command completed
	pRsp->status = IN4500(ai, STATUS);
	pRsp->rsp0 = IN4500(ai, RESP0);
	pRsp->rsp1 = IN4500(ai, RESP1);
	pRsp->rsp2 = IN4500(ai, RESP2);
	
	// clear stuck command busy if necessary
	if (IN4500(ai, COMMAND) & COMMAND_BUSY) {
		OUT4500(ai, EVACK, EV_CLEARCOMMANDBUSY);
	}
	// acknowledge processing the status/response
	OUT4500(ai, EVACK, EV_CMD);
	return SUCCESS;
}

/* Sets up the bap to start exchange data.  whichbap should
 * be one of the BAP0 or BAP1 defines */
static int bap_setup(struct airo_info *ai, u16 rid, u16 offset, int whichbap )
{
	int timeout = 50;
	int max_tries = 3;
	
	OUT4500(ai, SELECT0+whichbap, rid);
	OUT4500(ai, OFFSET0+whichbap, offset);
	while (1) {
		int status = IN4500(ai, OFFSET0+whichbap);
		if (status & BAP_BUSY) {
                        /* This isn't really a timeout, but its kinda
			   close */
			if (timeout--) { 
				continue;
			}
		} else if ( status & BAP_ERR ) {
			/* invalid rid or offset */
			printk( KERN_INFO "BAP error %x %d\n", 
				status, whichbap );
			return ERROR;
		} else if (status & BAP_DONE) { // success
			return SUCCESS;
		}
		if ( !(max_tries--) ) {
			printk( KERN_INFO 
				"BAP setup error too many retries\n" );
			return ERROR;
		}
		// -- PC4500 missed it, try again
		OUT4500(ai, SELECT0+whichbap, rid);
		OUT4500(ai, OFFSET0+whichbap, offset);
		timeout = 50;
	}
}

/* requires call to bap_setup() first */
static int bap_read(struct airo_info *ai, u16 *pu16Dst, int bytelen, 
		    int whichbap)
{
	bytelen = (bytelen + 1) & (~1); // round up to even value
	if ( !do8bitIO ) 
		insw( ai->dev->base_addr+DATA0+whichbap, pu16Dst, bytelen>>1 );
	else
		insb( ai->dev->base_addr+DATA0+whichbap, pu16Dst, bytelen );
	return SUCCESS;
}

/* requires call to bap_setup() first */
static int bap_write(struct airo_info *ai, const u16 *pu16Src, 
		     int bytelen, int whichbap)
{
	bytelen = (bytelen + 1) & (~1); // round up to even value
	if ( !do8bitIO ) 
		outsw( ai->dev->base_addr+DATA0+whichbap, 
		       pu16Src, bytelen>>1 );
	else
		outsb( ai->dev->base_addr+DATA0+whichbap, pu16Src, bytelen );
	return SUCCESS;
}

static int PC4500_accessrid(struct airo_info *ai, u16 rid, u16 accmd)
{
	Cmd cmd; /* for issuing commands */
	Resp rsp; /* response from commands */
	u16 status;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = accmd;
	cmd.parm0 = rid;
	status = issuecommand(ai, &cmd, &rsp);
	if (status != 0) return status;
	if ( (rsp.status & 0x7F00) != 0) {
		return (accmd << 8) + (rsp.rsp0 & 0xFF);
	}
	return 0;
}

/*  Note, that we are using BAP1 which is also used by transmit, so
 *  make sure this isnt called when a transmit is happening */
static int PC4500_readrid(struct airo_info *ai, u16 rid, void *pBuf, int len)
{
	u16 status;
	if ( (status = PC4500_accessrid(ai, rid, CMD_ACCESS)) != SUCCESS)
		return status;
	if (bap_setup(ai, rid, 0, BAP1) != SUCCESS) return ERROR;
	// read the rid length field
	bap_read(ai, pBuf, 2, BAP1);
	// length for remaining part of rid
	len = min(len, le16_to_cpu(*(u16*)pBuf)) - 2;
	
	if ( len <= 2 ) {
		printk( KERN_INFO 
			"Rid %x has a length of %d which is too short\n",
			(int)rid,
			(int)len );
		return -1;
	}
	// read remainder of the rid
	return bap_read(ai, ((u16*)pBuf)+1, len, BAP1);
}

/*  Note, that we are using BAP1 which is also used by transmit, so
 *  make sure this isnt called when a transmit is happening */
static int PC4500_writerid(struct airo_info *ai, u16 rid, 
			   const void *pBuf, int len)
{
	u16 status;
	// --- first access so that we can write the rid data
	if ( (status = PC4500_accessrid(ai, rid, CMD_ACCESS)) != 0)
		return status;
	// --- now write the rid data
	if (bap_setup(ai, rid, 0, BAP1) != SUCCESS) return ERROR;
	bap_write(ai, pBuf, len, BAP1);
	// ---now commit the rid data
	return PC4500_accessrid(ai, rid, 0x100|CMD_ACCESS);
}

/* Allocates a FID to be used for transmitting packets.  We only use
   one for now. */
static u16 transmit_allocate(struct airo_info *ai, int lenPayload)
{
	Cmd cmd;
	Resp rsp;
	u16 txFid;
	u16 txControl;

	cmd.cmd = CMD_ALLOCATETX;
	cmd.parm0 = lenPayload;
	if (issuecommand(ai, &cmd, &rsp) != SUCCESS) return 0;
	if ( (rsp.status & 0xFF00) != 0) return 0;
	/* wait for the allocate event/indication
	 * It makes me kind of nervous that this can just sit here and spin,
	 * but in practice it only loops like four times. */
	while ( (IN4500(ai, EVSTAT) & EV_ALLOC) == 0) ;
	// get the allocated fid and acknowledge
	txFid = IN4500(ai, TXALLOCFID);
	OUT4500(ai, EVACK, EV_ALLOC);
  
	/*  The CARD is pretty cool since it converts the ethernet packet
	 *  into 802.11.  Also note that we don't release the FID since we
	 *  will be using the same one over and over again. */
	/*  We only have to setup the control once since we are not
	 *  releasing the fid. */
	txControl = TXCTL_TXOK | TXCTL_TXEX | TXCTL_802_3
		| TXCTL_ETHERNET | TXCTL_NORELEASE;
	if (bap_setup(ai, txFid, 0x0008, BAP1) != SUCCESS) return ERROR;
	bap_write(ai, &txControl, sizeof(txControl), BAP1);

	return txFid;
}

/* In general BAP1 is dedicated to transmiting packets.  However,
   since we need a BAP when accessing RIDs, we also use BAP1 for that */
static int transmit_802_3_packet(struct airo_info *ai, u16 txFid, 
				 char *pPacket, int len)
{
	u16 payloadLen;
	Cmd cmd;
	Resp rsp;
	
	if (len < 12) {
		printk( KERN_INFO "Short packet %d\n", len );
		return ERROR;
	}
	
	// packet is destination[6], source[6], payload[len-12]
	// write the payload length and dst/src/payload
	if (bap_setup(ai, txFid, 0x0036, BAP1) != SUCCESS) return ERROR;
	/* The hardware addresses aren't counted as part of the payload, so
	 * we have to subtract the 12 bytes for the addresses off */
	payloadLen = cpu_to_le16(len - 12);
	bap_write(ai, &payloadLen, sizeof(payloadLen),BAP1);
	bap_write(ai, (const u16*)pPacket, len, BAP1);
	// issue the transmit command
	memset( &cmd, 0, sizeof( cmd ) );
	cmd.cmd = CMD_TRANSMIT;
	cmd.parm0 = txFid;
	if (issuecommand(ai, &cmd, &rsp) != SUCCESS) return ERROR;
	if ( (rsp.status & 0xFF00) != 0) return ERROR;
	return SUCCESS;
}

/*
 *  This is the proc_fs routines.  It is a bit messier than I would
 *  like!  Feel free to clean it up!
 */

/*
 *  Unfortunately sometime between 2.0 and 2.2 the proc interface changed...
 *  Unfortunately I dont know when it was...
 *  Im guessing it is sometime around 0x20155...  Anybody know?
 */

#if (LINUX_VERSION_CODE > 0x20155)
#define NULLOPS1 NULL,
#define NULLOPS2 , NULL, NULL, NULL, NULL, NULL
static ssize_t proc_read( struct file *file,
			  char *buffer,
			  size_t len,
			  loff_t *offset);

static ssize_t proc_write( struct file *file,
			   const char *buffer,
			   size_t len,
			   loff_t *offset );
static int proc_close( struct inode *inode, struct file *file );
#else
#define NULLOPS1 
#define NULLOPS2 
static int proc_read( struct inode *inode,
		      struct file *file,
		      char *buffer,
		      int len );

static int proc_write( struct inode *inode,
		       struct file *file,
		       const char *buffer,
		       int len );
static void proc_close( struct inode *inode, struct file *file );
#endif

static int proc_stats_open( struct inode *inode, struct file *file );
static int proc_statsdelta_open( struct inode *inode, struct file *file );
static int proc_status_open( struct inode *inode, struct file *file );
static int proc_SSID_open( struct inode *inode, struct file *file );
static int proc_config_open( struct inode *inode, struct file *file );
static int proc_wepkey_open( struct inode *inode, struct file *file );


static struct file_operations proc_statsdelta_ops = {
	NULL,
	proc_read,
	NULL, NULL, NULL, NULL, NULL,
	proc_statsdelta_open,
	NULLOPS1
	proc_close
	NULLOPS2
};

static struct inode_operations proc_inode_statsdelta_ops = {
	&proc_statsdelta_ops,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static struct file_operations proc_stats_ops = {
	NULL,
	proc_read,
	NULL, NULL, NULL, NULL, NULL,
	proc_stats_open,
	NULLOPS1
	proc_close
	NULLOPS2
};

static struct inode_operations proc_inode_stats_ops = {
	&proc_stats_ops,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static struct file_operations proc_status_ops = {
	NULL,
	proc_read,
	NULL, NULL, NULL, NULL, NULL,
	proc_status_open,
	NULLOPS1
	proc_close
	NULLOPS2
};

static struct inode_operations proc_inode_status_ops = {
	&proc_status_ops,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static struct file_operations proc_SSID_ops = {
	NULL,
	proc_read,
	proc_write,
	NULL, NULL, NULL, NULL,
	proc_SSID_open,
	NULLOPS1
	proc_close
	NULLOPS2
};

static struct inode_operations proc_inode_SSID_ops = {
	&proc_SSID_ops,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static struct file_operations proc_config_ops = {
	NULL,
	proc_read,
	proc_write,
	NULL, NULL, NULL, NULL,
	proc_config_open,
	NULLOPS1
	proc_close
	NULLOPS2
};

static struct inode_operations proc_inode_config_ops = {
	&proc_config_ops,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static struct file_operations proc_wepkey_ops = {
	NULL,
	proc_read,
	proc_write,
	NULL, NULL, NULL, NULL,
	proc_wepkey_open,
	NULLOPS1
	proc_close
	NULLOPS2
};

static struct inode_operations proc_inode_wepkey_ops = {
	&proc_wepkey_ops,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#if (LINUX_VERSION_CODE > 0x20155)
/*
 * We need to do reference counting here.  When the inode is first used,
 * this will be called with fill non-zero.  When it is released this
 * will be called with fill set to zero.
 */
static void airo_fill_inode( struct inode *i, int fill ) {
	if ( fill ) {
		MOD_INC_USE_COUNT;
	} else {
		MOD_DEC_USE_COUNT;
	}
}
#endif

static struct file_operations airo_file_ops = {
	NULL, // lseek
	NULL, // read
	NULL, // write
	NULL, // readdir
	NULL, // select
	NULL, // ioctl
	NULL, // mmap
	NULL, // open
	NULL, // release
};

static struct inode_operations airo_inode_ops = {
	&airo_file_ops,
	NULL, // create
	NULL, // lookup
};

static struct proc_dir_entry airo_entry = {
	0,
	7,
	"aironet",
	S_IFDIR | S_IRUGO | S_IXUGO,
	1,
	0, 0,
	44,
	&airo_inode_ops,
	0, // get_info
#if (LINUX_VERSION_CODE > 0x20155)
	airo_fill_inode
#endif
};

static struct proc_dir_entry wepkey_entry = {
	0, 6, "WepKey",
	S_IFREG | S_IWUSR, 2, 0, 0,
	13,
	&proc_inode_wepkey_ops, NULL
};

static struct proc_dir_entry statsdelta_entry = {
	0, 10, "StatsDelta",
	S_IFREG | S_IRUGO | S_IWUSR , 2, 0, 0,
	13,
	&proc_inode_statsdelta_ops, NULL
};

static struct proc_dir_entry stats_entry = {
	0, 5, "Stats",
	S_IFREG | S_IRUGO , 2, 0, 0,
	13,
	&proc_inode_stats_ops, NULL
};

static struct proc_dir_entry status_entry = {
	0, 6, "Status",
	S_IFREG | S_IRUGO , 2, 0, 0,
	13,
	&proc_inode_status_ops, NULL
};

static struct proc_dir_entry SSID_entry = {
	0, 4, "SSID",
	S_IFREG | S_IRUGO | S_IWUSR, 2, 0, 0,
	13,
	&proc_inode_SSID_ops, NULL
};

static struct proc_dir_entry config_entry = {
	0, 6, "Config",
	S_IFREG | S_IRUGO | S_IWUSR, 2, 0, 0,
	13,
	&proc_inode_config_ops, NULL
};

struct proc_data {
	int release_buffer;
	int readlen;
	char *rbuffer;
	int writelen;
	int maxwritelen;
	char *wbuffer;
	void (*on_close) (struct inode *, struct file *);
};

static int setup_proc_entry( struct device *dev,
			     struct airo_info *apriv ) {
	/* First setup the device directory */
	memset( &apriv->proc_entry, 0, sizeof( apriv->proc_entry ) );
	apriv->proc_entry.namelen = strlen( dev->name );
	apriv->proc_entry.name = dev->name;
	apriv->proc_entry.mode = S_IFDIR | S_IRUGO | S_IXUGO;
	apriv->proc_entry.nlink = 2;
	apriv->proc_entry.ops = airo_entry.ops;
	PROC_REGISTER( &airo_entry, &apriv->proc_entry );
	
	/* Setup the StatsDelta */
	memcpy( &apriv->proc_statsdelta_entry, &statsdelta_entry, 
		sizeof( statsdelta_entry ) );
	apriv->proc_statsdelta_entry.data = dev;
	PROC_REGISTER( &apriv->proc_entry, 
		       &apriv->proc_statsdelta_entry );
	
	/* Setup the Stats */
	memcpy( &apriv->proc_stats_entry, &stats_entry, 
		sizeof( stats_entry ) );
	apriv->proc_stats_entry.data = dev;
	PROC_REGISTER( &apriv->proc_entry, 
		       &apriv->proc_stats_entry );
	
	/* Setup the Status */
	memcpy( &apriv->proc_status_entry, &status_entry, 
		sizeof( status_entry ) );
	apriv->proc_status_entry.data = dev;
	PROC_REGISTER( &apriv->proc_entry, 
		       &apriv->proc_status_entry );
	
	/* Setup the Config */
	memcpy( &apriv->proc_config_entry, &config_entry, 
		sizeof( config_entry ) );
	apriv->proc_config_entry.data = dev;
	PROC_REGISTER( &apriv->proc_entry, 
		       &apriv->proc_config_entry );

	/* Setup the SSID */
	memcpy( &apriv->proc_SSID_entry, &SSID_entry, sizeof( SSID_entry ) );
	apriv->proc_SSID_entry.data = dev;
	PROC_REGISTER( &apriv->proc_entry, 
		       &apriv->proc_SSID_entry );
	
	/* Setup the WepKey */
	memcpy( &apriv->proc_wepkey_entry, &wepkey_entry, 
		sizeof( wepkey_entry ) );
	apriv->proc_wepkey_entry.data = dev;
	PROC_REGISTER( &apriv->proc_entry, 
		       &apriv->proc_wepkey_entry );
	
	return 0;
}

static int takedown_proc_entry( struct device *dev,
				struct airo_info *apriv ) {
	if ( !apriv->proc_entry.namelen ) return 0;
	proc_unregister( &apriv->proc_entry, apriv->proc_statsdelta_entry.low_ino );
	proc_unregister( &apriv->proc_entry, apriv->proc_stats_entry.low_ino );
	proc_unregister( &apriv->proc_entry, apriv->proc_status_entry.low_ino );
	proc_unregister( &apriv->proc_entry, apriv->proc_config_entry.low_ino );
	proc_unregister( &apriv->proc_entry, apriv->proc_SSID_entry.low_ino );
	proc_unregister( &apriv->proc_entry, apriv->proc_wepkey_entry.low_ino );
	proc_unregister( &airo_entry, apriv->proc_entry.low_ino );
	return 0;
}

/*
 *  What we want from the proc_fs is to be able to efficiently read
 *  and write the configuration.  To do this, we want to read the
 *  configuration when the file is opened and write it when the file is
 *  closed.  So basically we allocate a read buffer at open and fill it
 *  with data, and allocate a write buffer and read it at close.
 */

/*
 *  The read routine is generic, it relies on the preallocated rbuffer
 *  to supply the data.
 */
#if (LINUX_VERSION_CODE > 0x20155)
static ssize_t proc_read( struct file *file,
			  char *buffer,
			  size_t len,
			  loff_t *offset )
#else
static int proc_read( struct inode *inode,
		      struct file *file,
		      char *buffer,
		      int len ) 
#endif
{
	int i;
	int pos;
	struct proc_data *priv = (struct proc_data*)file->private_data;
	
	if( !priv->rbuffer ) return -EINVAL;
	
#if (LINUX_VERSION_CODE > 0x20155)
	pos = *offset;
#else
	pos = file->f_pos;
#endif
	for( i = 0; i+pos < priv->readlen && i < len; i++ ) {
		put_user( priv->rbuffer[i+pos], buffer+i );
	}
#if (LINUX_VERSION_CODE > 0x20155)
	*offset += i;
#else
	file->f_pos += i;
#endif
	return i;
}

/*
 *  The write routine is generic, it fills in a preallocated rbuffer
 *  to supply the data.
 */
#if (LINUX_VERSION_CODE > 0x20155)
static ssize_t proc_write( struct file *file,
			   const char *buffer,
			   size_t len,
			   loff_t *offset ) 
#else
static int proc_write( struct inode *inode,
		       struct file *file,
		       const char *buffer,
		       int len ) 
#endif
{
	int i;
	int pos;
	struct proc_data *priv = (struct proc_data*)file->private_data;
	
	if ( !priv->wbuffer ) {
		return -EINVAL;
	}
	
#if (LINUX_VERSION_CODE > 0x20155)
	pos = *offset;
#else
	pos = file->f_pos;
#endif
	
	for( i = 0; i + pos <  priv->maxwritelen &&
		     i < len; i++ ) {
#if (LINUX_VERSION_CODE > 0x20155)
		get_user( priv->wbuffer[i+pos], buffer + i );
#else
		priv->wbuffer[i+pos] = get_user( buffer + i );
#endif
	}
	if ( i+pos > priv->writelen ) priv->writelen = i+file->f_pos;
#if (LINUX_VERSION_CODE > 0x20155)
	*offset += i;
#else
	file->f_pos += i;
#endif
	return i;
}

static int proc_status_open( struct inode *inode, struct file *file ) {
	struct proc_data *data;
	unsigned long flags;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *apriv = (struct airo_info *)dev->priv;
	CapabilityRid cap_rid;
	StatusRid status_rid;
	
	MOD_INC_USE_COUNT;
	
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	
	file->private_data = kmalloc(sizeof(struct proc_data ), GFP_KERNEL);
	memset(file->private_data, 0, sizeof(struct proc_data));
	data = (struct proc_data *)file->private_data;
	data->rbuffer = kmalloc( 2048, GFP_KERNEL );
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	
	PC4500_readrid(apriv, 0xFF50, &status_rid,
		       sizeof(status_rid));
	PC4500_readrid(apriv, 0xFF00, &cap_rid,
		       sizeof(cap_rid));
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
	
	sprintf( data->rbuffer, "Mode: %x\n"
		 "Signal: %d\n"
		 "SSID: %-*s\n"
		 "AP: %-16s\n"
		 "Freq: %d\n"
		 "BitRate: %dmbs\n"
		 "Driver Version: %s\n"
		 "Device: %s\nManufacturer: %s\nFirmware Version: %s\n"
		 "Radio type: %x\nCountry: %x\nHardware Version: %x\n"
		 "Software Version: %x\nSoftware Subversion: %x\n"
		 "Boot block version: %x\n",
		 (int)le16_to_cpu(status_rid.mode),
		 (int)le16_to_cpu(status_rid.sigQuality),
		 (int)status_rid.SSIDlen,
		 status_rid.SSID,
		 status_rid.apName,
		 (int)(apriv->rateFreq >> 8 ),
		 (int)((apriv->rateFreq & 0xff)/2),
		 version,
		 cap_rid.prodName,
		 cap_rid.manName,
		 cap_rid.prodVer,
		 le16_to_cpu(cap_rid.radioType),
		 le16_to_cpu(cap_rid.country),
		 le16_to_cpu(cap_rid.hardVer),
		 (int)le16_to_cpu(cap_rid.softVer),
		 (int)le16_to_cpu(cap_rid.softSubVer),
		 (int)le16_to_cpu(cap_rid.bootBlockVer) );
	data->readlen = strlen( data->rbuffer );
	return 0;
}

static int proc_stats_rid_open(struct inode*, struct file*, u16);
static int proc_statsdelta_open( struct inode *inode, 
				 struct file *file ) {
	if (file->f_mode&FMODE_WRITE) {
	return proc_stats_rid_open(inode, file, RID_STATSDELTACLEAR);
	}
	return proc_stats_rid_open(inode, file, RID_STATSDELTA);
}

static int proc_stats_open( struct inode *inode, struct file *file ) {
	return proc_stats_rid_open(inode, file, RID_STATS);
}

static int proc_stats_rid_open( struct inode *inode, 
				struct file *file,
				u16 rid ) {
	struct proc_data *data;
	unsigned long flags;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *apriv = (struct airo_info *)dev->priv;
	char buffer[1024];
	int i, j;
	int *vals = (int*)&buffer[4];
	MOD_INC_USE_COUNT;
	
	
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	
	file->private_data = kmalloc(sizeof(struct proc_data ), GFP_KERNEL);
	memset(file->private_data, 0, sizeof(struct proc_data));
	data = (struct proc_data *)file->private_data;
	data->rbuffer = kmalloc( 4096, GFP_KERNEL );
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	
	PC4500_readrid(apriv, rid, buffer,
		       sizeof(buffer));
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
	
        j = 0;
	for(i=0; (int)statsLabels[i]!=-1 && 
		    i*4<le16_to_cpu(*(u16*)buffer); i++){
                if (!statsLabels[i]) continue;
		if (j+strlen(statsLabels[i])+16>4096) {
			printk(KERN_WARNING
			       "Potentially disasterous buffer overflow averted!\n");
			break;
		}
		j+=sprintf( data->rbuffer+j, "%s: %d\n",statsLabels[i],
			    le32_to_cpu(vals[i]));
        }
	if (i*4>=le16_to_cpu(*(u16*)buffer)){
		printk(KERN_WARNING
		       "Got a short rid\n");
	}
	data->readlen = j;
	return 0;
}

static int get_dec_u16( char *buffer, int *start, int limit ) {
	u16 value;
	int valid = 0;
	for( value = 0; buffer[*start] >= '0' &&
		     buffer[*start] <= '9' &&
		     *start < limit; (*start)++ ) {
		valid = 1;
		value *= 10;
		value += buffer[*start] - '0';
	}
	if ( !valid ) return -1;
	return value;
}

static void proc_config_on_close( struct inode *inode, struct file *file ) {
	struct proc_data *data = file->private_data;
	unsigned long flags;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *ai = (struct airo_info*)dev->priv;
	ConfigRid config;
	Cmd cmd;
	Resp rsp;
	char *line;
	
	if ( !data->writelen ) return;
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	memset(&cmd, 0, sizeof(Cmd));
	cmd.cmd = MAC_DISABLE; // disable in case already enabled
	issuecommand(ai, &cmd, &rsp);
	PC4500_readrid(ai, RID_ACTUALCONFIG, &config,
		       sizeof(config));

	line = data->wbuffer;
	while( line[0] ) {
		/*** Mode processing */
		if ( !strncmp( line, "Mode: ", 6 ) ) {
			line += 6;
			if ( line[0] == 'a' ) config.opmode = 0;
			else config.opmode = cpu_to_le16(1);
		}
		
		/*** Radio status */
		else if (!strncmp(line,"Radio: ", 7)) {
			line += 7;
			if (!strncmp(line,"off",3)) {
				ai->flags |= FLAG_RADIO_OFF;
			} else {
				ai->flags &= ~FLAG_RADIO_OFF;
			}
		}
		/*** NodeName processing */
		else if ( !strncmp( line, "NodeName: ", 10 ) ) {
			int j;
			
			line += 10;
			memset( config.nodeName, 0, 16 );
			/* Do the name, assume a space between the mode and node name */
			for( j = 0; j < 16 && line[j] != '\n'; j++ ) {
				config.nodeName[j] = line[j];
			}
		} 
		
		/*** PowerMode processing */
		else if ( !strncmp( line, "PowerMode: ", 11 ) ) {
			line += 11;
			if ( !strncmp( line, "PSPCAM", 6 ) ) {
				config.powerSaveMode = 
					cpu_to_le16(POWERSAVE_PSPCAM);
			} else if ( !strncmp( line, "PSP", 3 ) ) {
				config.powerSaveMode = 
					cpu_to_le16(POWERSAVE_PSP);
			} else {
				config.powerSaveMode = 
					cpu_to_le16(POWERSAVE_CAM);
			}	
		} else if ( !strncmp( line, "DataRates: ", 11 ) ) {
			int v, i = 0, k = 0; /* i is index into line, 
						k is index to rates */
			
			line += 11;
			while((v = get_dec_u16(line, &i, 3))!=-1) {
				config.rates[k++] = (u8)v;
				line += i + 1;
				i = 0;
			}
		} else if ( !strncmp( line, "Channel: ", 9 ) ) {
			int v, i = 0;
			line += 9;
			v = get_dec_u16(line, &i, i+3);
			if ( v != -1 ) 
				config.channelSet = (u16)cpu_to_le16(v);
		} else if ( !strncmp( line, "XmitPower: ", 11 ) ) {
			int v, i = 0;
			line += 11;
			v = get_dec_u16(line, &i, i+3);
			if ( v != -1 ) config.txPower = 
					       (u16)cpu_to_le16(v);
		} else if ( !strncmp( line, "WEP: ", 5 ) ) {
			line += 5;
			switch( line[0] ) {
			case 's':
				config.authType = cpu_to_le16(AUTH_SHAREDKEY);
				break;
			case 'e':
				config.authType = cpu_to_le16(AUTH_ENCRYPT);
				break;
			default:
				config.authType = cpu_to_le16(AUTH_OPEN);
				break;
			}
		} else if ( !strncmp( line, "LongRetryLimit: ", 16 ) ) {
			int v, i = 0;
			
			line += 16;
			v = get_dec_u16(line, &i, 3);
			v = (v<0) ? 0 : ((v>255) ? 255 : v);
			config.longRetryLimit = (u16)cpu_to_le16(v);
		} else if ( !strncmp( line, "ShortRetryLimit: ", 17 ) ) {
			int v, i = 0;
			
			line += 17;
			v = get_dec_u16(line, &i, 3);
			v = (v<0) ? 0 : ((v>255) ? 255 : v);
			config.shortRetryLimit = (u16)cpu_to_le16(v);
		} else if ( !strncmp( line, "RTSThreshold: ", 14 ) ) {
			int v, i = 0;
			
			line += 14;
			v = get_dec_u16(line, &i, 4);
			v = (v<0) ? 0 : ((v>2312) ? 2312 : v);
			config.rtsThres = (u16)cpu_to_le16(v);
		} else if ( !strncmp( line, "TXMSDULifetime: ", 16 ) ) {
			int v, i = 0;
			
			line += 16;
			v = get_dec_u16(line, &i, 5);
			v = (v<0) ? 0 : v;
			config.txLifetime = (u16)cpu_to_le16(v);
		} else if ( !strncmp( line, "RXMSDULifetime: ", 16 ) ) {
			int v, i = 0;
			
			line += 16;
			v = get_dec_u16(line, &i, 5);
			v = (v<0) ? 0 : v;
			config.rxLifetime = (u16)cpu_to_le16(v);
		} else if ( !strncmp( line, "TXDiversity: ", 13 ) ) {
			config.txDiversity = 
				(line[13]=='l') ? cpu_to_le16(1) :
				((line[13]=='r')? cpu_to_le16(2):
				 cpu_to_le16(3));
		} else if ( !strncmp( line, "RXDiversity: ", 13 ) ) {
			config.rxDiversity = 
				(line[13]=='l') ? cpu_to_le16(1) :
				((line[13]=='r')? cpu_to_le16(2):
				 cpu_to_le16(3));
		} else if ( !strncmp( line, "FragThreshold: ", 15 ) ) {
			int v, i = 0;
			
			line += 15;
			v = get_dec_u16(line, &i, 4);
			v = (v<256) ? 256 : ((v>2312) ? 2312 : v);
			v = v & 0xfffe; /* Make sure its even */
			config.fragThresh = (u16)cpu_to_le16(v);
		} else if (!strncmp(line, "Modulation: ", 12)) {
		        line += 12;
			switch(*line) {
			case 'd':  config.modulation=MOD_DEFAULT; break;
			case 'c':  config.modulation=MOD_CCK; break;
			case 'm':  config.modulation=MOD_MOK; break;
			default:
				printk( KERN_WARNING "Unknown modulation\n" );
			}
		} else {
			printk( KERN_INFO "Couldn't figure out %s\n", line );
		}
		while( line[0] && line[0] != '\n' ) line++;
		if ( line[0] ) line++;
	}
	ai->config = config;
	PC4500_writerid(ai, RID_CONFIG, &config,
			sizeof(config));
	enable_MAC(ai, &rsp);
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
}

static int proc_config_open( struct inode *inode, struct file *file ) {
	struct proc_data *data;
	unsigned long flags;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *ai = (struct airo_info*)dev->priv;
	ConfigRid config;
	int i;
	
	MOD_INC_USE_COUNT;
	
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	
	file->private_data = kmalloc(sizeof(struct proc_data ), GFP_KERNEL);
	memset(file->private_data, 0, sizeof(struct proc_data));
	data = (struct proc_data *)file->private_data;
	data->rbuffer = kmalloc( 2048, GFP_KERNEL );
	data->wbuffer = kmalloc( 2048, GFP_KERNEL );
	memset( data->wbuffer, 0, 2048 );
	data->maxwritelen = 2048;
	data->on_close = proc_config_on_close;
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	PC4500_readrid(ai, RID_ACTUALCONFIG, &config,
		       sizeof(config));
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
	
	i = sprintf( data->rbuffer, 
		     "Mode: %s\n"
		     "Radio: %s\n"
		     "NodeName: %-16s\n"
		     "PowerMode: %s\n"
		     "DataRates: %d %d %d %d %d %d %d %d\n"
		     "Channel: %d\n"
		     "XmitPower: %d\n",
		     config.opmode == 0 ? "adhoc" : 
		     config.opmode == 1 ? "ESS" : "Error",
		     ai->flags&FLAG_RADIO_OFF ? "off" : "on",
		     config.nodeName,
		     config.powerSaveMode == 0 ? "CAM" :
		     config.powerSaveMode == 1 ? "PSP" :
		     config.powerSaveMode == 2 ? "PSPCAM" : "Error",
		     (int)config.rates[0],
		     (int)config.rates[1],
		     (int)config.rates[2],
		     (int)config.rates[3],
		     (int)config.rates[4],
		     (int)config.rates[5],
		     (int)config.rates[6],
		     (int)config.rates[7],
		     (int)le16_to_cpu(config.channelSet),
		     (int)le16_to_cpu(config.txPower)
		);
	sprintf( data->rbuffer + i,
		 "LongRetryLimit: %d\n"
		 "ShortRetryLimit: %d\n"
		 "RTSThreshold: %d\n"
		 "TXMSDULifetime: %d\n"
		 "RXMSDULifetime: %d\n"
		 "TXDiversity: %s\n"
		 "RXDiversity: %s\n"
		 "FragThreshold: %d\n"
		 "WEP: %s\n"
		 "Modulation: %s\n",
		 (int)le16_to_cpu(config.longRetryLimit),
		 (int)le16_to_cpu(config.shortRetryLimit),
		 (int)le16_to_cpu(config.rtsThres),
		 (int)le16_to_cpu(config.txLifetime),
		 (int)le16_to_cpu(config.rxLifetime),
		 config.txDiversity == 1 ? "left" :
		 config.txDiversity == 2 ? "right" : "both",
		 config.rxDiversity == 1 ? "left" :
		 config.rxDiversity == 2 ? "right" : "both",
		 (int)le16_to_cpu(config.fragThresh),
		 config.authType == AUTH_ENCRYPT ? "encrypt" :
		 config.authType == AUTH_SHAREDKEY ? "shared" : "open",
		 config.modulation == 0 ? "default" :
		 config.modulation == MOD_CCK ? "cck" :
		 config.modulation == MOD_MOK ? "mok" : "error"
		);
	data->readlen = strlen( data->rbuffer );
	return 0;
}

static void proc_SSID_on_close( struct inode *inode, struct file *file ) {
	struct proc_data *data = (struct proc_data *)file->private_data;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *ai = (struct airo_info*)dev->priv;
	SsidRid SSID_rid;
	int i;
	int offset = 0;
	
	if ( !data->writelen ) return;
	
	memset( &SSID_rid, 0, sizeof( SSID_rid ) );
	
	for( i = 0; i < 3; i++ ) {
		int j;
		for( j = 0; j+offset < data->writelen && j < 16 &&
			     data->wbuffer[offset+j] != '\n'; j++ ) {
			SSID_rid.ssids[i].ssid[j] = data->wbuffer[offset+j];
		}
		if ( j == 0 ) break;
		SSID_rid.ssids[i].len = cpu_to_le16(j);
		offset += j;
		while( data->wbuffer[offset] != '\n' && 
		       offset < data->writelen ) offset++;
		offset++;
	}
	do_writerid(ai, RID_SSID, &SSID_rid, sizeof( SSID_rid ));
}

/* This function wraps PC4500_writerid with a MAC disable */
static int do_writerid( struct airo_info *ai, u16 rid, const void *rid_data,
			int len ) {
	int rc;
	unsigned long flags;
	Cmd cmd;
	Resp rsp;
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = MAC_DISABLE; // disable in case already enabled
	issuecommand(ai, &cmd, &rsp);
	rc = PC4500_writerid(ai, 
			     rid, rid_data, len);
	enable_MAC(ai, &rsp);
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
	return rc;
}

static int set_wep_key(struct airo_info *ai, const char *key, u16 keylen ) {
	static const unsigned char macaddr[6] = { 0x01, 0, 0, 0, 0, 0 };
	WepKeyRid wkr;
	wkr.len = cpu_to_le16(sizeof(wkr));
	wkr.kindex = cpu_to_le16(0);
	wkr.klen = cpu_to_le16(5);
	memcpy( wkr.key, key, 5 );
	memcpy( wkr.mac, macaddr, 6 );
	do_writerid(ai, RID_WEP_TEMP, &wkr, sizeof(wkr));
	do_writerid(ai, RID_WEP_PERM, &wkr, sizeof(wkr));
	return 0;
}

static u8 hexVal(char c) {
	if (c>='0' && c<='9') return c -= '0';
	if (c>='a' && c<='f') return c -= 'a'-10;
	if (c>='A' && c<='F') return c -= 'A'-10;
	return 0;
}

static void proc_wepkey_on_close( struct inode *inode, struct file *file ) {
	struct proc_data *data;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *ai = (struct airo_info*)dev->priv;
	int i;
	char key[5];

	memset(key, 0, sizeof(key));
	
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	data = (struct proc_data *)file->private_data;
	if ( !data->writelen ) return;
	
	for( i = 0; i < 15 && data->wbuffer[i]; i++ ) {
		switch(i%3) {
		case 0:
			key[i/3] = hexVal(data->wbuffer[i])<<4;
			break;
		case 1:
			key[i/3] |= hexVal(data->wbuffer[i]);
			break;
		}
	}
	set_wep_key(ai, key, sizeof(key));
}

static int proc_wepkey_open( struct inode *inode, struct file *file ) {
	struct proc_data *data;
	unsigned long flags;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *ai = (struct airo_info*)dev->priv;
	char *ptr;
	WepKeyRid wkr;
	
	MOD_INC_USE_COUNT;
	
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	
	file->private_data = kmalloc(sizeof(struct proc_data ), GFP_KERNEL);
	memset(file->private_data, 0, sizeof(struct proc_data));
	memset(&wkr, 0, sizeof(wkr));
	data = (struct proc_data *)file->private_data;
	data->rbuffer = kmalloc( 80, GFP_KERNEL );
	data->writelen = 0;
	data->maxwritelen = 80;
	data->wbuffer = kmalloc( 80, GFP_KERNEL );
	memset( data->wbuffer, 0, 80 );
	data->on_close = proc_wepkey_on_close;
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	PC4500_readrid(ai, RID_WEP_PERM, &wkr, sizeof(wkr));
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
	ptr = data->rbuffer;
	sprintf( ptr, "%02x:%02x:%02x:%02x:%02x\n",
		 wkr.key[0],
		 wkr.key[1],
		 wkr.key[2],
		 wkr.key[3],
		 wkr.key[4] );
	data->readlen = strlen( data->rbuffer );
	return 0;
}

static int proc_SSID_open( struct inode *inode, struct file *file ) {
	struct proc_data *data;
	unsigned long flags;
	struct proc_dir_entry *dp = inode->u.generic_ip;
	struct device *dev = dp->data;
	struct airo_info *ai = (struct airo_info*)dev->priv;
	int i;
	char *ptr;
	SsidRid SSID_rid;

	MOD_INC_USE_COUNT;
	
	dp = (struct proc_dir_entry *) inode->u.generic_ip;
	
	file->private_data = kmalloc(sizeof(struct proc_data ), GFP_KERNEL);
	memset(file->private_data, 0, sizeof(struct proc_data));
	data = (struct proc_data *)file->private_data;
	data->rbuffer = kmalloc( 104, GFP_KERNEL );
	data->writelen = 0;
	data->maxwritelen = 33*3;
	data->wbuffer = kmalloc( 33*3, GFP_KERNEL );
	memset( data->wbuffer, 0, 33*3 );
	data->on_close = proc_SSID_on_close;
	
#ifdef DO_SPIN_LOCK
	spin_lock_irqsave(&xxx_lock, flags);
#else
	save_flags(flags);
	cli();
#endif
	PC4500_readrid(ai, RID_SSID, 
		       &SSID_rid, sizeof( SSID_rid ));
#ifdef DO_SPIN_LOCK
	spin_unlock_irqrestore(&xxx_lock, flags);
#else
	restore_flags(flags);
#endif
	ptr = data->rbuffer;
	for( i = 0; i < 3; i++ ) {
		int j;
		if ( !SSID_rid.ssids[i].len ) break;
		for( j = 0; j < 16 && 
			     j < le16_to_cpu(SSID_rid.ssids[i].len) && 
			     SSID_rid.ssids[i].ssid[j]; j++ ) {
			*ptr++ = SSID_rid.ssids[i].ssid[j]; 
		}
		*ptr++ = '\n';
	}
	*ptr = '\0';
	data->readlen = strlen( data->rbuffer );
	return 0;
}

#if (LINUX_VERSION_CODE > 0x20155)
static int proc_close( struct inode *inode, struct file *file ) 
#else
static void proc_close( struct inode *inode, struct file *file ) 
#endif
{
	struct proc_data *data = (struct proc_data *)file->private_data;
	if ( data->on_close != NULL ) data->on_close( inode, file );
	MOD_DEC_USE_COUNT;
	if ( data->rbuffer ) kfree( data->rbuffer );
	if ( data->wbuffer ) kfree( data->wbuffer );
	kfree( data );
#if (LINUX_VERSION_CODE > 0x20155)
	return 0;
#endif
}

static struct device_list {
	struct device *dev;
	struct device_list *next;
} *airo_devices = 0;

/* Since the card doesnt automatically switch to the right WEP mode,
   we will make it do it.  If the card isn't associated, every secs we
   will switch WEP modes to see if that will help.  If the card is
   associated we will check every minute to see if anything has
   changed. */
static void timer_func( u_long data ) {
	struct device *dev = (struct device*)data;
	struct airo_info *apriv = (struct airo_info *)dev->priv;
	u16 linkstat = IN4500(apriv, LINKSTAT);
	
	if (linkstat != 0x400 ) {
		struct timer_list *timer = &apriv->timer;
		ConfigRid config = apriv->config;
		int i;
		
		switch(apriv->authtype) {
		case AUTH_ENCRYPT:
			/* So escalate to SHAREDKEY */
			config.authType = AUTH_SHAREDKEY;
			/* The current hardware can't do more than
			   2mbs with encryption */
			for(i=0; i<8; i++) {
				if (config.rates[i] > 4) {
					config.rates[i] = 0;
				}
			}
			apriv->authtype = AUTH_SHAREDKEY;
			break;
		case AUTH_SHAREDKEY:
			/* Drop to open */
			config.authType = AUTH_OPEN;
			apriv->authtype = AUTH_OPEN;
			break;
		default:  /* We'll default to open */
			/* So escalate to ENCRYPT */
			config.authType = AUTH_ENCRYPT;
			/* The current hardware can't do more than
			   2mbs with encryption */
			for(i=0; i<8; i++) {
				if (config.rates[i] > 4) {
					config.rates[i] = 0;
				}
			}
			apriv->authtype = AUTH_ENCRYPT;
		}
		setup_card(apriv, dev->dev_addr, &config);
		timer->expires = RUN_AT(HZ*5);  /*Check every 5 secs
						  until everything is OK*/
	} else {
		struct timer_list *timer = &apriv->timer;
		timer->expires = RUN_AT(HZ*60);  /*Check every 60 secs
						   until everything is OK*/
	}
	add_timer(&apriv->timer);
}

static void add_airo_dev( struct device *dev ) {
	struct device_list *node =
		(struct device_list*)kmalloc( sizeof( *node ), GFP_KERNEL );
	if ( !node ) {
		printk( KERN_WARNING "airo_pci:  Out of memory\n" );
	} else {
		if ( auto_wep ) {
			struct timer_list *timer = 
				&((struct airo_info*)dev->priv)->timer;
			
			timer->function = timer_func;
			timer->data = (u_long)dev;
			timer->prev = timer->next = 0;
			/*Start off checking 5 secs */
			timer->expires = RUN_AT( HZ * 5 );  
			add_timer(timer);
		}
		
		node->dev = dev;
		node->next = airo_devices;
		airo_devices = node;
	}
}

static void del_airo_dev( struct device *dev ) {
	struct device_list **p = &airo_devices;
	while( *p && ( (*p)->dev != dev ) )
		p = &(*p)->next;
	if ( *p && (*p)->dev == dev )
		*p = (*p)->next; 
}

int init_module( void )
{
	int i;
	
#if (LINUX_VERSION_CODE > 0x20155)
	airo_entry.ops->lookup = 
		proc_dir_inode_operations.lookup;
	airo_entry.ops->default_file_ops->readdir = 
		proc_dir_inode_operations.default_file_ops->readdir;
#else
	airo_entry.ops = proc_net.ops;
#endif
	PROC_REGISTER( &proc_root, &airo_entry );
	
	for( i = 0; i < 4 && io[i] && irq[i]; i++ ) {
		printk( KERN_INFO 
			"airo:  Trying to configure ISA adapter at irq=%d io=0x%x\n",
			irq[i], io[i] );
		init_airo_card( irq[i], io[i] );
	}
	
#ifdef CONFIG_PCI
	if ( pcibios_present() ) {
		int i;
		printk( KERN_INFO "airo:  Probing for PCI adapters\n" );
		for( i = 0; card_ids[i].vendor; i++ ) {
#if (LINUX_VERSION_CODE > 0x20155)
			struct pci_dev *dev = 0;
			while((dev = pci_find_device(card_ids[i].vendor, card_ids[i].id,
						     dev))) {
				init_airo_card( dev->irq, 
						dev->base_address[2] & PCI_BASE_ADDRESS_IO_MASK );
			}
#else
			int j;
			unsigned char bus, fun;
			
			/* We are running fast and loose here, it would be nice to fix it... */
			for( j = 0; j < 4; j++ ) {
				unsigned char irq;
				unsigned int io;
				if ( pcibios_find_device( card_ids[i].vendor, card_ids[i].id,
							  j, &bus, &fun ) != PCIBIOS_SUCCESSFUL ) break;
				pcibios_read_config_byte( bus, fun, PCI_INTERRUPT_LINE, &irq );
				pcibios_read_config_dword( bus, fun, PCI_BASE_ADDRESS_2, &io );
				io &= PCI_BASE_ADDRESS_IO_MASK;
				init_airo_card( irq, io );
			}
#endif
		}
		printk( KERN_INFO "airo:  Finished probing for PCI adapters\n" );
	}
#endif
	return 0;
}

void cleanup_module( void )
{
	while( airo_devices ) {
		printk( KERN_WARNING "Unregistering %s\n", airo_devices->dev->name );
		stop_airo_card( airo_devices->dev );
	}
	proc_unregister( &proc_root, airo_entry.low_ino );
}

  
