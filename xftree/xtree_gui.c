/*
 * xtree_gui.c
 *
 * Copyright (C) 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia 2001, for xfce project
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

#define __XFTREE_GUI_MAIN__

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
#include <gtk/gtkenums.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "xtree_gui.h"
#include "xtree_functions.h"
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
#include "xtree_mess.h"
#include "xtree_pasteboard.h"
#include "xtree_go.h"
#include "xtree_cb.h"
#include "xtree_toolbar.h"
#include "icons/page.xpm"
#include "icons/page_lnk.xpm"
#include "icons/dir_close.xpm"
#include "icons/dir_pd.xpm"
#include "icons/dir_close_lnk.xpm"
#include "icons/dir_open.xpm"
#include "icons/dir_open_lnk.xpm"
#include "icons/dir_up.xpm"
#include "icons/exe.xpm"
#include "icons/exe_lnk.xpm"
#include "icons/char_dev.xpm"
#include "icons/fifo.xpm"
#include "icons/socket.xpm"
#include "icons/block_dev.xpm"
#include "icons/stale_lnk.xpm"
#include "icons/xftree_icon.xpm"
#include "icons/tar.xpm"
#include "icons/compressed.xpm"
#include "icons/text.xpm"
#include "icons/image.xpm"
#include "icons/core.xpm"

#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif



static GtkTargetEntry target_table[] = {
  {"text/uri-list", 0, TARGET_URI_LIST},
  {"text/plain", 0, TARGET_PLAIN},
  {"STRING", 0, TARGET_STRING}
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

#define ACCEL	2
typedef struct
{
  char *label;
  void *func;
  int data;
  gint key;
  guint8 mod;
}
menu_entry;


static GtkAccelGroup *accel;
/*
 * start the marked program on double click
 */
static gint
on_double_click (GtkWidget * ctree, GdkEventButton * event, void *menu)
{
  GtkCTreeNode *node;
  entry *en, *up;
  char cmd[(PATH_MAX + 3) * 2];
  char *wd;
  cfg *win;
  reg_t *prg;
  gint row, col;
  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
  {
    /* double_click
     */

    /* check if the double click was over a directory
     */
    row = -1;
    gtk_clist_get_selection_info (GTK_CLIST (ctree), event->x, event->y, &row, &col);
    if (row > -1)
    {
      node = gtk_ctree_node_nth (GTK_CTREE (ctree), row);
      en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
      if (EN_IS_DIR (en) && ((event->state & (GDK_MOD1_MASK | GDK_CONTROL_MASK)) || (preferences & DOUBLE_CLICK_GOTO)))
      {
        /* Alt or Ctrl button is pressed, it's the same as _go_to().. */
	go_to (GTK_CTREE (ctree), GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list), en->path, en->flags);
	return (TRUE);
      }
    }
    if (!count_selection (GTK_CTREE (ctree), &node))
    {
      return (TRUE);
    }
    if (!node)
    {
      return (TRUE);
    }
    en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);

    if (en->type & FT_DIR_UP)
    {
      cb_go_up(NULL,GTK_CTREE (ctree)); 	    
   /*   node = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
      go_to (GTK_CTREE (ctree), node, uri_clear_path (en->path), en->flags);*/
      return (TRUE);
    }
    if (!(en->type & FT_FILE))
      return (FALSE);
    up = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), GTK_CTREE_ROW (node)->parent);
    cursor_wait (GTK_WIDGET (ctree));

    wd = getcwd (NULL, PATH_MAX);
    chdir (up->path);
    if (en->type & FT_EXE)
    {				/*io_can_exec (en->path)) */
      if (event->state & GDK_MOD1_MASK)
	sprintf (cmd, "%s -e \"%s\" &", TERMINAL, en->path);
      else
	sprintf (cmd, "\"%s\" &", en->path);
      io_system (cmd);
    }
    else
    {
      /* call open with dialog */
      win = gtk_object_get_user_data (GTK_OBJECT (ctree));
      prg = reg_prog_by_file (win->reg, en->path);
      if (prg)
      {
	if (prg->arg)
	  sprintf (cmd, "\"%s\" %s \"%s\" &", prg->app, prg->arg, en->path);
	else
	  sprintf (cmd, "\"%s\" \"%s\" &", prg->app, en->path);
	io_system (cmd);
      }
      else
      {
	xf_dlg_open_with (win->top,win->xap, "", en->path);
      }
    }
    chdir (wd);
    free (wd);
    cursor_reset (GTK_WIDGET (ctree));
    return (TRUE);
  }
  return (FALSE);
}

/*
 */
static gint
on_click_column (GtkCList * clist, gint column, gpointer data)
{
  int num;
  GtkCTreeNode *node;
  GList *selection = NULL;

  if (column != clist->sort_column)
    gtk_clist_set_sort_column (clist, column);
  else
  {
    if (clist->sort_type == GTK_SORT_ASCENDING)
      clist->sort_type = GTK_SORT_DESCENDING;
    else
      clist->sort_type = GTK_SORT_ASCENDING;
  }
  num = count_selection (GTK_CTREE (clist), &node);
  if (num)
  {
    for (selection = g_list_copy (GTK_CLIST (clist)->selection); selection; selection = selection->next)
    {
      node = selection->data;
      if (!GTK_CTREE_ROW (node)->children || (!GTK_CTREE_ROW (node)->expanded))
      {
	/* select parent node */
	node = GTK_CTREE_ROW (node)->parent;
      }
      gtk_ctree_sort_node (GTK_CTREE (clist), node);
    }
  }
  else
  {
    gtk_clist_sort (clist);
  }
  g_list_free (selection);
  return TRUE;
}

