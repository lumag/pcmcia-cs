/********************************************************************
 *	WaveLAN/IEEE PCMCIA Linux network device driver
 *
 *	by Andreas Neuhaus <andy@fasta.fh-dortmund.de>
 *	http://www.fasta.fh-dortmund.de/users/andy/wvlan/
 *
 *	This driver is free software; you can redistribute and/or
 *	modify it under the terms of the GNU General Public License;
 *	either version 2 of the license, or (at your option) any
 *	later version.
 *	Please send comments/bugfixes/patches to the above address.
 *
 *	Based on the Linux PCMCIA Programmer's Guide
 *	and Lucent Technologies' HCF-Light library (wvlan47).
 *
 *	See 'man 4 wvlan_cs' for more information.
 *
 * HISTORY
 *	v1.0.2	2000/01/07
 *		Merged driver into the PCMCIA Card Services package
 *			(thanks to David Hinds).
 *		Removed README, added man page (man 4 wvlan_cs).
 *
 *	v1.0.1	1999/09/02
 *		Interrupts are now disabled during ioctl to prevent being
 *			disturbed during our fiddling with the NIC (occured
 *			when using wireless tools while heavy loaded link).
 *		Fixed a problem with more than 6 spy addresses (thanks to
 *			Thomas Ekstrom).
 *		Hopefully fixed problems with bigger packet sizes than 1500.
 *		When you changed parameters that were specified at module
 *			load time later with wireless_tools and the card was
 *			reset afterward (e.g. by a Tx timeout), all changes
 *			were lost. Changes will stay now after a reset.
 *		Rewrote some parts of this README, added card LEDs description.
 *		Added medium_reservation, ap_density, frag_threshold and
 *			transmit_rate to module parameters.
 *		Applying the patch now also modifies the files SUPPORTED.CARDS
 *			and MAINTAINERS.
 *		Signal/noise levels are now reported in dBm (-102..-10).
 *		Added support for the new wireless extension (get wireless_
 *			tools 19). Credits go to Jean Tourrilhes for all
 *			the following:
 *		Setting channel by giving frequency value is now available.
 *		Setting/getting ESSID/BSSID/station-name is now possible
 *			via iwconfig.
 *		Support to set/get the desired/current bitrate.
 *		Support to set/get RTS threshold.
 *		Support to set/get fragmentation threshold.
 *		Support to set/get AP density.
 *		Support to set/get port type.
 *		Fixed a problem with ioctl calls when setting station/network
 *			name, where the new name string wasn't in kernel space
 *			(thanks to Danton Nunes).
 *		Driver sucessful tested on AlphaLinux (thanks to Peter Olson).
 *		Driver sucessful tested with WaveLAN Turbo cards.
 *
 *	v0.2.7	1999/07/20
 *		Changed release/detach code to fix hotswapping with 2.2/2.3
 *			kernels (thanks to Derrick J Brashear).
 *		Fixed wrong adjustment of receive buffer length. This was only
 *			a problem when a higher level protocol relies on correct
 *			length information, so it never occured with IPv4
 *			(thanks to Henrik Gulbrandsen).
 *
 *	v0.2.6	1999/05/04
 *		Added wireless spy and histogram support. Signal levels
 *			are now reported in ad-hoc mode correctly, but you
 *			need to use iwspy for it, because we can 'hear' more
 *			than one remote host in ad-hoc mode (thanks
 *			to Albert K T Hui for the code and to Richard van
 *			Leeuwen for the technical details).
 *		Fixed a bug with wrong tx_bytes count.
 *		Added GPL file wvlan.COPYING.
 *
 *	v0.2.5	1999/03/12
 *		Hopefully fixed problems with the Makefile patch.
 *		Changed the interrupt service routine to do never lock up
 *			in an endless loop (if this ever would happen...).
 *		Missed a conditional which made the driver unable to compile
 *			on 2.0.x kernels (thanks to Glenn D. Golden).
 *
 *	v0.2.4	1999/03/10
 *		Tested in ad-hoc mode and with access point (many thanks
 *			to Frank Bruegmann, who made some hardware available
 *			to me so that I can now test it myself).
 *		Change the interrupt service routine to repeat on frame
 *			reception and deallocate the NICs receiving frame
 *			buffer correctly (thanks to Glenn D. Golden).
 *		Fixed a problem with checksums where under some circumstances
 *			an incorrect packet wasn't recognized. Switched
 *			on the kernel checksum checking (thanks to Glenn D. Golden).
 *		Setting the channel value is now checked against valid channel
 *			values which are read from the card.
 *		Added private ioctl (iwconfig priv) station_name, network_name
 *			and current_network. It needs an iwconfig capable of
 *			setting and gettings strings (thanks to Justin Seger).
 *		Ioctl (iwconfig) should now return the real current used channel.
 *		Setting the channel value is now only valid using ad-hoc mode.
 *			It's useless when using an access points.
 *		Renamed the ssid parameter to network_name and made it work
 *			correctly for all port_types. It should work now
 *			in ad-hoc networks as well as with access points.
 *		Added entries for the NCR WaveLAN/IEEE and the Cabletron
 *			RoamAbout 802.11 DS card (thanks to Richard van Leeuwen)
 *		Support to count the received and transmitted bytes
 *			if kernel version >2.1.25.
 *		Changed the reset method in case of Tx-timeouts.
 *		PM suspend/resume should work now (thanks to Dave Kristol).
 *		Changed installation and driver package. Read INSTALL in this
 *			file for information how it works now.
 *
 *	v0.2.3	1999/02/25
 *		Added support to set the own SSID
 *		Changed standard channel setting to 3 so that it works
 *			with Windows without specifying a channel (the
 *			Windows driver seem to default to channel 3).
 *		Fixed two problems with the Ethernet-II frame de/encapsulation.
 *
 *	v0.2.2	1999/02/07
 *		First public beta release.
 *		Added support to get link quality via iwconfig.
 *		Added support to change channel via iwconfig.
 *		Added changeable MTU setting (thanks to Tomasz Motylewski).
 *		Added Ethernet-II frame de/encapsulation, because
 *			HCF-Light doesn't support it.
 *
 *	v0.2.1	1999/02/03
 *		Added channel parameter.
 *		Rewrote the driver with information made public
 *			in Lucent's HCF-Light library. The HCF was
 *			slightly modified to get rid of the compiler
 *			warnings. The filenames were prefixed with
 *			wvlan_ to better fit into the pcmcia package.
 *
 *	v0.1d	1998/12/21
 *		Fixed a problem where the NIC was crashing during heavy
 *			loaded transmissions. Interrupts are now disabled
 *			during wvlan_tx() function. Seems to work fine now.
 *
 *	v0.1c	1998/12/20
 *		Driver works fine with ad-hoc network.
 *
 *	v0.1b	1998/12/19
 *		First successful send-tests.
 *
 *	v0.1a	1998/12/18
 *		First tests with card functions.
 */

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/system.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>

#ifdef HAS_WIRELESS_EXTENSIONS
#include <linux/wireless.h>
#if WIRELESS_EXT < 5
#error "Wireless extension v5 or newer required"
#endif
#define WIRELESS_SPY		// enable iwspy support
#undef HISTOGRAM		// disable histogram of signal levels
#endif

// This is needed for station_name, but we may not compile WIRELESS_EXT
#ifndef IW_ESSID_MAX_SIZE
#define IW_ESSID_MAX_SIZE	32
#endif /* IW_ESSID_MAX_SIZE */

#include "wvlan_hcf.h"

/* #define PCMCIA_DEBUG 1	// For developer only :-) */


/********************************************************************
 * DEBUG
 */
#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>=(n)) printk(KERN_DEBUG args);
#else
#define DEBUG(n, args...) {}
#endif
#define DEBUG_INFO		1
#define DEBUG_NOISY		2
#define DEBUG_TXRX		3
#define DEBUG_CALLTRACE		4
#define DEBUG_INTERRUPT		5


/********************************************************************
 * MISC
 */
static char *version = "1.0.2";
static dev_info_t dev_info = "wvlan_cs";
static dev_link_t *dev_list = NULL;

// Module parameters
static u_int irq_mask = 0xdeb8;				// Interrupt mask
static int irq_list[4] = { -1 };			// Interrupt list (alternative)
static int port_type = 1;				// Port-type [1]
static char station_name[IW_ESSID_MAX_SIZE+1] = "\0";	// Name of station []
static char network_name[IW_ESSID_MAX_SIZE+1] = "\0";	// Name of network []
static int channel = 3;					// Channel [3]
static int ap_density = 1;				// AP density [1]
static int medium_reservation = 2347;			// RTS threshold [2347]
static int frag_threshold = 2346;			// Fragmentation threshold [2346]
static int transmit_rate = 3;				// Transmit rate [3]
static int eth = 0;
static int mtu = 1500;
MODULE_PARM(irq_mask, "i");
MODULE_PARM(irq_list, "1-4i");
MODULE_PARM(port_type, "i");
MODULE_PARM(station_name, "c" __MODULE_STRING(IW_ESSID_MAX_SIZE));
MODULE_PARM(network_name, "c" __MODULE_STRING(IW_ESSID_MAX_SIZE));
MODULE_PARM(channel, "i");
MODULE_PARM(ap_density, "i");
MODULE_PARM(medium_reservation, "i");
MODULE_PARM(frag_threshold, "i");
MODULE_PARM(transmit_rate, "i");
MODULE_PARM(eth, "i");
MODULE_PARM(mtu, "i");

// Ethernet timeout is ((400*HZ)/1000), but we should use a higher
// value, because wireless transmissions are much slower
#define TX_TIMEOUT ((4000*HZ)/1000)

// Ethernet-II snap header
static char snap_header[] = { 0x00, 0x00, 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };

// Valid MTU values (HCF_MAX_MSG (2304) is the max hardware frame size)
#define WVLAN_MIN_MTU 256
#define WVLAN_MAX_MTU (HCF_MAX_MSG - sizeof(snap_header))

// Frequency list (map channels to frequencies)
const long frequency_list[] = { 2412, 2417, 2422, 2427, 2432, 2437, 2442,
				2447, 2452, 2457, 2462, 2467, 2472, 2484 };

// Bit-rate list in 1/2 Mb/s (at least up to turbo - first is dummy)
const int rate_list[] = { 0, 2, 4, -255, 10, 20, -4, -10, 0, 0, 0, 0 };

// Keep track of wvlanX devices
#define MAX_WVLAN_CARDS 16
static struct net_device *wvlandev_index[MAX_WVLAN_CARDS];

