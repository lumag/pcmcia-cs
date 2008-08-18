/*======================================================================

    PCMCIA Card Services driver

    Written by David Hinds, dhinds@hyper.stanford.edu
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <asm/system.h>

#define IN_CARD_SERVICES
#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/ss.h>
#include <pcmcia/cs.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include "cs_internal.h"
#include "rsrc_mgr.h"

#ifdef CONFIG_APM
#include <linux/apm_bios.h>
static int handle_apm_event(apm_event_t event);
#endif

#ifdef PCMCIA_DEBUG
int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
static const char *version =
"cs.c 1.165 1998/02/18 23:55:46 (David Hinds)";
#endif

static const char *release = "Linux PCMCIA Card Services " CS_RELEASE;
static const char *kernel = "kernel build: " UTS_RELEASE " " UTS_VERSION;
static const char *options = "options: "
#ifdef CONFIG_CARDBUS
" [cardbus]"
#endif
#ifdef CONFIG_APM
" [apm]"
#endif
#if !defined(CONFIG_CARDBUS) && !defined(CONFIG_APM)
" none"
#endif
;

#if (LINUX_VERSION_CODE <= VERSION(2,1,17))

#undef CONFIG_MODVERSIONS
static struct symbol_table cs_symtab = {
#include <linux/symtab_begin.h>
#if (LINUX_VERSION_CODE >= VERSION(1,3,0))
#undef X
#define X(sym) { (void *)&sym, SYMBOL_NAME_STR(sym) }
#endif
    X(register_ss_entry),
    X(unregister_ss_entry),
    X(CardServices),
    X(MTDHelperEntry),
    X(find_mem_region),
    X(release_mem_region),
#include <linux/symtab_end.h>
};

#else

#define register_symtab(n)
EXPORT_SYMBOL(register_ss_entry);
EXPORT_SYMBOL(unregister_ss_entry);
EXPORT_SYMBOL(CardServices);
EXPORT_SYMBOL(MTDHelperEntry);
EXPORT_SYMBOL(find_mem_region);
EXPORT_SYMBOL(release_mem_region);

#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

static int setup_delay		= HZ/20;	/* ticks */
static int resume_delay		= HZ/5;		/* ticks */
static int shutdown_delay	= HZ/40;	/* ticks */
static int vcc_settle		= HZ*0.3;	/* ticks */
static int reset_time		= 10;		/* usecs */
static int unreset_delay	= HZ/10;	/* ticks */
static int unreset_check	= HZ/10;	/* ticks */
static int unreset_limit	= 30;		/* unreset_check's */

/* Access speed for attribute memory windows */
static int cis_speed		= 300;	/* ns */

/* Access speed for IO windows */
static int io_speed		= 0;	/* ns */

MODULE_PARM(setup_delay, "i");
MODULE_PARM(resume_delay, "i");
MODULE_PARM(shutdown_delay, "i");
MODULE_PARM(vcc_settle, "i");
MODULE_PARM(reset_time, "i");
MODULE_PARM(unreset_delay, "i");
MODULE_PARM(unreset_check, "i");
MODULE_PARM(unreset_limit, "i");
MODULE_PARM(cis_speed, "i");
MODULE_PARM(io_speed, "i");

/*====================================================================*/

static socket_state_t dead_socket = {
    0, SS_DETECT, 0, 0, 0
};

/* Table of sockets */
socket_t sockets = 0;
socket_info_t *socket_table[MAX_SOCK];

/*====================================================================*/

/* String tables for error messages */

typedef struct lookup_t {
    int key;
    char *msg;
} lookup_t;

static const lookup_t error_table[] = {
    { CS_SUCCESS,		"Operation succeeded" },
    { CS_BAD_ADAPTER,		"Bad adapter" },
    { CS_BAD_ATTRIBUTE, 	"Bad attribute", },
    { CS_BAD_BASE,		"Bad base address" },
    { CS_BAD_EDC,		"Bad EDC" },
    { CS_BAD_IRQ,		"Bad IRQ" },
    { CS_BAD_OFFSET,		"Bad offset" },
    { CS_BAD_PAGE,		"Bad page number" },
    { CS_READ_FAILURE,		"Read failure" },
    { CS_BAD_SIZE,		"Bad size" },
    { CS_BAD_SOCKET,		"Bad socket" },
    { CS_BAD_TYPE,		"Bad type" },
    { CS_BAD_VCC,		"Bad Vcc" },
    { CS_BAD_VPP,		"Bad Vpp" },
    { CS_BAD_WINDOW,		"Bad window" },
    { CS_WRITE_FAILURE,		"Write failure" },
    { CS_NO_CARD,		"No card present" },
    { CS_UNSUPPORTED_FUNCTION,	"Usupported function" },
    { CS_UNSUPPORTED_MODE,	"Unsupported mode" },
    { CS_BAD_SPEED,		"Bad speed" },
    { CS_BUSY,			"Resource busy" },
    { CS_GENERAL_FAILURE,	"General failure" },
    { CS_WRITE_PROTECTED,	"Write protected" },
    { CS_BAD_ARG_LENGTH,	"Bad argument length" },
    { CS_BAD_ARGS,		"Bad arguments" },
    { CS_CONFIGURATION_LOCKED,	"Configuration locked" },
    { CS_IN_USE,		"Resource in use" },
    { CS_NO_MORE_ITEMS,		"No more items" },
    { CS_OUT_OF_RESOURCE,	"Out of resource" },
    { CS_BAD_HANDLE,		"Bad handle" },
    { CS_BAD_TUPLE,		"Bad CIS tuple" }
};
#define ERROR_COUNT (sizeof(error_table)/sizeof(lookup_t))

static const lookup_t service_table[] = {
    { AccessConfigurationRegister,	"AccessConfigurationRegister" },
    { AdjustResourceInfo,		"AdjustResourceInfo" },
    { CheckEraseQueue,			"CheckEraseQueue" },
    { CloseMemory,			"CloseMemory" },
    { DeregisterClient,			"DeregisterClient" },
    { GetCardServicesInfo,		"GetCardServicesInfo" },
    { GetClientInfo,			"GetClientInfo" },
    { GetConfigurationInfo,		"GetConfigurationInfo" },
    { GetEventMask,			"GetEventMask" },
    { GetFirstClient,			"GetFirstClient" },
    { GetFirstRegion,			"GetFirstRegion" },
    { GetFirstTuple,			"GetFirstTuple" },
    { GetNextClient,			"GetNextClient" },
    { GetNextRegion,			"GetNextRegion" },
    { GetNextTuple,			"GetNextTuple" },
    { GetStatus,			"GetStatus" },
    { GetTupleData,			"GetTupleData" },
    { MapMemPage,			"MapMemPage" },
    { ModifyConfiguration,		"ModifyConfiguration" },
    { ModifyWindow,			"ModifyWindow" },
    { OpenMemory,			"OpenMemory" },
    { ParseTuple,			"ParseTuple" },
    { ReadMemory,			"ReadMemory" },
    { RegisterClient,			"RegisterClient" },
    { RegisterMTD,			"RegisterMTD" },
    { ReleaseConfiguration,		"ReleaseConfiguration" },
    { ReleaseIO,			"ReleaseIO" },
    { ReleaseIRQ,			"ReleaseIRQ" },
    { ReleaseWindow,			"ReleaseWindow" },
    { RequestConfiguration,		"RequestConfiguration" },
    { RequestIO,			"RequestIO" },
    { RequestIRQ,			"RequestIRQ" },
    { RequestSocketMask,		"RequestSocketMask" },
    { RequestWindow,			"RequestWindow" },
    { ResetCard,			"ResetCard" },
    { SetEventMask,			"SetEventMask" },
    { ValidateCIS,			"ValidateCIS" },
    { WriteMemory,			"WriteMemory" },
    { BindDevice,			"BindDevice" },
    { BindMTD,				"BindMTD" },
    { ReportError,			"ReportError" },
    { SuspendCard,			"SuspendCard" },
    { ResumeCard,			"ResumeCard" }
};
#define SERVICE_COUNT (sizeof(service_table)/sizeof(lookup_t))

/*======================================================================

    Setting a card's COR seems to need a short delay.  We have to
    busy loop because it might happen in an interrupt routine.  So
    here's a generic busy loop.
    
======================================================================*/

static void busy_loop(u_long len)
{
    u_long timeout = jiffies + len;
    u_long flags;
    save_flags(flags);
    sti();
    while (timeout >= jiffies)
	;
    restore_flags(flags);
} /* busy_loop */

/*======================================================================

    Reset a socket to the default state
    
======================================================================*/