/*
 * popup context menu
 */
static gint
on_button_press (GtkWidget * widget, GdkEventButton * event, void *data)
{
  GtkCTree *ctree = GTK_CTREE (widget);
  GtkWidget **menu = (GtkWidget **) data;
  GtkCTreeNode *node;
  int num, row, column = MN_NONE;

  if (event->button == 3)
  {
    num = selection_type (ctree, &node);
    if (!num)
    {
      row = -1;
      gtk_clist_get_selection_info (GTK_CLIST (widget), event->x, event->y, &row, &column);
      if (row > -1)
      {
	gtk_clist_select_row (GTK_CLIST (ctree), row, 0);
	if (GTK_CLIST (ctree)->selection)
	  num = selection_type (ctree, &node);
      }
    }
    gtk_menu_popup (GTK_MENU (menu[num]), NULL, NULL, NULL, NULL, 3, event->time);
    return TRUE;
  }
  return FALSE;
}

/*
 * handle keyboard short cuts
 */
static gint
on_key_press (GtkWidget * ctree, GdkEventKey * event, void *menu)
{
  int num, i;
  entry *en;
  GtkCTreeNode *node;
  GdkEventButton evbtn;

  switch (event->keyval)
  {
  case GDK_Delete:
    cb_delete (NULL, GTK_CTREE (ctree));
    return (TRUE);
    break;
  case GDK_Return:
    num = g_list_length (GTK_CLIST (ctree)->row_list);
    for (i = 0; i < num; i++)
    {
      if (GTK_CLIST (ctree)->focus_row == i)
      {
	node = gtk_ctree_node_nth (GTK_CTREE (ctree), i);
	en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
	if (EN_IS_DIR (en) && !(en->type & FT_DIR_UP))
	{
	  if (event->state & (GDK_MOD1_MASK | GDK_CONTROL_MASK))
	  {
	    /* Alt or Ctrl button is pressed, it's the same as _go_to().. */
	    go_to (GTK_CTREE (ctree), GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list), en->path, en->flags);
	    return (TRUE);
	    break;
	  }
	  if (!GTK_CTREE_ROW (node)->expanded)
	    gtk_ctree_expand (GTK_CTREE (ctree), node);
	  else
	    gtk_ctree_collapse (GTK_CTREE (ctree), node);
	  return (TRUE);
	  break;
	}
      }
    }

    evbtn.type = GDK_2BUTTON_PRESS;
    evbtn.button = 1;
    evbtn.state = event->state;
    on_double_click (ctree, &evbtn, menu);
    return (TRUE);
    break;
  default:
    if ((event->keyval >= GDK_A) && (event->keyval <= GDK_z) && (event->state <= GDK_SHIFT_MASK))
    {
      num = g_list_length (GTK_CLIST (ctree)->row_list);
      for (i = 0; i < num; i++)
      {
	node = gtk_ctree_node_nth (GTK_CTREE (ctree), i);
	en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
	if (en->label && (*en->label == (char) event->keyval) && gtk_ctree_node_is_visible (GTK_CTREE (ctree), node))
	{
	  GTK_CLIST (ctree)->focus_row = i;
	  gtk_ctree_unselect_recursive (GTK_CTREE (ctree), NULL);
	  gtk_clist_moveto (GTK_CLIST (ctree), i, COL_NAME, 0, 0);
	  gtk_clist_select_row (GTK_CLIST (ctree), i, COL_NAME);
	  break;
	}
      }
      return (TRUE);
    }
    break;
  }
  return (FALSE);
}


/*
 * my own sort function
 * honor if an entry is a directory or a file
 */
static gint
my_compare (GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
  GtkCTreeRow *row1 = (GtkCTreeRow *) ptr1;
  GtkCTreeRow *row2 = (GtkCTreeRow *) ptr2;
  entry *en1, *en2;
  char *loc1=NULL,*loc2=NULL;
  int type1, type2;

  en1 = row1->row.data;
  en2 = row2->row.data;
  type1 = en1->type & (FT_DIR | FT_FILE);
  type2 = en2->type & (FT_DIR | FT_FILE);
  if (type1 != type2)
  {
    /* i want to have the directories at the top  */
    return (type1 < type2 ? -1 : 1);
  }

  
/* subsort by filetype */
  if (preferences&SUBSORT_BY_FILETYPE) {
	  loc1=strrchr(en1->label,'.');
	  loc2=strrchr(en2->label,'.');
  }
 
  if (clist->sort_column != COL_NAME)
  {
    /* use default compare function which we have saved before  */
    GtkCListCompareFunc compare;
    cfg *win;
    win = gtk_object_get_user_data (GTK_OBJECT (clist));
    compare = (GtkCListCompareFunc) win->compare;
    if (preferences&SUBSORT_BY_FILETYPE) {
	  if ((!loc1)&&(!loc2)) return compare (clist, ptr1, ptr2);
	  if ((!loc1)&&(loc2)) return strcmp (".",loc2);
	  if ((loc1)&&(!loc2)) return strcmp (loc1,".");
	  if (strcmp(loc1,loc2)) return strcmp(loc1,loc2);
    }
    return compare (clist, ptr1, ptr2);
  }
  
  if (preferences&SUBSORT_BY_FILETYPE) {
    if ((!loc1)&&(!loc2)) return strcmp (en1->label, en2->label);
    if ((!loc1)&&(loc2)) return strcmp (".",loc2);
    if ((loc1)&&(!loc2)) return strcmp (loc1,".");
    if (strcmp(loc1,loc2)) return strcmp(loc1,loc2);
  }
  return strcmp (en1->label, en2->label);
}


