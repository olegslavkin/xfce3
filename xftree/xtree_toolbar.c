/* copywrite 2001 edscott wilson garcia under GNU/GPL 
 * 
 * Routines to override original ones in xtree_gui.c
 *
 * original create_toolbar (GtkWidget * top, GtkWidget * ctree, cfg * win)
 * by Rasca, Berlin and Olivier Fourdan, very much changed here.
 * 
 * xfce project: http://www.xfce.org
 * 
 * xtree_toolbar.c: toolbar routines for xftree
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
#include "xtree_cb.h"
#include "icons.h"

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

#include "xtree_mess.h"
#include "xtree_pasteboard.h"
/* only 32 elements allowed in TOOLBARICONS */
#define TOOLBARICONS \
  {_("New window"),	new_win_xpm,	cb_new_window},	\
  {_("Terminal"),	comp1_xpm,	cb_term},	\
  {_("Close window"),	closewin_xpm,	cb_destroy}, \
  {_("Run ..."), 	tb_macro_xpm,	cb_exec}, \
  {_("Reload ..."), 	reload_xpm,	cb_refresh}, \
  {_("Go back ..."), 	go_back_xpm,	cb_go_back}, \
  {_("Go to ..."), 	go_to_xpm,	cb_go_to}, \
  {_("Go up"), 		go_up_xpm,	cb_go_up}, \
  {_("Go home"),	home_xpm,	cb_go_home}, \
  {_("Copy ..."), 	tb_copy_xpm,	cb_copy}, \
  {_("Paste ..."), 	tb_paste_xpm,	cb_paste}, \
  {_("Delete ..."),	delete_xpm,	cb_delete}, \
  {_("Find ..."), 	tb_find_xpm,	cb_find}, \
  {_("Differences ..."),tb_vsplit_xpm,	cb_diff}, \
  {_("New Folder"),	new_dir_xpm,	cb_new_subdir}, \
  {_("New file ..."),	new_file_xpm,	cb_new_file}, \
  {_("Properties"),	appinfo_xpm,	cb_props}, \
  {_("Open Trash"),	trash_xpm,	cb_open_trash}, \
  {_("Empty Trash"),	empty_trash_xpm,cb_empty_trash}, \
  {_("Toggle Dotfiles"),dotfile_xpm,	on_dotfiles}, \
  {NULL,NULL,NULL} 
  

typedef struct boton_icono {
	char *text;
	char **icon;
	gpointer function;
} boton_icono;


GtkWidget *
create_toolbar (GtkWidget * top, GtkWidget * ctree, cfg * win)
{
  int i;
  unsigned int mask;
  GtkWidget *toolbar;
  boton_icono toolbarIcon[]={TOOLBARICONS};

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_space_style ((GtkToolbar *) toolbar, GTK_TOOLBAR_SPACE_LINE);
  gtk_toolbar_set_button_relief ((GtkToolbar *) toolbar, GTK_RELIEF_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 2);

  for (i=0;(toolbarIcon[i].text != NULL)&&(i<32);i++) {
     if (preferences & LARGE_TOOLBAR) mask=stateTB[1]; else mask=stateTB[0];
     if (mask & (0x01<<i)) {
       gtk_toolbar_append_item ((GtkToolbar *) toolbar,
	toolbarIcon[i].text,toolbarIcon[i].text,toolbarIcon[i].text,
	MyCreateFromPixmapData (toolbar, toolbarIcon[i].icon), 
	GTK_SIGNAL_FUNC (toolbarIcon[i].function), 
	(gpointer) ctree);
     }
  }
  return toolbar;
}


static unsigned int initial_preferences,initial_mask[2];
static GtkWidget *config_toolbar_dialog=NULL;
//static  GSList *r_group=NULL;

static void destroy_config_toolbar(GtkWidget * widget,gpointer data){
  gtk_widget_destroy (widget);
}

static void cancel_config_toolbar(GtkWidget * widget,GtkWidget *ctree){
      int i;
  destroy_config_toolbar(config_toolbar_dialog,NULL);
  for (i=0;i<2;i++) stateTB[i]=initial_mask[i];
  save_defaults(NULL);
  return;
}

static void ok_Sconfig_toolbar(GtkWidget * widget,GtkWidget *ctree){
  destroy_config_toolbar(config_toolbar_dialog,NULL);
  preferences &= (LARGE_TOOLBAR ^ 0xffffffff);
  save_defaults(NULL);
  if (
    ((stateTB[0]!=initial_mask[0])&&(!(preferences&LARGE_TOOLBAR))) 
    ||( (preferences|HIDE_TOOLBAR) != (initial_preferences|HIDE_TOOLBAR))
     ) {
     redraw_top (ctree);
     /*fprintf(stderr,"dbg:regen toolbar..\n");*/
  }
}
static void ok_Lconfig_toolbar(GtkWidget * widget,GtkWidget *ctree){
  destroy_config_toolbar(config_toolbar_dialog,NULL);
  preferences |= LARGE_TOOLBAR;
  save_defaults(NULL);
  if (
    ((stateTB[1]!=initial_mask[1])&&(preferences&LARGE_TOOLBAR)) 
    ||( (preferences|HIDE_TOOLBAR) != (initial_preferences|HIDE_TOOLBAR))
     ) {
     redraw_top (ctree);
     /*fprintf(stderr,"dbg:regen toolbar..\n");*/
  }
}


static void toggle_toolbars(unsigned int mask,int which){
	/*fprintf(stderr,"dbg:toggling toolbar %d\n",which);*/
	stateTB[which] ^= mask;
	save_defaults (NULL);
	return;
}

