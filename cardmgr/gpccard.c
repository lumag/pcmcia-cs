/*======================================================================

    GTK-based PCMCIA device monitor tool

    cardinfo.c 1.3 2004/02/28 16:58:59

    The contents of this file are subject to the Mozilla Public
    License Version 1.1 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
    are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.

    Alternatively, the contents of this file may be used under the
    terms of the GNU Public License version 2 (the "GPL"), in which
    case the provisions of the GPL are applicable instead of the
    above.  If you wish to allow the use of your version of this file
    only under the terms of the GPL and not to allow others to use
    your version of this file under the MPL, indicate your decision
    by deleting the provisions above and replace them with the notice
    and other provisions required by the GPL.  If you do not delete
    the provisions above, a recipient may use your version of this
    file under either the MPL or the GPL.
    
======================================================================*/

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <pcmcia/config.h>
#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

#include <gtk/gtk.h>

/*====================================================================*/

typedef struct socket_info_t {
    int fd, o_state;
    char name[10];
    char info[200];
    GtkWidget *label;
} socket_info_t;

#define MAX_SOCK 8

static int ns;
static socket_info_t st[MAX_SOCK];

static char *stabfile;

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
    int fd;
    char **p, fn[64];

    for (p = paths; *p; p++) {
	sprintf(fn, "%s/gc-%d", *p, getpid());
	if (mknod(fn, (S_IFCHR|S_IREAD), dev) == 0) {
	    fd = open(fn, O_RDONLY);
	    unlink(fn);
	    if (fd >= 0)
		return fd;
	    if (errno == ENODEV)
		break;
	}
    }
    return -1;
} /* open_dev */

/*====================================================================*/

#if 0
static int get_tuple(int n, cisdata_t code, ds_ioctl_arg_t *arg)
{
    socket_info_t *s = &st[n];
    
    arg->tuple.DesiredTuple = code;
    arg->tuple.Attributes = 0;
    if (ioctl(s->fd, DS_GET_FIRST_TUPLE, arg) != 0)
	return -1;
    arg->tuple.TupleOffset = 0;
    if (ioctl(s->fd, DS_GET_TUPLE_DATA, arg) != 0)
	return -1;
    if (ioctl(s->fd, DS_PARSE_TUPLE, arg) != 0)
	return -1;
    return 0;
}
#endif

/*====================================================================*/

static char *expand(char *dev)
{
    static char path[256];
    strcpy(path, "/dev/");
    strcat(path, dev);
    if (access(path, F_OK) == 0)
	return path;
#if 0
    f = fopen("/proc/partitions", "r");
    if (f) {
	while (fgets(s, sizeof(s)-1, f)) {
	    if 
	}
    }
    fclose(f);
#endif
    return dev;
}

static int do_update(void)
{
    FILE *f;
    int i, j;
    char s[80], *t, d[80];
    struct stat buf;
    static time_t last = 0, sz = 0;

    if ((stat(stabfile, &buf) != 0) ||
	((buf.st_mtime <= last) && (buf.st_size == sz)))
	return 0;

    f = fopen(stabfile, "r");
    if ((f == NULL) || (flock(fileno(f), LOCK_SH) != 0))
	return 0;

    last = buf.st_mtime;
    sz = buf.st_size;
    fgetc(f);
    for (i = 0; i < ns; i++) {
	if (!fgets(s, 80, f)) break;
	s[strlen(s)-1] = *d = '\0';
	strcpy(st[i].info, s+9);
	for (;;) {
	    int c = fgetc(f);
	    if ((c == EOF) || (c == 'S'))
		break;
	    fgets(s, 80, f);
	    for (t = s, j = 0; j < 4; j++)
		t = strchr(t, '\t')+1;
	    t[strcspn(t, "\t\n")] = '\0';
	    if (*d != '\0')
		strcat(d, ", ");
	    strcat(d, expand(t));
	}
	if (*d) {
	    strcat(st[i].info, "\n  device(s): ");
	    strcat(st[i].info, d);
	}
    }
    flock(fileno(f), LOCK_UN);
    fclose(f);

    for (i = 0; i < ns; i++) {
	config_info_t cfg;
	char r[80], *p;
	ioaddr_t stop;

	if (strncmp(st[i].info, "unsupported card", 16) == 0) {
	}

	*r = '\0';
	memset(&cfg, 0, sizeof(cfg));
	ioctl(st[i].fd, DS_GET_CONFIGURATION_INFO, &cfg);
	if (!(cfg.Attributes & CONF_VALID_CLIENT)) {
	    continue;
	}
	strcat(st[i].info, "\n  resources: ");
	if (cfg.AssignedIRQ != 0)
	    sprintf(r, "irq %d", cfg.AssignedIRQ);
	if (cfg.NumPorts1 > 0) {
	    if (*r) strcat(r, ", ");
	    p = r + strlen(r);
	    stop = cfg.BasePort1+cfg.NumPorts1;
	    if (cfg.NumPorts2 > 0) {
		if (stop == cfg.BasePort2)
		    sprintf(p, "io %#x-%#x", cfg.BasePort1,
			    stop+cfg.NumPorts2-1);
		else
		    sprintf(p, "io %#x-%#x, %#x-%#x", cfg.BasePort1, stop-1,
			    cfg.BasePort2, cfg.BasePort2+cfg.NumPorts2-1);
	    } else
		sprintf(p, "io %#x-%#x", cfg.BasePort1, stop-1);
	}
	strcat(st[i].info, r);
    }

    return 1;
}

