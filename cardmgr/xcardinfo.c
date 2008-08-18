/* X PCMCIA device control program

   xcardinfo.c,v 1.2 2004/02/28 16:59:10 root Exp

   This software may be used and distributed according to the terms of
   GNU GENERAL PUBLIC LICENSE Version 2 WITHOUT ANY WARRATY.

   This code is based on cardinfo.c from David A. Hinds
   <dahinds@users.sourceforge.net>. Portions created by David A. Hinds
   are Copyright (C) 1999 David A. Hinds.

   Other portions written by TANAKA Katsunori <tkatsu@sky.zero.ad.jp>
   are Copyright (C) 2001 TANAKA Katsunori. */

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>

#undef Status
#include <pcmcia/config.h>
#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

/*====================================================================*/

typedef enum s_state {
  S_EMPTY, S_PRESENT, S_READY, S_BUSY, S_SUSPEND
} s_state;

typedef struct field_t {
  char str[80];
  Widget w;
} field_t;

typedef struct flag_t {
  int val;
  Widget w;
} flag_t;

typedef struct socket_info_t {
  int fd, o_state;
  char menu_name[16];
  Widget menu, menu_shell;
  field_t card, state, dev, io, irq;
  flag_t cd, vcc, vpp, wp;
} socket_info_t;

#define MAX_SOCK 8

static int ns;
static socket_info_t st[MAX_SOCK];

static Widget event_log, event_log_box;

static char *pidfile = "/var/run/cardmgr.pid";
static char *stabfile;

/*====================================================================*/

typedef struct event_tag_t {
  event_t event;
  char *name;
} event_tag_t;

static event_tag_t event_tag[] = {
  { CS_EVENT_CARD_INSERTION, "card insertion" },
  { CS_EVENT_CARD_REMOVAL, "card removal" },
  { CS_EVENT_RESET_PHYSICAL, "prepare for reset" },
  { CS_EVENT_CARD_RESET, "card reset successful" },
  { CS_EVENT_RESET_COMPLETE, "reset request complete" },
  { CS_EVENT_EJECTION_REQUEST, "user eject request" },
  { CS_EVENT_INSERTION_REQUEST, "user insert request" },
  { CS_EVENT_PM_SUSPEND, "suspend card" },
  { CS_EVENT_PM_RESUME, "resume card" },
  { CS_EVENT_REQUEST_ATTENTION, "request attention" },
};
#define NTAGS (sizeof(event_tag)/sizeof(event_tag_t))

/*====================================================================*/

static int lookup_dev(char *name)
{
  FILE *f;
  int n;
  char s[32], t[32];

  f = fopen("/proc/devices", "r");
  if (f == NULL)
    return -errno;
  while (fgets(s, 32, f) != NULL) {
    if (sscanf(s, "%d %s", &n, t) == 2)
      if (strcmp(name, t) == 0)
	break;
  }
  fclose(f);
  if (strcmp(name, t) == 0)
    return n;
  else
    return -ENODEV;
} /* lookup_dev */

/*====================================================================*/

static int open_dev(dev_t dev)
{
  static char *paths[] = {
    "/var/lib/pcmcia", "/var/run", "/dev", "/tmp", NULL
  };
  char **p, fn[64];
  int fd;

  for (p = paths; *p; p++) {
    sprintf(fn, "%s/ci-%d", *p, getpid());
    if (mknod(fn, (S_IFCHR|S_IREAD), dev) == 0) {
      fd = open(fn, O_RDONLY);
      unlink(fn);
      if (fd >= 0)
	return fd;
    }
  }
  return -1;
} /* open_dev */

/*====================================================================*/

static void do_popdown(Widget w, XtPointer client, XtPointer call) {
  XtPopdown((Widget)client);
}

static void do_alert(char *fmt, ...)
{
  char msg[132];
  va_list args;
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  {
    Widget shell, dialog;
    shell = XtVaCreatePopupShell("shell", transientShellWidgetClass,
				 event_log,
				 XtNtitle, "Alert",
				 NULL);
    dialog = XtVaCreateManagedWidget("dialog", dialogWidgetClass,
				     shell,
				     XtNlabel, msg,
				     NULL);
    XawDialogAddButton(dialog, "dismiss", do_popdown, shell);
    XtPopup(shell, XtGrabExclusive);
  }
  va_end(args);
} /* do_alert */