static void toggle_toolbars0(GtkWidget * widget,gpointer data){
	unsigned int mask;
	mask=(unsigned int)((long)data);
	toggle_toolbars(mask,0);
	return;
}

static void toggle_toolbars1(GtkWidget * widget,gpointer data){
	unsigned int mask;
	mask=(unsigned int)((long)data);
	toggle_toolbars(mask,1);
	return;
}

static void toggle_toolbar(GtkWidget * widget, GtkWidget *ctree){
	cfg *win;
  	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
			
	preferences ^= HIDE_TOOLBAR;
	if (preferences & HIDE_TOOLBAR) {
		if (GTK_WIDGET_VISIBLE(win->toolbar)) gtk_widget_hide(win->toolbar);
	}
	else if (!GTK_WIDGET_VISIBLE(win->toolbar)) gtk_widget_show(win->toolbar);
}

static GtkWidget *toolbar_config(GtkWidget *ctree){
  GtkWidget *scrolledwindow,*table,*viewport,*widget,*hbox;
  int i;
  boton_icono toolbarIcon[]={TOOLBARICONS};
  char *labels[]={
	  _("Action"),
	  _("Small icons. "),
	  _("Large icons. ")
  };
  char *button_label[]={
	  _("Apply large icons"),
	  _("Apply small icons"),
	  _("Cancel")
  };
  gpointer button_function[]={
	ok_Lconfig_toolbar,
	ok_Sconfig_toolbar,
	cancel_config_toolbar
  };

  
  config_toolbar_dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (config_toolbar_dialog), _("Configure toolbar"));
  gtk_signal_connect (GTK_OBJECT (config_toolbar_dialog), "destroy", 
		  GTK_SIGNAL_FUNC (destroy_config_toolbar), NULL);
  gtk_window_position (GTK_WINDOW (config_toolbar_dialog), GTK_WIN_POS_CENTER);
  gtk_widget_realize (config_toolbar_dialog);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), 
		  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_ref (scrolledwindow);
  gtk_object_set_data_full (GTK_OBJECT (config_toolbar_dialog), "scrolledwindowT", scrolledwindow,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config_toolbar_dialog)->vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);
 // gtk_container_add (GTK_CONTAINER (config_toolbar_dialog), scrolledwindow);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport);
  gtk_object_set_data_full (GTK_OBJECT (config_toolbar_dialog), "viewportT", viewport,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport);

  table = gtk_table_new (4, 4, FALSE);
  gtk_widget_ref (table);
  gtk_object_set_data_full (GTK_OBJECT (config_toolbar_dialog), "table", table,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (viewport), table);

  /* table titles */
  for (i=0;i<3;i++) {
    widget=gtk_label_new(labels[i]);gtk_widget_show (widget);
    gtk_table_attach (GTK_TABLE (table), widget, i+1,i+2,0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  }
  
  /* table entries */
  for (i=0;toolbarIcon[i].text != NULL;i++) {
     int j;
     gpointer toolbar_func[]={
	     toggle_toolbars0,
	     toggle_toolbars1
     };
     widget = MyCreateFromPixmapData (config_toolbar_dialog, toolbarIcon[i].icon); 
     gtk_widget_show (widget);
     gtk_table_attach (GTK_TABLE (table), 
		     widget, 
		     0,1,i+1, i+2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
     
     widget=gtk_label_new( toolbarIcon[i].text);
     gtk_widget_show (widget);
     gtk_table_attach (GTK_TABLE (table), 
		     widget, 
		     1,2, i+1, i+2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
     for (j=0;j<2;j++) {
       widget=gtk_check_button_new ();
       if (stateTB[j] & (1<<i)) gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
       gtk_widget_show (widget);
       gtk_signal_connect (GTK_OBJECT (widget), "toggled", GTK_SIGNAL_FUNC (toolbar_func[j]), 
		     (gpointer)((long) (1 << i)));
       gtk_table_attach (GTK_TABLE (table), 
		     widget, 
		     j+2,j+3, i+1, i+2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
     }
     
   }

  /* hide show toolbar */
  hbox=gtk_hbox_new(TRUE,5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config_toolbar_dialog)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  if (preferences & HIDE_TOOLBAR) gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
  
  widget = gtk_check_button_new_with_label (_("Hide toolbar"));
  gtk_box_pack_end (GTK_BOX (hbox),widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  /* toggle button before connecting */
  if (preferences & HIDE_TOOLBAR) gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
  gtk_signal_connect (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (toggle_toolbar),
		   (gpointer) ctree);
  
  
  /* buttons */
  for (i=0;i<3;i++) { 
    widget = gtk_button_new_with_label (button_label[i]);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(config_toolbar_dialog)->action_area),widget, FALSE, FALSE, 0);
    gtk_widget_show (widget);
    gtk_signal_connect (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (button_function[i]),
		  (gpointer) ctree);
  }
  
  gtk_widget_set_usize(config_toolbar_dialog,-1,333);
  gtk_window_set_modal (GTK_WINDOW (config_toolbar_dialog), TRUE);
  gtk_widget_show (config_toolbar_dialog);
  return config_toolbar_dialog;

}

void cb_config_toolbar(GtkWidget *widget,GtkWidget *ctree){ 
  cfg *win;
  int i;
  //if (r_group) g_slist_free (r_group);  r_group=NULL;
  initial_preferences=preferences;
  for (i=0;i<2;i++) initial_mask[i]=stateTB[i];
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (win && win->top) 
    gtk_window_set_transient_for (GTK_WINDOW (toolbar_config(ctree)), GTK_WINDOW (win->top)); 	
}

  
  
     	   

	