// Local data for netdevice
struct net_local {
	dev_node_t		node;
	struct net_device	*dev;		// backtrack device
	dev_link_t		*link;		// backtrack link
	IFB_STRCT		ifb;		// WaveLAN structure
	struct net_device_stats	stats;		// device stats
#ifdef WIRELESS_EXT
	struct iw_statistics	wstats;		// wireless stats
#ifdef WIRELESS_SPY
	int			spy_number;
	u_char			spy_address[IW_MAX_SPY][MAC_ADDR_SIZE];
	struct iw_quality	spy_stat[IW_MAX_SPY];
#endif
#ifdef HISTOGRAM
	int			his_number;
	u_char			his_range[16];
	u_long			his_sum[16];
#endif
#endif /* WIRELESS_EXT */
};

// Shortcuts
#ifdef WIRELESS_EXT
typedef struct iw_statistics	iw_stats;
typedef struct iw_quality	iw_qual;
typedef struct iw_freq		iw_freq;
#endif /* WIRELESS_EXT */

// Show CardServices error message (syslog)
static void cs_error (client_handle_t handle, int func, int ret)
{
	error_info_t err = { func, ret };
	CardServices(ReportError, handle, &err);
}


/********************************************************************
 * FUNCTION PROTOTYPES
 */
int wvlan_hw_setmaxdatalen (IFBP ifbp, int maxlen);
int wvlan_hw_getmacaddr (IFBP ifbp, char *mac, int len);
int wvlan_hw_getchannellist (IFBP ifbp);
int wvlan_hw_setporttype (IFBP ifbp, int ptype);
int wvlan_hw_getporttype (IFBP ifbp);
int wvlan_hw_setstationname (IFBP ifbp, char *name);
int wvlan_hw_getstationname (IFBP ifbp, char *name, int len);
int wvlan_hw_setssid (IFBP ifbp, char *name);
int wvlan_hw_getssid (IFBP ifbp, char *name, int len, int cur);
int wvlan_hw_getbssid (IFBP ifbp, char *mac, int len);
int wvlan_hw_setchannel (IFBP ifbp, int channel);
int wvlan_hw_getchannel (IFBP ifbp);
int wvlan_hw_getcurrentchannel (IFBP ifbp);
int wvlan_hw_setthreshold (IFBP ifbp, int thrh, int mode);
int wvlan_hw_getthreshold (IFBP ifbp, int mode);
int wvlan_hw_setbitrate (IFBP ifbp, int brate);
int wvlan_hw_getbitrate (IFBP ifbp, int cur);
int wvlan_hw_getratelist (IFBP ifbp, char *brlist);
#ifdef WIRELESS_EXT
int wvlan_hw_getfrequencylist (IFBP ifbp, iw_freq *list, int max);
int wvlan_getbitratelist (IFBP ifbp, __s32 *list, int max);
#endif /* WIRELESS_EXT */

int wvlan_hw_config (struct net_device *dev);
int wvlan_hw_shutdown (struct net_device *dev);
int wvlan_hw_reset (struct net_device *dev);

struct net_device_stats *wvlan_get_stats (struct net_device *dev);
#ifdef WIRELESS_EXT
int wvlan_ioctl (struct net_device *dev, struct ifreq *rq, int cmd);
struct iw_statistics *wvlan_get_wireless_stats (struct net_device *dev);
#ifdef WIRELESS_SPY
static inline void wvlan_spy_gather (struct net_device *dev, u_char *mac, u_char *stats);
#endif
#ifdef HISTOGRAM
static inline void wvlan_his_gather (struct net_device *dev, u_char *stats);
#endif
#endif /* WIRELESS_EXT */
int wvlan_change_mtu (struct net_device *dev, int new_mtu);

int wvlan_tx (struct sk_buff *skb, struct net_device *dev);
void wvlan_rx (struct net_device *dev, int len);

static int wvlan_init (struct net_device *dev);
static int wvlan_open (struct net_device *dev);
static int wvlan_close (struct net_device *dev);

static void wvlan_interrupt IRQ (int irq, void *dev_id, struct pt_regs *regs);

static int wvlan_config (dev_link_t *link);
static void wvlan_release (u_long arg);

static dev_link_t *wvlan_attach (void);
static void wvlan_detach (dev_link_t *link);

static int wvlan_event (event_t event, int priority, event_callback_args_t *args);

extern int init_module (void);
extern void cleanup_module (void);


/********************************************************************
 * HARDWARE SETTINGS
 */
// Stupid constants helping clarity
#define WVLAN_CURRENT	1
#define WVLAN_DESIRED	0

int wvlan_hw_setmaxdatalen (IFBP ifbp, int maxlen)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_CNF_MAX_DATA_LEN;
	ltv.id[0] = maxlen;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_put_info(CFG_CNF_MAX_DATA_LEN:0x%x) returned 0x%x\n", dev_info, maxlen, rc);
	return rc;
}

int wvlan_hw_getmacaddr (IFBP ifbp, char *mac, int len)
{
	CFG_MAC_ADDR_STRCT ltv;
	int rc, l;

	ltv.len = 4;
	ltv.typ = CFG_CNF_OWN_MAC_ADDR;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CNF_OWN_MAC_ADDR) returned 0x%x\n", dev_info, rc);
	if (rc)
		return rc;
	l = min(len, ltv.len*2);
	memcpy(mac, (char *)ltv.mac_addr, l);
	return 0;
}

int wvlan_hw_getchannellist (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, chlist;

	ltv.len = 2;
	ltv.typ = CFG_CHANNEL_LIST;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	chlist = ltv.id[0];
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CHANNEL_LIST):0x%x returned 0x%x\n", dev_info, chlist, rc);
	return rc ? 0 : chlist;
}

int wvlan_hw_setporttype (IFBP ifbp, int ptype)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_CNF_PORT_TYPE;
	ltv.id[0] = ptype;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_put_info(CFG_CNF_PORT_TYPE:0x%x) returned 0x%x\n", dev_info, ptype, rc);
	return rc;
}

int wvlan_hw_getporttype (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, ptype;

	ltv.len = 2;
	ltv.typ = CFG_CNF_PORT_TYPE;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	ptype = ltv.id[0];
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CNF_PORT_TYPE):0x%x returned 0x%x\n", dev_info, ptype, rc);
	return rc ? 0 : ptype;
}

int wvlan_hw_setstationname (IFBP ifbp, char *name)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;
	ltv.typ = CFG_CNF_OWN_NAME;
	l = min(strlen(name), ltv.len*2);
	ltv.id[0] = l;
	memcpy((char *) &ltv.id[1], name, l);
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_put_info(CFG_CNF_OWN_NAME:'%s') returned 0x%x\n", dev_info, name, rc);
	return rc;
}

int wvlan_hw_getstationname (IFBP ifbp, char *name, int len)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;
	ltv.typ = CFG_CNF_OWN_NAME;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CNF_OWN_NAME) returned 0x%x\n", dev_info, rc);
	if (rc)
		return rc;
	if (ltv.id[0])
		l = min(len, ltv.id[0]);
	else
		l = min(len, ltv.len*2);	/* It's a feature */
	memcpy(name, (char *) &ltv.id[1], l);
	name[l] = 0;
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CNF_OWN_NAME):'%s'\n", dev_info, name);
	return 0;
}

int wvlan_hw_setssid (IFBP ifbp, char *name)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;
	if (port_type == 3)
		ltv.typ = CFG_CNF_OWN_SSID;
	else
		ltv.typ = CFG_CNF_DESIRED_SSID;
	l = min(strlen(name), ltv.len*2);
	ltv.id[0] = l;
	memcpy((char *) &ltv.id[1], name, l);
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_put_info(CFG_CNF_OWN/DESIRED_SSID:'%s') returned 0x%x\n", dev_info, name, rc);
	return rc;
}

int wvlan_hw_getssid (IFBP ifbp, char *name, int len, int cur)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;
	if (cur)
		ltv.typ = CFG_CURRENT_SSID;
	else
		if (port_type == 3)
			ltv.typ = CFG_CNF_OWN_SSID;
		else
			ltv.typ = CFG_CNF_DESIRED_SSID;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CNF_OWN/DESIRED/CURRENT_SSID) returned 0x%x\n", dev_info, rc);
	if (rc)
		return rc;
	if (ltv.id[0])
	{
		l = min(len, ltv.id[0]);
		memcpy(name, (char *) &ltv.id[1], l);
	}
	else
		l = 0;
	name[l] = '\0';
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CNF_OWN/DESIRED/CURRENT_SSID):'%s'\n", dev_info, name);
	return 0;
}

int wvlan_hw_getbssid (IFBP ifbp, char *mac, int len)
{
	CFG_MAC_ADDR_STRCT ltv;
	int rc, l;

	ltv.len = 4;
	ltv.typ = CFG_CURRENT_BSSID;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CURRENT_BSSID) returned 0x%x\n", dev_info, rc);
	if (rc)
		return rc;
	l = min(len, ltv.len*2);
	memcpy(mac, (char *)ltv.mac_addr, l);
	return 0;
}

int wvlan_hw_setchannel (IFBP ifbp, int channel)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_CNF_OWN_CHANNEL;
	ltv.id[0] = channel;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_put_info(CFG_CNF_OWN_CHANNEL:0x%x) returned 0x%x\n", dev_info, channel, rc);
	return rc;
}

int wvlan_hw_getchannel (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, channel;

	ltv.len = 2;
	ltv.typ = CFG_CNF_OWN_CHANNEL;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	channel = ltv.id[0];
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CNF_OWN_CHANNEL):0x%x returned 0x%x\n", dev_info, channel, rc);
	return rc ? 0 : channel;
}

int wvlan_hw_getcurrentchannel (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, channel;

	ltv.len = 2;
	ltv.typ = CFG_CURRENT_CHANNEL;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	channel = ltv.id[0];
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CURRENT_CHANNEL):0x%x returned 0x%x\n", dev_info, channel, rc);
	return rc ? 0 : channel;
}

int wvlan_hw_setthreshold (IFBP ifbp, int thrh, int cmd)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = cmd;
	ltv.id[0] = thrh;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_put_info(0x%x:0x%x) returned 0x%x\n", dev_info, cmd, thrh, rc);
	return rc;
}

int wvlan_hw_getthreshold (IFBP ifbp, int mode)
{
	CFG_ID_STRCT ltv;
	int rc, thrh;

	ltv.len = 2;
	ltv.typ = mode;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	thrh = ltv.id[0];
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(0x%x):0x%x returned 0x%x\n", dev_info, mode, thrh, rc);
	return rc ? 0 : thrh;
}

