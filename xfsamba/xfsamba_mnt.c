/*  gui modules for xfsamba
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* for _( definition, it also includes config.h : */
#include "my_intl.h"
#include "constant.h"
/* for pixmap creation routines : */
#include "xfce-common.h"
#include "xpmext.h"

/* local xfsamba includes : */
#undef XFSAMBA_MAIN
#include "xfsamba.h"
#include "xfsamba_dnd.h"
#include "tubo.h"

/*********************************************************/
/**** mount stuff ***********/
static char *mnt_point=NULL;
static void *mnt_fork=NULL;
static int
mount_stderr (int n, void *data)
{
  char *line;
  if (n) return TRUE;/* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);
  xf_dlg_warning(smb_nav,line);
  return TRUE;
}

/* FIXME: each smbmount must keep track of it's mount point */
/* FIXME: clean up routine must smbunmount all mounted stuff */
static void
unmountForkOver (void)
{
/* do a check to verify if unmounted and warn otherwise. */
	/* cat /etc/mtab | grep $mount_point */
	/* if output, sent it with a warning */
   return;
}
static void
unmountFork (void){
   execlp ("smbumount", "smbumount",  NMBcommand, (char *) 0);
   fprintf(stderr,_("Cannot execute smbumount\n"));
}

static void
mount_xftree_ForkOver (void)
{
  /* mount point at NMBcommand */
  Tubo(unmountFork,unmountForkOver,TRUE,parse_stderr,mount_stderr);
  return;
}
static void
mount_xftree_Fork (void){
  execlp ("xftree", "xftree",    NMBcommand,(char *) 0);
  fprintf(stderr,_("Cannot execute xftree\n"));
}
static void
mountForkOver (void)
{
  /* mount point at NMBcommand */
  Tubo(mount_xftree_Fork,mount_xftree_ForkOver,TRUE,parse_stderr,parse_stderr); 
  return;
}

static void
mountFork (void)
{
  char *the_netbios;
  the_netbios = (char *) malloc (strlen ((char *) NMBnetbios) + strlen ((char *) NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);
  execlp ("smbmount", "smbmount", the_netbios, NMBcommand, "-o", NMBpassword, (char *) 0);
}

void
cb_mount (GtkWidget * item, GtkWidget * ctree){
  GList *s;
  smb_entry *en;
  char *mount_point,*argv[5];

	/* check if a share is selected */
  if ( g_list_length (GTK_CLIST (shares)->selection)==0){
	xf_dlg_error(smb_nav,_("Error"),_("No top level share selected"));
	return;
  }
  s = GTK_CLIST (shares)->selection;
  en = gtk_ctree_node_get_row_data ((GtkCTree *)shares, s->data);
  if (!(en->type & S_T_SHARE)){
	xf_dlg_error(smb_nav,_("Error"),_("No top level share selected"));
	return;
  }
	
	/* sane xftree. if no xftree, no mount */
	argv[0]="xftree";
	if (!sane(argv[0])){
		xf_dlg_error(smb_nav,_("Could not find"),argv[0]);
		return;
	}
	argv[0]="smbmount";
	if (!sane(argv[0])){
		xf_dlg_error(smb_nav,_("Could not find"),argv[0]);
		return;
	}
	argv[0]="smbmnt";
	if (!sane(argv[0])){
		xf_dlg_error(smb_nav,_("Could not find"),argv[0]);
		return;
	}
	/* query for mountpoint. default is to create a dir at /tmp/whatever */

	mount_point=(char *)xf_dlg_string(smb_nav,_("Mount point"),"/tmp/xfsamba");
	if (!mount_point) return;
	if (mnt_point) g_free(mnt_point);
	mnt_point=g_strdup(mount_point);
	if ((mkdir(mount_point, 0xFFFF)<0)&&(errno != EEXIST)){
		xf_dlg_error(smb_nav,strerror(errno),mount_point);
		return;
	}
	/* do the smb mount, if error return */
	
  while (mnt_fork){
  	while (gtk_events_pending()) gtk_main_iteration();
	usleep(500);
  }
  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, en->share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  sprintf (NMBpassword,"username=%s", thisN->password);
  sprintf(NMBcommand,"%s",mount_point);
  mnt_fork=Tubo(mountFork, mountForkOver, TRUE, parse_stderr,mount_stderr);
  return;
}