static void init_socket(socket_info_t *s)
{
    int i;
    pcmcia_io_map io = { 0, 0, 0, 0, 1 };
    pcmcia_mem_map mem = { 0, 0, 0, 0, 0x1000, 0 };

    s->socket = dead_socket;
    s->ss_entry(s->sock, SS_SetSocket, &s->socket);
    for (i = 0; i < 2; i++) {
	io.map = i;
	s->ss_entry(s->sock, SS_SetIOMap, &io);
    }
    for (i = 0; i < 4; i++) {
	mem.map = i;
	s->ss_entry(s->sock, SS_SetMemMap, &mem);
    }
}

/*======================================================================

    Low-level PCMCIA interface drivers need to register with Card
    Services using these calls.
    
======================================================================*/

static void setup_socket(u_long i);
static void shutdown_socket(u_long i);
static void reset_socket(u_long i);
static void unreset_socket(u_long i);
static void parse_events(void *info, u_int events);

int register_ss_entry(int nsock, ss_entry_t ss_entry)
{
    int i, ns;
    socket_info_t *s;

    DEBUG(0, "cs: register_ss_entry(%d, 0x%p)\n", nsock, ss_entry);

    MOD_INC_USE_COUNT;

    for (ns = 0; ns < nsock; ns++) {
	s = kmalloc(sizeof(struct socket_info_t), GFP_KERNEL);
	memset(s, 0, sizeof(socket_info_t));
    
	s->ss_entry = ss_entry;
	s->sock = ns;
	s->setup.data = sockets;
	s->setup.function = &setup_socket;
	s->shutdown.data = sockets;
	s->shutdown.function = &shutdown_socket;
	/* base address = 0, map = 0 */
	s->cis_mem.flags = MAP_ACTIVE;
	s->cis_mem.speed = cis_speed;
	s->erase_busy.next = s->erase_busy.prev = &s->erase_busy;
	init_waitqueue(&s->mtd_ready);
	
	for (i = 0; i < sockets; i++)
	    if (socket_table[i] == NULL) break;
	socket_table[i] = s;
	if (i == sockets) sockets++;

	init_socket(s);
	ss_entry(ns, SS_InquireSocket, &s->cap);
    }
    
    return 0;
} /* register_ss_entry */

/*====================================================================*/

void unregister_ss_entry(ss_entry_t ss_entry)
{
    int i, j;
    socket_info_t *s;
    
    for (i = 0; i < sockets; i++) {
	s = socket_table[i];
	if (s->ss_entry == ss_entry) {
	    init_socket(s);
	    s->ss_entry = NULL;
	    release_mem_region(s->cis_mem.sys_start, 0x1000);
	    kfree_s(s, sizeof(struct socket_info_t));
	    socket_table[i] = NULL;
	    for (j = i; j < sockets-1; j++)
		socket_table[j] = socket_table[j+1];
	    sockets--;
	}
    }
    
    MOD_DEC_USE_COUNT;
} /* unregister_ss_entry */

/*======================================================================

    Shutdown_Socket() and setup_socket() are scheduled using add_timer
    calls by the main event handler when card insertion and removal
    events are received.  Shutdown_Socket() unconfigures a socket and
    turns off socket power.  Setup_socket() turns on socket power
    and resets the socket, in two stages.

======================================================================*/

static void free_regions(memory_handle_t *list)
{
    memory_handle_t tmp;
    while (*list != NULL) {
	tmp = *list;
	*list = tmp->info.next;
	tmp->region_magic = 0;
	kfree_s(tmp, sizeof(*tmp));
    }
}

static int send_event(socket_info_t *s, event_t event, int priority);

static void shutdown_socket(u_long i)
{
    socket_info_t *s = socket_table[i];
    
    sti();
    DEBUG(1, "cs: shutdown_socket(%ld)\n", i);

    /* Blank out the socket state */
    s->state &= SOCKET_PRESENT|SOCKET_SETUP_PENDING;
    init_socket(s);
    s->irq.AssignedIRQ = s->irq.Config = 0;
    s->functions = 0;
    s->lock_count = 0;
    s->cis_used = 0;
    if (s->config) {
	kfree(s->config);
	s->config = NULL;
    }
    free_regions(&s->a_region);
    free_regions(&s->c_region);
} /* shutdown_socket */

static void setup_socket(u_long i)
{
    int val;
    socket_info_t *s = socket_table[i];

    sti();
    s->ss_entry(s->sock, SS_GetStatus, &val);
    if (val & SS_DETECT) {
	DEBUG(1, "cs: setup_socket(%ld): applying power\n", i);
	s->state |= SOCKET_PRESENT;
	s->socket.flags = 0;
	s->socket.Vcc = 50;
#ifdef CONFIG_CARDBUS
	if (val & SS_CARDBUS) {
	    s->socket.Vcc = 33;
	    s->state |= SOCKET_CARDBUS;
	}
#endif
	s->ss_entry(s->sock, SS_SetSocket, &s->socket);
	s->setup.function = &reset_socket;
	s->setup.expires = RUN_AT(vcc_settle);
	add_timer(&s->setup);
    }
#ifdef PCMCIA_DEBUG
    else
	DEBUG(0, "cs: setup_socket(%ld): no card!\n", i);
#endif
} /* setup_socket */

/*======================================================================

    Reset_socket() and unreset_socket() handle hard resets.  Resets
    have several causes: card insertion, a call to reset_socket, or
    recovery from a suspend/resume cycle.  Unreset_socket() sends
    a CS event that matches the cause of the reset.
    
======================================================================*/

static void reset_socket(u_long i)
{
    socket_info_t *s = socket_table[i];

    DEBUG(1, "cs: resetting socket %ld\n", i);
    s->socket.flags |= SS_OUTPUT_ENA | SS_RESET;
    s->ss_entry(s->sock, SS_SetSocket, &s->socket);
    udelay((long)reset_time);
    s->socket.flags &= ~SS_RESET;
    s->ss_entry(s->sock, SS_SetSocket, &s->socket);
    s->unreset_timeout = 0;
    s->setup.expires = RUN_AT(unreset_delay);
    s->setup.function = &unreset_socket;
    add_timer(&s->setup);
} /* reset_socket */

#define EVENT_MASK \
(SOCKET_SETUP_PENDING|SOCKET_SUSPEND|SOCKET_RESET_PENDING)

static void unreset_socket(u_long i)
{
    socket_info_t *s = socket_table[i];
    int val;

    sti();
    s->ss_entry(s->sock, SS_GetStatus, &val);
    if (val & SS_READY) {
	DEBUG(1, "cs: reset done on socket %ld\n", i);
	if (s->state & SOCKET_SUSPEND) {
	    s->state &= ~EVENT_MASK;
	    if (verify_cis_cache(s) != 0)
		parse_events(s, SS_DETECT);
	    else
		send_event(s, CS_EVENT_PM_RESUME, CS_EVENT_PRI_LOW);
	} else if (s->state & SOCKET_SETUP_PENDING) {
	    send_event(s, CS_EVENT_CARD_INSERTION, CS_EVENT_PRI_LOW);
	    s->state &= ~SOCKET_SETUP_PENDING;
	} else {
	    send_event(s, CS_EVENT_CARD_RESET, CS_EVENT_PRI_LOW);
	    s->reset_handle->event_callback_args.info = NULL;
	    EVENT(s->reset_handle, CS_EVENT_RESET_COMPLETE,
		  CS_EVENT_PRI_LOW);
	    s->state &= ~EVENT_MASK;
	}
    } else {
	DEBUG(2, "cs: socket %ld not ready yet\n", i);
	if (s->unreset_timeout > unreset_limit) {
	    printk(KERN_NOTICE "cs: socket %ld timed out during"
		   " reset\n", i);
	    s->state &= ~EVENT_MASK;
	} else {
	    s->unreset_timeout++;
	    s->setup.expires = RUN_AT(unreset_check);
	    add_timer(&s->setup);
	}
    }
} /* unreset_socket */

/*======================================================================

    The central event handler.  Send_event() sends an event to all
    valid clients.  Parse_events() interprets the event bits from
    a card status change report.  Do_shotdown() handles the high
    priority stuff associated with a card removal.
    
======================================================================*/

static int send_event(socket_info_t *s, event_t event, int priority)
{
    client_t *client = s->clients;
    int ret;
    DEBUG(1, "cs: send_event(sock %d, event %d, pri %d)\n",
	  s->sock, event, priority);
    ret = 0;
    for (; client; client = client->next) { 
	if (client->state & (CLIENT_UNBOUND|CLIENT_STALE))
	    continue;
	if (client->EventMask & event) {
	    ret = EVENT(client, event, priority);
	    if (ret != 0)
		return ret;
	}
    }
    return ret;
} /* send_event */