int wvlan_hw_setbitrate (IFBP ifbp, int brate)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_TX_RATE_CONTROL;
	ltv.id[0] = brate;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	DEBUG(DEBUG_NOISY, "%s: hcf_put_info(CFG_TX_RATE_CONTROL:0x%x) returned 0x%x\n", dev_info, brate, rc);
	return rc;
}

int wvlan_hw_getbitrate (IFBP ifbp, int cur)
{
	CFG_ID_STRCT ltv;
	int rc, brate;

	ltv.len = 2;
	ltv.typ = cur ? CFG_CURRENT_TX_RATE : CFG_TX_RATE_CONTROL;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	brate = ltv.id[0];
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_TX_RATE_CONTROL):0x%x returned 0x%x\n", dev_info, brate, rc);
	return rc ? 0 : brate;
}

int wvlan_hw_getratelist(IFBP ifbp, char *brlist)
{
	CFG_ID_STRCT ltv;
	int rc, brnum;

	ltv.len = 10;
	ltv.typ = CFG_SUPPORTED_DATA_RATES;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	brnum = ltv.id[0];
	memcpy(brlist, (char *) &ltv.id[1], brnum);
	DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_CHANNEL_LIST):0x%x returned 0x%x\n", dev_info, brnum, rc);
	return rc ? 0 : brnum;
}

#ifdef WIRELESS_EXT
int wvlan_hw_getfrequencylist(IFBP ifbp, iw_freq *list, int max)
{
	int chlist = wvlan_hw_getchannellist(ifbp);
	int i, k = 0;

	/* Compute maximum number of freq to scan */
	if(max > 15)
		max = 15;

	/* Check availability */
	for(i = 0; i < max; i++)
		if((1 << i) & chlist)
		{
#if WIRELESS_EXT > 7
			list[k].i = i + 1;	/* Set the list index */
#endif WIRELESS_EXT
			list[k].m = frequency_list[i] * 100000;
			list[k++].e = 1;	/* Values in table in MHz -> * 10^5 * 10 */
		}

	return k;
}

int wvlan_getbitratelist(IFBP ifbp, __s32 *list, int max)
{
	char brlist[9];
	int brnum = wvlan_hw_getratelist(ifbp, brlist);
	int i;

	/* Compute maximum number of freq to scan */
	if(brnum > max)
		brnum = max;

	/* Convert to Mb/s */
	for(i = 0; i < max; i++)
		list[i] = (brlist[i] & 0x7F) * 500000;

	return brnum;
}
#endif /* WIRELESS_EXT */


/********************************************************************
 * HARDWARE CONFIG / SHUTDOWN / RESET
 */
int wvlan_hw_config (struct net_device *dev)
{
	struct net_local *local = (struct net_local *) dev->priv;
	int rc, i, chlist;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_hw_config(%s)\n", dev->name);

	// Init the HCF library
	hcf_connect(&local->ifb, dev->base_addr);

	// Init hardware and turn on interrupts
	rc = hcf_action(&local->ifb, HCF_ACT_CARD_IN);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_CARD_IN) returned 0x%x\n", dev_info, rc);
#if defined(PCMCIA_DEBUG) && (PCMCIA_DEBUG>=DEBUG_INTERRUPT)
	local->ifb.IFB_IntEnMask |= HREG_EV_TICK;
#endif
	rc = hcf_action(&local->ifb, HCF_ACT_INT_ON);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_INT_ON) returned 0x%x\n", dev_info, rc);

	// Set hardware parameters
	rc = wvlan_hw_setmaxdatalen(&local->ifb, HCF_MAX_MSG);
	if (rc)
		return rc;
	rc = wvlan_hw_setporttype(&local->ifb, port_type);
	if (rc)
		return rc;
	if (*station_name)
		rc = wvlan_hw_setstationname(&local->ifb, station_name);
	if (rc)
		return rc;
	if (*network_name)
		rc = wvlan_hw_setssid(&local->ifb, network_name);
	if (rc)
		return rc;
	rc = wvlan_hw_setthreshold(&local->ifb, ap_density, CFG_CNF_SYSTEM_SCALE);
	if (rc)
		return rc;
	rc = wvlan_hw_setthreshold(&local->ifb, medium_reservation, CFG_RTS_THRH);
	if (rc)
		return rc;
	rc = wvlan_hw_setthreshold(&local->ifb, frag_threshold, CFG_FRAGMENTATION_THRH);
	if (rc)
		return rc;
	rc = wvlan_hw_setthreshold(&local->ifb, transmit_rate, CFG_TX_RATE_CONTROL);
	if (rc)
		return rc;

	// Check valid channel settings
	if (port_type == 3)
	{
		chlist = wvlan_hw_getchannellist(&local->ifb);
		printk(KERN_INFO "%s: Valid channels: ", dev_info);
		for (i=1; i<17; i++)
			if (1<<(i-1) & chlist)
				printk("%d ", i);
		printk("\n");
		if (channel<1 || channel>16 || !(1<<(channel-1) & chlist))
			printk(KERN_WARNING "%s: Channel value of %d is invalid!\n", dev_info, channel);
		else
			rc = wvlan_hw_setchannel(&local->ifb, channel);
		if (rc)
			return rc;
	}

	// Enable hardware
	rc = hcf_enable(&local->ifb, 0);
	DEBUG(DEBUG_NOISY, "%s: hcf_enable(0) returned 0x%x\n", dev_info, rc);
	if (rc)
		return rc;

	// Get MAC address
	rc = wvlan_hw_getmacaddr(&local->ifb, dev->dev_addr, ETH_ALEN);
	if (rc)
		return rc;
	printk(KERN_INFO "%s: MAC address on %s is ", dev_info, dev->name);
	for (i=0; i<ETH_ALEN; i++)
		printk("%02x ", dev->dev_addr[i]);
	printk("\n");

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_hw_config()\n");
	return 0;
}

int wvlan_hw_shutdown (struct net_device *dev)
{
	struct net_local *local = (struct net_local *) dev->priv;
	int rc;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_hw_shutdown(%s)\n", dev->name);

	// Disable and shutdown hardware
	rc = hcf_disable(&local->ifb, 0);
	DEBUG(DEBUG_NOISY, "%s: hcf_disable(0) returned 0x%x\n", dev_info, rc);
	rc = hcf_action(&local->ifb, HCF_ACT_INT_OFF);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_INT_OFF) returned 0x%x\n", dev_info, rc);
	rc = hcf_action(&local->ifb, HCF_ACT_CARD_OUT);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_CARD_OUT) returned 0x%x\n", dev_info, rc);

	// Release HCF library
	hcf_disconnect(&local->ifb);

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_hw_shutdown()\n");
	return 0;
}

int wvlan_hw_reset (struct net_device *dev)
{
	struct net_local *local = (struct net_local *) dev->priv;
	int rc;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_hw_reset(%s)\n", dev->name);

	// Disable hardware
	rc = hcf_disable(&local->ifb, 0);
	DEBUG(DEBUG_NOISY, "%s: hcf_disable(0) returned 0x%x\n", dev_info, rc);
	rc = hcf_action(&local->ifb, HCF_ACT_INT_OFF);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_INT_OFF) returned 0x%x\n", dev_info, rc);

	// Re-Enable hardware
	rc = hcf_action(&local->ifb, HCF_ACT_INT_ON);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_INT_ON) returned 0x%x\n", dev_info, rc);
	rc = hcf_enable(&local->ifb, 0);
	DEBUG(DEBUG_NOISY, "%s: hcf_enable(0) returned 0x%x\n", dev_info, rc);

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_hw_reset()\n");
	return rc;
}


/********************************************************************
 * NET STATS / IOCTL
 */
struct net_device_stats *wvlan_get_stats (struct net_device *dev)
{
	DEBUG(DEBUG_CALLTRACE, "<> wvlan_get_stats(%s)\n", dev->name);
	return(&((struct net_local *) dev->priv)->stats);
}

