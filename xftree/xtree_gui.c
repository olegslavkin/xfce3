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

/*FIXME: initialize all pixmaps and pixmasks to NULL */

GdkPixmap  *gPIX_dir_close=NULL,  *gPIX_dir_open=NULL;
GdkBitmap *gPIM_dir_close=NULL, *gPIM_dir_open=NULL;

static GdkPixmap * gPIX_page=NULL, *gPIX_page_lnk=NULL, *gPIX_dir_pd=NULL, 
	*gPIX_dir_close_lnk=NULL, *gPIX_dir_open_lnk=NULL, *gPIX_dir_up=NULL, 
	*gPIX_char_dev=NULL, *gPIX_fifo=NULL, *gPIX_socket=NULL, 
	*gPIX_block_dev=NULL, *gPIX_exe=NULL, *gPIX_stale_lnk=NULL, *gPIX_exe_lnk=NULL;

static GdkBitmap * gPIM_page=NULL, *gPIM_page_lnk=NULL, *gPIM_dir_pd=NULL, 
	*gPIM_dir_close_lnk=NULL, *gPIM_dir_open_lnk=NULL, *gPIM_dir_up=NULL, 
	*gPIM_char_dev=NULL, *gPIM_fifo=NULL, *gPIM_socket=NULL, 
	*gPIM_block_dev=NULL, *gPIM_exe=NULL, *gPIM_stale_lnk=NULL, *gPIM_exe_lnk=NULL;


int move_dir (char *source, char *label, char *target, int trash);

static GtkAccelGroup *accel;

gint update_timer (GtkCTree * ctree);

/*
 */
#define FATAL 1
#define alloc_error_fatal() alloc_error(__FILE__,__LINE__,FATAL)
/*
static int errno_error(GtkWidget *parent,char *path){
	return xf_dlg_error (parent,strerror(errno),path);
}*/
static void
alloc_error (char *file, int num, int mode)
{
  fprintf (stderr, _("dbg:malloc error in %s at line %d\n"), file, num);
  /*xf_dlg_error (parent,strerror(errno),file);*/
  if (mode == FATAL) exit (1);
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
  int type1, type2;

  en1 = row1->row.data;
  en2 = row2->row.data;
  type1 = en1->type & (FT_DIR | FT_FILE);
  type2 = en2->type & (FT_DIR | FT_FILE);
  if (type1 != type2)
  {
    /* i want to have the directories at the top
     */
    return (type1 < type2 ? -1 : 1);
  }
  if (clist->sort_column != COL_NAME)
  {
    /* use default compare function which we have saved before
     */
    GtkCListCompareFunc compare;
    cfg *win;
    win = gtk_object_get_user_data (GTK_OBJECT (clist));
    compare = (GtkCListCompareFunc) win->compare;
    return compare (clist, ptr1, ptr2);
  }
  return strcmp (en1->label, en2->label);
}

/*
 * count the number of  selected nodes, if there are no nodes selected
 * in "first" the root node is returned
 */

int
count_selection (GtkCTree * ctree, GtkCTreeNode ** first)
{
  int num = 0;
  GList *list;

  *first = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);

  list = GTK_CLIST (ctree)->selection;
  num = g_list_length (list);
  if (num <= 0)
  {
    return (0);
  }
  *first = GTK_CTREE_NODE (GTK_CLIST (ctree)->selection->data);
  return (num);
}


/*
 * called if a node will be destroyed
 * so free all private data
 */
void
node_destroy (gpointer p)
{
  entry *en = (entry *) p;
  entry_free (en);
}