/*====================================================================*/

void hide(GtkWidget *w, gpointer data)
{
    gtk_widget_hide((GtkWidget *)data);
}

void hide_key(GtkWidget *w, GdkEvent *e, gpointer data)
{
    gtk_widget_hide((GtkWidget *)data);
}

void quit (GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

gint do_dialog(gpointer data)
{
    int *count = data;
    static GtkWidget *dialog = NULL;
    GtkWidget *label, *frame, *vbox, *hbox, *button;
    int i;
    static GdkGeometry sz = { min_width: 300 };
    
    if (!do_update()) {
	(*count)--;
	return (*count > 0);
    }

    if (!dialog) {
	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog), "key_press_event",
			   (GtkSignalFunc)hide_key, dialog);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   (GtkSignalFunc)hide, dialog);
	gtk_window_set_position(GTK_WINDOW(dialog),
				GTK_WIN_POS_CENTER);
	gtk_window_set_geometry_hints(GTK_WINDOW(dialog), NULL,
				      &sz, GDK_HINT_MIN_SIZE);

	hbox = gtk_hbox_new(FALSE, 0);
	button = gtk_button_new_with_label("hide");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc)hide, dialog);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	button = gtk_button_new_with_label("quit");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc)quit, dialog);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   hbox, TRUE, TRUE, 0);
	gtk_widget_show_all(hbox);

	vbox = gtk_vbox_new(FALSE, 5);
	for (i = 0; i < ns; i++) {
	    frame = gtk_frame_new(st[i].name);
	    hbox = gtk_hbox_new(FALSE, 0);
	    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	    label = st[i].label = gtk_label_new(st[i].info);
	    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	    gtk_container_add(GTK_CONTAINER(frame), hbox);
	    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	}
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			   vbox, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);
    }
    for (i = 0; i < ns; i++) {
	gtk_label_set_text(GTK_LABEL(st[i].label), st[i].info);
    }
    if (!GTK_WIDGET_MAPPED(dialog))
	gtk_widget_show_all(dialog);
    else
	gdk_window_raise(dialog->window);
    gdk_keyboard_grab(dialog->window, FALSE, GDK_CURRENT_TIME);
    return 1;
}

void sock_event(gpointer data, gint fd, GdkInputCondition cond)
{
    int event;
    static int count, tag = 0;

    read(fd, &event, 4);
    if (tag)
	gtk_timeout_remove(tag);
    count = 40;
    tag = gtk_timeout_add(100, do_dialog, &count);
}

int main(int argc, char *argv[])
{
    int i, major;

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

#if 0
    if ((ret = fork()) > 0) exit(0);
    if (ret == -1)
	perror("forking");
    if (setsid() < 0)
	perror("detaching from tty");
#endif

    gtk_init(&argc, &argv);
    for (i = 0; i < ns; i++) {
	sprintf(st[i].name, "Socket %d", i);
	gdk_input_add(st[i].fd, GDK_INPUT_READ, sock_event, NULL);
    }
    gtk_main();

    return 0;
}