static void do_shutdown(socket_info_t *s)
{
    client_t *client;
    if (s->state & SOCKET_SHUTDOWN_PENDING)
	return;
    s->state |= SOCKET_SHUTDOWN_PENDING;
    send_event(s, CS_EVENT_CARD_REMOVAL, CS_EVENT_PRI_HIGH);
    for (client = s->clients; client; client = client->next)
	if (!(client->Attributes & INFO_MASTER_CLIENT))
	    client->state |= CLIENT_STALE;
    if (s->state & (SOCKET_SETUP_PENDING|SOCKET_RESET_PENDING)) {
#ifdef PCMCIA_DEBUG
	printk(KERN_DEBUG "cs: flushing pending setup\n");
#endif
	del_timer(&s->setup);
	s->state &= ~EVENT_MASK;
    }
    s->shutdown.expires = RUN_AT(shutdown_delay);
    add_timer(&s->shutdown);
    s->state &= ~SOCKET_PRESENT;
}

static void parse_events(void *info, u_int events)
{
    socket_info_t *s = info;
    if (events & SS_DETECT) {
	int status;
	cli();
	if ((s->state & SOCKET_PRESENT) &&
	    !(s->state & SOCKET_SUSPEND))
	    do_shutdown(s);
	s->ss_entry(s->sock, SS_GetStatus, &status);
	if (status & SS_DETECT) {
	    if (s->state & SOCKET_SETUP_PENDING) {
		del_timer(&s->setup);
		DEBUG(1, "cs: delaying pending setup\n");
	    }
	    s->state |= SOCKET_SETUP_PENDING;
	    s->setup.function = &setup_socket;
	    if (s->state & SOCKET_SUSPEND)
		s->setup.expires = RUN_AT(resume_delay);
	    else
		s->setup.expires = RUN_AT(setup_delay);
	    add_timer(&s->setup);
	}
	sti();
    } else if ((s->state & SOCKET_PRESENT) &&
	       (s->state & SOCKET_SUSPEND))
	do_shutdown(s);
    if (events & SS_BATDEAD)
	send_event(s, CS_EVENT_BATTERY_DEAD, CS_EVENT_PRI_LOW);
    if (events & SS_BATWARN)
	send_event(s, CS_EVENT_BATTERY_LOW, CS_EVENT_PRI_LOW);
    if (events & SS_READY) {
	if (!(s->state & SOCKET_RESET_PENDING))
	    send_event(s, CS_EVENT_READY_CHANGE, CS_EVENT_PRI_LOW);
#ifdef PCMCIA_DEBUG
	else DEBUG(1, "cs: ready change during reset\n");
#endif
    }
} /* parse_events */

/*======================================================================

    Another event handler, for power management events.

    This does not comply with the latest PCMCIA spec for handling
    power management events.
    
======================================================================*/

#ifdef CONFIG_APM
static int handle_apm_event(apm_event_t event)
{
    int i, stat;
    socket_info_t *s;
    static int down = 0;
    
    switch (event) {
    case APM_SYS_SUSPEND:
    case APM_USER_SUSPEND:
	DEBUG(1, "cs: received suspend notification\n");
	if (down) {
	    printk(KERN_DEBUG "cs: received extra suspend event\n");
	    break;
	}
	down = 1;
	for (i = 0; i < sockets; i++) {
	    s = socket_table[i];
	    if ((s->state & SOCKET_PRESENT) &&
		!(s->state & SOCKET_SUSPEND)){
		send_event(s, CS_EVENT_PM_SUSPEND, CS_EVENT_PRI_LOW);
		s->ss_entry(s->sock, SS_SetSocket, &dead_socket);
		s->state |= SOCKET_SUSPEND;
	    }
	}
	break;
    case APM_NORMAL_RESUME:
    case APM_CRITICAL_RESUME:
	DEBUG(1, "cs: received resume notification\n");
	if (!down) {
	    printk(KERN_DEBUG "cs: received bogus resume event\n");
	    break;
	}
	down = 0;
	for (i = 0; i < sockets; i++) {
	    s = socket_table[i];
	    /* Do this just to reinitialize the socket */
	    s->ss_entry(s->sock, SS_SetSocket, &dead_socket);
	    s->ss_entry(s->sock, SS_GetStatus, &stat);
	    /* If there was or is a card here, we need to do something
	       about it... but parse_events will sort it all out. */
       	    if ((s->state & SOCKET_PRESENT) || (stat & SS_DETECT))
		parse_events(s, SS_DETECT);
	}
	break;
    }
    return 0;
} /* handle_apm_event */
#endif

/*======================================================================

    Special stuff for managing IO windows, because they are scarce.
    
======================================================================*/

static int alloc_io_space(socket_info_t *s, u_int attr, ioaddr_t *base,
			  ioaddr_t num, char *name)
{
    int i;
    ioaddr_t try;
    
    for (i = 0; i < MAX_IO_WIN; i++) {
	if (s->io[i].NumPorts == 0) {
	    if (find_io_region(base, num, name) == 0) {
		s->io[i].Attributes = attr;
		s->io[i].BasePort = *base;
		s->io[i].NumPorts = s->io[i].InUse = num;
		break;
	    } else
		return 1;
	} else if (s->io[i].Attributes != attr)
	    continue;
	/* Try to extend top of window */
	try = s->io[i].BasePort + s->io[i].NumPorts;
	if ((*base == 0) || (*base == try))
	    if (find_io_region(&try, num, name) == 0) {
		*base = try;
		s->io[i].NumPorts += num;
		s->io[i].InUse += num;
		break;
	    }
	/* Try to extend bottom of window */
	try = s->io[i].BasePort - num;
	if ((*base == 0) || (*base == try))
	    if (find_io_region(&try, num, name) == 0) {
		s->io[i].BasePort = *base = try;
		s->io[i].NumPorts += num;
		s->io[i].InUse += num;
		break;
	    }
    }
    return (i == MAX_IO_WIN);
} /* alloc_io_space */

static void release_io_space(socket_info_t *s, ioaddr_t base,
			     ioaddr_t num)
{
    int i;
    release_region(base, num);
    for (i = 0; i < MAX_IO_WIN; i++) {
	if ((s->io[i].BasePort <= base) &&
	    (s->io[i].BasePort+s->io[i].NumPorts >= base+num)) {
	    s->io[i].InUse -= num;
	    /* Free the window if no one else is using it */
	    if (s->io[i].InUse == 0)
		s->io[i].NumPorts = 0;
	}
    }
}

/*======================================================================

    Access_configuration_register() reads and writes configuration
    registers in attribute memory.  Memory window 0 is reserved for
    this and the tuple reading services.
    
======================================================================*/

static int access_configuration_register(client_handle_t handle,
					 conf_reg_t *reg)
{
    socket_info_t *s;
    config_t *c;
    int addr;
    u_char val;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    s = SOCKET(handle);
    if (handle->Function == BIND_FN_ALL) {
	if (reg->Function >= s->functions)
	    return CS_BAD_ARGS;
	c = &s->config[reg->Function];
    } else
	c = CONFIG(handle);
    if (!(c->state & CONFIG_LOCKED))
	return CS_CONFIGURATION_LOCKED;

    if (setup_cis_mem(s) != 0)
	return CS_OUT_OF_RESOURCE;
    addr = (c->ConfigBase + reg->Offset) >> 1;
    
    switch (reg->Action) {
    case CS_READ:
	read_cis_mem(s, 1, addr, 1, &val);
	reg->Value = val;
	break;
    case CS_WRITE:
	val = reg->Value;
	write_cis_mem(s, 1, addr, 1, &val);
	break;
    default:
	return CS_BAD_ARGS;
	break;
    }
    return CS_SUCCESS;
} /* access_configuration_register */

/*======================================================================

    Bind_device() associates a device driver with a particular socket.
    It is normally called by Driver Services after it has identified
    a newly inserted card.  An instance of that driver will then be
    eligible to register as a client of this socket.
    
======================================================================*/

static int bind_device(bind_req_t *req)
{
    client_t *client;
    socket_info_t *s;
    ss_callback_t call;
    int status;

    if (CHECK_SOCKET(req->Socket))
	return CS_BAD_SOCKET;
    s = SOCKET(req);

    MOD_INC_USE_COUNT;
    if (s->clients == NULL) {
	call.handler = &parse_events;
	call.info = s;
	s->ss_entry(req->Socket, SS_RegisterCallback, &call);
	s->ss_entry(req->Socket, SS_GetStatus, &status);
	if (status & SS_DETECT) {
	    s->state |= SOCKET_SETUP_PENDING;
	    setup_socket(req->Socket);
	}
    }

    client = (client_t *)kmalloc(sizeof(client_t), GFP_KERNEL);
    memset(client, '\0', sizeof(client_t));
    client->client_magic = CLIENT_MAGIC;
    client->dev_info = req->dev_info;
    client->Socket = req->Socket;
    client->Function = req->Function;
    client->state = CLIENT_UNBOUND;
    client->erase_busy.next = &client->erase_busy;
    client->erase_busy.prev = &client->erase_busy;
    init_waitqueue(&client->mtd_req);
    client->next = s->clients;
    s->clients = client;
    DEBUG(1, "cs: bind_device(): client 0x%p, sock %d, dev %s\n",
	  client, client->Socket, (char *)client->dev_info);
    return CS_SUCCESS;
} /* bind_device */