int
selection_type (GtkCTree * ctree, GtkCTreeNode ** first)
{
  int num = 0;
  GList *list;
  GtkCTreeNode *node;
  entry *en;

  list = GTK_CLIST (ctree)->selection;

  *first = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
  if (g_list_length (list) <= 0)
    return (0);

  while (list)
  {
    node = list->data;
    en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
    if ((en->type & FT_DIR) || (en->type & FT_DIR_UP) || (en->type & FT_DIR_PD))
      num |= MN_DIR;
    else
      num |= MN_FILE;
    list = list->next;
  }

  *first = GTK_CTREE_NODE (GTK_CLIST (ctree)->selection->data);
  return (num);
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
 * what should we do here?
 */
void
menu_detach ()
{
  printf (_("menu_detach()\n"));
}

void
tree_unselect  (GtkCTree *ctree,GList *node,gpointer user_data)
{
  gtk_ctree_unselect_recursive (ctree, NULL);
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

void
ctree_freeze (GtkCTree * ctree)
{
  cursor_wait (GTK_WIDGET (ctree));
  gtk_clist_freeze (GTK_CLIST (ctree));
}

void
ctree_thaw (GtkCTree * ctree)
{
  gtk_clist_thaw (GTK_CLIST (ctree));
  cursor_reset (GTK_WIDGET (ctree));
}

/*
 */
GtkCTreeNode *
add_node (GtkCTree * ctree, GtkCTreeNode * parent, GtkCTreeNode * sibling, char *label, char *path, int *type, int flags)
{
  entry *en;
  GtkCTreeNode *item;
  gchar *text[COLUMNS];
  gchar size[16] = { "" };
  gchar date[32] = { "" };

  if (!label || !path)
  {
    return NULL;
  }
  if (*type & FT_DUMMY)
  {
    en = entry_new ();
    en->label = g_strdup (label);
    en->path = g_strdup (path);
    en->type = FT_DIR | FT_DUMMY;
  }
  else
  {
    en = entry_new_by_path_and_label (path, label);
    if (!en)
    {
      return 0;
    }
    en->flags = flags;

    sprintf (date, "%02d-%02d-%02d  %02d:%02d", en->date.year, en->date.month, en->date.day, en->date.hour, en->date.min);
    if (en->size < 0)
    {
      sprintf (size, "?(ERR %d)", -en->size);
    }
    else
    {
      sprintf (size, "%10d", (int) en->size);
    }
  }
  text[COL_NAME] = en->label;
  text[COL_DATE] = date;
  text[COL_SIZE] = size;

  if (en->type & FT_EXE)
  {
    if (en->type & FT_LINK)
    {
      item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_exe_lnk, gPIM_exe_lnk, NULL, NULL, TRUE, FALSE);
    }
    else
    {
      item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_exe, gPIM_exe, NULL, NULL, TRUE, FALSE);
    }
  }
  else if (en->type & FT_FILE)
  {
    if (en->type & FT_LINK)
    {
      item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_page_lnk, gPIM_page_lnk, NULL, NULL, TRUE, FALSE);
    }
    else
    {
      item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_page, gPIM_page, NULL, NULL, TRUE, FALSE);
    }
  }
  else if (en->type & FT_DIR_UP)
  {
    item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_dir_up, gPIM_dir_up, NULL, NULL, TRUE, FALSE);
  }
  else if (en->type & FT_DIR_PD)
  {
    item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_dir_pd, gPIM_dir_pd, gPIX_dir_pd, gPIM_dir_pd, FALSE, FALSE);
  }
  else if (en->type & FT_DIR)
  {
    if (en->type & FT_LINK)
    {
      item = gtk_ctree_insert_node (ctree, parent, sibling, text, SPACING, gPIX_dir_close_lnk, gPIM_dir_close_lnk, gPIX_dir_open_lnk, gPIM_dir_open_lnk, FALSE, FALSE);
    }
    else
    {
      item = gtk_ctree_insert_node (ctree, parent, sibling, text, SPACING, gPIX_dir_close, gPIM_dir_close, gPIX_dir_open, gPIM_dir_open, FALSE, FALSE);
    }
  }
  else if (en->type & FT_CHAR_DEV)
  {
    item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_char_dev, gPIM_char_dev, NULL, NULL, TRUE, FALSE);
  }
  else if (en->type & FT_BLOCK_DEV)
  {
    item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_block_dev, gPIM_block_dev, NULL, NULL, TRUE, FALSE);
  }
  else if (en->type & FT_FIFO)
  {
    item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_fifo, gPIM_fifo, NULL, NULL, TRUE, FALSE);
  }
  else if (en->type & FT_SOCKET)
  {
    item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_socket, gPIM_socket, NULL, NULL, TRUE, FALSE);
  }
  else
  {
    item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, gPIX_stale_lnk, gPIM_stale_lnk, NULL, NULL, TRUE, FALSE);
  }
  if (item)
    gtk_ctree_node_set_row_data_full (ctree, item, en, node_destroy);
  *type = en->type;
  return (item);
}