/* FIXME: this routine should me a loop to reduce excessive lines of code 
 * (make for easier maintaince) */
GtkWidget *
create_menu (GtkWidget * top, GtkWidget * ctree, cfg * win,GtkWidget *hlpmenu)
{
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;

  menubar = gtk_menu_bar_new ();
  gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menubar), GTK_SHADOW_NONE);
  gtk_widget_show (menubar);

  /* Create "File" menu */
  menuitem = gtk_menu_item_new_with_label (_("File"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_menu_item_new_with_label (_("New window"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_new_window), (gpointer) ctree);

  menuitem = gtk_menu_item_new_with_label (_("Open in terminal"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_term), (gpointer) ctree);

  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("New Folder ..."));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_new_subdir), (gpointer) ctree);

  menuitem = gtk_menu_item_new_with_label (_("New file ..."));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_new_file), (gpointer) ctree);

  menuitem = gtk_menu_item_new_with_label (_("Delete ..."));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_delete), (gpointer) ctree);

  
  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Close window"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_destroy), (gpointer) ctree);

  menuitem = gtk_menu_item_new_with_label (_("Quit"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_quit), (gpointer) ctree);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_q,GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  /* Create "Edit" menu */
  menuitem = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_menu_item_new_with_label (_("Copy to pasteboard"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_copy), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_c,GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  menuitem = gtk_menu_item_new_with_label (_("Insert from pasteboard"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_paste), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_i,GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  menuitem = gtk_menu_item_new_with_label (_("List pasteboard contents"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_paste_show), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_l,GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  menuitem = gtk_menu_item_new_with_label (_("Clear pasteboard"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_clean_pasteboard), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_k,GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);


  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Select all"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_select), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Unselect"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_unselect), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Toggle Dotfiles"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (on_dotfiles), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Open Trash"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_open_trash), (gpointer) ctree);

  menuitem = gtk_menu_item_new_with_label (_("Empty Trash"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_empty_trash), (gpointer) ctree);


  
  /* Create "Tools" menu */
  menuitem = gtk_menu_item_new_with_label (_("Tools"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_menu_item_new_with_label (_("Run program ..."));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_exec), (gpointer) ctree);

  menuitem = gtk_menu_item_new_with_label (_("Find ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_find), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Properties ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_props), (gpointer) ctree);

  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Differences ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_diff), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Patch viewer ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_diff), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  /* Create "Go To" menu */
  menuitem = gtk_menu_item_new_with_label (_("Go"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_menu_item_new_with_label (_("Go home"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_go_home), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Go back"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_go_back), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Go up"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_go_up), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Go to ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_go_to), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Update ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_reload), (gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

/*  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);*/

  /* Create "Preferences" menu */
  menuitem = gtk_menu_item_new_with_label (_("Preferences"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_check_menu_item_new_with_label (_("Subsort by file type"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (SUBSORT_BY_FILETYPE & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_subsort), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_t, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  shortcut_menu (menu, _("Save geometry on exit"), (gpointer) cb_toggle_preferences, 
		  (gpointer)((long)(SAVE_GEOMETRY)) );
  shortcut_menu (menu, _("Double click does GOTO"), (gpointer) cb_toggle_preferences, 
		  (gpointer)((long)(DOUBLE_CLICK_GOTO)) );
  shortcut_menu (menu, _("Short titles"), (gpointer) cb_toggle_preferences, 
		  (gpointer)((long)(SHORT_TITLES)) );
  shortcut_menu (menu, _("Drag does copy"), (gpointer) cb_toggle_preferences, 
		  (gpointer)((long)(DRAG_DOES_COPY)) );
  
  menuitem = gtk_menu_item_new_with_label (_("Set background color"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_select_colors), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_menu_item_new_with_label (_("Set font"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_select_font), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Hide menu"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (HIDE_MENU & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_hide_menu), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_m,GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  /* hidden element (already in configuration dialog) */
  menuitem = gtk_check_menu_item_new_with_label (_("Hide toolbar"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (HIDE_MENU & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_toolbar), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_t,GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  menuitem = gtk_check_menu_item_new_with_label (_("Hide titles"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (HIDE_MENU & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_hide_titles), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_h,GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);


  menuitem = gtk_check_menu_item_new_with_label (_("Hide dates"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (HIDE_DATE & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_hide_date), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_d,GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);


  menuitem = gtk_check_menu_item_new_with_label (_("Hide sizes"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (HIDE_SIZE & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_hide_size), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel);
  gtk_widget_add_accelerator (menuitem, "activate", accel, GDK_s,GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);


  menuitem = gtk_menu_item_new_with_label (_("Configure toolbar"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_config_toolbar), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Create "Help" menu */
  menuitem = gtk_menu_item_new_with_label (_("Help"));
  gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  
  menuitem = gtk_menu_item_new_with_label (_("Drag and drop"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_dnd_help), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);


  menuitem = gtk_menu_item_new_with_label (_("Default shortcut keys"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),hlpmenu);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Custom shortcut keys"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_custom_SCK), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("About ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_about), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);


  return menubar;
}

/* pixmap list */
typedef struct pixmap_list {
GdkPixmap  **pixmap;
GdkBitmap  **pixmask;
char **xpm;
} pixmap_list;

typedef struct gen_pixmap_list {
GdkPixmap  **pixmap;
GdkPixmap  **pixmask;
char **xpm;
char *c;
} gen_pixmap_list;

/* masks that are duplicated elsewhere are initialized to NULL */
pixmap_list pixmaps[]={
	{&gPIX_page,		&gPIM_page,		page_xpm},
	{&gPIX_core,		NULL,			core_xpm},
	{&gPIX_text,		NULL,			text_xpm},
	{&gPIX_compressed,	NULL,			compressed_xpm},
	{&gPIX_image,		NULL,			image_xpm},
	{&gPIX_tar,		NULL,			tar_xpm},
	{&gPIX_page_lnk,	NULL,			page_lnk_xpm},
	{&gPIX_dir_pd,		NULL,			dir_pd_xpm},
	{&gPIX_dir_open,	&gPIM_dir_open,		dir_open_xpm},
	{&gPIX_dir_open_lnk,	NULL,			dir_open_lnk_xpm},
	{&gPIX_dir_close,	&gPIM_dir_close,	dir_close_xpm},
	{&gPIX_dir_close_lnk,	NULL,			dir_close_lnk_xpm},
	{&gPIX_dir_up,		NULL,			dir_up_xpm},
	{&gPIX_exe,		&gPIM_exe,		exe_xpm},
	{&gPIX_exe_lnk,		NULL,			exe_lnk_xpm},
	{&gPIX_char_dev,	&gPIM_char_dev,		char_dev_xpm},
	{&gPIX_block_dev,	&gPIM_block_dev,	block_dev_xpm},
	{&gPIX_fifo,		&gPIM_fifo,		fifo_xpm},
	{&gPIX_socket,		&gPIM_socket,		socket_xpm},
	{&gPIX_stale_lnk,	&gPIM_stale_lnk,	stale_lnk_xpm},
	{NULL,NULL,NULL}
};

gen_pixmap_list gen_pixmaps[]={
	{&gPIX_pageC,	NULL,	page_xpm,	"c"},
	{&gPIX_pageH,	NULL,	page_xpm,	"h"},
	{&gPIX_pageF,	NULL,	page_xpm,	"f"},
	{NULL,NULL,NULL}
};

static void scale_pixmap(GtkWidget *hack,int h,GtkWidget *ctree,char **xpm,
		GdkPixmap **pixmap, GdkBitmap **pixmask){
#ifdef HAVE_GDK_PIXBUF
	if (h>0){
		GdkPixbuf *orig_pixbuf,*new_pixbuf;
		float r=0;
		int w,x,y;
	  	orig_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)(xpm));
		w=gdk_pixbuf_get_width (orig_pixbuf);
		if (w) r=(float)h/w; if (r<1.0) r=1.0;
		x=r*w;
		y=r*gdk_pixbuf_get_height (orig_pixbuf);

  		new_pixbuf  = gdk_pixbuf_scale_simple (orig_pixbuf,x,y,GDK_INTERP_NEAREST);
  		gdk_pixbuf_render_pixmap_and_mask (new_pixbuf,pixmap,pixmask,
				gdk_pixbuf_get_has_alpha (new_pixbuf));
  		gdk_pixbuf_unref (orig_pixbuf);
  		gdk_pixbuf_unref (new_pixbuf);

	} else
#endif	
	      *pixmap = MyCreateGdkPixmapFromData(xpm,hack,pixmask,FALSE);
	return ;

}

void create_pixmaps(int h,GtkWidget *ctree){
  GtkStyle  *style;
  static GdkColormap *colormap=NULL;
  static GdkGC* gc=NULL; 
  int i;
  static GtkWidget *hack=NULL; 
  /* hack: to be able to use icons globally, independent of xftree window.*/
  if (!hack) {hack = gtk_window_new (GTK_WINDOW_POPUP); gtk_widget_realize (hack);}
  
#ifndef HAVE_GDK_PIXBUF
  else return; /* don't recreate pixmaps without gdk-pixbuf  */
#endif
  
  if (!gc) {
	GdkColor fore,back;
	
       /*if (!gdk_color_black (colormap,&fore)) fprintf(stderr,"DBG: no black\n");*/
   	fore.red=fore.green=0;
	fore.blue=65535;
	if (!colormap) {
		colormap = gdk_colormap_get_system();
		gdk_colormap_alloc_color (colormap,&fore,FALSE,TRUE);
	}
        if (!gdk_color_white (colormap,&back)) fprintf(stderr,"DBG: no white\n");
	gc = gdk_gc_new (hack->window);
	gdk_gc_set_foreground (gc,&fore);
	gdk_gc_set_background (gc,&back);
  }
	
  for (i=0;pixmaps[i].pixmap != NULL; i++){ 
	  if (*(pixmaps[i].pixmap) != NULL) gdk_pixmap_unref(*(pixmaps[i].pixmap));
	  if ((pixmaps[i].pixmask)&&(*(pixmaps[i].pixmask) != NULL)) gdk_bitmap_unref(*(pixmaps[i].pixmask));
	  scale_pixmap(hack,h,ctree,pixmaps[i].xpm,pixmaps[i].pixmap,pixmaps[i].pixmask);
  }
 
  style=gtk_widget_get_style (ctree);
  for (i=0;gen_pixmaps[i].pixmap != NULL; i++){
	int x,y,ch;
	if (*(gen_pixmaps[i].pixmap) != NULL) gdk_pixmap_unref(*(gen_pixmaps[i].pixmap));
	scale_pixmap(hack,h,ctree,gen_pixmaps[i].xpm,gen_pixmaps[i].pixmap,gen_pixmaps[i].pixmask);
 	ch=gdk_char_height (style->font,gen_pixmaps[i].c[0]);
        x=h/5;
        y=h/2+2*ch/3;
  	/*fprintf(stderr,"dbg: drawing...ch=%d, y=%d, pixH=%d\n",ch,y,h);*/
	/*gdk_draw_line ((GdkDrawable *)(*(pixmaps[i].pixmap)),gc,0,0,30,30);*/
	gdk_draw_text ((GdkDrawable *)(*(gen_pixmaps[i].pixmap)),style->font,gc,
				x,y,gen_pixmaps[i].c,1);
	  
  }
   
  return;
}

/*
 * create a new toplevel window
 */
cfg *
new_top (char *path, char *xap, char *trash, GList * reg, int width, int height, int flags)
{
  GtkWidget *vbox;
  GtkWidget *handlebox1;
  GtkWidget *handlebox2;
  GtkWidget *handlebox4;
  GtkWidget *menutop;
  GtkWidget *scrolled;
  GtkWidget *toolbar;
  GtkWidget *ctree;
  GtkWidget **menu;
  GtkWidget *menu_item;
  GtkCTreeNode *root;
  gchar *label[COLUMNS];
  gchar *titles[COLUMNS];
  char *icon_name;
  entry *en;
  cfg *win;
  GtkAccelGroup *inner_accel;
  int i;

/* keyboard shortcuts used to be bugged because of conflicting entries.
 * these macros should make it easier to avoid conflicting entries.
 * Please place any duplicate entries as a macro. (since all entries
 * duplicate at least with help menu, all should be here).
 *
 * All should appear in help menu, if not, something is wrong
 * (delete is the only exception).
 * 
 * note: "unselect all" was eliminated since the function call was identical
 * to plain unselect. 
 * */
#define COMMON_MENU_1 \
    {N_("Open in new"), (gpointer) cb_new_window, 0, GDK_w,GDK_CONTROL_MASK},\
    {N_("Open in terminal"), (gpointer) cb_term, 0, GDK_t,GDK_CONTROL_MASK},\
    {NULL, NULL, 0}

#define COMMON_HELP_0 \
    {N_("Copy to pasteboard"), (gpointer) cb_copy, 0, GDK_c,GDK_CONTROL_MASK},\
    {N_("Insert from pasteboard"), (gpointer) cb_paste, 0, GDK_i,GDK_CONTROL_MASK},\
    {N_("List pasteboard contents"), (gpointer) cb_paste_show, 0, GDK_l,GDK_CONTROL_MASK},\
    {N_("Clear pasteboard"), (gpointer) cb_clean_pasteboard, 0, GDK_k,GDK_CONTROL_MASK},\
    {NULL, NULL, 0}
      
#define COMMON_HELP_1 \
    {N_("Go to marked"), (gpointer) cb_go_to, 0, GDK_g,GDK_MOD1_MASK}    
   
#define COMMON_HELP_2 \
     {N_("Open with ..."), (gpointer) cb_open_with, 0, GDK_o,GDK_MOD1_MASK},\
     {N_("Register ..."), (gpointer) cb_register, 0, GDK_r,GDK_MOD1_MASK},\
     {N_("Duplicate"), (gpointer) cb_duplicate, 0, GDK_c,GDK_MOD1_MASK} 
     
#define COMMON_MENU_NEW \
    {N_("New Folder"), (gpointer) cb_new_subdir, 0, GDK_n,GDK_MOD1_MASK},\
    {N_("New file"), (gpointer) cb_new_file, 0,GDK_n,GDK_CONTROL_MASK}
    
#define COMMON_MENU_2 \
    {N_("Properties ..."), (gpointer) cb_props, 0, GDK_p,GDK_CONTROL_MASK},\
    {N_("Rename ..."), (gpointer) cb_rename, 0, GDK_r,GDK_CONTROL_MASK},\
    {N_("Delete ..."), (gpointer) cb_delete, 0},\
    {N_("Show disk usage ..."), (gpointer) cb_du, 0, GDK_v,GDK_CONTROL_MASK},\
    {NULL, NULL, 0}
    
#define COMMON_MENU_GOTO \
    {N_("Find ..."), (gpointer) cb_find, 0, GDK_f,GDK_CONTROL_MASK},\
    {N_("Go home"), (gpointer) cb_go_home, 0,GDK_h,GDK_CONTROL_MASK},\
    {N_("Go back"), (gpointer) cb_go_back, 0,GDK_b,GDK_CONTROL_MASK},\
    {N_("Go up"), (gpointer) cb_go_up, 0,GDK_a,GDK_CONTROL_MASK},\
    {N_("Go to"), (gpointer) cb_go_to, 0,GDK_g,GDK_CONTROL_MASK},\
    {N_("Update"), (gpointer) cb_reload, 0,GDK_u,GDK_MOD1_MASK},\
    {NULL, NULL, 0}
    
#define COMMON_MENU_SELECT \
    {N_("Select all"), (gpointer) cb_select, 0, GDK_s,GDK_CONTROL_MASK},\
    {N_("Unselect"), (gpointer) cb_unselect, 0,GDK_u,GDK_CONTROL_MASK},\
    {N_("Toggle Dotfiles"), (gpointer) on_dotfiles, 0, GDK_d,GDK_CONTROL_MASK},\
    {NULL, NULL, 0}
    
#define COMMON_MENU_LAST \
    {N_("Run program ..."), (gpointer) cb_exec, WINCFG,GDK_x,GDK_CONTROL_MASK},\
    {N_("Open Trash"), (gpointer) cb_open_trash, 0, GDK_o,GDK_CONTROL_MASK},\
    {N_("Empty Trash"), (gpointer) cb_empty_trash, 0, GDK_e,GDK_CONTROL_MASK},\
    {N_("Close window"), (gpointer) cb_destroy, 0, GDK_z,GDK_CONTROL_MASK}
/* quit only on main menu now, so that default geometry is saved correctly with cb_quit.*/
    
#define COMMON_HELP_3 \
    {NULL, NULL, 0}, \
    {N_("Subsort by file type"), NULL, 0, GDK_t, GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Sort by file name"), NULL, 0, GDK_n, GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Sort by file size"), NULL, 0, GDK_s,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Sort by file date"), NULL, 0, GDK_d,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {NULL, NULL, 0}, \
    {N_("Hide/show menu"), NULL, 0, GDK_m,GDK_MOD1_MASK}, \
    {N_("Hide/show toolbar"), NULL, 0, GDK_t,GDK_MOD1_MASK}, \
    {N_("Hide/show titles"), NULL, 0, GDK_h,GDK_MOD1_MASK}, \
    {N_("Hide/show dates"), NULL, 0, GDK_d,GDK_MOD1_MASK}, \
    {N_("Hide/show sizes"), NULL, 0, GDK_s,GDK_MOD1_MASK}, \
    {NULL, NULL, 0}, \
    {N_("Quit ..."), NULL, 0, GDK_q,GDK_CONTROL_MASK}

  menu_entry dir_mlist[] = {
    COMMON_MENU_1,
    COMMON_MENU_NEW,
    COMMON_MENU_2,
    COMMON_HELP_1,
    COMMON_MENU_GOTO,
    COMMON_MENU_SELECT,
    COMMON_MENU_LAST
  };
#define LAST_DIR_MENU_ENTRY (sizeof(dir_mlist)/sizeof(menu_entry))

  menu_entry file_mlist[] = {
     COMMON_MENU_1,
     COMMON_HELP_2,
     COMMON_MENU_2,
     COMMON_MENU_GOTO,
     COMMON_MENU_SELECT,
     COMMON_MENU_LAST
  };
#define LAST_FILE_MENU_ENTRY (sizeof(file_mlist)/sizeof(menu_entry))

  menu_entry mixed_mlist[] = {
    COMMON_MENU_1,
    COMMON_MENU_2,
    COMMON_MENU_GOTO,
    COMMON_MENU_SELECT,
    COMMON_MENU_LAST
  };
#define LAST_MIXED_MENU_ENTRY (sizeof(mixed_mlist)/sizeof(menu_entry))

  menu_entry none_mlist[] = {
    COMMON_MENU_1,
    COMMON_MENU_NEW,
    COMMON_MENU_GOTO,
    COMMON_MENU_SELECT,
    COMMON_MENU_LAST
  };
#define LAST_NONE_MENU_ENTRY (sizeof(none_mlist)/sizeof(menu_entry))
  menu_entry help_mlist[] = {
    COMMON_MENU_1,
    COMMON_HELP_0,
    COMMON_MENU_NEW,
    {NULL, NULL, 0},
    COMMON_HELP_2,
    COMMON_MENU_2,
    COMMON_MENU_GOTO,
    COMMON_HELP_1,
    COMMON_MENU_SELECT,
    COMMON_MENU_LAST,
    COMMON_HELP_3    
  };
#define LAST_HELP_MENU_ENTRY (sizeof(help_mlist)/sizeof(menu_entry))

  read_defaults();
  if (SAVE_GEOMETRY & preferences)
  {
	  width=geometryX;
	  height=geometryY;
  }
  
  /* Set up X error Handler */
  XSetErrorHandler ((XErrorHandler) ErrorHandler);

  win = g_malloc (sizeof (cfg));
  win->dnd_row = -1;
  win->dnd_has_drag = 0;
  win->gogo =NULL;
  menu = g_malloc (sizeof (GtkWidget) * MENUS);
  titles[COL_NAME] = _("Name");
  titles[COL_SIZE] = _("Size (bytes)");
  titles[COL_DATE] = _("Last changed");
  label[COL_NAME] = path;
  label[COL_SIZE] = "";
  label[COL_DATE] = "";
  win->top = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy ((GtkWindow *)win->top,FALSE,TRUE,FALSE);
  gtk_widget_realize (win->top);
    
  win->gogo = pushgo(path,win->gogo);
                                              
  top_register (win->top);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (win->top), vbox);
  gtk_widget_show (vbox);

  handlebox1 = gtk_handle_box_new ();
  gtk_container_border_width (GTK_CONTAINER (handlebox1), 2);
  gtk_box_pack_start (GTK_BOX (vbox), handlebox1, FALSE, FALSE, 0);
  gtk_widget_show (handlebox1);

  handlebox2 = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), handlebox2, FALSE, FALSE, 0);
  gtk_widget_show (handlebox2);
  
  handlebox4 = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), handlebox4, FALSE, FALSE, 0);
  gtk_widget_show (handlebox4);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  ctree = gtk_ctree_new_with_titles (COLUMNS, 0, titles);
  gtk_clist_set_auto_sort (GTK_CLIST (ctree), FALSE);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree-expand", GTK_SIGNAL_FUNC (tree_unselect),ctree);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree-collapse", GTK_SIGNAL_FUNC (tree_unselect),ctree);
  
  gtk_signal_connect (GTK_OBJECT (win->top), "destroy", GTK_SIGNAL_FUNC (on_destroy), (gpointer) ctree);
  gtk_signal_connect (GTK_OBJECT (win->top), "delete_event", GTK_SIGNAL_FUNC (on_delete), (gpointer) ctree);

  accel = gtk_accel_group_new ();
  gtk_accel_group_attach (accel, GTK_OBJECT (win->top));

  gtk_widget_add_accelerator (GTK_WIDGET (GTK_CLIST (ctree)->column[COL_NAME].button), "clicked", accel, GDK_n, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (GTK_WIDGET (GTK_CLIST (ctree)->column[COL_DATE].button), "clicked", accel, GDK_d, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (GTK_WIDGET (GTK_CLIST (ctree)->column[COL_SIZE].button), "clicked", accel, GDK_s, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  win->compare = (gpointer) GTK_CLIST (ctree)->compare;
  win->trash = g_strdup (trash);
  win->xap = g_strdup (xap);
  win->reg = reg;
  win->width = width;
  win->height = height;
  gtk_object_set_user_data (GTK_OBJECT (ctree), win);
  if (preferences & CUSTOM_COLORS) set_colors(ctree); 
  if (preferences & CUSTOM_FONT) create_pixmaps(set_fontT(ctree),ctree);
  else create_pixmaps(-1,ctree);
  gtk_clist_set_compare_func (GTK_CLIST (ctree), my_compare);
  gtk_clist_set_shadow_type (GTK_CLIST (ctree), GTK_SHADOW_IN);

  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), 0, TRUE);
  gtk_clist_set_column_resizeable (GTK_CLIST (ctree), 1, TRUE);
  gtk_clist_set_column_resizeable (GTK_CLIST (ctree), 2, TRUE);
  gtk_clist_set_column_width (GTK_CLIST (ctree), 2, 115);

  gtk_clist_set_selection_mode (GTK_CLIST (ctree), GTK_SELECTION_EXTENDED);
  gtk_ctree_set_line_style (GTK_CTREE (ctree), GTK_CTREE_LINES_NONE);
  gtk_clist_set_reorderable (GTK_CLIST (ctree), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled), ctree);

  
/* FIXME: these menus should be created in a loop, to reduce excessive lines of code */
  
 /**** help menu, non operation, for displaying default kyeboard shortcuts */
  menu[MN_HLP] = gtk_menu_new ();
  inner_accel = gtk_accel_group_new ();
  gtk_accel_group_attach (inner_accel, GTK_OBJECT (menu[MN_HLP]));


  for (i = 0; i < LAST_HELP_MENU_ENTRY; i++)
  {
    if (help_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (help_mlist[i].label);
    else
      menu_item = gtk_menu_item_new ();
    gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (cb_default_SCK),ctree);
    if (help_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", inner_accel, help_mlist[i].key, help_mlist[i].mod,GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_HLP]), menu_item);
    gtk_widget_show (menu_item);
  }


 /***** directory list ...***/ 
 
  menu[MN_DIR] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_DIR]), accel);


  for (i = 0; i < LAST_DIR_MENU_ENTRY; i++)
  {
    if (dir_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(dir_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (dir_mlist[i].func)
    {
      if (dir_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (dir_mlist[i].func), win);
      else if (dir_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (dir_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (dir_mlist[i].func), (gpointer) ctree);
    }
    if (dir_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, dir_mlist[i].key, dir_mlist[i].mod,GTK_ACCEL_VISIBLE );
    }
    gtk_menu_append (GTK_MENU (menu[MN_DIR]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_DIR]), ctree, (GtkMenuDetachFunc) menu_detach);

/**** file list ...  */  
  menu[MN_FILE] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_FILE]), accel);
  for (i = 0; i < LAST_FILE_MENU_ENTRY; i++)
  {
    if (file_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(file_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (file_mlist[i].func)
    {
      if (file_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (file_mlist[i].func), win);
      else if (file_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (file_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (file_mlist[i].func), (gpointer) ctree);
    }
    if (file_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, file_mlist[i].key, file_mlist[i].mod, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_FILE]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_FILE]), ctree, (GtkMenuDetachFunc) menu_detach);

  /*** mixed list... */
  menu[MN_MIXED] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_MIXED]), accel);
  for (i = 0; i < LAST_MIXED_MENU_ENTRY; i++)
  {
    if (mixed_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(mixed_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (mixed_mlist[i].func)
    {
      if (mixed_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (mixed_mlist[i].func), win);
      else if (mixed_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (mixed_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (mixed_mlist[i].func), (gpointer) ctree);
    }
    if (mixed_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, mixed_mlist[i].key, mixed_mlist[i].mod, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_MIXED]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_MIXED]), ctree, (GtkMenuDetachFunc) menu_detach);

  menu[MN_NONE] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_NONE]), accel);

  for (i = 0; i < LAST_NONE_MENU_ENTRY; i++)
  {
    if (none_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(none_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (none_mlist[i].func)
    {
      if (none_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (none_mlist[i].func), win);
      else if (none_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (none_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (none_mlist[i].func), (gpointer) ctree);
    }
    if (none_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, none_mlist[i].key, none_mlist[i].mod, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_NONE]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_NONE]), ctree, (GtkMenuDetachFunc) menu_detach);

/* first pixmap appearance */
  
  root = gtk_ctree_insert_node (GTK_CTREE (ctree), NULL, NULL, label, 8, gPIX_dir_close, gPIM_dir_close, gPIX_dir_open, gPIM_dir_open, FALSE, TRUE);
  en = entry_new_by_path_and_label (path, path);
  if (!en)
  {
    exit (1);
  }
  en->flags = flags;

  gtk_ctree_node_set_row_data_full (GTK_CTREE (ctree), root, en, node_destroy);
  add_subtree (GTK_CTREE (ctree), root, path, 2, flags);


  gtk_signal_connect (GTK_OBJECT (ctree), "tree_expand", GTK_SIGNAL_FUNC (on_expand), path);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree_collapse", GTK_SIGNAL_FUNC (on_collapse), path);
  gtk_signal_connect (GTK_OBJECT (ctree), "click_column", GTK_SIGNAL_FUNC (on_click_column), en);
  gtk_signal_connect_after (GTK_OBJECT (ctree), "button_press_event", GTK_SIGNAL_FUNC (on_double_click), root);
  gtk_signal_connect (GTK_OBJECT (ctree), "button_press_event", GTK_SIGNAL_FUNC (on_button_press), menu);
  gtk_signal_connect (GTK_OBJECT (ctree), "key_press_event", GTK_SIGNAL_FUNC (on_key_press), menu);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data), win);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_data_get", GTK_SIGNAL_FUNC (on_drag_data_get), win);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_motion", GTK_SIGNAL_FUNC (on_drag_motion), NULL);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_end", GTK_SIGNAL_FUNC (on_drag_end), win);

  set_title (win->top, en->path);
  if (win->width > 0 && win->height > 0)
  {
    gtk_window_set_default_size (GTK_WINDOW (win->top), width, height);
  }

  win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
  gtk_drag_source_set (ctree, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
  gtk_drag_dest_set (ctree, GTK_DEST_DEFAULT_DROP, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
	  
  menutop = create_menu (win->top, ctree, win, menu[MN_HLP]);
  gtk_container_add (GTK_CONTAINER (handlebox1), menutop);
  gtk_widget_show (menutop);
  win->menu = menutop;

  
  toolbar = create_toolbar (win->top, ctree, win,FALSE);
  gtk_container_add (GTK_CONTAINER (handlebox2), toolbar);
  gtk_widget_show (toolbar);
  win->toolbar=toolbar;
  
  toolbar = create_toolbar (win->top, ctree, win,TRUE);
  gtk_container_add (GTK_CONTAINER (handlebox4), toolbar);
  gtk_widget_show (toolbar);
  win->toolbarO=toolbar;

  gtk_widget_show_all (win->top);

  /* hide what must be hidden */
  if (preferences & HIDE_TOOLBAR) {
	  if (GTK_WIDGET_VISIBLE(win->toolbar->parent))
		  gtk_widget_hide(win->toolbar->parent);
	  if (GTK_WIDGET_VISIBLE(win->toolbarO->parent))
		  gtk_widget_hide(win->toolbarO->parent);
  }
  if (preferences & HIDE_MENU) {
	  if (GTK_WIDGET_VISIBLE(win->menu->parent))
		  gtk_widget_hide(win->menu->parent);
  }
  if (preferences & HIDE_TITLES) {
	gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_NAME].button));
	gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_DATE].button));
	gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_SIZE].button));
  }
  
  icon_name = strrchr (path, '/');
  if ((icon_name) && (!(*(++icon_name))))
    icon_name = NULL;

  set_icon (win->top, (icon_name ? icon_name : "/"), xftree_icon_xpm);
  if (preferences & HIDE_TOOLBAR) gtk_widget_hide(handlebox2);

  gtk_clist_set_column_visibility ((GtkCList *)ctree,2,!(preferences & HIDE_DATE));
  gtk_clist_set_column_visibility ((GtkCList *)ctree,1,!(preferences & HIDE_SIZE));
  
  if (preferences & LARGE_TOOLBAR) gtk_widget_hide((win->toolbar)->parent);
  else gtk_widget_hide((win->toolbarO)->parent);
  
  return (win);
}

/* create a new toplevel tree widget */

void
gui_main (char *path, char *xap_path, char *trash, char *reg_file, wgeo_t * geo, int flags)
{
  GList *reg;
  cfg *new_win;

  if (!io_is_directory (path))
  {
    fprintf(stderr,"%s: %s\n",path, strerror (errno));
    return;
  }
  reg = reg_build_list (reg_file);
  if (SAVE_GEOMETRY & preferences){
	  geo->width=geometryX;
	  geo->height=geometryY;
  }
  new_win = new_top (path, xap_path, trash, reg, geo->width, geo->height, flags);
  if (geo->x > -1 && geo->y > -1)
  {
    gint x,y;
    gtk_widget_set_uposition (new_win->top, geo->x, geo->y);
    gdk_window_get_root_origin ((new_win->top)->window, &x, &y);
    gtk_widget_set_uposition (new_win->top,geo->x+(geo->x-x), geo->y+(geo->y-y) );
    gdk_flush();
    /*fprintf(stderr,"root: x=%d,y=%d\n",x,y);*/
  }

  gtk_main ();
  save_defaults(NULL);
  exit (0);
}
