/*======================================================================

    PC Card CIS dump utility

    dump_cis.c 1.29 1998/05/14 09:17:46

    The contents of this file are subject to the Mozilla Public
    License Version 1.0 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
    
======================================================================*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

/*====================================================================*/

static int lookup_dev(char *name)
{
    FILE *f;
    int n;
    char s[32], t[32];
    
    f = fopen("/proc/devices", "r");
    if (f == NULL)
	return -1;
    while (fgets(s, 32, f) != NULL) {
	if (sscanf(s, "%d %s", &n, t) == 2)
	    if (strcmp(name, t) == 0)
		break;
    }
    fclose(f);
    if (strcmp(name, t) == 0)
	return n;
    else
	return -1;
}

/*====================================================================*/

static int open_dev(dev_t dev)
{
    char *fn;
    int fd;
    
    if ((fn = tmpnam(NULL)) == NULL)
	return -1;
    if (mknod(fn, (S_IFCHR|S_IREAD|S_IWRITE), dev) != 0)
	return -1;
    fd = open(fn, O_RDONLY);
    unlink(fn);
    return fd;
}

/*====================================================================*/

static void print_tuple(tuple_parse_t *tup)
{
    int i;
    printf("  Offset: 0x%2.2x, tuple: %#2.2x, link: %#2.2x\n",
	   tup->tuple.CISOffset, tup->tuple.TupleCode,
	   tup->tuple.TupleLink);
    for (i = 0; i < tup->tuple.TupleDataLen; i++) {
	if ((i % 16) == 0) printf("    ");
	printf("%2.2x ", (u_char)tup->data[i]);
	if ((i % 16) == 15) printf("\n");
    }
    if ((i % 16) != 0) printf("\n");
}

/*====================================================================*/

static void print_funcid(cistpl_funcid_t *fn)
{
    printf("  Function: ");
    switch (fn->func) {
    case CISTPL_FUNCID_MULTI:
	printf("multi-function"); break;
    case CISTPL_FUNCID_MEMORY:
	printf("memory card"); break;
    case CISTPL_FUNCID_SERIAL:
	printf("serial port"); break;
    case CISTPL_FUNCID_PARALLEL:
	printf("parallel port"); break;
    case CISTPL_FUNCID_FIXED:
	printf("fixed disk"); break;
    case CISTPL_FUNCID_VIDEO:
	printf("video adapter"); break;
    case CISTPL_FUNCID_NETWORK:
	printf("network adapter"); break;
    case CISTPL_FUNCID_AIMS:
	printf("auto-inc mass storage"); break;
    case CISTPL_FUNCID_SCSI:
	printf("SCSI adapter"); break;
    default:
	printf("unknown"); break;
    }
    if (fn->sysinit & CISTPL_SYSINIT_POST)
	printf(" [post]");
    if (fn->sysinit & CISTPL_SYSINIT_ROM)
	printf(" [rom]");
    printf("\n");
}

/*====================================================================*/

static void print_size(u_int size)
{
    if (size < 1024)
	printf("%u bytes", size);
    else if (size < 1024*1024)
	printf("%u kb", size/1024);
    else
	printf("%u mb", size/(1024*1024));
}

static void print_time(u_int tm, u_long scale)
{
    float t = (float)(tm) * scale * 0.000000001;
    if (t > 1.0)
	printf("%.1f s", t);
    else if (t > 0.001)
	printf("%.0f ms", t*1000.0);
    else if (t > 0.000001)
	printf("%.0f us", t*1000000.0);
    else
	printf("%.0f ns", t*1000000000.0);
}

static void print_volt(u_long vi)
{
    float v = vi * 0.00001;
    if (v > 1.0)
	printf("%.1f V", v);
    else if (v > 0.001)
	printf("%.0f mV", v*1000.0);
    else
	printf("%.0f uV", v*1000000.0);
}
    