void
update_node (GtkCTree * ctree, GtkCTreeNode * node, int type, char *label)
{
  if (!ctree || !node || !label)
  {
    return;
  }

  if (type & FT_EXE)
  {
    if (type & FT_LINK)
    {
      gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_exe_lnk, gPIM_exe_lnk, NULL, NULL, TRUE, FALSE);
    }
    else
    {
      gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_exe, gPIM_exe, NULL, NULL, TRUE, FALSE);
    }
  }
  else if (type & FT_FILE)
  {
    if (type & FT_LINK)
    {
      gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_page_lnk, gPIM_page_lnk, NULL, NULL, TRUE, FALSE);
    }
    else
    {
      gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_page, gPIM_page, NULL, NULL, TRUE, FALSE);
    }
  }
  else if (type & FT_DIR_UP)
  {
    gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_dir_up, gPIM_dir_up, NULL, NULL, TRUE, FALSE);
  }
  else if (type & FT_DIR_PD)
  {
    gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_dir_pd, gPIM_dir_pd, gPIX_dir_pd, gPIM_dir_pd, FALSE, FALSE);
  }
  else if (type & FT_DIR)
  {
    if (type & FT_LINK)
    {
      gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_dir_close_lnk, gPIM_dir_close_lnk, gPIX_dir_open_lnk, gPIM_dir_open_lnk, FALSE, FALSE);
    }
    else
    {
      gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_dir_close, gPIM_dir_close, gPIX_dir_open, gPIM_dir_open, FALSE, FALSE);
    }
  }
  else if (type & FT_CHAR_DEV)
  {
    gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_char_dev, gPIM_char_dev, NULL, NULL, TRUE, FALSE);
  }
  else if (type & FT_BLOCK_DEV)
  {
    gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_block_dev, gPIM_block_dev, NULL, NULL, TRUE, FALSE);
  }
  else if (type & FT_FIFO)
  {
    gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_fifo, gPIM_fifo, NULL, NULL, TRUE, FALSE);
  }
  else if (type & FT_SOCKET)
  {
    gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_socket, gPIM_socket, NULL, NULL, TRUE, FALSE);
  }
  else
  {
    gtk_ctree_set_node_info (ctree, node, label, SPACING, gPIX_stale_lnk, gPIM_stale_lnk, NULL, NULL, TRUE, FALSE);
  }
}

/*
 */
void
add_subtree (GtkCTree * ctree, GtkCTreeNode * root, char *path, int depth, int flags)
{
  DIR *dir;
  struct dirent *de;
  GtkCTreeNode *item = NULL, *first = NULL;
  char *base;
  gchar complete[PATH_MAX + NAME_MAX + 1];
  gchar label[NAME_MAX + 1];
  int add_slash = no, len, d_len;
  int type = 0;

  if (depth == 0)
    return;

  if (!path)
    return;
  len = strlen (path);
  if (!len)
    return;
  if (path[len - 1] != '/')
  {
    add_slash = yes;
    len++;
  }
  base = g_malloc (len + 1);
  if (!base)
    alloc_error_fatal ();
  strcpy (base, path);
  if (add_slash)
    strcat (base, "/");

  if (depth == 1)
  {
    /* create dummy entry */
    sprintf (complete, "%s.", base);
    type = FT_DUMMY;
    add_node (GTK_CTREE (ctree), root, NULL, ".", complete, &type, flags);
    g_free (base);
    return;
  }
  dir = opendir (path);
  if (!dir)
  {
    g_free (base);
    return;
  }
  while ((de = readdir (dir)) != NULL)
  {
    type = 0;
    item = NULL;
    d_len = strlen (de->d_name);
    if (io_is_dirup (de->d_name))
      type |= FT_DIR_UP | FT_DIR;
    else if ((*de->d_name == '.') && ((flags & IGNORE_HIDDEN) && (d_len >= 1)))
      continue;
    sprintf (complete, "%s%s", base, de->d_name);
    strcpy (label, de->d_name);
    if ((!io_is_current (de->d_name)))
      item = add_node (GTK_CTREE (ctree), root, first, label, complete, &type, flags);
    if ((type & FT_DIR) && (!(type & FT_DIR_UP)) && (!(type & FT_DIR_PD)) && (io_is_valid (de->d_name)) && item)
      add_subtree (ctree, item, complete, depth - 1, flags);
    else if (!first)
      first = item;
  }
  g_free (base);
  closedir (dir);
  gtk_ctree_sort_node (ctree, root);
}

