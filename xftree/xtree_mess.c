/* copywrite 2001 edscott wilson garcia under GNU/GPL 
 * 
 * xtree_mess.c: messages and configuration routines for xftree
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
#include "../xfdiff/xfdiff_colorsel.h"
#include "xtree_go.h"


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

#define XTREE_MESS_MAIN
#include "xtree_mess.h"

#define BYTES "bytes"

void set_colors(GtkWidget * ctree){
	GtkStyle*  style;
	int red,green,blue;

	red = ctree_color.red & 0xffff;
	green = ctree_color.green & 0xffff;
	blue = ctree_color.blue & 0xffff;
	style=gtk_widget_get_style (ctree);
	
	style->base[GTK_STATE_ACTIVE].red=red;
	style->base[GTK_STATE_ACTIVE].green=green;
	style->base[GTK_STATE_ACTIVE].blue=blue;
	  
	style->base[GTK_STATE_NORMAL].red=red;
	style->base[GTK_STATE_NORMAL].green=green;
	style->base[GTK_STATE_NORMAL].blue=blue;
	  
	style->bg[GTK_STATE_NORMAL].red=red;
	style->bg[GTK_STATE_NORMAL].green=green;
	style->bg[GTK_STATE_NORMAL].blue=blue;
	  
	style->fg[GTK_STATE_SELECTED].red=red;
	style->fg[GTK_STATE_SELECTED].green=green;
	style->fg[GTK_STATE_SELECTED].blue=blue;
	  /* foregrounds */
	style->fg[GTK_STATE_NORMAL].red=(red^0xffff)&(0xffff);
	style->fg[GTK_STATE_NORMAL].green=(green^0xffff)&(0xffff);
	style->fg[GTK_STATE_NORMAL].blue=(blue^0xffff)&(0xffff);
	  
	style->bg[GTK_STATE_SELECTED].red=(red^0xffff)&(0xffff);
	style->bg[GTK_STATE_SELECTED].green=(green^0xffff)&(0xffff);
	style->bg[GTK_STATE_SELECTED].blue=(blue^0xffff)&(0xffff);
	gtk_widget_set_style (ctree,style);
	gtk_widget_ensure_style (ctree);
}

void
cb_select_colors (GtkWidget * widget, GtkWidget * ctree)
{
#if 0
  gdouble colors[4];
  gdouble *newcolor;
  char *geometry;
  cfg *win;
  gint wm_offsetX,wm_offsetY;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if ((geometry=(char *)malloc(64))==NULL) return;
  gdk_window_get_root_origin (((GtkWidget *) (win->top))->window, &wm_offsetX, &wm_offsetY);
  /*fprintf(stderr,"exiting: x=%d,y=%d\n",wm_offsetX, wm_offsetY);*/
  sprintf(geometry,"%dx%d+%d+%d",
		  win->top->allocation.width,
		  win->top->allocation.height,
		  wm_offsetX,
		  wm_offsetY);

  colors[0] = ((gdouble) ctree_color.red) / COLOR_GDK;
  colors[1] = ((gdouble) ctree_color.green) / COLOR_GDK;
  colors[2] = ((gdouble) ctree_color.blue) / COLOR_GDK;
  
  newcolor=xfdiff_colorselect (colors);
  if (newcolor){
      ctree_color.red   = ((guint) (newcolor[0] * COLOR_GDK));
      ctree_color.green = ((guint) (newcolor[1] * COLOR_GDK));
      ctree_color.blue  = ((guint) (newcolor[2] * COLOR_GDK));
      preferences |= CUSTOM_COLORS;
  } else preferences &= (CUSTOM_COLORS ^ 0xffffffff);
  save_defaults (NULL);
  execlp("xftree","xftree",((golist *)(win->gogo))->path,"-g",geometry,0);
  fprintf(stderr,"this shouldn't happen: cb_select_colors()\n");
#endif
  return;
}


void
cb_change_toolbar (GtkWidget * widget, GtkWidget * ctree)
{
#if 0
  cfg *win;
  gint wm_offsetX,wm_offsetY;
  char *geometry;
  
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if ((geometry=(char *)malloc(64))==NULL) return;
  gdk_window_get_root_origin (((GtkWidget *) (win->top))->window, &wm_offsetX, &wm_offsetY);
  /*fprintf(stderr,"exiting: x=%d,y=%d\n",wm_offsetX, wm_offsetY);*/
  sprintf(geometry,"%dx%d+%d+%d",
		  win->top->allocation.width,
		  win->top->allocation.height,
		  wm_offsetX,
		  wm_offsetY);

  preferences ^= LARGE_TOOLBAR;
  save_defaults (NULL);
  execlp("xftree","xftree",((golist *)(win->gogo))->path,"-g",geometry,0);
  fprintf(stderr,"this shouldn't happen: cb_select_colors()\n");
#endif
  return;
}




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