#ifdef WIRELESS_EXT
int wvlan_ioctl (struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct net_local *local = (struct net_local *) dev->priv;
	struct iwreq *wrq = (struct iwreq *) rq;
	unsigned long flags;
	int rc = 0;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_ioctl(%s, cmd=0x%x)\n", dev->name, cmd);

	// Only super-user may write any setting
	if (IW_IS_SET(cmd) && cmd >= SIOCSIWNAME && !suser())
		return -EPERM;

	// Disable interrupts
	// This prevents from being disturbed by an interrupt
	// during our fiddling with the NIC.
	// TODO: Generally this shouldn't be only here, but also
	// at many other places where an interruption of our
	// talk to the card is undesired. Maybe hcfio_string
	// should generally handle it.
	save_flags(flags); cli();

	switch (cmd)
	{
		// Get name
		case SIOCGIWNAME:
			strcpy(wrq->u.name, "WaveLAN/IEEE");
			break;

		// Set frequency/channel
		case SIOCSIWFREQ:
			// If setting by frequency, convert to a channel
			if((wrq->u.freq.e == 1) &&
			   (wrq->u.freq.m >= (int) 2.412e8) &&
			   (wrq->u.freq.m <= (int) 2.487e8))
			{
				int f = wrq->u.freq.m / 100000;
				int c = 0;
				while((c < 14) && (f != frequency_list[c]))
					c++;
				// Hack to fall through...
				wrq->u.freq.e = 0;
				wrq->u.freq.m = c + 1;
			}
			// Setting by channel number
			if ((port_type != 3) || (wrq->u.freq.m > 1000) || (wrq->u.freq.e > 0))
				rc = -EOPNOTSUPP;
			else
			{
				int channel = wrq->u.freq.m;
				int chlist = wvlan_hw_getchannellist(&local->ifb);
				if (channel<1 || channel>16 || !(1<<(channel-1) & chlist))
				{
					DEBUG(DEBUG_INFO, "%s: New channel value of %d for %s is invalid!\n", dev_info, wrq->u.freq.m, dev->name);
					rc = -EINVAL;
				}
				else
				{
					wvlan_hw_setchannel(&local->ifb, wrq->u.freq.m);
					wvlan_hw_reset(dev);
					channel = wrq->u.freq.m;
				}
			}
			break;

		// Get frequency/channel
		case SIOCGIWFREQ:
#ifdef WEXT_USECHANNELS
			wrq->u.freq.m = wvlan_hw_getcurrentchannel(&local->ifb);
			wrq->u.freq.e = 0;
#else
			{
				int f = wvlan_hw_getcurrentchannel(&local->ifb);
				wrq->u.freq.m = frequency_list[f-1] * 100000;
				wrq->u.freq.e = 1;
			}
#endif
			break;

#if WIRELESS_EXT > 5
		// Set desired network name (ESSID)
		case SIOCSIWESSID:
			if (wrq->u.data.pointer)
			{
				char	essid[IW_ESSID_MAX_SIZE + 1];

				/* Check if we asked for `any' */
				if(wrq->u.data.flags == 0)
				{
					essid[0] = '\0';
				}
				else
				{
					/* Check the size of the string */
					if(wrq->u.data.length >
					   IW_ESSID_MAX_SIZE + 1)
					{
						rc = -E2BIG;
						break;
					}
					copy_from_user(essid,
						       wrq->u.data.pointer,
						       wrq->u.data.length);
					essid[IW_ESSID_MAX_SIZE] = '\0';
				}
				wvlan_hw_setssid(&local->ifb, essid);
				wvlan_hw_reset(dev);
				strncpy(network_name, essid, sizeof(network_name)-1);
			}
			break;

		// Get current network name (ESSID)
		case SIOCGIWESSID:
			if (wrq->u.data.pointer)
			{
				char essid[IW_ESSID_MAX_SIZE + 1];
				/* Get the essid that was set */
				wvlan_hw_getssid(&local->ifb, essid,
						 IW_ESSID_MAX_SIZE,
						 WVLAN_DESIRED);
				/* If it was set to any, get the current one */
				if(strlen(essid) == 0)
					wvlan_hw_getssid(&local->ifb, essid,
							 IW_ESSID_MAX_SIZE,
							 WVLAN_CURRENT);

				/* Push it out ! */
				wrq->u.data.length = strlen(essid) + 1;
				wrq->u.data.flags = 1; /* active */
				copy_to_user(wrq->u.data.pointer, essid, sizeof(essid));
			}
			break;

		// Get current Access Point (BSSID)
		case SIOCGIWAP:
			wvlan_hw_getbssid(&local->ifb, wrq->u.ap_addr.sa_data, ETH_ALEN);
			wrq->u.ap_addr.sa_family = ARPHRD_ETHER;
			break;

#endif	/* WIRELESS_EXT > 5 */

#if WIRELESS_EXT > 7
		// Set desired station name
		case SIOCSIWNICKN:
			if (wrq->u.data.pointer)
			{
				char	name[IW_ESSID_MAX_SIZE + 1];

				/* Check the size of the string */
				if(wrq->u.data.length > IW_ESSID_MAX_SIZE + 1)
				{
					rc = -E2BIG;
					break;
				}
				copy_from_user(name, wrq->u.data.pointer, wrq->u.data.length);
				name[IW_ESSID_MAX_SIZE] = '\0';
				wvlan_hw_setstationname(&local->ifb, name);
				wvlan_hw_reset(dev);
				strncpy(station_name, name, sizeof(station_name)-1);
			}
			break;

		// Get current station name
		case SIOCGIWNICKN:
			if (wrq->u.data.pointer)
			{
				char name[IW_ESSID_MAX_SIZE + 1];
				wvlan_hw_getstationname(&local->ifb, name, IW_ESSID_MAX_SIZE);
				wrq->u.data.length = strlen(name) + 1;
				copy_to_user(wrq->u.data.pointer, name, sizeof(name));
			}
			break;

		// Set the desired bit-rate
		case SIOCSIWRATE:
			{
				// Start the magic...
				char	brlist[9];
				int	brnum = wvlan_hw_getratelist(&local->ifb, brlist);
				int	brate = wrq->u.bitrate.value/500000;
				int	wvrate = 0;

				// Auto or fixed ?
				if(wrq->u.bitrate.fixed == 0)
					// Let be simple for now...
					wvrate = 3;
					// We could read the last desired
					// channel and set corresp. auto mode
				else if((wrq->u.bitrate.value <= (brnum * 2 - 1)) &&
					(wrq->u.bitrate.value > 0))
				{
					// Setting by channel index
					wvrate = wrq->u.bitrate.value;
				}
				else
				{
					// Setting by frequency value
					// Find index in magic table
					while((rate_list[wvrate] != brate) &&
					      (wvrate < (brnum * 2)))
						wvrate++;

					// Check if in range
					if((wvrate < 1) ||
					   (wvrate >= (brnum * 2)))
					{
						rc = -EINVAL;
						break;
					}
				}
				wvlan_hw_setbitrate(&local->ifb, wvrate);
				wvlan_hw_reset(dev);
				transmit_rate = wvrate;
			}
			break;

		// Get the current bit-rate
		case SIOCGIWRATE:
			{
				int	wvrate = wvlan_hw_getbitrate(&local->ifb, WVLAN_DESIRED);
				int	brate = rate_list[wvrate];

				// Auto ?
				if (brate < 0)
				{
					wrq->u.bitrate.fixed = 0;
					wvrate = wvlan_hw_getbitrate(&local->ifb, WVLAN_CURRENT);
					brate = rate_list[wvrate];
				}
				else
					wrq->u.bitrate.fixed = 1;

				wrq->u.bitrate.value = brate * 500000;
			}
			break;

		// Set the desired RTS threshold
		case SIOCSIWRTS:
			{
				int rthr = wrq->u.rts.value;
				// if(wrq->u.rts.fixed == 0) we should complain
				if(rthr == -1)
					rthr = 2347;
				if((rthr < 0) || (rthr > 2347))
				{
					rc = -EINVAL;
					break;
				}
				wvlan_hw_setthreshold(&local->ifb, rthr, CFG_RTS_THRH);
				wvlan_hw_reset(dev);
				medium_reservation = rthr;
			}
			break;

		// Get the current RTS threshold
		case SIOCGIWRTS:
			wrq->u.rts.value = wvlan_hw_getthreshold(&local->ifb, CFG_RTS_THRH);
			wrq->u.rts.fixed = 1;
			break;

		// Set the desired fragmentation threshold
		case SIOCSIWFRAG:
			{
				int fthr = wrq->u.frag.value;
				// if(wrq->u.frag.fixed == 0) should complain
				if(fthr == -1)
					fthr = 2346;
				if((fthr < 256) || (fthr > 2346) ||
				   ((fthr % 2) == 1))
				{
					rc = -EINVAL;
					break;
				}
				wvlan_hw_setthreshold(&local->ifb, fthr, CFG_FRAGMENTATION_THRH);
				wvlan_hw_reset(dev);
			}
			break;

		// Get the current fragmentation threshold
		case SIOCGIWFRAG:
			wrq->u.frag.value = wvlan_hw_getthreshold(&local->ifb, CFG_FRAGMENTATION_THRH);
			wrq->u.frag.fixed = 1;
			break;

		// Set the desired AP density
		case SIOCSIWSENS:
			{
				int dens = wrq->u.sens.value;
				if((dens < 1) || (dens > 3))
				{
					rc = -EINVAL;
					break;
				}
				wvlan_hw_setthreshold(&local->ifb, dens, CFG_CNF_SYSTEM_SCALE);
				wvlan_hw_reset(dev);
				ap_density = dens;
			}
			break;

		// Get the current AP density
		case SIOCGIWSENS:
			wrq->u.sens.value = wvlan_hw_getthreshold(&local->ifb, CFG_CNF_SYSTEM_SCALE);
			wrq->u.sens.fixed = 0;	/* auto */
			break;
#endif	/* WIRELESS_EXT > 7 */

		// Get range of parameters
		case SIOCGIWRANGE:
			if (wrq->u.data.pointer)
			{
				struct iw_range range;
				rc = verify_area(VERIFY_WRITE, wrq->u.data.pointer, sizeof(struct iw_range));
				if (rc)
					break;
				wrq->u.data.length = sizeof(range);
				range.throughput = 2.0 * 1024 * 1024;	// 2 Mbps
				range.min_nwid = 0x0000;
				range.max_nwid = 0x0000;
				range.num_channels = 14;
				range.num_frequency = wvlan_hw_getfrequencylist(&local->ifb,
						      range.freq,
						      IW_MAX_FREQUENCIES);
				range.sensitivity = 3;
				if (port_type == 3 && local->spy_number == 0)
				{
					range.max_qual.qual = 0;
					range.max_qual.level = 0;
					range.max_qual.noise = 0;
				}
				else
				{
					range.max_qual.qual = 0x8b - 0x2f;
					range.max_qual.level = 0x2f - 0x95 - 1;
					range.max_qual.noise = 0x2f - 0x95 - 1;
				}
#if WIRELESS_EXT > 7
				range.num_bitrates = wvlan_getbitratelist(&local->ifb,
							range.bitrate,
							IW_MAX_BITRATES);
				range.min_rts = 0;
				range.max_rts = 2347;
				range.min_frag = 256;
				range.max_frag = 2346;
#endif	/* WIRELESS_EXT > 7 */
				copy_to_user(wrq->u.data.pointer, &range, sizeof(struct iw_range));
			}
			break;

#ifdef WIRELESS_SPY
		// Set the spy list
		case SIOCSIWSPY:
			if (wrq->u.data.length > IW_MAX_SPY)
			{
				rc = -E2BIG;
				break;
			}
			local->spy_number = wrq->u.data.length;
			if (local->spy_number > 0)
			{
				struct sockaddr address[IW_MAX_SPY];
				int i;
				rc = verify_area(VERIFY_READ, wrq->u.data.pointer, sizeof(struct sockaddr) * local->spy_number);
				if (rc)
					break;
				copy_from_user(address, wrq->u.data.pointer, sizeof(struct sockaddr) * local->spy_number);
				for (i=0; i<local->spy_number; i++)
					memcpy(local->spy_address[i], address[i].sa_data, MAC_ADDR_SIZE);
				memset(local->spy_stat, 0, sizeof(struct iw_quality) * IW_MAX_SPY);
				DEBUG(DEBUG_INFO, "%s: New spy list:\n", dev_info);
				for (i=0; i<wrq->u.data.length; i++)
					DEBUG(DEBUG_INFO, "%s: %d - %02x:%02x:%02x:%02x:%02x:%02x\n", dev_info, i+1,
						local->spy_address[i][0], local->spy_address[i][1],
						local->spy_address[i][2], local->spy_address[i][3],
						local->spy_address[i][4], local->spy_address[i][5]);
			}
			break;

		// Get the spy list
		case SIOCGIWSPY:
			wrq->u.data.length = local->spy_number;
			if ((local->spy_number > 0) && (wrq->u.data.pointer))
			{
				struct sockaddr address[IW_MAX_SPY];
				int i;
				rc = verify_area(VERIFY_WRITE, wrq->u.data.pointer, (sizeof(struct iw_quality)+sizeof(struct sockaddr)) * IW_MAX_SPY);
				if (rc)
					break;
				for (i=0; i<local->spy_number; i++)
				{
					memcpy(address[i].sa_data, local->spy_address[i], MAC_ADDR_SIZE);
					address[i].sa_family = AF_UNIX;
				}
				copy_to_user(wrq->u.data.pointer, address, sizeof(struct sockaddr) * local->spy_number);
				copy_to_user(wrq->u.data.pointer + (sizeof(struct sockaddr)*local->spy_number), local->spy_stat, sizeof(struct iw_quality) * local->spy_number);
				for (i=0; i<local->spy_number; i++)
					local->spy_stat[i].updated = 0;
			}
			break;
#endif /* WIRELESS_SPY */

#ifdef HISTOGRAM
		// Set the histogram range
		case SIOCDEVPRIVATE + 0xd:
			if (wrq->u.data.length > 16)
			{
				rc = -E2BIG;
				break;
			}
			local->his_number = wrq->u.data.length;
			if (local->his_number > 0)
			{
				rc = verify_area(VERIFY_READ, wrq->u.data.pointer, sizeof(char) * local->his_number);
				if (rc)
					break;
				copy_from_user(local->his_range, wrq->u.data.pointer, sizeof(char) * local->his_number);
				memset(local->his_sum, 0, sizeof(long) * 16);
			}
			break;

		// Get the histogram statistic
		case SIOCDEVPRIVATE + 0xe:
			wrq->u.data.length = local->his_number;
			if ((local->his_number > 0) && (wrq->u.data.pointer))
			{
				rc = verify_area(VERIFY_WRITE, wrq->u.data.pointer, sizeof(long) * 16);
				if (rc)
					break;
				copy_to_user(wrq->u.data.pointer, local->his_sum, sizeof(long) * local->his_number);
			}
			break;
#endif /* HISTOGRAM */

		// Get valid private ioctl calls
		case SIOCGIWPRIV:
			if (wrq->u.data.pointer)
			{
				struct iw_priv_args priv[] = {
					{ SIOCDEVPRIVATE + 0x0, IW_PRIV_TYPE_CHAR | 32, IW_PRIV_TYPE_CHAR | 32, "station_name" },
					{ SIOCDEVPRIVATE + 0x1, IW_PRIV_TYPE_CHAR | 32, IW_PRIV_TYPE_CHAR | 32, "network_name" },
					{ SIOCDEVPRIVATE + 0x2, 0, IW_PRIV_TYPE_CHAR | 32, "current_network" },
					{ SIOCDEVPRIVATE + 0xb, IW_PRIV_TYPE_BYTE | IW_PRIV_SIZE_FIXED | 1, 0, "set_port" },
					{ SIOCDEVPRIVATE + 0xc, 0, IW_PRIV_TYPE_BYTE | IW_PRIV_SIZE_FIXED | 1, "get_port" },
#ifdef HISTOGRAM
					{ SIOCDEVPRIVATE + 0xd, IW_PRIV_TYPE_BYTE | 16, 0, "sethisto" },
					{ SIOCDEVPRIVATE + 0xe, 0, IW_PRIV_TYPE_INT | 16, "gethisto" },
#endif
#ifdef PCMCIA_DEBUG
					{ SIOCDEVPRIVATE + 0xf, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "debug_getinfo" },
#endif
				};
				rc = verify_area(VERIFY_WRITE, wrq->u.data.pointer, sizeof(priv));
				if (rc)
					break;
				wrq->u.data.length = sizeof(priv) / sizeof(priv[0]);
				copy_to_user(wrq->u.data.pointer, priv, sizeof(priv));
			}
			break;

		// Get/set station name
		case SIOCDEVPRIVATE + 0x0:
			if (wrq->u.data.pointer)
			{
				if (wrq->u.data.length > 1)
				{
					char buf[32];
					copy_from_user(buf, wrq->u.data.pointer, 32);
					if (!suser())
						rc = -EPERM;
					else
					{
						wvlan_hw_setstationname(&local->ifb, buf);
						wvlan_hw_reset(dev);
						strncpy(station_name, buf, sizeof(station_name)-1);
					}
				}
				else
				{
					char buf[32];
					wvlan_hw_getstationname(&local->ifb, buf, 32);
					wrq->u.data.length = strlen(buf) + 1;
					copy_to_user(wrq->u.data.pointer, buf, sizeof(buf));
				}
			}
			break;

		// Get/set network name
		case SIOCDEVPRIVATE + 0x1:
			if (wrq->u.data.pointer)
			{
				if (wrq->u.data.length > 1)
				{
					char buf[32];
					copy_from_user(buf, wrq->u.data.pointer, 32);
					if (!suser())
						rc = -EPERM;
					else
					{
						wvlan_hw_setssid(&local->ifb, buf);
						wvlan_hw_reset(dev);
						strncpy(network_name, buf, sizeof(network_name)-1);
					}
				}
				else
				{
					char buf[32];
					wvlan_hw_getssid(&local->ifb, buf, 32, WVLAN_DESIRED);
					wrq->u.data.length = strlen(buf) + 1;
					copy_to_user(wrq->u.data.pointer, buf, sizeof(buf));
				}
			}
			break;

		// Get current network name
		case SIOCDEVPRIVATE + 0x2:
			if (wrq->u.data.pointer)
			{
				char buf[32];
				wvlan_hw_getssid(&local->ifb, buf, 32, WVLAN_CURRENT);
				wrq->u.data.length = strlen(buf) + 1;
				copy_to_user(wrq->u.data.pointer, buf, sizeof(buf));
			}
			break;

		// Set port type
		case SIOCDEVPRIVATE + 0xb:
			{
				char ptype = *(wrq->u.name);

				if(!suser())
					rc = -EPERM;
				else if ((ptype < 1) || (ptype > 3))
					rc = -EINVAL;
				else
				{
					wvlan_hw_setporttype(&local->ifb, ptype);
					wvlan_hw_reset(dev);
					port_type = ptype;
				}
			}
			break;

		// Get port type
		case SIOCDEVPRIVATE + 0xc:
			*(wrq->u.name) = wvlan_hw_getporttype(&local->ifb);
			break;

#ifdef PCMCIA_DEBUG
		// Get info from card and dump answer to syslog (debug purpose only)
		case SIOCDEVPRIVATE + 0xf:
			{
				CFG_ID_STRCT ltv;
				char *p;
				int typ = *((int *) wrq->u.name);
				ltv.len = 18;
				ltv.typ = typ;
				rc = hcf_get_info(&local->ifb, (LTVP) &ltv);
				if (rc)
					printk(KERN_DEBUG "%s: hcf_get_info(0x%x) returned error 0x%x\n", dev_info, typ, rc);
				else
				{
					p = (char *) &ltv.id;
					printk(KERN_DEBUG "%s: hcf_get_info(0x%x) returned %d words:\n", dev_info, ltv.typ, ltv.len);
					printk(KERN_DEBUG "%s: hex-dump: ", dev_info);
					for (rc=0; rc<(ltv.len); rc++)
						printk("%04x ", ltv.id[rc]);
					printk("\n");
					printk(KERN_DEBUG "%s: ascii-dump: '", dev_info);
					for (rc=0; rc<(ltv.len*2); rc++)
						printk("%c", (p[rc]>31) ? p[rc] : '.');
					printk("'\n");
				}
			}
			break;
#endif /* PCMCIA_DEBUG */

		// All other calls are currently unsupported
		default:
			rc = -EOPNOTSUPP;
	}

	// Re-enable interrupts
	restore_flags(flags);

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_ioctl()\n");
	return rc;
}