/*
 */
void
on_dotfiles (GtkWidget * item, GtkCTree * ctree)
{
  GtkCTreeNode *node, *child;
  entry *en;

  gtk_clist_freeze (GTK_CLIST (ctree));

  /* use first selection if available
   */
  count_selection (ctree, &node);
  en = gtk_ctree_node_get_row_data (ctree, node);
  if (!(en->type & FT_DIR))
  {
    /* select parent node */
    node = GTK_CTREE_ROW (node)->parent;
    en = gtk_ctree_node_get_row_data (ctree, node);
  }
  /* Ignore toggle on parent directory */
  if (en->type & FT_DIR_UP)
  {
    gtk_clist_thaw (GTK_CLIST (ctree));
    return;
  }
  child = GTK_CTREE_ROW (node)->children;
  while (child)
  {
    gtk_ctree_remove_node (ctree, child);
    child = GTK_CTREE_ROW (node)->children;
  }
  if (en->flags & IGNORE_HIDDEN)
    en->flags &= ~IGNORE_HIDDEN;
  else
    en->flags |= IGNORE_HIDDEN;
  add_subtree (ctree, node, en->path, 2, en->flags);
  gtk_ctree_expand (ctree, node);
  gtk_clist_thaw (GTK_CLIST (ctree));
}


/*
 */
void
on_expand (GtkCTree * ctree, GtkCTreeNode * node, char *path)
{
  GtkCTreeNode *child;
  entry *en;

  ctree_freeze (ctree);
  child = GTK_CTREE_ROW (node)->children;
  while (child)
  {
    gtk_ctree_remove_node (ctree, child);
    child = GTK_CTREE_ROW (node)->children;
  }
  en = gtk_ctree_node_get_row_data (ctree, node);
  add_subtree (ctree, node, en->path, 2, en->flags);
  ctree_thaw (ctree);
}

/*
 * unmark all marked on collapsion
 */
void
on_collapse (GtkCTree * ctree, GtkCTreeNode * node, char *path)
{
  GtkCTreeNode *child;
  /* unselect all children */
  child = GTK_CTREE_NODE (GTK_CTREE_ROW (node)->children);
  while (child)
  {
    gtk_ctree_unselect (ctree, child);
    child = GTK_CTREE_ROW (child)->sibling;
  }
}


void
set_title (GtkWidget * w, const char *path)
{
  char *title;
  title = (char *)malloc((strlen("XFTree: ")+strlen(path)+1)*sizeof(char));
  if (!title) return; 
  if (preferences&SHORT_TITLES) {
	  char *word;
	  word = strrchr (path,'/');
	  if (word) sprintf(title,"%s",(word[1]==0)?word:word+1);
	  else  sprintf(title,"XFTree");	  
  }
  else sprintf (title, "XFTree: %s", path);
  gtk_window_set_title (GTK_WINDOW (gtk_widget_get_toplevel (w)), title);
  free(title);

  Awin=w;
  {
    char *tmp_path;
    tmp_path=(char *)malloc((strlen(path)+1)*sizeof(char));
    if (tmp_path) strcpy(tmp_path,path);
    if (Apath) free(Apath);
    Apath=tmp_path;
  }
}


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
 */