/*======================================================================

    Bind_mtd() associates a device driver with a particular memory
    region.  It is normally called by Driver Services after it has
    identified a memory device type.  An instance of the corresponding
    driver will then be able to register to control this region.
    
======================================================================*/

static int bind_mtd(mtd_bind_t *req)
{
    socket_info_t *s;
    memory_handle_t region;
    
    if (CHECK_SOCKET(req->Socket))
	return CS_BAD_SOCKET;
    s = SOCKET(req);

    if (req->Attributes & REGION_TYPE_AM)
	region = s->a_region;
    else
	region = s->c_region;

    while (region) {
	if (region->info.CardOffset == req->CardOffset) break;
	region = region->info.next;
    }
    if (!region || (region->mtd != NULL))
	return CS_BAD_OFFSET;
    region->dev_info = req->dev_info;
    
    DEBUG(1, "cs: bind_mtd(): attr 0x%x, offset 0x%x, dev %s\n",
	  req->Attributes, req->CardOffset, (char *)req->dev_info);
    return CS_SUCCESS;
} /* bind_mtd */

/*====================================================================*/

static int deregister_client(client_handle_t handle)
{
    client_t **client;
    socket_info_t *s;
    memory_handle_t region;
    u_long flags;
    int i, sn;
    
    DEBUG(1, "cs: deregister_client(%p)\n", handle);
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    if (handle->state &
	(CLIENT_IRQ_REQ|CLIENT_IO_REQ|CLIENT_CONFIG_LOCKED))
	return CS_IN_USE;
    for (i = 0; i < MAX_WIN; i++)
	if (handle->state & CLIENT_WIN_REQ(i))
	    return CS_IN_USE;

    /* Disconnect all MTD links */
    s = SOCKET(handle);
    if (handle->mtd_count) {
	for (region = s->a_region; region; region = region->info.next)
	    if (region->mtd == handle) region->mtd = NULL;
	for (region = s->c_region; region; region = region->info.next)
	    if (region->mtd == handle) region->mtd = NULL;
    }
    
    save_flags(flags);
    cli();
    sn = handle->Socket; s = socket_table[sn];
    client = &s->clients;
    while ((*client) && ((*client) != handle))
	client = &(*client)->next;
    if (*client == NULL)
	return CS_BAD_HANDLE;
    *client = handle->next;
    handle->client_magic = 0;
    kfree_s(handle, sizeof(struct client_t));

    MOD_DEC_USE_COUNT;
    if (s->clients == NULL) {
	s->ss_entry(sn, SS_RegisterCallback, NULL);
    }
    
    restore_flags(flags);
    return CS_SUCCESS;
} /* deregister_client */

/*====================================================================*/

static int get_configuration_info(client_handle_t handle,
				  config_info_t *config)
{
    socket_info_t *s;
    config_t *c;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    s = SOCKET(handle);
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;

#ifdef CONFIG_CARDBUS
    if (s->state & SOCKET_CARDBUS) {
	u_char fn = config->Function;
	if (handle->Function == BIND_FN_ALL) {
	    if (fn && (fn >= s->functions))
		return CS_BAD_ARGS;
	} else
	    fn = handle->Function;
	memset(config, 0, sizeof(config_info_t));
	config->Function = fn;
	config->Vcc = s->socket.Vcc;
	config->Vpp1 = config->Vpp2 = s->socket.Vpp;
	config->Option = s->cap.cardbus;
	if (s->cb_config) {
	    config->Attributes = CONF_VALID_CLIENT;
	    config->IntType = INT_CARDBUS;
	    config->AssignedIRQ = s->irq.AssignedIRQ;
	}
	return CS_SUCCESS;
    }
#endif
    
    if (handle->Function == BIND_FN_ALL) {
	if (config->Function && (config->Function >= s->functions))
	    return CS_BAD_ARGS;
    } else {
	config->Function = handle->Function;
    }
    c = (s->config != NULL) ? &s->config[config->Function] : NULL;
    
    if ((c == NULL) || !(c->state & CONFIG_LOCKED)) {
	config->Attributes = 0;
	config->Vcc = s->socket.Vcc;
	config->Vpp1 = config->Vpp2 = s->socket.Vpp;
	return CS_SUCCESS;
    }
    
    /* !!! This is a hack !!! */
    memcpy(&config->Attributes, &c->Attributes, sizeof(config_t));
    config->Attributes |= CONF_VALID_CLIENT;
    config->CardValues = c->CardValues;
    config->IRQAttributes = c->irq.Attributes;
    config->AssignedIRQ = s->irq.AssignedIRQ;
    config->BasePort1 = c->io.BasePort1;
    config->NumPorts1 = c->io.NumPorts1;
    config->Attributes1 = c->io.Attributes1;
    config->BasePort2 = c->io.BasePort2;
    config->NumPorts2 = c->io.NumPorts2;
    config->Attributes2 = c->io.Attributes2;
    config->IOAddrLines = c->io.IOAddrLines;
    
    return CS_SUCCESS;
} /* get_configuration_info */

/*======================================================================

    Return information about this version of Card Services.
    
======================================================================*/

static int get_card_services_info(servinfo_t *info)
{
    info->Signature[0] = 'C';
    info->Signature[1] = 'S';
    info->Count = sockets;
    info->Revision = CS_RELEASE_CODE;
    info->CSLevel = 0x0210;
    info->VendorString = (char *)release;
    return CS_SUCCESS;
} /* get_card_services_info */

/*======================================================================

    Note that get_first_client() *does* recognize the Socket field
    in the request structure.
    
======================================================================*/

static int get_first_client(client_handle_t *handle, client_req_t *req)
{
    socket_t s;
    if (req->Attributes & CLIENT_THIS_SOCKET)
	s = req->Socket;
    else
	s = 0;
    if (CHECK_SOCKET(req->Socket))
	return CS_BAD_SOCKET;
    if (socket_table[s]->clients == NULL)
	return CS_NO_MORE_ITEMS;
    *handle = socket_table[s]->clients;
    return CS_SUCCESS;
} /* get_first_client */

/*====================================================================*/

static int get_next_client(client_handle_t *handle, client_req_t *req)
{
    socket_info_t *s;
    if ((handle == NULL) || CHECK_HANDLE(*handle))
	return CS_BAD_HANDLE;
    if ((*handle)->next == NULL) {
	if (req->Attributes & CLIENT_THIS_SOCKET)
	    return CS_NO_MORE_ITEMS;
	s = SOCKET(*handle);
	if (s->clients == NULL)
	    return CS_NO_MORE_ITEMS;
	*handle = s->clients;
    } else
	*handle = (*handle)->next;
    return CS_SUCCESS;
} /* get_next_client */

/*======================================================================

    Get the current socket state bits.  We don't support the latched
    SocketState yet.  We also don't support the Pin Replacement
    Register.
    
======================================================================*/

static int get_status(client_handle_t handle, status_t *status)
{
    socket_info_t *s;
    config_t *c;
    int val;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    s = SOCKET(handle);
    s->ss_entry(s->sock, SS_GetStatus, &val);
    status->CardState = status->SocketState = 0;
    status->CardState |= (val & SS_DETECT) ? CS_EVENT_CARD_DETECT : 0;
#ifdef CONFIG_CARDBUS
    status->CardState |= (val & SS_CARDBUS) ? CS_EVENT_CB_DETECT : 0;
#endif
    if (s->state & SOCKET_SUSPEND)
	status->CardState |= CS_EVENT_PM_SUSPEND;
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    if (s->state & SOCKET_SETUP_PENDING)
	status->CardState |= CS_EVENT_CARD_INSERTION;
    
    /* Get info from the PRR, if necessary */
    if (handle->Function == BIND_FN_ALL) {
	if (status->Function && (status->Function >= s->functions))
	    return CS_BAD_ARGS;
	c = (s->config != NULL) ? &s->config[status->Function] : NULL;
    } else
	c = CONFIG(handle);
    if ((c != NULL) && (c->state & CONFIG_LOCKED) &&
	(c->IntType & INT_MEMORY_AND_IO)) {
	u_char reg;
	if (c->Present & PRESENT_PIN_REPLACE) {
	    read_cis_mem(s, 1, (c->ConfigBase+CISREG_PRR)>>1, 1, &reg);
	    status->CardState |=
		(reg & PRR_WP_STATUS) ? CS_EVENT_WRITE_PROTECT : 0;
	    status->CardState |=
		(reg & PRR_READY_STATUS) ? CS_EVENT_READY_CHANGE : 0;
	    status->CardState |=
		(reg & PRR_BVD2_STATUS) ? CS_EVENT_BATTERY_LOW : 0;
	    status->CardState |=
		(reg & PRR_BVD1_STATUS) ? CS_EVENT_BATTERY_DEAD : 0;
	} else {
	    /* No PRR?  Then assume we're always ready */
	    status->CardState |= CS_EVENT_READY_CHANGE;
	}
	if (c->Present & PRESENT_EXT_STATUS) {
	    read_cis_mem(s, 1, (c->ConfigBase+CISREG_ESR)>>1, 1, &reg);
	    status->CardState |=
		(reg & ESR_REQ_ATTN) ? CS_EVENT_REQUEST_ATTENTION : 0;
	}
	return CS_SUCCESS;
    }
    status->CardState |=
	(val & SS_WRPROT) ? CS_EVENT_WRITE_PROTECT : 0;
    status->CardState |=
	(val & SS_BATDEAD) ? CS_EVENT_BATTERY_DEAD : 0;
    status->CardState |=
	(val & SS_BATWARN) ? CS_EVENT_BATTERY_LOW : 0;
    status->CardState |=
	(val & SS_READY) ? CS_EVENT_READY_CHANGE : 0;
    return CS_SUCCESS;
} /* get_status */