static void print_current(u_long ii)
{
    float i = ii * 0.0000001;
    if (i > 1.0)
	printf("%.1f A", i);
    else if (i > 0.001)
	printf("%.0f mA", i*1000.0);
    else
	printf("%.0f uA", i*1000000.0);
}

static void print_speed(u_int b)
{
    if (b < 1024)
	printf("%u bits/sec", b);
    else if (b < 1024*1024)
	printf("%u kbits/sec", b/1024);
    else
	printf("%u mbits/sec", b/(1024*1024));
}

/*====================================================================*/

static const char *dtype[] = {
    "NULL", "ROM", "OTPROM", "EPROM", "EEPROM", "FLASH", "SRAM",
    "DRAM", "rsvd", "rsvd", "rsvd", "rsvd", "rsvd", "Function-specific",
    "Extended", "rsvd"
};

static void print_device(cistpl_device_t *dev)
{
    int i;
    for (i = 0; i < dev->ndev; i++) {
	printf("    %s: ", dtype[dev->dev[i].type]);
	printf("%u ns, ", dev->dev[i].speed);
	print_size(dev->dev[i].size);
	printf("\n");
    }
    if (dev->ndev == 0)
	printf("    No device info.\n");
}

/*====================================================================*/

static void print_power(char *tag, cistpl_power_t *power)
{
    int first = 1;
    printf("  %s: ", tag);
    if (power->present & (1<<CISTPL_POWER_VNOM)) {
	first = 0;
	printf(" Vnom ");
	print_volt(power->param[CISTPL_POWER_VNOM]);
    }
    if (power->present & (1<<CISTPL_POWER_VMIN)) {
	if (!first) putchar(','); else first = 0;
	printf(" Vmin ");
	print_volt(power->param[CISTPL_POWER_VMIN]);
    }
    if (power->present & (1<<CISTPL_POWER_VMAX)) {
	if (!first) putchar(','); else first = 0;
 	printf(" Vmax ");
	print_volt(power->param[CISTPL_POWER_VMAX]);
    }
    printf("\n       "); first = 1;
    if (power->present & (1<<CISTPL_POWER_ISTATIC)) {
	first = 0;
	printf(" Istatic ");
	print_current(power->param[CISTPL_POWER_ISTATIC]);
    }
    if (power->present & (1<<CISTPL_POWER_IAVG)) {
	if (!first) putchar(','); else first = 0;
	printf(" Iavg ");
	print_current(power->param[CISTPL_POWER_IAVG]);
    }
    if (power->present & (1<<CISTPL_POWER_IPEAK)) {
	if (!first) putchar(','); else first = 0;
	printf(" Ipeak ");
	print_current(power->param[CISTPL_POWER_IPEAK]);
    }
    if (power->present & (1<<CISTPL_POWER_IDOWN)) {
	if (!first) putchar(','); else first = 0;
	printf(" Idown ");
	print_current(power->param[CISTPL_POWER_IDOWN]);
    }
    if (power->flags & CISTPL_POWER_HIGHZ_OK) {
	if (!first) putchar(','); else first = 0;
	printf(" highz OK");
    }
    if (power->flags & CISTPL_POWER_HIGHZ_REQ) {
	if (!first) putchar(','); else first = 0;
	printf(" highz");
    }
    printf("\n");
}

/*====================================================================*/