static int
node_has_child (GtkCTree * ctree, GtkCTreeNode * node, char *label)
{
  GtkCTreeNode *child;
  entry *en;

  child = GTK_CTREE_ROW (node)->children;
  while (child)
  {
    en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), child);
    if (strcmp (en->label, label) == 0)
    {
      return (1);
    }
    child = GTK_CTREE_ROW (child)->sibling;
  }
  return (0);
}

/*
 * update a node and its childs if visible
 * return 1 if some nodes have removed or added, else 0
 *
 */
int
update_tree (GtkCTree * ctree, GtkCTreeNode * node)
{
  GtkCTreeNode *child = NULL, *new_child = NULL, *next;
  entry *en, *child_en;
  struct dirent *de;
  DIR *dir;
  char compl[PATH_MAX + 1];
  char label[NAME_MAX + 1];
  int type, p_len, changed, tree_updated, root_changed;
  gchar size[16];
  gchar date[32];
  cfg *win;
  gboolean manage_timeout;

  if (!ctree)
    return 0;

  if (!node)
    return 0;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  manage_timeout = (win->timer != 0);

  if (manage_timeout)
  {
    gtk_timeout_remove (win->timer);
    win->timer = 0;
  }

  tree_updated = FALSE;
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
  if ((root_changed = entry_update (en)) == ERROR)
  {
    next = GTK_CTREE_ROW (node)->sibling;
    gtk_ctree_remove_node (ctree, node);
    if (!next)
    {
      if (manage_timeout)
      {
	win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
      }
      return TRUE;
    }
    node = next;
  }
  child = GTK_CTREE_ROW (node)->children;
  while (child)
  {
    child_en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), child);
    if ((changed = entry_update (child_en)) == ERROR)
    {
      gtk_ctree_remove_node (ctree, child);
      /* realign the list */
      child = GTK_CTREE_ROW (node)->children;
      tree_updated = TRUE;
      continue;
    }
    else if (changed == TRUE)
    {
      /* update the labels */
      sprintf (date, "%02d-%02d-%02d  %02d:%02d", child_en->date.year, child_en->date.month, child_en->date.day, child_en->date.hour, child_en->date.min);
      sprintf (size, "%10d", (int) child_en->size);
      gtk_ctree_node_set_text (ctree, child, COL_DATE, date);
      gtk_ctree_node_set_text (ctree, child, COL_SIZE, size);
    }
    if (entry_type_update (child_en) == TRUE)
    {
      update_node (ctree, child, child_en->type, child_en->label);
      tree_updated = TRUE;
    }
    if (!(GTK_CTREE_ROW (child)->children) && (io_is_valid (child_en->label)) && !(child_en->type & FT_DIR_UP) && !(child_en->type & FT_DIR_PD) && (child_en->type & FT_DIR))
      add_subtree (GTK_CTREE (ctree), child, child_en->path, 1, child_en->flags);
    child = GTK_CTREE_ROW (child)->sibling;
  }

  if ((root_changed || tree_updated) && (en->type & FT_DIR))
  {
    if (GTK_CTREE_ROW (node)->expanded)
    {
      /* may be there are new files */
      dir = opendir (en->path);
      if (!dir)
      {
	if (manage_timeout)
	{
	  win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
	}
	return TRUE;
      }
      p_len = strlen (en->path);
      while ((de = readdir (dir)) != NULL)
      {
	if (io_is_hidden (de->d_name) && (en->flags & IGNORE_HIDDEN))
	  continue;
	if (io_is_current (de->d_name))
	  continue;
	strcpy (label, de->d_name);
	if (!node_has_child (ctree, node, label) && !(io_is_current (label)))
	{
	  if (io_is_root (label))
	    sprintf (compl, "%s%s", en->path, label);
	  else
	    sprintf (compl, "%s/%s", en->path, label);
	  type = 0;
	  new_child = NULL;
	  if (!io_is_current (label) && label)
	    new_child = add_node (ctree, node, NULL, label, compl, &type, en->flags);
	  if ((type & FT_DIR) && (io_is_valid (label)) && !(type & FT_DIR_UP) && !(type & FT_DIR_PD) && new_child)
	    add_subtree (ctree, new_child, compl, 1, en->flags);
	  if (entry_type_update (en) == TRUE)
	    update_node (ctree, node, en->type, en->label);
	  entry_update (en);
	  tree_updated = TRUE;
	}
      }
      closedir (dir);
    }
    else if ((GTK_CTREE_ROW (node)->children) && (io_is_valid (en->label)) && !(en->type & FT_DIR_UP) && !(en->type & FT_DIR_PD))
    {
      add_subtree (GTK_CTREE (ctree), node, en->path, 1, en->flags);
      if (entry_type_update (en) == TRUE)
	update_node (ctree, node, en->type, en->label);
      entry_update (en);
    }
    if (tree_updated)
    {
      gtk_ctree_sort_node (GTK_CTREE (ctree), node);
    }
  }
  if (manage_timeout)
  {
    win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
  }
  return (tree_updated);
}