/*======================================================================

    Change the card address of an already open memory window.
    
======================================================================*/

static int map_mem_page(window_handle_t win, memreq_t *req)
{
    if ((win == NULL) || (win->magic != WINDOW_MAGIC))
	return CS_BAD_HANDLE;
    if (req->Page != 0)
	return CS_BAD_PAGE;

    win->ctl.card_start = req->CardOffset;
    win->sock->ss_entry(win->sock->sock, SS_SetMemMap, &win->ctl);
    
    return CS_SUCCESS;
} /* map_mem_page */

/*======================================================================

    Modify a locked socket configuration
    
======================================================================*/

static int modify_configuration(client_handle_t handle,
				modconf_t *mod)
{
    socket_info_t *s;
    config_t *c;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    s = SOCKET(handle); c = CONFIG(handle);
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    if (!(c->state & CONFIG_LOCKED))
	return CS_CONFIGURATION_LOCKED;
    
    if (mod->Attributes & CONF_IRQ_CHANGE_VALID) {
	if (mod->Attributes & CONF_ENABLE_IRQ) {
	    c->Attributes |= CONF_ENABLE_IRQ;
	    s->socket.io_irq = s->irq.AssignedIRQ;
	} else {
	    c->Attributes &= ~CONF_ENABLE_IRQ;
	    s->socket.io_irq = 0;
	}
	s->ss_entry(s->sock, SS_SetSocket, &s->socket);
    }

    if (mod->Attributes & CONF_VCC_CHANGE_VALID) {
	c->Vcc = s->socket.Vcc = mod->Vcc;
	if (s->ss_entry(s->sock, SS_SetSocket, &s->socket))
	    return CS_BAD_VCC;
    }

    /* We only allow changing Vpp1 and Vpp2 to the same value */
    if ((mod->Attributes & CONF_VPP1_CHANGE_VALID) &&
	(mod->Attributes & CONF_VPP2_CHANGE_VALID)) {
	if (mod->Vpp1 != mod->Vpp2)
	    return CS_BAD_VPP;
	c->Vpp1 = c->Vpp2 = s->socket.Vpp = mod->Vpp1;
	if (s->ss_entry(s->sock, SS_SetSocket, &s->socket))
	    return CS_BAD_VPP;
    } else if ((mod->Attributes & CONF_VPP1_CHANGE_VALID) ||
	       (mod->Attributes & CONF_VPP2_CHANGE_VALID))
	return CS_BAD_VPP;

    return CS_SUCCESS;
} /* modify_configuration */

/*======================================================================

    Modify the attributes of a window returned by RequestWindow.

======================================================================*/

static int modify_window(window_handle_t win, modwin_t *req)
{
    if ((win == NULL) || (win->magic != WINDOW_MAGIC))
	return CS_BAD_HANDLE;

    win->ctl.flags &= ~(MAP_ATTRIB|MAP_ACTIVE);
    if (req->Attributes & WIN_MEMORY_TYPE)
	win->ctl.flags |= MAP_ATTRIB;
    if (req->Attributes & WIN_ENABLE)
	win->ctl.flags |= MAP_ACTIVE;
    if (req->Attributes & WIN_DATA_WIDTH)
	win->ctl.flags |= MAP_16BIT;
    if (req->Attributes & WIN_USE_WAIT)
	win->ctl.flags |= MAP_USE_WAIT;
    win->ctl.speed = req->AccessSpeed;
    win->sock->ss_entry(win->sock->sock, SS_SetMemMap, &win->ctl);
    
    return CS_SUCCESS;
} /* modify_window */

/*======================================================================

    Register_client() uses the dev_info_t handle to match the
    caller with a socket.  The driver must have already been bound
    to a socket with bind_device() -- in fact, bind_device()
    allocates the client structure that will be used.
    
======================================================================*/

static int register_client(client_handle_t *handle, client_reg_t *req)
{
    client_t *client;
    socket_info_t *s;
    socket_t ns;
    
    /* Look for unbound client with matching dev_info */
    client = NULL;
    for (ns = 0; ns < sockets; ns++) {
	client = socket_table[ns]->clients;
	while (client != NULL) {
	    if ((client->dev_info == req->dev_info) &&
		(client->state & CLIENT_UNBOUND)) break;
	    client = client->next;
	}
	if (client != NULL) break;
    }
    s = socket_table[ns];
    
    if (client == NULL)
	return CS_OUT_OF_RESOURCE;
    *handle = client;
    client->state &= ~CLIENT_UNBOUND;
    client->Socket = ns;
    client->Attributes = req->Attributes;
    client->EventMask = req->EventMask;
    client->event_handler = req->event_handler;
    client->event_callback_args = req->event_callback_args;
    client->event_callback_args.client_handle = client;

#ifdef CONFIG_CARDBUS
    if (s->state & SOCKET_CARDBUS)
	client->state |= CLIENT_CARDBUS;
#endif
    
    if ((!(s->state & SOCKET_CARDBUS)) && (s->functions == 0) &&
	(client->Function != BIND_FN_ALL)) {
	cistpl_longlink_mfc_t mfc;
	if (read_tuple(client, CISTPL_LONGLINK_MFC, &mfc)
	    == CS_SUCCESS)
	    s->functions = mfc.nfn;
	else
	    s->functions = 1;
	s->config = kmalloc(sizeof(config_t) * s->functions,
			    GFP_KERNEL);
	memset(s->config, 0, sizeof(config_t) * s->functions);
    }
    
    DEBUG(1, "cs: register_client(): client 0x%p, sock %d, dev %s\n",
	  client, client->Socket, (char *)client->dev_info);
    if (client->EventMask & CS_EVENT_REGISTRATION_COMPLETE)
	EVENT(client, CS_EVENT_REGISTRATION_COMPLETE, CS_EVENT_PRI_LOW);
    if ((socket_table[ns]->state & SOCKET_PRESENT) &&
	!(socket_table[ns]->state & SOCKET_SETUP_PENDING)) {
	if (client->EventMask & CS_EVENT_CARD_INSERTION)
	    EVENT(client, CS_EVENT_CARD_INSERTION, CS_EVENT_PRI_LOW);
	else
	    client->PendingEvents |= CS_EVENT_CARD_INSERTION;
    }
    return CS_SUCCESS;
} /* register_client */

/*====================================================================*/

static int release_configuration(client_handle_t handle,
				 socket_t *Socket)
{
    pcmcia_io_map io;
    socket_info_t *s;
    int i;
    
    if (CHECK_HANDLE(handle) ||
	!(handle->state & CLIENT_CONFIG_LOCKED))
	return CS_BAD_HANDLE;
    handle->state &= ~CLIENT_CONFIG_LOCKED;
    s = SOCKET(handle);
    
#ifdef CONFIG_CARDBUS
    if (handle->state & CLIENT_CARDBUS) {
	cb_disable(s);
	return CS_SUCCESS;
    }
#endif
    
    if (!(handle->state & CLIENT_STALE)) {
	config_t *c = CONFIG(handle);
	if (--(s->lock_count) == 0) {
	    s->socket.flags = SS_OUTPUT_ENA;
	    s->socket.Vpp = 0;
	    s->socket.io_irq = 0;
	    s->ss_entry(s->sock, SS_SetSocket, &s->socket);
	}
	if (c->state & CONFIG_IO_REQ)
	    for (i = 0; i < MAX_IO_WIN; i++) {
		if (s->io[i].NumPorts == 0)
		    continue;
		s->io[i].Config--;
		if (s->io[i].Config != 0)
		    continue;
		io.map = i;
		s->ss_entry(s->sock, SS_GetIOMap, &io);
		io.flags &= ~MAP_ACTIVE;
		s->ss_entry(s->sock, SS_SetIOMap, &io);
	    }
	c->state &= ~CONFIG_LOCKED;
    }
    
    return CS_SUCCESS;
} /* release_configuration */