static void print_cftable_entry(cistpl_cftable_entry_t *entry)
{
    int i;
    
    printf("  Config entry %#2.2x%s:\n", entry->index,
	   (entry->flags & CISTPL_CFTABLE_DEFAULT) ? " (default)" : "");

    if (entry->flags & ~CISTPL_CFTABLE_DEFAULT) {
	printf(" ");
	if (entry->flags & CISTPL_CFTABLE_BVDS)
	    printf(" [bvd]");
	if (entry->flags & CISTPL_CFTABLE_WP)
	    printf(" [wp]");
	if (entry->flags & CISTPL_CFTABLE_RDYBSY)
	    printf(" [rdybsy]");
	if (entry->flags & CISTPL_CFTABLE_MWAIT)
	    printf(" [mwait]");
	if (entry->flags & CISTPL_CFTABLE_AUDIO)
	    printf(" [audio]");
	if (entry->flags & CISTPL_CFTABLE_READONLY)
	    printf(" [readonly]");
	if (entry->flags & CISTPL_CFTABLE_PWRDOWN)
	    printf(" [pwrdown]");
	printf("\n");
    }
    
    if (entry->vcc.present)
	print_power("Vcc", &entry->vcc);
    if (entry->vpp1.present)
	print_power("Vpp1", &entry->vpp1);
    if (entry->vpp2.present)
	print_power("Vpp2", &entry->vpp2);

    if ((entry->timing.wait != 0) || (entry->timing.ready != 0) ||
	(entry->timing.reserved != 0)) {
	printf("  Timing:");
	if (entry->timing.wait != 0) {
	    printf(" wait ");
	    print_time(entry->timing.wait, entry->timing.waitscale);
	}
	if (entry->timing.ready != 0) {
	    printf(" ready ");
	    print_time(entry->timing.ready, entry->timing.rdyscale);
	}
	if (entry->timing.reserved != 0) {
	    printf(" reserved ");
	    print_time(entry->timing.reserved, entry->timing.rsvscale);
	}
	printf("\n");
    }
    
    if (entry->io.nwin) {
	cistpl_io_t *io = &entry->io;
	printf("  I/O windows: [lines = %d]",
	       io->flags & CISTPL_IO_LINES_MASK);
	if (io->flags & CISTPL_IO_8BIT) printf(" [8 bit]");
	if (io->flags & CISTPL_IO_16BIT) printf(" [16 bit]");
	if (io->flags & CISTPL_IO_RANGE) printf(" [range]");
	printf("\n");
	for (i = 0; i < io->nwin; i++)
	    printf("    base = %#4.4x, length = %#4.4x\n",
		   io->win[i].base, io->win[i].len);
    }

    if (entry->irq.IRQInfo1) {
	printf("  Interrupt ");
	if (entry->irq.IRQInfo1 & IRQ_INFO2_VALID)
	    printf("mask = 0x%04x", entry->irq.IRQInfo2);
	else
	    printf("%u", entry->irq.IRQInfo1 & IRQ_MASK);
	if (entry->irq.IRQInfo1 & IRQ_LEVEL_ID) printf(" [level]");
	if (entry->irq.IRQInfo1 & IRQ_PULSE_ID) printf(" [pulse]");
	if (entry->irq.IRQInfo1 & IRQ_SHARE_ID) printf(" [shared]");
	printf("\n");
    }

    if (entry->mem.nwin) {
	cistpl_mem_t *mem = &entry->mem;
	printf("  Memory windows:\n");
	for (i = 0; i < mem->nwin; i++)
	    printf("    card = 0x%4.4x, host = 0x%4.4lx, length = "
		   "0x%4.4x\n", mem->win[i].card_addr,
		   (u_long)mem->win[i].host_addr, mem->win[i].len);
    }

    if (entry->subtuples)
	printf("  %d bytes in subtuples\n", entry->subtuples);
    
}

/*====================================================================*/