/*
 * if window manager send delete event
 */
gint on_delete (GtkWidget * w, GdkEvent * event, gpointer data)
{
  return (FALSE);
}


/*
 * check if a node is a directory and is visible and expanded
 * will be called for every node
 */
static void
get_visible_or_parent (GtkCTree * ctree, GtkCTreeNode * node, gpointer data)
{
  GtkCTreeNode *child;
  GList **list = (GList **) data;

  if (GTK_CTREE_ROW (node)->is_leaf)
    return;

  if (gtk_ctree_node_is_visible (ctree, node) && GTK_CTREE_ROW (node)->expanded)
  {
    /* we can't remove a node or something else here,
     * that would break the calling gtk_ctree_pre_recursive()
     * so we remember the node in a linked list
     */
    *list = g_list_append (*list, node);
    return;
  }

  /* check if at least one child is visible
   */
  child = GTK_CTREE_ROW (node)->children;
  while (child)
  {
    if (gtk_ctree_node_is_visible (GTK_CTREE (ctree), child))
    {
      *list = g_list_append (*list, node);
      return;
    }
    child = GTK_CTREE_ROW (child)->sibling;
  }
}

/*
 * timer function to update the view
 */
gint update_timer (GtkCTree * ctree)
{
  GList *list = NULL, *tmp;
  GtkCTreeNode *node;
  cfg *win;
  gboolean manage_timeout;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  manage_timeout = (win->timer != 0);

  if (manage_timeout)
  {
    gtk_timeout_remove (win->timer);
    win->timer = 0;
  }


  /* get a list of directories we have to check
   */
  gtk_ctree_post_recursive (ctree, GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list), get_visible_or_parent, &list);

  tmp = list;
  while (tmp)
  {
    node = tmp->data;
    if (update_tree (ctree, node) == TRUE)
    {
      break;
    }
    tmp = tmp->next;
  }
  g_list_free (list);
  if (manage_timeout)
  {
    win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
  }
  return (TRUE);
}

XErrorHandler ErrorHandler (Display * dpy, XErrorEvent * event)
{
  char buf[64];
  if ((event->error_code == BadWindow) || (event->error_code == BadDrawable) || (event->request_code == X_GetGeometry) || (event->request_code == X_ChangeProperty) || (event->request_code == X_SetInputFocus) || (event->request_code == X_ChangeWindowAttributes) || (event->request_code == X_GrabButton) || (event->request_code == X_ChangeWindowAttributes) || (event->request_code == X_InstallColormap))
    return (0);

  XGetErrorText (dpy, event->error_code, buf, 63);
  fprintf (stderr, "xftree: Fatal XLib internal error\n");
  fprintf (stderr, "%s\n", buf);
  fprintf (stderr, "Request %d, Error %d\n", event->request_code, event->error_code);
  exit (1);
  return (0);
}

/* FIXME: this routine should me a loop to reduce code size */
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