/*======================================================================

    Release_io() releases the I/O ranges allocated by a client.  This
    may be invoked some time after a card ejection has already dumped
    the actual socket configuration, so if the client is "stale", we
    don't bother checking the port ranges against the current socket
    values.
    
======================================================================*/

static int release_io(client_handle_t handle, io_req_t *req)
{
    socket_info_t *s;
    
    if (CHECK_HANDLE(handle) || !(handle->state & CLIENT_IO_REQ))
	return CS_BAD_HANDLE;
    handle->state &= ~CLIENT_IO_REQ;
    s = SOCKET(handle);
    
#ifdef CONFIG_CARDBUS
    if (handle->state & CLIENT_CARDBUS) {
	cb_release(s);
	return CS_SUCCESS;
    }
#endif
    
    if (!(handle->state & CLIENT_STALE)) {
	config_t *c = CONFIG(handle);
	if (c->state & CONFIG_LOCKED)
	    return CS_CONFIGURATION_LOCKED;
	if ((c->io.BasePort1 != req->BasePort1) ||
	    (c->io.NumPorts1 != req->NumPorts1) ||
	    (c->io.BasePort2 != req->BasePort2) ||
	    (c->io.NumPorts2 != req->NumPorts2))
	    return CS_BAD_ARGS;
	c->state &= ~CONFIG_IO_REQ;
    }

    release_io_space(s, req->BasePort1, req->NumPorts1);
    if (req->NumPorts2)
	release_io_space(s, req->BasePort2, req->NumPorts2);
    
    return CS_SUCCESS;
} /* release_io */

/*====================================================================*/

static int release_irq(client_handle_t handle, irq_req_t *req)
{
    if (CHECK_HANDLE(handle) || !(handle->state & CLIENT_IRQ_REQ))
	return CS_BAD_HANDLE;
    handle->state &= ~CLIENT_IRQ_REQ;
    
    if (!(handle->state & CLIENT_STALE)) {
	socket_info_t *s = SOCKET(handle);
	config_t *c = CONFIG(handle);
	if (c->state & CONFIG_LOCKED)
	    return CS_CONFIGURATION_LOCKED;
	if (c->irq.Attributes != req->Attributes)
	    return CS_BAD_ATTRIBUTE;
	if (s->irq.AssignedIRQ != req->AssignedIRQ)
	    return CS_BAD_IRQ;
	c->state &= ~CONFIG_IRQ_REQ;
	s->irq.Config--;
    }
    
    if (req->Attributes & IRQ_HANDLE_PRESENT) {
	FREE_IRQ(req->AssignedIRQ, req->Instance);
	if (req->Instance)
	    IRQ_MAP(req->AssignedIRQ, NULL);
    }
    
    undo_irq(req->Attributes, req->AssignedIRQ);
    
    return CS_SUCCESS;
} /* release_irq */

/*====================================================================*/

static int release_window(window_handle_t win)
{
    socket_info_t *s;
    
    if ((win == NULL) || (win->magic != WINDOW_MAGIC))
	return CS_BAD_HANDLE;
    s = win->sock;
    if (!(win->handle->state & CLIENT_WIN_REQ(win->index)))
	return CS_BAD_HANDLE;

    /* Shut down memory window */
    win->ctl.flags &= ~MAP_ACTIVE;
    s->ss_entry(s->sock, SS_SetMemMap, &win->ctl);
    s->state &= ~SOCKET_WIN_REQ(win->index);

    /* Release system memory */
    release_mem_region(win->base, win->size);
    win->handle->state &= ~CLIENT_WIN_REQ(win->index);

    win->magic = 0;
    
    return CS_SUCCESS;
} /* release_window */

/*====================================================================*/

static int request_configuration(client_handle_t handle,
				 config_req_t *req)
{
    int i;
    u_int base;
    socket_info_t *s;
    config_t *c;
    pcmcia_io_map iomap;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    i = handle->Socket; s = socket_table[i];
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    
#ifdef CONFIG_CARDBUS
    if (handle->state & CLIENT_CARDBUS) {
	cb_enable(s);
	handle->state |= CLIENT_CONFIG_LOCKED;
	s->lock_count++;
	return CS_SUCCESS;
    }
#endif
    
    c = CONFIG(handle);
    if (c->state & CONFIG_LOCKED)
	return CS_CONFIGURATION_LOCKED;
    if (setup_cis_mem(s) != 0)
	return CS_OUT_OF_RESOURCE;

    /* Do power control */
    s->socket.Vcc = req->Vcc;
    if (s->ss_entry(s->sock, SS_SetSocket, &s->socket))
	return CS_BAD_VCC;
    
    if (req->Vpp1 != req->Vpp2)
	return CS_BAD_VPP;
    s->socket.Vpp = req->Vpp1;
    if (s->ss_entry(s->sock, SS_SetSocket, &s->socket))
	return CS_BAD_VPP;
    
    c->Vcc = req->Vcc; c->Vpp1 = c->Vpp2 = req->Vpp1;
    
    /* Pick memory or I/O card, DMA mode, interrupt */
    c->IntType = req->IntType;
    c->Attributes = req->Attributes;
    if (req->IntType & INT_MEMORY_AND_IO)
	s->socket.flags |= SS_IOCARD;
    if (req->Attributes & CONF_ENABLE_DMA)
	s->socket.flags |= SS_DMA_MODE;
    if (req->Attributes & CONF_ENABLE_SPKR)
	s->socket.flags |= SS_SPKR_ENA;
    if (req->Attributes & CONF_ENABLE_IRQ)
	s->socket.io_irq = s->irq.AssignedIRQ;
    else
	s->socket.io_irq = 0;
    s->ss_entry(s->sock, SS_SetSocket, &s->socket);
    s->lock_count++;
    
    /* Set up CIS configuration registers */
    base = c->ConfigBase = req->ConfigBase;
    c->Present = c->CardValues = req->Present;
    if (req->Present & PRESENT_COPY) {
	c->Copy = req->Copy;
	write_cis_mem(s, 1, (base + CISREG_SCR)>>1, 1, &c->Copy);
    }
    if (req->Present & PRESENT_OPTION) {
	if (s->functions == 1)
	    c->Option = req->ConfigIndex & COR_CONFIG_MASK;
	else {
	    c->Option = req->ConfigIndex & COR_MFC_CONFIG_MASK;
	    c->Option |= COR_FUNC_ENA|COR_ADDR_DECODE|COR_IREQ_ENA;
	}
	if (c->state & CONFIG_IRQ_REQ)
	    if (!(c->irq.Attributes & IRQ_FORCED_PULSE))
		c->Option |= COR_LEVEL_REQ;
	write_cis_mem(s, 1, (base + CISREG_COR)>>1, 1, &c->Option);
	busy_loop(HZ/25);
    }
    if (req->Present & PRESENT_STATUS) {
	c->Status = req->Status;
	write_cis_mem(s, 1, (base + CISREG_CCSR)>>1, 1, &c->Status);
    }
    if (req->Present & PRESENT_PIN_REPLACE) {
	c->Pin = req->Pin;
	write_cis_mem(s, 1, (base + CISREG_PRR)>>1, 1, &c->Pin);
    }
    if (req->Present & PRESENT_EXT_STATUS) {
	c->ExtStatus = req->ExtStatus;
	write_cis_mem(s, 1, (base + CISREG_ESR)>>1, 1, &c->ExtStatus);
    }
    if (req->Present & PRESENT_IOBASE_0) {
	i = c->io.BasePort1 & 0xff;
	write_cis_mem(s, 1, (base + CISREG_IOBASE_0)>>1, 1, &i);
	i = (c->io.BasePort1 >> 8) & 0xff;
	write_cis_mem(s, 1, (base + CISREG_IOBASE_1)>>1, 1, &i);
    }
    if (req->Present & PRESENT_IOSIZE) {
	i = c->io.NumPorts1 + c->io.NumPorts2 - 1;
	write_cis_mem(s, 1, (base + CISREG_IOSIZE)>>1, 1, &i);
    }
    
    /* Configure I/O windows */
    if (c->state & CONFIG_IO_REQ) {
	iomap.speed = io_speed;
	for (i = 0; i < MAX_IO_WIN; i++)
	    if (s->io[i].NumPorts != 0) {
		iomap.map = i;
		iomap.flags = MAP_ACTIVE;
		switch (s->io[i].Attributes & IO_DATA_PATH_WIDTH) {
		case IO_DATA_PATH_WIDTH_16:
		    iomap.flags |= MAP_16BIT; break;
		case IO_DATA_PATH_WIDTH_AUTO:
		    iomap.flags |= MAP_AUTOSZ; break;
		default:
		    break;
		}
		iomap.start = s->io[i].BasePort;
		iomap.stop = iomap.start + s->io[i].NumPorts - 1;
		s->ss_entry(s->sock, SS_SetIOMap, &iomap);
		s->io[i].Config++;
	    }
    }
    
    c->state |= CONFIG_LOCKED;
    handle->state |= CLIENT_CONFIG_LOCKED;
    return CS_SUCCESS;
} /* request_configuration */