static void print_cftable_entry_cb(cistpl_cftable_entry_cb_t *entry)
{
    int i;
    
    printf("  Config entry %#2.2x%s:\n", entry->index,
	   (entry->flags & CISTPL_CFTABLE_DEFAULT) ? " (default)" : "");

    if (entry->flags & ~CISTPL_CFTABLE_DEFAULT) {
	printf(" ");
	if (entry->flags & CISTPL_CFTABLE_MASTER)
	    printf(" [master]");
	if (entry->flags & CISTPL_CFTABLE_INVALIDATE)
	    printf(" [invalidate]");
	if (entry->flags & CISTPL_CFTABLE_VGA_PALETTE)
	    printf(" [vga palette]");
	if (entry->flags & CISTPL_CFTABLE_PARITY)
	    printf(" [parity]");
	if (entry->flags & CISTPL_CFTABLE_WAIT)
	    printf(" [wait]");
	if (entry->flags & CISTPL_CFTABLE_SERR)
	    printf(" [serr]");
	if (entry->flags & CISTPL_CFTABLE_FAST_BACK)
	    printf(" [fast back]");
	if (entry->flags & CISTPL_CFTABLE_BINARY_AUDIO)
	    printf(" [binary audio]");
	if (entry->flags & CISTPL_CFTABLE_PWM_AUDIO)
	    printf(" [pwm audio]");
	printf("\n");
    }
    
    if (entry->vcc.present)
	print_power("Vcc", &entry->vcc);
    if (entry->vpp1.present)
	print_power("Vpp1", &entry->vpp1);
    if (entry->vpp2.present)
	print_power("Vpp2", &entry->vpp2);

    if (entry->io) {
	printf("  IO base registers: ");
	for (i = 0; i < 8; i++)
	    if (entry->io & (1<<i)) printf(" %d", i);
	printf("\n");
    }

    if (entry->irq.IRQInfo1) {
	printf("  Interrupt ");
	if (entry->irq.IRQInfo1 & IRQ_INFO2_VALID)
	    printf("mask = 0x%#4.4x", entry->irq.IRQInfo2);
	else
	    printf("%u", entry->irq.IRQInfo1 & IRQ_MASK);
	if (entry->irq.IRQInfo1 & IRQ_LEVEL_ID) printf(" [level]");
	if (entry->irq.IRQInfo1 & IRQ_PULSE_ID) printf(" [pulse]");
	if (entry->irq.IRQInfo1 & IRQ_SHARE_ID) printf(" [shared]");
	printf("\n");
    }

    if (entry->mem) {
	printf("  Memory base registers: ");
	for (i = 0; i < 8; i++)
	    if (entry->mem & (1<<i)) printf(" %d", i);
	printf("\n");
    }

    if (entry->subtuples)
	printf("  %d bytes in subtuples\n", entry->subtuples);
    
}

/*====================================================================*/

static void print_jedec(cistpl_jedec_t *jedec)
{
    int i;
    for (i = 0; i < jedec->nid; i++) {
	printf("    mfr 0x%02x, info 0x%02x\n",
	       jedec->id[i].mfr, jedec->id[i].info);
    }
}

/*====================================================================*/

static void print_device_geo(cistpl_device_geo_t *geo)
{
    int i;
    for (i = 0; i < geo->ngeo; i++) {
	printf("    bus %d, erase %#x, read %#x, write %#x, "
	       "partition %#x, interleave %#x\n",
	       geo->geo[i].buswidth, geo->geo[i].erase_block,
	       geo->geo[i].read_block, geo->geo[i].write_block,
	       geo->geo[i].partition, geo->geo[i].interleave);
    }
}

/*====================================================================*/

static void print_org(cistpl_org_t *org)
{
    printf("  Data organization: ");
    switch (org->data_org) {
    case CISTPL_ORG_FS:
	printf("filesystem"); break;
    case CISTPL_ORG_APPSPEC:
	printf("app-specific"); break;
    case CISTPL_ORG_XIP:
	printf("executable code"); break;
    default:
	if (org->data_org < 0x80)
	    printf("reserved");
	else
	    printf("vendor-specific");
    }
    printf(", \"%s\"\n", org->desc);
}

/*====================================================================*/

static void print_serial(cistpl_funce_t *funce)
{
}

/*====================================================================*/