/*====================================================================*/

static void do_menu(Widget w, XtPointer client, XtPointer call)
{
  int ret = 0;

  int i, item;
  i = (int)client / 10;
  item = (int)client % 10;
  switch (item) {
  case 1:
    /* do_opts(); */ break;
  case 2:
    ret = ioctl(st[i].fd, DS_RESET_CARD); break;
  case 3:
    ret = ioctl(st[i].fd, DS_SUSPEND_CARD); break;
  case 4:
    ret = ioctl(st[i].fd, DS_RESUME_CARD); break;
  case 5:
    ret = ioctl(st[i].fd, DS_EJECT_CARD); break;
  case 6:
    ret = ioctl(st[i].fd, DS_INSERT_CARD); break;
  }
  if (ret != 0)
    do_alert("ioctl() operation failed: %s", strerror(errno));
} /* do_menu */

/*====================================================================*/

static void do_quit(Widget w, XtPointer client, XtPointer call)
{
  exit(0);
}

/*====================================================================*/

static void do_reset(Widget w, XtPointer client, XtPointer call)
{
  FILE *f;
  pid_t pid;

  f = fopen(pidfile, "r");
  if (f == NULL) {
    do_alert("Could not open pidfile: %s", strerror(errno));
    return;
  }
  if (fscanf(f, "%d", &pid) != 1) {
    do_alert("Could not read pidfile");
    return;
  }
  if (kill(pid, SIGHUP) != 0)
    do_alert("Could not signal cardmgr: %s", strerror(errno));
}

/*====================================================================*/

void update_field(field_t *field, char *new)
{
  if (strcmp(field->str, new) != 0) {
    strcpy(field->str, new);
    XtVaSetValues(field->w,
		  XtNlabel, new,
		  NULL);
  }
}

void update_flag(flag_t *flag, int new)
{
  if (flag->val != new) {
    flag->val = new;
    if (new)
      XtManageChild(flag->w);
    else
      XtUnmanageChild(flag->w);
  }
}

/*====================================================================*/