/*======================================================================
  
    Request_io() reserves ranges of port addresses for a socket.
    I have not implemented range sharing or alias addressing.
    
======================================================================*/

static int request_io(client_handle_t handle, io_req_t *req)
{
    socket_info_t *s;
    config_t *c;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    s = SOCKET(handle);
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;

#ifdef CONFIG_CARDBUS
    if (handle->state & CLIENT_CARDBUS) {
	handle->state |= CLIENT_IO_REQ;
	return cb_config(s);
    }
#endif
    
    c = CONFIG(handle);
    if (c->state & CONFIG_LOCKED)
	return CS_CONFIGURATION_LOCKED;
    if (c->state & CONFIG_IO_REQ)
	return CS_IN_USE;
    if (req->Attributes1 & (IO_SHARED | IO_FORCE_ALIAS_ACCESS))
	return CS_BAD_ATTRIBUTE;
    if ((req->NumPorts2 > 0) &&
	(req->Attributes2 & (IO_SHARED | IO_FORCE_ALIAS_ACCESS)))
	return CS_BAD_ATTRIBUTE;

    if (alloc_io_space(s, req->Attributes1, &req->BasePort1,
		       req->NumPorts1, (char *)handle->dev_info))
	return CS_IN_USE;

    if (req->NumPorts2) {
	if (alloc_io_space(s, req->Attributes2, &req->BasePort2,
			   req->NumPorts2, (char *)handle->dev_info)) {
	    release_io_space(s, req->BasePort1, req->NumPorts1);
	    return CS_IN_USE;
	}
    }

    c->io = *req;
    c->state |= CONFIG_IO_REQ;
    handle->state |= CLIENT_IO_REQ;
    return CS_SUCCESS;
} /* request_io */

/*======================================================================

    Request_irq() reserves an irq for this client.

    Also, since Linux only reserves irq's when they are actually
    hooked, we don't guarantee that an irq will still be available
    when the configuration is locked.  Now that I think about it,
    there might be a way to fix this using a dummy handler.
    
======================================================================*/

static int cs_request_irq(client_handle_t handle, irq_req_t *req)
{
    socket_info_t *s;
    config_t *c;
    int try, ret, irq;
    u_int mask;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    s = SOCKET(handle);
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    c = CONFIG(handle);
    if (c->state & CONFIG_LOCKED)
	return CS_CONFIGURATION_LOCKED;
    if (c->state & CONFIG_IRQ_REQ)
	return CS_IN_USE;
    
    ret = CS_BAD_ARGS;
    if (s->irq.AssignedIRQ != 0) {
	/* If the interrupt is already assigned, it must match */
	irq = s->irq.AssignedIRQ;
	if (req->IRQInfo1 & IRQ_INFO2_VALID) {
	    mask = req->IRQInfo2 & s->cap.irq_mask;
	    ret = ((mask >> irq) & 1) ? 0 : ret;
	} else
	    ret = ((req->IRQInfo1 & IRQ_MASK) == irq) ? 0 : ret;
    } else {
	if (req->IRQInfo1 & IRQ_INFO2_VALID) {
	    mask = req->IRQInfo2 & s->cap.irq_mask;
	    for (try = 0; try < 2; try++) {
		for (irq = 0; irq < 16; irq++)
		    if ((mask >> irq) & 1) {
			ret = try_irq(req->Attributes, irq, try);
			if (ret == 0) break;
		    }
		if (ret == 0) break;
	    }
	} else {
	    irq = req->IRQInfo1 & IRQ_MASK;
	    ret = try_irq(req->Attributes, irq, 1);
	}
    }
    if (ret != 0) return ret;

    if (req->Attributes & IRQ_HANDLE_PRESENT) {
	if (REQUEST_IRQ(irq, req->Handler,
			((req->Attributes & IRQ_TYPE_DYNAMIC_SHARING) || 
			 (s->functions > 1)) ? SA_SHIRQ : 0,
			(char *)handle->dev_info, req->Instance))
	    return CS_IN_USE;
	if (req->Instance)
	    IRQ_MAP(irq, req->Instance);
    }

    c->irq.Attributes = req->Attributes;
    s->irq.AssignedIRQ = req->AssignedIRQ = irq;
    s->irq.Config++;
    
    c->state |= CONFIG_IRQ_REQ;
    handle->state |= CLIENT_IRQ_REQ;
    return CS_SUCCESS;
} /* cs_request_irq */

/*======================================================================

    Request_window() establishes a mapping between card memory space
    and system memory space.

    Hmmm, maybe we should check the return value from the ioctl.
    
======================================================================*/

static int request_window(client_handle_t *handle, win_req_t *req)
{
    socket_info_t *s;
    window_t *win;
    int w;
    
    if (CHECK_HANDLE(*handle))
	return CS_BAD_HANDLE;
    s = SOCKET(*handle);
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    if (req->Attributes & (WIN_PAGED | WIN_SHARED))
	return CS_BAD_ATTRIBUTE;

    for (w = 0; w < MAX_WIN; w++)
	if (!(s->state & SOCKET_WIN_REQ(w))) break;
    if (w == MAX_WIN)
	return CS_OUT_OF_RESOURCE;

    /* Allocate system memory window */
    win = &s->win[w];
    win->magic = WINDOW_MAGIC;
    win->index = w;
    win->handle = *handle;
    win->sock = s;
    win->base = (u_long)req->Base;
    win->size = req->Size;
    if (find_mem_region(&win->base, win->size,
			(char *)(*handle)->dev_info))
	return CS_IN_USE;
    req->Base = (caddr_t)win->base;
    (*handle)->state |= CLIENT_WIN_REQ(w);

    /* Configure the PCMCIA controller */
    win->ctl.map = w+1;
    win->ctl.flags = 0;
    win->ctl.speed = req->AccessSpeed;
    if (req->Attributes & WIN_MEMORY_TYPE)
	win->ctl.flags |= MAP_ATTRIB;
    if (req->Attributes & WIN_ENABLE)
	win->ctl.flags |= MAP_ACTIVE;
    if (req->Attributes & WIN_DATA_WIDTH)
	win->ctl.flags |= MAP_16BIT;
    if (req->Attributes & WIN_USE_WAIT)
	win->ctl.flags |= MAP_USE_WAIT;
    win->ctl.sys_start = (u_long)req->Base;
    win->ctl.sys_stop = (u_long)req->Base + req->Size-1;
    win->ctl.card_start = 0;
    s->ss_entry(s->sock, SS_SetMemMap, &win->ctl);
    s->state |= SOCKET_WIN_REQ(w);

    /* Return window handle */
    *handle = (client_handle_t)win;
    
    return CS_SUCCESS;
} /* request_window */

/*======================================================================

    I'm not sure which "reset" function this is supposed to use,
    but for now, it uses the low-level interface's reset, not the
    CIS register.
    
======================================================================*/

static int reset_card(client_handle_t handle, client_req_t *req)
{
    int i, ret;
    socket_info_t *s;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    i = handle->Socket; s = socket_table[i];
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    if (s->state & SOCKET_RESET_PENDING)
	return CS_IN_USE;
    s->state |= SOCKET_RESET_PENDING;

    ret = send_event(s, CS_EVENT_RESET_REQUEST, CS_EVENT_PRI_LOW);
    if (ret != 0) {
	s->state &= ~SOCKET_RESET_PENDING;
	handle->event_callback_args.info = (void *)(u_long)ret;
	EVENT(handle, CS_EVENT_RESET_COMPLETE, CS_EVENT_PRI_LOW);
    } else {
	DEBUG(1, "cs: resetting socket %d\n", i);
	send_event(s, CS_EVENT_RESET_PHYSICAL, CS_EVENT_PRI_LOW);
	s->reset_handle = handle;
	reset_socket(i);
    }
    return CS_SUCCESS;
} /* reset_card */

/*======================================================================

    These shut down or wake up a socket.  They are sort of user
    initiated versions of the APM suspend and resume actions.
    
======================================================================*/

