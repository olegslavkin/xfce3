/* copywrite 2001 edscott wilson garcia under GNU/GPL 
* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* probably overkill with all these includes: */
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "xtree_gui.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_prop.h"
#include "gtk_dnd.h"
#include "xtree_cfg.h"
#include "xtree_dnd.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "top.h"
#include "reg.h"
#include "xfcolor.h"
#include "xfce-common.h"
#include "../xfsamba/tubo.h"


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* this file is for processing certain common messages.
 * override warning to begin with */

#define SAVE_GEOMETRY 		0x01
#define DOUBLE_CLICK_GOTO 	0x02

#define XFTREE_CONFIG_FILE "xftreerc"

int preferences,geometryX,geometryY;

#define BYTES "bytes"

char * override_txt(char *new_file,char *old_file)
{
  gboolean old_exists=FALSE;
  struct stat nst,ost;
  static char *message=NULL;
  char *ot=_("Override ?"),*otime=NULL,*ntime;
  char *with=_("with");
  int i,osize=0,nsize;
  
  if (message) {free (message);}
  if (lstat (new_file, &nst) == ERROR){
    fprintf(stderr,"this should never happen: override_txt()\n");
    return ot;
  }
  if (lstat (old_file, &ost) != ERROR){
    old_exists=TRUE;
    osize=1;
    i=ost.st_size;
    otime=(char *)malloc( strlen(ctime(&(ost.st_mtime))) + 1 );
    strcpy(otime,ctime(&(ost.st_mtime)) );
    while (i) {i = i/10; osize++;}
  }
  nsize=1;
  i=nst.st_size;
  while (i) {i = i/10; nsize++;}
  ntime=ctime(&(nst.st_mtime));
   
  i= 1 + strlen(ot) +1+ 
	  strlen(new_file) +1+ strlen(ntime) +1+ nsize +1+ strlen("bytes") + 1;
  if (old_exists) {
    i = i + 
	  + strlen(with) +1+
	  strlen(old_file) +1+ strlen(otime) +1+ osize +1+ strlen("bytes") + 1;
  }
  message=(char *)malloc(i*sizeof(char));
  if (!message) {return ot;}
  if (old_exists){
	sprintf(message,"%s\n%s %s %ld %s\n%s\n%s %s %ld %s\n",ot,
			new_file,ntime,nst.st_size,BYTES,
			with,
			old_file,otime,ost.st_size,BYTES);
	free(otime);
  }
  else
	sprintf(message,"%s\n%s %s %ld %s\n",ot,
			new_file,ntime,nst.st_size,BYTES);
  return message;
}
			 
GtkWidget *
shortcut_menu (GtkWidget * parent, char *txt, gpointer func, gpointer data)
{
  static GtkWidget *menuitem;
  int togglevalue;

    togglevalue =(int) ((long) data) ;
    menuitem = gtk_check_menu_item_new_with_label (txt);
    GTK_CHECK_MENU_ITEM (menuitem)->active = (togglevalue & preferences)?1:0;
 /*   printf("dbg:pref=%d,toggle=%d,%s result=%d\n",preferences,togglevalue,txt,togglevalue & preferences);*/
    gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
#ifdef __GTK_2_0
    /*  */
    gtk_menu_append (GTK_MENU_SHELL (parent), menuitem);
#else
    gtk_menu_append (GTK_MENU (parent), menuitem);
#endif
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (func), (gpointer) data);
  gtk_widget_show (menuitem);
  return menuitem;
}

void save_defaults (void)
{
  FILE *defaults;
  char *homedir;
  int len;

  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFTREE_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
    my_show_message (_("Default xftreerc file cannot be created\n"));
    return;
  }
  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"), XFTREE_CONFIG_FILE);
  defaults = fopen (homedir, "w");
  free (homedir);

  if (!defaults)
  {
    my_show_message (_("Default xftreerc file cannot be created\n"));
    return;
  }
  fprintf (defaults, "# file created by xftree, if removed xftree returns to defaults.\n");
  fprintf (defaults, "preferences : %d\n", preferences);
  if (preferences & SAVE_GEOMETRY) {
	  /*printf("dbg:x=%d,y=%d\n",geometryX,geometryY);*/
	  fprintf (defaults, "geometry : %d,%d\n",geometryX,geometryY);
  }
  fclose (defaults);
  return;
}

void read_defaults(void){
  FILE *defaults;
  char *homedir,*word;
  int len;

  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFTREE_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
    my_show_message (_("Default xftreerc file cannot be read\n"));
    return;
  }
  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"),XFTREE_CONFIG_FILE );
  defaults = fopen (homedir, "r");
  free (homedir);

  if (!defaults) return;

  homedir = (char *)malloc(256);
  while (!feof(defaults)){
	fgets(homedir,255,defaults);
	if (feof(defaults))break;
	if (strstr(homedir,"preferences :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");
		if (!word) break;
		preferences=atoi(word);
	}
	if (strstr(homedir,"geometry :")){
		strtok(homedir,":");
		word=strtok(NULL,",");if (!word) break;
		geometryX=atoi(word);
		word=strtok(NULL,"\n");if (!word) break;
		geometryY=atoi(word);
	}
  }
  free(homedir);
  fclose(defaults);  
  
}
			  
void
cb_toggle_preferences (GtkWidget * widget, gpointer data)
{
  int toggler;
  toggler = (long)(data);
  preferences ^= toggler;
  save_defaults ();
}
			  
	  
  
  
  
     	   

	