struct iw_statistics *wvlan_get_wireless_stats (struct net_device *dev)
{
	struct net_local *local = (struct net_local *) dev->priv;
	CFG_COMMS_QUALITY_STRCT ltv;
	int rc;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_get_wireless_stats(%s)\n", dev->name);

	local->wstats.status = 0;
	if (port_type != 3)
	{
		ltv.len = 4;
		ltv.typ = CFG_COMMS_QUALITY;
		rc = hcf_get_info(&local->ifb, (LTVP) &ltv);
		DEBUG(DEBUG_NOISY, "%s: hcf_get_info(CFG_COMMS_QUALITY) returned 0x%x\n", dev_info, rc);
		local->wstats.qual.qual = max(min(ltv.coms_qual, 0x8b-0x2f), 0);
		local->wstats.qual.level = max(min(ltv.signal_lvl, 0x8a), 0x2f) - 0x95;
		local->wstats.qual.noise = max(min(ltv.noise_lvl, 0x8a), 0x2f) - 0x95;
		local->wstats.qual.updated = 7;
	}
	else
	{
		// Quality levels cannot be determined in ad-hoc mode,
		// because we can 'hear' more that one remote station.
		// If a spy address is defined, we report stats of the
		// first spy address
		local->wstats.qual.qual = 0;
		local->wstats.qual.level = 0;
		local->wstats.qual.noise = 0;
		local->wstats.qual.updated = 0;
		if (local->spy_number > 0)
		{
			local->wstats.qual.qual = local->spy_stat[0].qual;
			local->wstats.qual.level = local->spy_stat[0].level;
			local->wstats.qual.noise = local->spy_stat[0].noise;
			local->wstats.qual.updated = local->spy_stat[0].updated;
		}
	}

	// Packets discarded in the wireless adapter due to wireless specific problems
	local->wstats.discard.nwid = 0;
	local->wstats.discard.code = local->ifb.IFB_NIC_Tallies.RxWEPUndecryptable;
	local->wstats.discard.misc = local->ifb.IFB_NIC_Tallies.RxFCSErrors +
					local->ifb.IFB_NIC_Tallies.RxDiscards_NoBuffer +
					local->ifb.IFB_NIC_Tallies.TxDiscardsWrongSA;

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_get_wireless_stats()\n");
	return (&local->wstats);
}

#ifdef WIRELESS_SPY
static inline void wvlan_spy_gather (struct net_device *dev, u_char *mac, u_char *stats)
{
	struct net_local *local = (struct net_local *)dev->priv;
	int i;

	// Gather wireless spy statistics: for each packet, compare the
	// source address with out list, and if match, get the stats...
	for (i=0; i<local->spy_number; i++)
		if (!memcmp(mac, local->spy_address[i], MAC_ADDR_SIZE))
		{
			local->spy_stat[i].qual = stats[2];
			local->spy_stat[i].level = stats[0] - 0x95;
			local->spy_stat[i].noise = stats[1] - 0x95;
			local->spy_stat[i].updated = 7;
		}
}
#endif /* WIRELESS_SPY */