static int suspend_card(client_handle_t handle, client_req_t *req)
{
    int i;
    socket_info_t *s;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    i = handle->Socket; s = socket_table[i];
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    if (s->state & SOCKET_SUSPEND)
	return CS_IN_USE;

    DEBUG(1, "cs: suspending socket %d\n", i);
    send_event(s, CS_EVENT_PM_SUSPEND, CS_EVENT_PRI_LOW);
    s->ss_entry(s->sock, SS_SetSocket, &dead_socket);
    s->state |= SOCKET_SUSPEND;

    return CS_SUCCESS;
} /* suspend_card */

static int resume_card(client_handle_t handle, client_req_t *req)
{
    int i;
    socket_info_t *s;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    i = handle->Socket; s = socket_table[i];
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;
    if (!(s->state & SOCKET_SUSPEND))
	return CS_IN_USE;

    DEBUG(1, "cs: waking up socket %d\n", i);
    setup_socket(i);

    return CS_SUCCESS;
} /* resume_card */

/*======================================================================

    These handle user requests to eject or insert a card.
    
======================================================================*/

static int eject_card(client_handle_t handle, client_req_t *req)
{
    int i, ret;
    socket_info_t *s;
    u_long flags;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    i = handle->Socket; s = socket_table[i];
    if (!(s->state & SOCKET_PRESENT))
	return CS_NO_CARD;

    DEBUG(1, "cs: user eject request on socket %d\n", i);

    ret = send_event(s, CS_EVENT_EJECTION_REQUEST, CS_EVENT_PRI_LOW);
    if (ret != 0)
	return ret;

    save_flags(flags);
    cli();
    do_shutdown(s);
    restore_flags(flags);
    return CS_SUCCESS;
    
} /* eject_card */

static int insert_card(client_handle_t handle, client_req_t *req)
{
    int i, status;
    socket_info_t *s;
    u_long flags;
    
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    i = handle->Socket; s = socket_table[i];
    if (s->state & SOCKET_PRESENT)
	return CS_IN_USE;

    DEBUG(1, "cs: user insert request on socket %d\n", i);

    save_flags(flags);
    cli();
    if (!(s->state & SOCKET_SETUP_PENDING)) {
	s->state |= SOCKET_SETUP_PENDING;
	restore_flags(flags);
	s->ss_entry(i, SS_GetStatus, &status);
	if (status & SS_DETECT)
	    setup_socket(i);
	else {
	    s->state &= ~SOCKET_SETUP_PENDING;
	    return CS_NO_CARD;
	}
    } else
	restore_flags(flags);

    return CS_SUCCESS;
} /* insert_card */

/*======================================================================

    Maybe this should send a CS_EVENT_CARD_INSERTION event if we
    haven't sent one to this client yet?
    
======================================================================*/

static int set_event_mask(client_handle_t handle, eventmask_t *mask)
{
    u_int events, bit;
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    if (handle->Attributes & CONF_EVENT_MASK_VALID)
	return CS_BAD_SOCKET;
    handle->EventMask = mask->EventMask;
    events = handle->PendingEvents & handle->EventMask;
    handle->PendingEvents -= events;
    while (events != 0) {
	bit = ((events ^ (events-1)) + 1) >> 1;
	EVENT(handle, bit, CS_EVENT_PRI_LOW);
	events -= bit;
    }
    return CS_SUCCESS;
} /* set_event_mask */

/*====================================================================*/

static int report_error(client_handle_t handle, error_info_t *err)
{
    int i;
    char *serv;

    if (CHECK_HANDLE(handle))
	printk(KERN_NOTICE);
    else
	printk(KERN_NOTICE "%s: ", (char *)(handle->dev_info));
    
    for (i = 0; i < SERVICE_COUNT; i++)
	if (service_table[i].key == err->func) break;
    if (i < SERVICE_COUNT)
	serv = service_table[i].msg;
    else
	serv = "Unknown service number";

    for (i = 0; i < ERROR_COUNT; i++)
	if (error_table[i].key == err->retcode) break;
    if (i < ERROR_COUNT)
	printk("%s: %s\n", serv, error_table[i].msg);
    else
	printk("%s: Unknown error code %#x\n", serv, err->retcode);

    return CS_SUCCESS;
} /* report_error */

/*====================================================================*/

int CardServices(int func, void *a1, void *a2, void *a3)
{

#ifdef PCMCIA_DEBUG
    if (pc_debug > 1) {
	int i;
	for (i = 0; i < SERVICE_COUNT; i++)
	    if (service_table[i].key == func) break;
	if (i < SERVICE_COUNT)
	    printk(KERN_DEBUG "cs: CardServices(%s, 0x%p, 0x%p)\n",
		   service_table[i].msg, a1, a2);
	else
	    printk(KERN_DEBUG "cs: CardServices(Unknown func %d, "
		   "0x%p, 0x%p)\n", func, a1, a2);
    }
#endif
    switch (func) {
    case AccessConfigurationRegister:
	return access_configuration_register(a1, a2); break;
    case AdjustResourceInfo:
	return adjust_resource_info(a1, a2); break;
    case CheckEraseQueue:
	return check_erase_queue(a1); break;
    case CloseMemory:
	return close_memory(a1); break;
    case CopyMemory:
	return copy_memory(a1, a2); break;
    case DeregisterClient:
	return deregister_client(a1); break;
    case DeregisterEraseQueue:
	return deregister_erase_queue(a1); break;
    case GetFirstClient:
	return get_first_client(a1, a2); break;
    case GetCardServicesInfo:
	return get_card_services_info(a1); break;
    case GetConfigurationInfo:
	return get_configuration_info(a1, a2); break;
    case GetNextClient:
	return get_next_client(a1, a2); break;
    case GetFirstRegion:
	return get_first_region(a1, a2); break;
    case GetFirstTuple:
	return get_first_tuple(a1, a2); break;
    case GetNextRegion:
	return get_next_region(a1, a2); break;
    case GetNextTuple:
	return get_next_tuple(a1, a2); break;
    case GetStatus:
	return get_status(a1, a2); break;
    case GetTupleData:
	return get_tuple_data(a1, a2); break;
    case MapMemPage:
	return map_mem_page(a1, a2); break;
    case ModifyConfiguration:
	return modify_configuration(a1, a2); break;
    case ModifyWindow:
	return modify_window(a1, a2); break;
    case OpenMemory:
	return open_memory(a1, a2);
    case ParseTuple:
	return parse_tuple(a1, a2, a3); break;
    case ReadMemory:
	return read_memory(a1, a2, a3); break;
    case RegisterClient:
	return register_client(a1, a2); break;
    case RegisterEraseQueue:
	return register_erase_queue(a1, a2); break;
    case RegisterMTD:
	return register_mtd(a1, a2); break;
    case ReleaseConfiguration:
	return release_configuration(a1, a2); break;
    case ReleaseIO:
	return release_io(a1, a2); break;
    case ReleaseIRQ:
	return release_irq(a1, a2); break;
    case ReleaseWindow:
	return release_window(a1); break;
    case RequestConfiguration:
	return request_configuration(a1, a2); break;
    case RequestIO:
	return request_io(a1, a2); break;
    case RequestIRQ:
	return cs_request_irq(a1, a2); break;
    case RequestWindow:
	return request_window(a1, a2); break;
    case ResetCard:
	return reset_card(a1, a2); break;
    case SetEventMask:
	return set_event_mask(a1, a2); break;
    case ValidateCIS:
	return validate_cis(a1, a2); break;
    case WriteMemory:
	return write_memory(a1, a2, a3); break;
    case BindDevice:
	return bind_device(a1); break;
    case BindMTD:
	return bind_mtd(a1); break;
    case ReportError:
	return report_error(a1, a2); break;
    case SuspendCard:
	return suspend_card(a1, a2); break;
    case ResumeCard:
	return resume_card(a1, a2); break;
    case EjectCard:
	return eject_card(a1, a2); break;
    case InsertCard:
	return insert_card(a1, a2); break;
    default:
	return CS_UNSUPPORTED_FUNCTION; break;
    }
    
} /* CardServices */

/*====================================================================*/

int init_module(void)
{
    printk(KERN_INFO "%s\n", release);
    printk(KERN_INFO "  %s\n", kernel);
    printk(KERN_INFO "  %s\n", options);
    DEBUG(0, "%s\n", version);
#ifdef CONFIG_APM
    apm_register_callback(&handle_apm_event);
#endif
    register_symtab(&cs_symtab);
    return 0;
}

void cleanup_module(void)
{
    printk(KERN_INFO "unloading PCMCIA Card Services\n");
#ifdef CONFIG_APM
    apm_unregister_callback(&handle_apm_event);
#endif
    release_resource_db();
}