static void print_fixed(cistpl_funce_t *funce)
{
    cistpl_ide_interface_t *i;
    cistpl_ide_feature_t *f;
    
    switch (funce->type) {
    case CISTPL_FUNCE_IDE_IFACE:
	i = (cistpl_ide_interface_t *)(funce->data);
	printf("  Fixed-disk interface: ");
	if (i->interface == CISTPL_IDE_INTERFACE)
	    printf("IDE\n");
	else
	    printf("Undefined\n");
	break;
    case CISTPL_FUNCE_IDE_MASTER:
    case CISTPL_FUNCE_IDE_SLAVE:
	f = (cistpl_ide_feature_t *)(funce->data);
	printf("  Fixed-disk features:");
	if (f->feature1 & CISTPL_IDE_SILICON)
	    printf(" [silicon]");
	else
	    printf(" [rotating]");
	if (f->feature1 & CISTPL_IDE_UNIQUE)
	    printf(" [unique]");
	if (f->feature1 & CISTPL_IDE_DUAL)
	    printf(" [dual]");
	else
	    printf(" [single]");
	if (f->feature1 && f->feature2)
	    printf("\n    ");
	if (f->feature2 & CISTPL_IDE_HAS_SLEEP)
	    printf(" [sleep]");
	if (f->feature2 & CISTPL_IDE_HAS_STANDBY)
	    printf(" [standby]");
	if (f->feature2 & CISTPL_IDE_HAS_IDLE)
	    printf(" [idle]");
	if (f->feature2 & CISTPL_IDE_LOW_POWER)
	    printf(" [low power]");
	if (f->feature2 & CISTPL_IDE_REG_INHIBIT)
	    printf(" [reg inhibit]");
	if (f->feature2 & CISTPL_IDE_HAS_INDEX)
	    printf(" [index]");
	if (f->feature2 & CISTPL_IDE_IOIS16)
	    printf(" [iois16]");
	printf("\n");
	break;
    }
}

/*====================================================================*/

static const char *tech[] = {
    "Undefined", "ARCnet", "Ethernet", "Token-Ring", "Localtalk",
    "FDDI/CDDI", "ATM", "Wireless"
};

static const char *media[] = {
    "Undefined", "Unshielded Twisted Pair", "Shielded Twisted Pair",
    "Thin Coax", "Thick Coax", "Fiber", "900 MHz", "2.4 GHz",
    "5.4 GHz", "Diffuse Infrared", "Point-to-Point Infrared"
};

static void print_network(cistpl_funce_t *funce)
{
    cistpl_lan_tech_t *t;
    cistpl_lan_speed_t *s;
    cistpl_lan_media_t *m;
    cistpl_lan_node_id_t *n;
    cistpl_lan_connector_t *c;
    int i;
    
    switch (funce->type) {
    case CISTPL_FUNCE_LAN_TECH:
	t = (cistpl_lan_tech_t *)(funce->data);
	printf("  LAN technology: %s\n", tech[t->tech]);
	break;
    case CISTPL_FUNCE_LAN_SPEED:
	s = (cistpl_lan_speed_t *)(funce->data);
	printf("  LAN speed: ");
	print_speed(s->speed);
	printf("\n");
	break;
    case CISTPL_FUNCE_LAN_MEDIA:
	m = (cistpl_lan_media_t *)(funce->data);
	printf("  LAN media: %s\n", media[m->media]);
	break;
    case CISTPL_FUNCE_LAN_NODE_ID:
	n = (cistpl_lan_node_id_t *)(funce->data);
	printf("  LAN node ID:");
	for (i = 0; i < n->nb; i++)
	    printf(" %02x", n->id[i]);
	printf("\n");
	break;
    case CISTPL_FUNCE_LAN_CONNECTOR:
	c = (cistpl_lan_connector_t *)(funce->data);
	printf("  LAN connector: ");
	if (c->code == 0)
	    printf("Open connector standard\n");
	else
	    printf("Closed connector standard\n");
	break;
    }
}

/*====================================================================*/

void print_vers_2(cistpl_vers_2_t *v2)
{
    printf("  Version 0x%2.2x, compliance 0x%2.2x, dindex 0x%4.4x\n",
	   v2->vers, v2->comply, v2->dindex);
    printf("  vspec8 = 0x%2.2x, vspec9 = 0x%2.2x, nhdr = %d\n",
	   v2->vspec8, v2->vspec9, v2->nhdr);
    printf("  Vendor \"%s\"\n", v2->str+v2->vendor);
    printf("  Info: %s\n", v2->str+v2->info);
}