#ifdef HISTOGRAM
static inline void wvlan_his_gather (struct net_device *dev, u_char *stats)
{
	struct net_local *local = (struct net_local *)dev->priv;
	u_char level = stats[0] - 0x2f;
	int i;

	// Calculate a histogram of the signal level. Each time the
	// level goes into our defined set of interval, we increment
	// the count.
	i = 0;
	while ((i < (local->his_number-1)) && (level >= local->his_range[i++]));
	local->his_sum[i]++;
}
#endif /* HISTOGRAM */
#endif /* WIRELESS_EXT */

int wvlan_change_mtu (struct net_device *dev, int new_mtu)
{
	if (new_mtu < WVLAN_MIN_MTU || new_mtu > WVLAN_MAX_MTU)
	{
		DEBUG(DEBUG_INFO, "%s: New MTU of %d for %s out of range!\n", dev_info, new_mtu, dev->name);
		return -EINVAL;
	}
	dev->mtu = new_mtu;
	DEBUG(DEBUG_INFO, "%s: MTU of %s set to %d bytes\n", dev_info, dev->name, new_mtu);
	return 0;
}


/********************************************************************
 * NET TX / RX
 */
int wvlan_tx (struct sk_buff *skb, struct net_device *dev)
{
	struct net_local *local = (struct net_local *)dev->priv;
	unsigned long flags;
	int rc, len;
	char *p;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_tx(%s)\n", dev->name);

	// We normally shouldn't be called if dev->tbusy is set, but
	// the existing kernel code does anyway. So we'll check if the
	// last transmission has timed out and reset the device in case
	if (dev->tbusy)
	{
		DEBUG(DEBUG_TXRX, "%s: wvlan_tx(%s) called while busy!\n", dev_info, dev->name);
		if ((jiffies - dev->trans_start) < TX_TIMEOUT)
			return 1;
		if (!dev->start)
		{
			printk(KERN_WARNING "%s: %s Tx on stopped device!\n", dev_info, dev->name);
			return 1;
		}

		// Reset card in case of Tx timeout
		printk(KERN_WARNING "%s: %s Tx timed out! Resetting card\n", dev_info, dev->name);
		wvlan_hw_shutdown(dev);
		wvlan_hw_config(dev);
	}

	skb_tx_check(dev, skb);
#if (LINUX_VERSION_CODE < VERSION(2,1,79))
	skb->arp = 1;
#endif

	// Disable interrupts
	// This prevents from being disturbed by an interrupt
	// during our transmission to the NIC.
	// TODO: Generally this shouldn't be only here, but also
	// at many other places where an interruption of our
	// talk to the card is undesired. Maybe hcfio_string
	// should generally handle it.
	save_flags(flags); cli();

	// Send packet
	dev->tbusy = 1;
	p = skb->data;
	len = (ETH_ZLEN < skb->len) ? skb->len : ETH_ZLEN;
	dev->trans_start = jiffies;

	// Add Ethernet-II frame encapsulation, because
	// HCF-light doesn't support that.
	if (p[13] + (p[12] << 8) > 1500)
	{
		hcf_put_data(&local->ifb, p, 12, 0);
		len += sizeof(snap_header);
		snap_header[1] = (len-0x0e) & 0xff;
		snap_header[0] = (char)((len-0x0e) >> 8);
		hcf_put_data(&local->ifb, snap_header, sizeof(snap_header), 0);
		hcf_put_data(&local->ifb, p+12, len-12-sizeof(snap_header), 0);
	}
	else
		hcf_put_data(&local->ifb, p, len, 0);

	// Send packet
	rc = hcf_send(&local->ifb, 0);
	add_tx_bytes(&local->stats, len);

	// Re-enable interrupts
	restore_flags(flags);

	// It might be no good idea doing a printk() debug output during
	// disabled interrupts (I'm not sure...). So better do it here.
	DEBUG(DEBUG_TXRX, "%s: Sending 0x%x octets\n", dev_info, len);
	DEBUG(DEBUG_NOISY, "%s: hcf_send() returned 0x%x\n", dev_info, rc);

	DEV_KFREE_SKB(skb);
	DEBUG(DEBUG_CALLTRACE, "<- wvlan_tx()\n");
	return 0;
}

void wvlan_rx (struct net_device *dev, int len)
{
	struct net_local *local = (struct net_local *)dev->priv;
	struct sk_buff *skb;
	char *p;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_rx(%s)\n", dev->name);

	// Create skb packet
	skb = dev_alloc_skb(len+2);
	if (!skb)
	{
		printk(KERN_WARNING "%s: %s Rx cannot allocate buffer for new packet\n", dev_info, dev->name);
		local->stats.rx_dropped++;
		return;
	}
	DEBUG(DEBUG_TXRX, "%s: Receiving 0x%x octets\n", dev_info, len);

	// Align IP on 16b boundary
	skb_reserve(skb, 2);
	p = skb_put(skb, len);
	dev->last_rx = jiffies;

	// Add Ethernet-II frame decapsulation, because
	// HCF-light doesn't support that.
	if (local->ifb.IFB_RxStat == 0x2000 || local->ifb.IFB_RxStat == 0x4000)
	{
		hcf_get_data(&local->ifb, 0, p, 12);
		hcf_get_data(&local->ifb, 12+sizeof(snap_header), p+12, len-12-sizeof(snap_header));
		skb_trim(skb, len-sizeof(snap_header));
	}
	else
		hcf_get_data(&local->ifb, 0, p, len);

	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_NONE;

	// Hand the packet over to the kernel
	netif_rx(skb);
	local->stats.rx_packets++;
	add_rx_bytes(&local->stats, len);

#ifdef WIRELESS_EXT
#if defined(WIRELESS_SPY) || defined(HISTOGRAM)
	if (
#ifdef WIRELESS_SPY
		(local->spy_number > 0) ||
#endif
#ifdef HISTOGRAM
		(local->his_number > 0) ||
#endif
		0 )
	{
#if (LINUX_VERSION_CODE >= VERSION(1,3,0))
		char *srcaddr = skb->mac.raw + MAC_ADDR_SIZE;
#else
		char *srcaddr = skb->data + MAX_ADDR_SIZE;
#endif
		u_char stats[3];
		int rc, i;
		local->wstats.status = 0;

		// Using spy support with port_type==1 will really
		// slow down everything, because the signal quality
		// must be queried for each packet here.
		// If the user really asks for it (set some address in the
		// spy list), we do it, but he will pay the price.
		// Note that to get here, you need both WIRELESS_SPY
		// compiled in AND some addresses in the list !!!
		// TODO: Get and cache stats here so that they
		// are available but don't need to be retreived
		// every time a packet is received.
#if defined(HISTOGRAM)
		// We can't be clever...
		rc = hcf_get_data(&local->ifb, HFS_Q_INFO, stats, 2);
#else // Therefore WIRELESS_SPY only !!!
		memset(&stats, 0, sizeof(stats));
		// Query only for addresses in our list !
		for (i=0; i<local->spy_number; i++)
			if (!memcmp(srcaddr, local->spy_address[i], MAC_ADDR_SIZE))
			{
				rc = hcf_get_data(&local->ifb, HFS_Q_INFO, stats, 2);
				break;
			}
#endif
		DEBUG(DEBUG_NOISY, "%s: hcf_get_data(HFS_Q_INFO) returned 0x%x\n", dev_info, rc);
		stats[2] = stats[0];
		stats[0] = max(min(stats[1], 0x8a), 0x2f);
		stats[1] = max(min(stats[2], 0x8a), 0x2f);
		stats[2] = stats[0] - stats[1];
#ifdef WIRELESS_SPY
		wvlan_spy_gather(dev, srcaddr, stats);  
#endif
#ifdef HISTOGRAM
		wvlan_his_gather(dev, stats);
#endif
	}
#endif /* WIRELESS_SPY || HISTOGRAM */
#endif /* WIRELESS_EXT */

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_rx()\n");
}


/********************************************************************
 * NET INIT / OPEN / CLOSE
 */
static int wvlan_init (struct net_device *dev)
{
	DEBUG(DEBUG_CALLTRACE, "<> wvlan_init()\n");
	return 0;
}

static int wvlan_open (struct net_device *dev)
{
	struct net_local *local = (struct net_local *) dev->priv;
	struct dev_link_t *link = local->link;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_open(%s)\n", dev->name);

	// TODO: Power up the card here and power down on close?
	// For now this is done on device init, not on open
	// Might be better placed here so that some settings can
	// be made by shutting down the device without removing
	// the driver (iwconfig).
	// But this is no real problem for now :-)

	// Start reception and declare the driver ready
	if (!local->ifb.IFB_CardStat)
		return -ENODEV;
	dev->start = 1;
	dev->tbusy = 0;
	link->open++;
	MOD_INC_USE_COUNT;

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_open()\n");
	return 0;
}

static int wvlan_close (struct net_device *dev)
{
	struct net_local *local = (struct net_local *) dev->priv;
	struct dev_link_t *link = local->link;

	// If the device isn't open, then nothing to do
	if (!link->open)
	{
		DEBUG(DEBUG_CALLTRACE, "<> wvlan_close(%s)\n", dev->name);
		return 0;
	}

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_close(%s)\n", dev->name);

	// Close the device
	link->open--;
	MOD_DEC_USE_COUNT;

	// Check if card is still present
	if (dev->start)
	{
		dev->tbusy = 1;
		dev->start = 0;
		// TODO: Shutdown hardware (see wvlan_open)
	}
	else
		if (link->state & DEV_STALE_CONFIG)
			wvlan_release((u_long)link);

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_close()\n");
	return -EINVAL;
}


/********************************************************************
 * INTERRUPT HANDLER
 */
