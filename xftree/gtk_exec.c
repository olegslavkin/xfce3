/*
 * gtk_exec.c
 *
 * Copyright (C) 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia C-2001 for xfce project
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <glib.h>
#include <dirent.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include "constant.h"
#include "my_intl.h"
#include "gtk_exec.h"
#include "gtk_dlg.h"
#include "xtree_cfg.h"
#include "xtree_cb.h"
#include "io.h"
#include "reg.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

typedef struct
{
  GtkWidget *top;
  GtkWidget *combo;
  GtkWidget *check;
  GtkWidget *reg;
  cfg *win;
  char *cmd;
  char *file;
  int result;
  int in_terminal;
}
dlg;

static dlg dl;

/*
 */
static void
on_cancel (GtkWidget * btn, gpointer * data)
{
  if ((int) ((long) data) == DLG_RC_CANCEL)
  {
    gtk_widget_destroy (dl.top);
  }
  dl.result = DLG_RC_CANCEL;
  gtk_main_quit ();
}

/*
 */
static void
on_ok (GtkWidget * ok, gpointer data)
{
  char *temp;
  static char *last_temp=NULL;

  temp = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (dl.combo)->entry));
  if (last_temp) free(last_temp);
  last_temp=(char *)malloc(strlen(temp)+1);
  if (!last_temp) on_cancel (ok, (gpointer) ((long) DLG_RC_CANCEL));
  strcpy(last_temp,temp);	  


  if (strlen (last_temp))
  {
    dl.cmd = g_strdup (last_temp);

    dl.in_terminal = GTK_TOGGLE_BUTTON (dl.check)->active;
    gtk_widget_destroy (dl.top);
    dl.result = (int) ((long) data);
    if (dl.reg&&(gtk_toggle_button_get_active((GtkToggleButton *)dl.reg))){
      char  *sfx;
      sfx = strrchr (dl.file, '.');
      if (!sfx) {
	      sfx = strrchr (dl.file, '/');
	      if (sfx) sfx++;
      }
      if (sfx) {
	     dl.win->reg = reg_add_suffix (dl.win->reg, sfx, last_temp, NULL);
	     reg_save (dl.win->reg);
      }
	    
    }
    gtk_main_quit ();
  }
  else
    on_cancel (ok, (gpointer) ((long) DLG_RC_CANCEL));
}


/*
 * create a modal dialog and handle it
 * dlg_open_with is deprecated */
gint dlg_open_with (char *xap, char *defval, char *file){
	return (xf_dlg_open_with (NULL,xap,defval,file));
}
gint xf_dlg_open_with (GtkWidget *ctree,char *xap, char *defval, char *file)
{
  GtkWidget *ok = NULL, *cancel = NULL, *label, *box, *check;
  GList *apps=NULL;
  char cmd[(PATH_MAX + NAME_MAX) * 3 + 6];
  char *title;
  char *path;

  path=valid_path((GtkCTree *)ctree,FALSE);

  if (file)
  {
    title = _("Open with ...");
  }
  else
  {
    title = _("Run program ...");
  }

  dl.reg=NULL;
  dl.result = 0;
  dl.win=gtk_object_get_user_data (GTK_OBJECT (ctree));
  dl.in_terminal = 0;
  dl.file=file;
  dl.top = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dl.top), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (dl.top), title);
  gtk_signal_connect (GTK_OBJECT (dl.top), "destroy", GTK_SIGNAL_FUNC (on_cancel), (gpointer) ((long) DLG_RC_DESTROY));
  gtk_window_set_modal (GTK_WINDOW (dl.top), TRUE);
  if (dl.win->top) gtk_window_set_transient_for (GTK_WINDOW (dl.top), GTK_WINDOW (dl.win->top)); 
  
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dl.top)->vbox), 5);

  ok = gtk_button_new_with_label (_("Ok"));
  cancel = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), ok, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), cancel, TRUE, FALSE, 0);

  box = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->vbox), box, TRUE, TRUE, 0);

  dl.combo = gtk_combo_new ();
  apps = reg_app_list (dl.win->reg);
  if (apps)
    gtk_combo_set_popdown_strings (GTK_COMBO (dl.combo), apps);
  gtk_editable_select_region (GTK_EDITABLE (GTK_COMBO (dl.combo)->entry), 0, -1);
  gtk_combo_disable_activate (GTK_COMBO (dl.combo));
  if (defval)
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (dl.combo)->entry), defval);
  gtk_box_pack_start (GTK_BOX (box), dl.combo, TRUE, TRUE, 0);

  if (file)
  {
    label = gtk_label_new (file);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  }
  /* check button */
  box = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->vbox), box, TRUE, TRUE, 0);

  dl.check = check = gtk_check_button_new_with_label (_("Open in terminal"));
  gtk_box_pack_start (GTK_BOX (box), check, FALSE, FALSE, 0);
 
  if (file) { 
   dl.reg = gtk_check_button_new_with_label (_("Remember application"));
   gtk_box_pack_start (GTK_BOX (box), dl.reg, FALSE, FALSE, 0);
  }

  gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_OK));
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (dl.combo)->entry), "activate", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_OK));
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (on_cancel), (gpointer) ((long) DLG_RC_CANCEL));
  gtk_widget_grab_default (ok);
  gtk_widget_grab_focus (GTK_COMBO (dl.combo)->entry);

  gtk_widget_show_all (dl.top);
  gtk_main ();
  /* */
  if (dl.result == DLG_RC_OK)
  {
    if (!dl.cmd)
    {
      /* this should never happen
       */
      g_print (_("Fatal error in %s at %d\n"), __FILE__, __LINE__);
      exit (1);
    }
    chdir(path);
    if (dl.in_terminal)
    {
      /* start in terminal window */
      if (file)
      {
	sprintf (cmd, "%s -e %s \"%s\" &", TERMINAL, dl.cmd, file);
      }
      else
      {
	sprintf (cmd, "%s -e %s &", TERMINAL, dl.cmd);
      }
    }
    else
    {
      if (file)
      {
	sprintf (cmd, "%s \"%s\" &", dl.cmd, file);
      }
      else
      {
	sprintf (cmd, "%s &",dl.cmd);
      }
    }
    g_free (dl.cmd);
    io_system (cmd);
  }
  return (dl.result);
}