/*====================================================================*/

void print_parse(tuple_parse_t *tup)
{
    static int func = 0;
    int i;
    
    switch (tup->tuple.TupleCode) {
    case CISTPL_DEVICE:
    case CISTPL_DEVICE_A:
	if (tup->tuple.TupleCode == CISTPL_DEVICE)
	    printf("  Common memory devices: \n");
	else
	    printf("  Attribute memory devices: \n");
	print_device(&tup->parse.device);
	break;
    case CISTPL_CHECKSUM:
	printf("  Checksum start 0x%04x, len 0x%04x, sum 0x%02x\n",
	       tup->parse.checksum.addr, tup->parse.checksum.len,
	       tup->parse.checksum.sum);
	break;
    case CISTPL_LONGLINK_A:
	printf("  Long link attr 0x%04x\n", tup->parse.longlink.addr);
	break;
    case CISTPL_LONGLINK_C:
	printf("  Long link common 0x%04x\n", tup->parse.longlink.addr);
	break;
    case CISTPL_LONGLINK_MFC:
	printf("  Multifunction long links:\n");
	for (i = 0; i < tup->parse.longlink_mfc.nfn; i++)
	    printf("   function %d: %s 0x%04x\n", i,
		   tup->parse.longlink_mfc.fn[i].space ? "common" : "attr",
		   tup->parse.longlink_mfc.fn[i].addr);
	break;
    case CISTPL_NO_LINK:
	printf("  No long link present\n");
	break;
    case CISTPL_LINKTARGET:
	printf("  Link target\n");
	break;
    case CISTPL_VERS_1:
	printf("  Version %d.%d\n", tup->parse.version_1.major,
	       tup->parse.version_1.minor);
	for (i = 0; i < tup->parse.version_1.ns; i++) {
	    printf("%s", (i == 0) ? "  " : ", ");
	    printf("\"%s\"", tup->parse.version_1.str +
		   tup->parse.version_1.ofs[i]);
	}
	printf("\n");
	break;
    case CISTPL_ALTSTR:
	break;
    case CISTPL_JEDEC_A:
    case CISTPL_JEDEC_C:
	if (tup->tuple.TupleCode == CISTPL_JEDEC_C)
	    printf("  Common memory JEDEC: \n");
	else
	    printf("  Attribute memory JEDEC: \n");
	print_jedec(&tup->parse.jedec);
	break;
    case CISTPL_DEVICE_GEO:
    case CISTPL_DEVICE_GEO_A:
	if (tup->tuple.TupleCode == CISTPL_DEVICE_GEO)
	    printf("  Common memory device geometry: \n");
	else
	    printf("  Attribute memory device geometry: \n");
	print_device_geo(&tup->parse.device_geo);
	break;
    case CISTPL_MANFID:
	printf("  Manufacturer %#4.4x, card ID %#4.4x\n",
	       tup->parse.manfid.manf, tup->parse.manfid.card);
	break;
    case CISTPL_FUNCID:
	print_funcid(&tup->parse.funcid);
	func = tup->parse.funcid.func;
	break;
    case CISTPL_FUNCE:
	switch (func) {
	case CISTPL_FUNCID_SERIAL:
	    print_serial(&tup->parse.funce);
	    break;
	case CISTPL_FUNCID_FIXED:
	    print_fixed(&tup->parse.funce);
	    break;
	case CISTPL_FUNCID_NETWORK:
	    print_network(&tup->parse.funce);
	    break;
	}
	break;
    case CISTPL_BAR:
	printf("  Base address register %d: size = ",
	       tup->parse.bar.attr & CISTPL_BAR_SPACE);
	print_size(tup->parse.bar.size);
	if (tup->parse.bar.attr & CISTPL_BAR_SPACE_IO)
	    printf(" [io]");
	else
	    printf(" [mem]");
	if (tup->parse.bar.attr & CISTPL_BAR_PREFETCH)
	    printf(" [prefetch]");
	if (tup->parse.bar.attr & CISTPL_BAR_CACHEABLE)
	    printf(" [cacheable]");
	if (tup->parse.bar.attr & CISTPL_BAR_1MEG_MAP)
	    printf(" [below 1mb]");
	printf("\n");
	break;
    case CISTPL_CONFIG:
    case CISTPL_CONFIG_CB:
	printf("  Config base = %#4.4x, ", tup->parse.config.base);
	if (tup->tuple.TupleCode == CISTPL_CONFIG)
	    printf("mask = %#4.4x, ", tup->parse.config.rmask[0]);
	printf("last config index = %#2.2x\n",
	       tup->parse.config.last_idx);
	if (tup->parse.config.subtuples)
	    printf("  %d bytes in subtuples\n",
		   tup->parse.config.subtuples);
	break;
	printf("  Config base = %#4.4x, ", tup->parse.config.base);
	printf("last config index = %#2.2x\n",
	       tup->parse.config.last_idx);
	if (tup->parse.config.subtuples)
	    printf("  %d bytes in subtuples\n",
		   tup->parse.config.subtuples);
	break;
    case CISTPL_CFTABLE_ENTRY:
	print_cftable_entry(&tup->parse.cftable_entry);
	break;
    case CISTPL_CFTABLE_ENTRY_CB:
	print_cftable_entry_cb(&tup->parse.cftable_entry_cb);
	break;
    case CISTPL_VERS_2:
	print_vers_2(&tup->parse.vers_2);
	break;
    case CISTPL_ORG:
	print_org(&tup->parse.org);
	break;
    }
}