static void wvlan_interrupt IRQ (int irq, void *dev_id, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device *) dev_id;
	struct net_local *local = (struct net_local *) dev->priv;
	int rc, cnt, ev, len;

	DEBUG(DEBUG_INTERRUPT, "-> wvlan_interrupt(%d)\n", irq);

	// Check device
	if (!dev)
	{
		printk(KERN_WARNING "%s: IRQ %d for unknown device!\n", dev_info, irq);
		return;
	}

	// Turn off interrupts (lock)
	rc = hcf_action(&local->ifb, HCF_ACT_INT_OFF);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_INT_OFF) returned 0x%x\n", dev_info, rc);

	// Process pending interrupts.
	// We continue until hcf_service_nic tells that no received
	// frames are pending. However we should check to not lock up
	// here in an endless loop.
	cnt = 5;
	while (cnt--)
	{
		// Ask NIC why interrupt occured
		ev = hcf_service_nic(&local->ifb);
		DEBUG(DEBUG_NOISY, "%s: hcf_service_nic() returned 0x%x RscInd 0x%x tbusy %ld\n", dev_info, ev, local->ifb.IFB_PIFRscInd, dev->tbusy);

		// Transmission completion seem to be also signalled with ev==0
		// better check that out with RscInd and complete transfer also
		if (local->ifb.IFB_PIFRscInd && dev->tbusy)
			ev |= HREG_EV_TX;

		// HREG_EV_TICK: WMAC controller auxiliary timer tick
		if (ev & HREG_EV_TICK)
		{
			DEBUG(DEBUG_INFO,"%s: Auxiliary timer tick\n", dev_info);
		}

		// HREG_EV_RES: WMAC controller H/W error (wait timeout)
		if (ev & HREG_EV_RES)
		{
			// This message seems to occur often on heavy load
			// but it seem to don't have any effects on transmission
			// so we simply ignore it.
			//printk(KERN_WARNING "%s: WMAC H/W error (wait timeout, ignoring)!\n", dev_info);
		}

		// HREG_EV_INFO_DROP: WMAC did not have sufficient RAM to build unsollicited frame
		if (ev & HREG_EV_INFO_DROP)
			printk(KERN_WARNING "%s: WMAC did not have sufficient RAM to build unsollicited frame!\n", dev_info);

		// HREG_EV_INFO: WMAC controller asynchronous information frame
		if (ev & HREG_EV_INFO)
		{
			DEBUG(DEBUG_INFO, "%s: WMAC controller asynchronous information frame\n", dev_info);
		}

		// HREG_EV_CMD: WMAC controller command completed, status and response available
		//	unnecessary to handle here, it's handled by polling in HCF

		// HREG_EV_ALLOC: WMAC controller asynchronous part of allocation/reclaim completed
		//	also unnecessary to handle here, it's handled by polling in HCF

		// HREG_EV_TX_EXC: WMAC controller asynchronous transmission unsuccessful completed
		if (ev & HREG_EV_TX_EXC)
		{
			printk(KERN_WARNING "%s: WMAC controller asynchronous transmission unsuccessful completed\n", dev_info);
			dev->tbusy = 0;
			local->stats.tx_errors++;
		}

		// HREG_EV_TX: WMAC controller asynchronous transmission successful completed
		if (ev & HREG_EV_TX)
		{
			if (!dev->tbusy)
				printk(KERN_WARNING "%s: Non-existent Transmission successful completed!?\n", dev_info);
			DEBUG(DEBUG_TXRX, "%s: Transmission successful completed\n", dev_info);
			netif_wake_queue(dev);
			local->stats.tx_packets++;
		}

		// HREG_EV_RX: WMAC controller asynchronous receive frame
		// Break loop if no frame was received.
		if (!(ev & HREG_EV_RX))
			break;

		// If a frame was received, we process it and wrap back
		// up to the top of the while(1) loop so that hcf_service_nic()
		// gets called again after the frame drained from the NIC.
		// This allows us to find out if yet another frame has
		// arrived, and also to immediately acknowledge the just-
		// processed frame so that the NIC's buffer gets de-
		// allocated right away.
		len = local->ifb.IFB_RxLen;
		if (len)
		{
			DEBUG(DEBUG_INTERRUPT, "%s: Frame received. rx_len=0x%x\n", dev_info, len);
			wvlan_rx(dev, len);
		}
	}
	if (!cnt)
		printk(KERN_WARNING "%s: Maximum interrupt loops reached!\n", dev_info);

	// Turn back interrupts on (unlock)
	rc = hcf_action(&local->ifb, HCF_ACT_INT_ON);
	DEBUG(DEBUG_NOISY, "%s: hcf_action(HCF_ACT_INT_ON) returned 0x%x\n", dev_info, rc);

	DEBUG(DEBUG_INTERRUPT, "<- wvlan_interrupt()\n");
}


/********************************************************************
 * PCMCIA CONFIG / RELEASE
 */