static void do_update(Widget w, XtPointer client, XtPointer call)
{
  FILE *f;
  int i, j, event, ret, state;
  cs_status_t status;
  config_info_t cfg;
  char s[80], *t, d[80], io[20], irq[4];
  ioaddr_t stop;
  struct stat buf;
  static time_t last = 0;
  time_t now;
  struct tm *tm;
  fd_set fds;
  struct timeval timeout;

  XtAppAddTimeOut(XtDisplayToApplicationContext(XtDisplay(w)), 300,
		  (XtTimerCallbackProc)do_update, w);

  /* Poll for events */
  FD_ZERO(&fds);
  for (i = 0; i < ns; i++)
    FD_SET(st[i].fd, &fds);
  timeout.tv_sec = timeout.tv_usec = 0;
  ret = select(MAX_SOCK+4, &fds, NULL, NULL, &timeout);
  now = time(NULL);
  tm = localtime(&now);
  if (ret > 0) {
    for (i = 0; i < ns; i++) {
      if (!FD_ISSET(st[i].fd, &fds))
	continue;
      ret = read(st[i].fd, &event, 4);
      if (ret != 4) continue;
      for (j = 0; j < NTAGS; j++)
	if (event_tag[j].event == event) break;
      if (j == NTAGS)
	sprintf(s, "%2d:%02d:%02d  socket %d: unknown event 0x%x",
		tm->tm_hour, tm->tm_min, tm->tm_sec, i, event);
      else
	sprintf(s, "%2d:%02d:%02d  socket %d: %s", tm->tm_hour,
		tm->tm_min, tm->tm_sec, i, event_tag[j].name);
      {
  	Widget log;
	log = XtVaCreateManagedWidget("log", labelWidgetClass,
				      event_log_box,
				      XtNlabel, s,
				      XtNborderWidth, 0,
				      XtNinternalHeight, 0,
				      NULL);
      }
    }
  }

  if ((stat(stabfile, &buf) == 0) && (buf.st_mtime >= last)) {
    f = fopen(stabfile, "r");
    if (f == NULL)
      return;

    if (flock(fileno(f), LOCK_SH) != 0) {
      do_alert("flock(stabfile) failed: %s", strerror(errno));
      return;
    }
    last = now;
    fgetc(f);
    for (i = 0; i < ns; i++) {
      if (!fgets(s, 80, f)) break;
      s[strlen(s)-1] = '\0';
      update_field(&st[i].card, s+9);
      *d = '\0';
      for (;;) {
	int c = fgetc(f);
	if ((c == EOF) || (c == 'S')) {
  	  update_field(&st[i].dev, d);
	  break;
	} else {
	  fgets(s, 80, f);
	  for (t = s, j = 0; j < 4; j++)
	    t = strchr(t, '\t')+1;
	  t[strcspn(t, "\t\n")] = '\0';
	  if (*d == '\0')
	    strcpy(d, t);
	  else {
	    strcat(d, ", ");
	    strcat(d, t);
	  }
	}
      }
    }
    flock(fileno(f), LOCK_UN);
    fclose(f);
  }

  for (i = 0; i < ns; i++) {

    state = S_EMPTY;
    status.Function = 0;
    ioctl(st[i].fd, DS_GET_STATUS, &status);
    if (strcmp(st[i].card.str, "empty") == 0) {
      if (status.CardState & CS_EVENT_CARD_DETECT)
	state = S_PRESENT;
    } else {
      if (status.CardState & CS_EVENT_PM_SUSPEND)
	state = S_SUSPEND;
      else {
	if (status.CardState & CS_EVENT_READY_CHANGE)
	  state = S_READY;
	else
	  state = S_BUSY;
      }
    }

    if (state != st[i].o_state) {
      char buf[16];
      char *entry[] = {"opts", "reset", "suspend", "resume",
		       "eject", "insert"};
      st[i].o_state = state;
      for (j = 1; j <= 6; j++) {
	sprintf(buf, "%s%d", entry[j - 1], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, False,
		      NULL);
      }
      switch (state) {
      case S_EMPTY:
  	update_field(&st[i].state, "");
	break;
      case S_PRESENT:
	sprintf(buf, "%s%d", entry[5], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
  	update_field(&st[i].state, "");
	break;
      case S_READY:
	sprintf(buf, "%s%d", entry[0], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[1], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[2], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[4], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
  	update_field(&st[i].state, "ready");
	break;
      case S_BUSY:
	sprintf(buf, "%s%d", entry[0], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[1], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[2], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[4], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
  	update_field(&st[i].state, "not ready");
	break;
      case S_SUSPEND:
	sprintf(buf, "%s%d", entry[0], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[3], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
	sprintf(buf, "%s%d", entry[4], i);
	XtVaSetValues(XtNameToWidget(st[i].menu_shell, buf),
		      XtNsensitive, True,
		      NULL);
  	update_field(&st[i].state, "suspended");
	break;
      }
    }

    strcpy(io, "");
    strcpy(irq, "");
    memset(&cfg, 0, sizeof(cfg));
    ret = ioctl(st[i].fd, DS_GET_CONFIGURATION_INFO, &cfg);
    if (cfg.Attributes & CONF_VALID_CLIENT) {
      if (cfg.AssignedIRQ != 0)
	sprintf(irq, "%d", cfg.AssignedIRQ);
      if (cfg.NumPorts1 > 0) {
	stop = cfg.BasePort1+cfg.NumPorts1;
	if (cfg.NumPorts2 > 0) {
	  if (stop == cfg.BasePort2)
	    sprintf(io, "%#x-%#x", cfg.BasePort1,
		    stop+cfg.NumPorts2-1);
	  else
	    sprintf(io, "%#x-%#x, %#x-%#x", cfg.BasePort1, stop-1,
		    cfg.BasePort2, cfg.BasePort2+cfg.NumPorts2-1);
	} else
	  sprintf(io, "%#x-%#x", cfg.BasePort1, stop-1);
      }
    }
    update_field(&st[i].irq, irq);
    update_field(&st[i].io, io);

    update_flag(&st[i].cd, status.CardState & CS_EVENT_CARD_DETECT);
    update_flag(&st[i].vcc, cfg.Vcc > 0);
    update_flag(&st[i].vpp, cfg.Vpp1 > 0);
    update_flag(&st[i].wp, status.CardState & CS_EVENT_WRITE_PROTECT);
  }
}

/*====================================================================*/

int main(int argc, char **argv)
{
  int i, ret, major;
  char name[12];
  XtAppContext app_context;
  Widget toplevel, form, w;

  if (geteuid() != 0) {
    fprintf(stderr, "cardinfo must be setuid root\n");
    exit(EXIT_FAILURE);
  }

  if (access("/var/lib/pcmcia", R_OK) == 0) {
    stabfile = "/var/lib/pcmcia/stab";
  } else {
    stabfile = "/var/run/stab";
  }

  major = lookup_dev("pcmcia");
  if (major < 0) {
    if (major == -ENODEV)
      fprintf(stderr, "no pcmcia driver in /proc/devices\n");
    else
      perror("could not open /proc/devices");
    exit(EXIT_FAILURE);
  }

  for (ns = 0; ns < MAX_SOCK; ns++) {
    st[ns].fd = open_dev(makedev(major, ns));
    if (st[ns].fd < 0) break;
  }
  if (ns == 0) {
    fprintf(stderr, "no sockets found\n");
    exit(EXIT_FAILURE);
  }

  /* Switch back to real user privileges, to be safe */
#ifndef UNSAFE_TOOLS
  setuid(getuid());
#endif

  if ((ret = fork()) > 0) exit(0);
  if (ret == -1)
    perror("forking");
  if (setsid() < 0)
    perror("detaching from tty");

  toplevel = XtVaOpenApplication(&app_context, "cardinfo", NULL, 0,
				 &argc, argv, NULL,
				 applicationShellWidgetClass, NULL);

  form = XtVaCreateManagedWidget("form", formWidgetClass,
				 toplevel,
				 NULL);

  for (i = 0; i < ns; ++i) {
    char buf[16];
    sprintf(name, "Socket %d", i);
    sprintf(buf, "menu%d", i);
    sprintf(st[i].menu_name, "menu_shell%d", i);
    st[i].menu = XtVaCreateManagedWidget(buf, menuButtonWidgetClass,
					 form,
					 XtNlabel, name,
					 XtNmenuName, st[i].menu_name,
					 NULL);
    if (i > 0) {
      XtVaSetValues(st[i].menu,
		    XtNfromVert, st[i - 1].io.w,
		    NULL);
    }
    sprintf(buf, "menu_shell%d", i);
    st[i].menu_shell = XtVaCreatePopupShell(buf, simpleMenuWidgetClass,
					    st[i].menu,
					    NULL);
    {
      int j;
      char *entry[] = {"opts", "reset", "suspend", "resume",
		       "eject", "insert"};
      for (j = 0; j < 6; ++j) {
	sprintf(buf, "%s%d", entry[j], i);
	w = XtVaCreateManagedWidget(buf, smeBSBObjectClass,
				    st[i].menu_shell,
				    XtNlabel, entry[j],
				    NULL);
	if (j == 0) {
	  sprintf(buf, "%s...", entry[j]);
	  XtVaSetValues(w,
			XtNlabel, buf,
			NULL);
	}
	XtAddCallback(w, XtNcallback, do_menu, (XtPointer)j + 1 + 10 * i);
      }
    }
    sprintf(buf, "card%d", i);
    st[i].card.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					   form,
					   XtNwidth, 300,
					   XtNfromHoriz, st[i].menu,
  					   NULL);
    if (i > 0) {
      XtVaSetValues(st[i].card.w,
		  XtNfromVert, st[i - 1].io.w,
		  NULL);
    }

    sprintf(buf, "state_label%d", i);
    w = XtVaCreateManagedWidget(buf, labelWidgetClass,
				form,
				XtNlabel, "state:",
  				XtNfromHoriz, st[i].menu,
				XtNfromVert, st[i].menu,
				XtNborderWidth, 0,
				NULL);
    sprintf(buf, "state%d", i);
    st[i].state.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					    form,
					    XtNlabel, "",
					    XtNwidth, 70,
  					    XtNfromHoriz, w,
					    XtNfromVert, st[i].menu,
					    NULL);
    sprintf(buf, "cd%d", i);
    st[i].cd.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					 form,
					 XtNlabel, "CD",
					 XtNfromHoriz, st[i].state.w,
					 XtNfromVert, st[i].menu,
					 NULL);
    XtUnmanageChild(st[i].cd.w);
    sprintf(buf, "vcc%d", i);
    st[i].vcc.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					  form,
					  XtNlabel, "Vcc",
					  XtNfromHoriz, st[i].cd.w,
					  XtNfromVert, st[i].menu,
					  NULL);
    XtUnmanageChild(st[i].vcc.w);
    sprintf(buf, "vpp%d", i);
    st[i].vpp.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					  form,
					  XtNlabel, "Vpp",
					  XtNfromHoriz, st[i].vcc.w,
					  XtNfromVert, st[i].menu,
					  NULL);
    XtUnmanageChild(st[i].vpp.w);
    sprintf(buf, "wp%d", i);
    st[i].wp.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					 form,
					 XtNlabel, "WP",
					 XtNfromHoriz, st[i].vpp.w,
					 XtNfromVert, st[i].menu,
					 NULL);

    XtUnmanageChild(st[i].wp.w);
    sprintf(buf, "dev_label%d", i);
    w = XtVaCreateManagedWidget(buf, labelWidgetClass,
				form,
				XtNlabel, "device(s):",
  				XtNfromHoriz, st[i].menu,
				XtNfromVert, st[i].state.w,
				XtNborderWidth, 0,
				NULL);
    sprintf(buf, "dev%d", i);
    st[i].dev.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					  form,
					  XtNlabel, "",
					  XtNwidth, 140,
					  XtNfromHoriz, w,
					  XtNfromVert, st[i].state.w,
					  NULL);

    sprintf(buf, "io_label%d", i);
    w = XtVaCreateManagedWidget(buf, labelWidgetClass,
				form,
				XtNlabel, "IO ports:",
  				XtNfromHoriz, st[i].menu,
				XtNfromVert, st[i].dev.w,
				XtNborderWidth, 0,
				NULL);
    sprintf(buf, "io%d", i);
    st[i].io.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					 form,
					 XtNlabel, "",
					 XtNwidth, 120,
					 XtNfromHoriz, w,
					 XtNfromVert, st[i].dev.w,
					 NULL);
    sprintf(buf, "irq_label%d", i);
    w = XtVaCreateManagedWidget(buf, labelWidgetClass,
				form,
				XtNlabel, "interrupt:",
  				XtNfromHoriz, st[i].io.w,
				XtNfromVert, st[i].dev.w,
				XtNborderWidth, 0,
				NULL);
    sprintf(buf, "irq%d", i);
    st[i].irq.w = XtVaCreateManagedWidget(buf, labelWidgetClass,
					  form,
					  XtNlabel, "",
					  XtNwidth, 20,
					  XtNfromHoriz, w,
					  XtNfromVert, st[i].dev.w,
					  NULL);
  }

  event_log = XtVaCreateManagedWidget("event_log", viewportWidgetClass,
				      form,
				      XtNfromVert, st[ns - 1].io.w,
				      XtNheight, 40,
				      XtNwidth, 320,
				      XtNallowVert, True,
				      XtNallowHoriz, True,
				      XtNuseRight, True,
				      XtNuseBottom, True,
				      NULL);
  
  event_log_box = XtVaCreateManagedWidget("event_log_box", boxWidgetClass,
					  event_log,
  					  XtNhSpace, 0,
   					  XtNvSpace, 0,
  					  NULL);
  
  w = XtVaCreateManagedWidget("reset", commandWidgetClass,
			      form,
			      XtNfromHoriz, event_log,
			      XtNfromVert, st[ns - 1].io.w,
			      NULL);
  XtAddCallback(w, XtNcallback, do_reset, NULL);
  w = XtVaCreateManagedWidget("quit", commandWidgetClass,
			      form,
			      XtNfromHoriz, event_log,
			      XtNfromVert, w,
			      NULL);
  XtAddCallback(w, XtNcallback, do_quit, NULL);

  XtAppAddTimeOut(XtDisplayToApplicationContext(XtDisplay(toplevel)), 200,
		  (XtTimerCallbackProc)do_update, toplevel);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app_context);

  exit(EXIT_SUCCESS);
  return 0;
}