/*====================================================================*/

#define MAX_SOCKS 8

int main(int argc, char *argv[])
{
    int i, major, fd, ret;
    ds_ioctl_arg_t arg;

    major = lookup_dev("pcmcia");
    if (major < 0) {
	fprintf(stderr, "no pcmcia driver in /proc/devices\n");
	exit(EXIT_FAILURE);
    }
    for (i = 0; i < MAX_SOCKS; i++) {
	fd = open_dev((major<<8)+i);
	if (fd < 0) break;
	printf("Socket %d:\n", i);
	ret = ioctl(fd, DS_VALIDATE_CIS, &arg);
	if (ret != 0) {
	    perror("invalid CIS");
	    continue;
	}
	if (arg.cisinfo.Chains == 0) {
	    fprintf(stderr, "no CIS found\n");
	    continue;
	}
	arg.tuple.TupleDataMax = sizeof(arg.tuple_parse.data);
	arg.tuple.Attributes = TUPLE_RETURN_LINK | TUPLE_RETURN_COMMON;
	arg.tuple.DesiredTuple = RETURN_FIRST_TUPLE;
	arg.tuple.TupleOffset = 0;
	ret = ioctl(fd, DS_GET_FIRST_TUPLE, &arg);
	if (ret != 0) {
	    perror("no first tuple");
	    continue;
	}
	for (;;) {
	    if (ioctl(fd, DS_GET_TUPLE_DATA, &arg) == 0) {
		print_tuple(&arg.tuple_parse);
		ret = ioctl(fd, DS_PARSE_TUPLE, &arg);
		if (ret != 0) {
		    if (errno != ENOSYS)
			printf("  parse error: %s\n", strerror(errno));
		} else
		    print_parse(&arg.tuple_parse);
	    } else {
		printf("  get tuple data: %s\n", strerror(errno));
		break;
	    }
	    printf("\n");
	    ret = ioctl(fd, DS_GET_NEXT_TUPLE, &arg);
	    if (ret != 0) {
		if (errno != ENODATA)
		    printf("next tuple: %s\n", strerror(errno));
		break;
	    }
	}
    }
    return 0;
}