#define CS_CHECK(fn, args...) while ((last_ret=CardServices(last_fn=(fn),args))!=0) goto cs_failed
#define CFG_CHECK(fn, args...) if (CardServices(fn, args) != 0) goto next_entry
static int wvlan_config (dev_link_t *link)
{
	client_handle_t handle = link->handle;
	tuple_t tuple;
	cisparse_t parse;
	struct net_device *dev = (struct net_device *) link->priv;
	struct net_local *local = (struct net_local *) dev->priv;
	int last_fn, last_ret;
	u_char buf[64];
	win_req_t req;
	memreq_t map;
	int rc, i;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_config(0x%p)\n", link);

	// This reads the card's CONFIG tuple to find its configuration registers.
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	CS_CHECK(GetTupleData, handle, &tuple);
	CS_CHECK(ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];

	// Configure card
	link->state |= DEV_CONFIG;

	// In this loop, we scan the CIS for configuration table entries,
	// each of which describes a valid card configuration, including
	// voltage, IO window, memory window, and interrupt settings.
	// We make no assumptions about the card to be configured: we use
	// just the information available in the CIS.  In an ideal world,
	// this would work for any PCMCIA card, but it requires a complete
	// and accurate CIS.  In practice, a driver usually "knows" most of
	// these things without consulting the CIS, and most client drivers
	// will only use the CIS to fill in implementation-defined details.
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	while (1) {
		cistpl_cftable_entry_t dflt = { 0 };
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
		CFG_CHECK(GetTupleData, handle, &tuple);
		CFG_CHECK(ParseTuple, handle, &tuple, &parse);

		if (cfg->index == 0) goto next_entry;
		link->conf.ConfigIndex = cfg->index;

		// Does this card need audio output?
		if (cfg->flags & CISTPL_CFTABLE_AUDIO)
		{
			link->conf.Attributes |= CONF_ENABLE_SPKR;
			link->conf.Status = CCSR_AUDIO_ENA;
		}
	
		// Use power settings for Vcc and Vpp if present
		// Note that the CIS values need to be rescaled
		if (cfg->vcc.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vcc = cfg->vcc.param[CISTPL_POWER_VNOM]/10000;
		else if (dflt.vcc.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vcc = dflt.vcc.param[CISTPL_POWER_VNOM]/10000;

		if (cfg->vpp1.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 = cfg->vpp1.param[CISTPL_POWER_VNOM]/10000;
		else if (dflt.vpp1.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 = dflt.vpp1.param[CISTPL_POWER_VNOM]/10000;

		// Do we need to allocate an interrupt?
		if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1)
			link->conf.Attributes |= CONF_ENABLE_IRQ;

		// IO window settings
		link->io.NumPorts1 = link->io.NumPorts2 = 0;
		if ((cfg->io.nwin > 0) || (dflt.io.nwin > 0)) {
			cistpl_io_t *io = (cfg->io.nwin) ? &cfg->io : &dflt.io;
			link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
			if (!(io->flags & CISTPL_IO_8BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
			if (!(io->flags & CISTPL_IO_16BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
			link->io.BasePort1 = io->win[0].base;
			link->io.NumPorts1 = io->win[0].len;
			if (io->nwin > 1) {
				link->io.Attributes2 = link->io.Attributes1;
				link->io.BasePort2 = io->win[1].base;
				link->io.NumPorts2 = io->win[1].len;
			}
		}

		// This reserves IO space but doesn't actually enable it
		CFG_CHECK(RequestIO, link->handle, &link->io);

		// Now set up a common memory window, if needed.  There is room
		// in the dev_link_t structure for one memory window handle,
		// but if the base addresses need to be saved, or if multiple
		// windows are needed, the info should go in the private data
		// structure for this device.
		// Note that the memory window base is a physical address, and
		// needs to be mapped to virtual space with ioremap() before it
		// is used.
		if ((cfg->mem.nwin > 0) || (dflt.mem.nwin > 0)) {
			cistpl_mem_t *mem = (cfg->mem.nwin) ? &cfg->mem : &dflt.mem;
			req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM;
			req.Base = mem->win[0].host_addr;
			req.Size = mem->win[0].len;
			req.AccessSpeed = 0;
			link->win = (window_handle_t)link->handle;
			CFG_CHECK(RequestWindow, &link->win, &req);
			map.Page = 0; map.CardOffset = mem->win[0].card_addr;
			CFG_CHECK(MapMemPage, link->win, &map);
		}

		// If we got this far, we're cool!
		break;

next_entry:
		if (cfg->flags & CISTPL_CFTABLE_DEFAULT)
			dflt = *cfg;
		CS_CHECK(GetNextTuple, handle, &tuple);
	}

	// Allocate an interrupt line.  Note that this does not assign a
	// handler to the interrupt, unless the 'Handler' member of the
	// irq structure is initialized.
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
	{
		link->irq.Attributes = IRQ_TYPE_EXCLUSIVE | IRQ_HANDLE_PRESENT;
		link->irq.IRQInfo1 = IRQ_INFO2_VALID | IRQ_LEVEL_ID;
		if (irq_list[0] == -1)
			link->irq.IRQInfo2 = irq_mask;
		else
			for (i=0; i<4; i++)
				link->irq.IRQInfo2 |= 1 << irq_list[i];
		link->irq.Handler = wvlan_interrupt;
		link->irq.Instance = dev;
		CS_CHECK(RequestIRQ, link->handle, &link->irq);
	}

	// This actually configures the PCMCIA socket -- setting up
	// the I/O windows and the interrupt mapping, and putting the
	// card and host interface into "Memory and IO" mode.
	CS_CHECK(RequestConfiguration, link->handle, &link->conf);

	// Feed the netdevice with this info
	dev->irq = link->irq.AssignedIRQ;
	dev->base_addr = link->io.BasePort1;
	dev->tbusy = 0;

	// Report what we've done
	printk(KERN_INFO "%s: index 0x%02x: Vcc %d.%d", dev_info, link->conf.ConfigIndex, link->conf.Vcc/10, link->conf.Vcc%10);
	if (link->conf.Vpp1)
		printk(", Vpp %d.%d", link->conf.Vpp1/10, link->conf.Vpp1%10);
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		printk(", irq %d", link->irq.AssignedIRQ);
	if (link->io.NumPorts1)
		printk(", io 0x%04x-0x%04x", link->io.BasePort1, link->io.BasePort1+link->io.NumPorts1-1);
	if (link->io.NumPorts2)
		printk(" & 0x%04x-0x%04x", link->io.BasePort2, link->io.BasePort2+link->io.NumPorts2-1);
	if (link->win)
		printk(", mem 0x%06lx-0x%06lx", req.Base, req.Base+req.Size-1);
	printk("\n");

	link->state &= ~DEV_CONFIG_PENDING;

	// Make netdevice's name (if not ethX) and remember the device
	// Not very efficient here, this should go somewhere into dev_list,
	// but it works for now (taken from register_netdevice in kernel)
	if (!eth)
	{
		for (i=0; i<MAX_WVLAN_CARDS; ++i)
			if (!wvlandev_index[i])
			{
				sprintf(local->node.dev_name, "wvlan%d", i);
				wvlandev_index[i] = dev;
				break;
			}
	}

	// Register the netdevice
	rc = register_netdev(dev);
	if (rc)
	{
		printk(KERN_WARNING "%s: register_netdev() failed!\n", dev_info);
		wvlan_release((u_long)link);
		return 0;
	}
	printk(KERN_INFO "%s: Registered netdevice %s\n", dev_info, dev->name);

	link->dev = &local->node;

	// Success!
	DEBUG(DEBUG_CALLTRACE, "<- wvlan_config()\n");
	return 1;

cs_failed:
	cs_error(link->handle, last_fn, last_ret);
	wvlan_release((u_long)link);
	DEBUG(DEBUG_CALLTRACE, "<- wvlan_config()\n");
	return 0;
}

static void wvlan_release (u_long arg)
{
	dev_link_t *link = (dev_link_t *) arg;
	struct net_device *dev = (struct net_device *) link->priv;
	int i;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_release(0x%p)\n", link);

	// If the device is currently in use, we won't release
	// until it's actually closed.
	if (link->open)
	{
		DEBUG(DEBUG_INFO, "%s: wvlan_release: release postponed, %s still locked\n", dev_info, link->dev->dev_name);
		link->state |= DEV_STALE_CONFIG;
		return;
	}

	// Power down
	wvlan_hw_shutdown(dev);

	// Remove our device from index (only devices named wvlanX)
	for (i=0; i<MAX_WVLAN_CARDS; ++i)
		if (wvlandev_index[i] == dev)
		{
			wvlandev_index[i] = NULL;
			break;
		}

	if (link->win)
		CardServices(ReleaseWindow, link->win);
	CardServices(ReleaseConfiguration, link->handle);
	if (link->io.NumPorts1)
		CardServices(ReleaseIO, link->handle, &link->io);
	if (link->irq.AssignedIRQ)
		CardServices(ReleaseIRQ, link->handle, &link->irq);

	link->state &= ~(DEV_CONFIG | DEV_RELEASE_PENDING);

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_release()\n");
}


/********************************************************************
 * PCMCIA ATTACH / DETACH
 */
static dev_link_t *wvlan_attach (void)
{
	dev_link_t *link;
	struct net_device *dev;
	struct net_local *local;
	int rc;
	client_reg_t client_reg;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_attach()\n");

	// Flush stale links
	for (link=dev_list; link; link=link->next)
		if (link->state & DEV_STALE_LINK)
			wvlan_detach(link);

	// Initialize the dev_link_t structure
	link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
	memset(link, 0, sizeof(struct dev_link_t));
	link->release.function = &wvlan_release;
	link->release.data = (u_long) link;
	link->conf.Vcc = 50;
	link->conf.IntType = INT_MEMORY_AND_IO;

	// Allocate space for netdevice (private data of link)
	dev = kmalloc(sizeof(struct net_device), GFP_KERNEL);
	memset(dev, 0, sizeof(struct net_device));
	link->priv = dev;

	// Allocate space for netdevice priv (private data of netdevice)
	local = kmalloc(sizeof(struct net_local), GFP_KERNEL);
	memset(local, 0, sizeof(struct net_local));
	dev->priv = local;

	// Initialize specific data
	local->link = link;
	local->dev = dev;

	// Standard setup for generic data
	ether_setup(dev);

	// kernel callbacks
	dev->init = wvlan_init;
	dev->open = wvlan_open;
	dev->stop = wvlan_close;
	dev->hard_start_xmit = wvlan_tx;
	dev->get_stats = wvlan_get_stats;
#ifdef WIRELESS_EXT
	dev->do_ioctl = wvlan_ioctl;
	dev->get_wireless_stats = wvlan_get_wireless_stats;
#endif
	dev->change_mtu = wvlan_change_mtu;
//	dev->set_multicast_list = wvlan_set_multicast_list;
//	dev->set_mac_address = wvlan_set_mac_address;

	// Other netdevice data
	dev->name = local->node.dev_name;
	dev->tbusy = 1;
	dev->mtu = mtu;

	// Register with CardServices
	link->next = dev_list;
	dev_list = link;
	client_reg.dev_info = &dev_info;
	client_reg.Attributes = INFO_IO_CLIENT;
	client_reg.EventMask =	CS_EVENT_REGISTRATION_COMPLETE |
				CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
				CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
				CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &wvlan_event;
	client_reg.Version = 0x0210;
	client_reg.event_callback_args.client_data = link;

	rc = CardServices(RegisterClient, &link->handle, &client_reg);
	if (rc)
	{
		cs_error(link->handle, RegisterClient, rc);
		wvlan_detach(link);
		return NULL;
	}

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_attach()\n");
	return link;
}

static void wvlan_detach (dev_link_t *link)
{
	dev_link_t **linkp;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_detach(0x%p)\n", link);

	// Locate device structure
	for (linkp=&dev_list; *linkp; linkp=&(*linkp)->next)
		if (*linkp == link)
			break;
	if (!*linkp)
	{
		printk(KERN_WARNING "%s: Attempt to detach non-existing PCMCIA client!\n", dev_info);
		return;
	}

	// If the device is currently configured and active, we won't
	// actually delete it yet. Instead, it is marked so that when the
	// release() function is called, that will trigger a proper
	// detach()
	if (link->state & DEV_CONFIG)
	{
		DEBUG(DEBUG_INFO, "%s: wvlan_detach: detach postponed, %s still locked\n", dev_info, link->dev->dev_name);
		wvlan_release((u_long)link);
		if (link->state & DEV_STALE_CONFIG)
		{
			link->state |= DEV_STALE_LINK;
			return;
		}
	}

	// Break the line with CardServices
	if (link->handle)
		CardServices(DeregisterClient, link->handle);

	// Unlink device structure, free pieces
	*linkp = link->next;
	if (link->priv)
	{
		struct net_device *dev = (struct net_device *) link->priv;
		if (link->dev)
		{
			unregister_netdev(dev);
			DEBUG(DEBUG_INFO, "%s: Netdevice unregistered\n", dev_info);
		}
		if (dev->priv)
			kfree_s(dev->priv, sizeof(struct net_local));
		kfree_s(link->priv, sizeof(struct net_device));
	}
	kfree_s(link, sizeof(struct dev_link_t));

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_detach()\n");
}


/********************************************************************
 * PCMCIA EVENT HANDLER
 */
static int wvlan_event (event_t event, int priority, event_callback_args_t *args)
{
	dev_link_t *link = (dev_link_t *) args->client_data;
	struct net_device *dev = (struct net_device *) link->priv;

	DEBUG(DEBUG_CALLTRACE, "-> wvlan_event(%s, %d, 0x%p)\n",
		((event==CS_EVENT_REGISTRATION_COMPLETE) ? "registration complete" :
		((event==CS_EVENT_CARD_INSERTION) ? "card insertion" :
		((event==CS_EVENT_CARD_REMOVAL) ? "card removal" :
		((event==CS_EVENT_RESET_PHYSICAL) ? "physical physical" :
		((event==CS_EVENT_CARD_RESET) ? "card reset" :
		((event==CS_EVENT_PM_SUSPEND) ? "pm suspend" :
		((event==CS_EVENT_PM_RESUME) ? "pm resume" :
		"unknown"))))))), priority, args);

	switch (event)
	{
		case CS_EVENT_CARD_INSERTION:
			link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
			if (!wvlan_config(link) || wvlan_hw_config(dev))
			{
				dev->irq = 0;
				printk(KERN_WARNING "%s: Initialization failed!\n", dev_info);
			}
			break;

		case CS_EVENT_CARD_REMOVAL:
			link->state &= ~DEV_PRESENT;
			if (link->state & DEV_CONFIG)
			{
				dev->tbusy = 1;
				dev->start = 0;
				link->release.expires = RUN_AT(HZ/20);
				add_timer(&link->release);
			}
			break;

		case CS_EVENT_PM_SUSPEND:
			link->state |= DEV_SUSPEND;
		case CS_EVENT_RESET_PHYSICAL:
			if (link->state & DEV_CONFIG)
			{
				if (link->open)
				{
					dev->tbusy = 1;
					dev->start = 0;
				}
				CardServices(ReleaseConfiguration, link->handle);
			}
			break;

		case CS_EVENT_PM_RESUME:
			link->state &= ~DEV_SUSPEND;
		case CS_EVENT_CARD_RESET:
			if (link->state & DEV_CONFIG)
			{
				CardServices(RequestConfiguration, link->handle, &link->conf);
				if (link->open)
				{
					wvlan_hw_shutdown(dev);
					wvlan_hw_config(dev);
					dev->tbusy = 0;
					dev->start = 1;
				}
			}
			break;
	}

	DEBUG(DEBUG_CALLTRACE, "<- wvlan_event()\n");
	return 0;
}


/********************************************************************
 * MODULE INSERTION / REMOVAL
 */
extern int init_module (void)
{
	servinfo_t serv;

	DEBUG(DEBUG_CALLTRACE, "-> init_module()\n");

	printk(KERN_INFO "%s: WaveLAN/IEEE PCMCIA driver v%s\n", dev_info, version);
	printk(KERN_INFO "%s: (c) Andreas Neuhaus <andy@fasta.fh-dortmund.de>\n", dev_info);

	// Check CardServices release
	CardServices(GetCardServicesInfo, &serv);
	if (serv.Revision != CS_RELEASE_CODE)
	{
		printk(KERN_WARNING "%s: CardServices release does not match!\n", dev_info);
		return -1;
	}

	// Register PCMCIA driver
	register_pcmcia_driver(&dev_info, &wvlan_attach, &wvlan_detach);

	DEBUG(DEBUG_CALLTRACE, "<- init_module()\n");
	return 0;
}

extern void cleanup_module (void)
{
	DEBUG(DEBUG_CALLTRACE, "-> cleanup_module()\n");

	// Unregister PCMCIA driver
	unregister_pcmcia_driver(&dev_info);

	// Remove leftover devices
	if (dev_list)
		DEBUG(DEBUG_INFO, "%s: Removing leftover devices!\n", dev_info);
	while (dev_list)
	{
		if (dev_list->state & DEV_CONFIG)
			wvlan_release((u_long)dev_list);
		wvlan_detach(dev_list);
	}

	printk(KERN_INFO "%s: Driver unloaded\n", dev_info);
	DEBUG(DEBUG_CALLTRACE, "<- cleanup_module()\n");
}


/********************************************************************
 * EOF
 */