/*  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);*/

  /* Create "Preferences" menu */
  menuitem = gtk_menu_item_new_with_label (_("Preferences"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

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


pixmap_list pixmaps[]={
	{&gPIX_page,		&gPIM_page,		page_xpm},
	{&gPIX_page_lnk,	&gPIM_page_lnk,		page_lnk_xpm},
	{&gPIX_dir_pd,		&gPIM_dir_pd,		dir_pd_xpm},
	{&gPIX_dir_open,	&gPIM_dir_open,		dir_open_xpm},
	{&gPIX_dir_open_lnk,	&gPIM_dir_open_lnk,	dir_open_lnk_xpm},
	{&gPIX_dir_close,	&gPIM_dir_close,	dir_close_xpm},
	{&gPIX_dir_close_lnk,	&gPIM_dir_close_lnk,	dir_close_lnk_xpm},
	{&gPIX_dir_up,		&gPIM_dir_up,		dir_up_xpm},
	{&gPIX_exe,		&gPIM_exe,		exe_xpm},
	{&gPIX_exe_lnk,		&gPIM_exe_lnk,		exe_lnk_xpm},
	{&gPIX_char_dev,	&gPIM_char_dev,		char_dev_xpm},
	{&gPIX_block_dev,	&gPIM_block_dev,	block_dev_xpm},
	{&gPIX_fifo,		&gPIM_fifo,		fifo_xpm},
	{&gPIX_socket,		&gPIM_socket,		socket_xpm},
	{&gPIX_stale_lnk,	&gPIM_stale_lnk,	stale_lnk_xpm},
	{NULL,NULL,NULL}
};
void create_pixmaps(int h){
  int i;
  static GtkWidget *hack=NULL; 
  /* hack: to be able to use icons globally, independent of xftree window.*/
  if (!hack) {hack = gtk_window_new (GTK_WINDOW_POPUP); gtk_widget_realize (hack);}
#ifndef HAVE_GDK_PIXBUF
  else return -1; /* don't recreate pixmaps without gdk-pixbuf */
#endif
	
  for (i=0;pixmaps[i].pixmap != NULL; i++){
	  if (*(pixmaps[i].pixmap) != NULL) gdk_pixmap_unref(*(pixmaps[i].pixmap));
	  if (*(pixmaps[i].pixmask) != NULL) gdk_bitmap_unref(*(pixmaps[i].pixmask));
#ifdef HAVE_GDK_PIXBUF			 		
	  if (h<0) 
#endif
	      *(pixmaps[i].pixmap) = MyCreateGdkPixmapFromData(pixmaps[i].xpm,hack,
			      		pixmaps[i].pixmask,FALSE);
#ifdef HAVE_GDK_PIXBUF			 		
	  else {
  		GdkPixbuf *orig_pixbuf,*new_pixbuf;
		float r=0;
	  	orig_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)(pixmaps[i].xpm));
		if (gdk_pixbuf_get_width (orig_pixbuf)) r=(float)h/gdk_pixbuf_get_width (orig_pixbuf);
		if (r<1.0) r=1.0;
  		new_pixbuf  = gdk_pixbuf_scale_simple (orig_pixbuf, r*gdk_pixbuf_get_width (orig_pixbuf), 
		  r*gdk_pixbuf_get_height (orig_pixbuf), GDK_INTERP_NEAREST);
  		gdk_pixbuf_render_pixmap_and_mask (new_pixbuf, 
				pixmaps[i].pixmap,pixmaps[i].pixmask, 
				gdk_pixbuf_get_has_alpha (new_pixbuf));
  		gdk_pixbuf_unref (orig_pixbuf);
  		gdk_pixbuf_unref (new_pixbuf);
		 /*
		    *(pixmaps[i].pixmap) = duplicate_xpm(hack,pixmaps[i].xpm,pixmaps[i].pixmask);
		 */   
	  }
#endif
  }
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
     {N_("Duplicate"), (gpointer) cb_duplicate, 0, GDK_d,GDK_MOD1_MASK} 
     
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
    {N_("Sort by filename"), NULL, 0, GDK_n, GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Sort by size"), NULL, 0, GDK_s,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Sort by date"), NULL, 0, GDK_d,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
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
  if (preferences & CUSTOM_FONT) create_pixmaps(set_fontT(ctree));
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
  create_pixmaps(-1);

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