void save_defaults (GtkWidget *parent)
{
  FILE *defaults;
  char *homedir;
  int len;
  struct stat h_stat;

  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFTREE_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
failed:
    my_show_message (_("Default xftreerc file cannot be created\n"));
    /*if (parent) xf_dlg_error(parent,strerror(errno),_("Default xftreerc file cannot be created\n"));*/
    return;
  }
  /* if .xfce directory isnot there, create it. */
  snprintf (homedir, len, "%s/.xfce", (char *) getenv ("HOME"));
  if (stat(homedir,&h_stat) < 0){
	if (errno!=ENOENT) goto failed;
	if (mkdir(homedir,0770) < 0) goto failed;
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
  fprintf (defaults, "ctree_color : %d,%d,%d\n",ctree_color.red,ctree_color.green,ctree_color.blue);
  
  fclose (defaults);
  return;
}

void read_defaults(void){
  FILE *defaults;
  char *homedir,*word;
  int len;

  /* default custom colors: */
  ctree_color.red = ctree_color.green = ctree_color.blue = 10000;
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
	if (strstr(homedir,"ctree_color :")){
		strtok(homedir,":");
		word=strtok(NULL,",");if (!word) break;
		ctree_color.red=atoi(word);
		word=strtok(NULL,",");if (!word) break;
		ctree_color.green=atoi(word);
		word=strtok(NULL,"\n");if (!word) break;
		ctree_color.blue=atoi(word);
	}
  }
  free(homedir);
  fclose(defaults);  
  
}

/* from xtree_gui.c: */
void set_title (GtkWidget * w, const char *path);

void
cb_toggle_preferences (GtkWidget * widget, gpointer data)
{
  int toggler;
  toggler = (long)(data);
  preferences ^= toggler;
  save_defaults (NULL);
  if (toggler&SHORT_TITLES) {
	  if (Apath&&Awin) set_title(Awin,Apath);
  }
}
			  
void cb_custom_SCK(GtkWidget * item, GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  xf_dlg_info (win->top,N_("Xftree handles keyboard shortcuts dynamically.\n" 
"This means that you can open a menu,highlight an entry\nand press a keyboard key to create the shortcut.")
	    );
}

void cb_default_SCK(GtkWidget * item,  GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));	
  
  xf_dlg_info (win->top,_("Keyboard shortcuts are a fast way to access menu functions."));
	
}

static GtkWidget *cat=NULL,*text;
static void
on_clear (GtkWidget * widget, gpointer data)
{
  guint lg;
  lg = gtk_text_get_length (GTK_TEXT (text));
  gtk_text_backward_delete (GTK_TEXT (text), lg);
}

static void
on_ok (GtkWidget * widget, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (cat));
  gdk_window_withdraw ((GTK_WIDGET (cat))->window);
}

	  
void
show_cat (char *message)
{
  GtkWidget *bbox, *scrolled, *button;
  
  if ((!message) || (!strlen (message)))
    return;
  if (cat != NULL)
  {
    gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
    if (!GTK_WIDGET_VISIBLE (cat))
      gtk_widget_show (cat);
    return;
  }

  cat = gtk_dialog_new ();
  gtk_container_border_width (GTK_CONTAINER (cat), 5);

  gtk_window_position (GTK_WINDOW (cat), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (cat),_("Xftree results"));
  gtk_widget_realize (cat);


  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->vbox), scrolled, TRUE, TRUE, 0);
  gtk_widget_show (scrolled);

  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (text), TRUE);
  gtk_widget_show (text);
  gtk_container_add (GTK_CONTAINER (scrolled), text);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->action_area), bbox, FALSE, TRUE, 0);
  gtk_widget_show (bbox);
  button = gtk_button_new_with_label (_("Clear"));
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_clear), (gpointer) GTK_WIDGET (cat));

  button = gtk_button_new_with_label (_("Dismiss"));
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) GTK_WIDGET (cat));
  gtk_signal_connect (GTK_OBJECT (cat), "delete_event", GTK_SIGNAL_FUNC (on_ok), (gpointer) GTK_WIDGET (cat));

  gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
  gtk_widget_show (text);
/*  set_icon (cat, _("Show disk usage ..."), put_some_icon_here_xpm); FIXME*/
  gtk_widget_show (cat);

  return;

}

  
  
  
     	   

	
