/*
 * xtree_dnd.c
 *
 * Copyright (C) 1998 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 
 * Edscott Wilson Garcia 2001 for Xfce project (http://www.xfce.org)
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
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "my_intl.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "gtk_dlg.h"
#include "gtk_get.h"
#include "gtk_dnd.h"
#include "xtree_cfg.h"
#include "xtree_dnd.h"
#include "xtree_misc.h"
#include "xtree_cpy.h"
#include "xfce-common.h"
#include "xtree_mess.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif


extern int update_tree (GtkCTree * ctree, GtkCTreeNode * node);
static gboolean drop_cancelled=FALSE;

void cancel_drop(gboolean state)
{ 
	drop_cancelled=state;
}

/*
 * called if drop data will be received
 * signal: drag_data_received
 */
void
on_drag_data (GtkWidget * ctree, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, void *client)
{
  cfg *win = (cfg *) client;
  GList *list;
  entry *t_en, *s_en;
  GtkCTreeNode *node = NULL;
  uri *u;
  int nitems, action;
  int mode = 0;
  int row, col;
  GtkCList *clist;
  char *tmpfile=NULL;

  if (!ctree)
    return;

  clist = GTK_CLIST (ctree);

  if ((data->length < 0) || (data->format != 8))
  {
    gtk_drag_finish (context, FALSE, FALSE, time);
    return;
  }
  set_override(FALSE);

  action = context->action <= GDK_ACTION_DEFAULT ? GDK_ACTION_COPY : context->action;

  row = col = -1;
  y -= clist->column_title_area.height;
  gtk_clist_get_selection_info (clist, x, y, &row, &col);
  /* initialize row pointer */
  win->dnd_row = 0;

  if (row >= 0)
  {
    node = gtk_ctree_node_nth (GTK_CTREE (ctree), row);
    s_en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
    if ((((s_en->type & FT_DIR) && (access (s_en->path, W_OK | X_OK) == 0)) || ((s_en->type & FT_FILE) && (access (s_en->path, W_OK) == 0))))
      win->dnd_row = row;
    else
      win->dnd_row = 0;
  }
  else if (win->dnd_row >= 0)
    win->dnd_row = 0;
  

  switch (info)
  {
  case TARGET_XTREE_WIDGET:
  case TARGET_XTREE_WINDOW:
  case TARGET_STRING:
  case TARGET_URI_LIST:
    /*fprintf(stderr,"dbg:at dnd 1\n");*/
    if (action == GDK_ACTION_MOVE)
    {
      mode = TR_MOVE;
    }
    else if (action == GDK_ACTION_COPY)
    {
      mode = TR_COPY;
    }
    else if (action == GDK_ACTION_LINK)
    {
      mode = TR_LINK;
    }
    else
    {
      xf_dlg_error (win->top,_("Unknown action !"), NULL);
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }
    nitems = uri_parse_list ((const char *) data->data, &list);
    if (!nitems) break; /* of course */
    uri_remove_file_prefix_from_list (list);
    node = gtk_ctree_node_nth (GTK_CTREE (ctree), win->dnd_row);
    t_en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
    /* this garantees that target will always be a directory
     * and thus no need to check further down
     * */
    /*fprintf(stderr,"dbg:at dnd 2 (items=%d)\n",nitems);*/
    if (!(t_en->type & FT_DIR)) /* target is not a directory */
    {
      node = GTK_CTREE_ROW (node)->parent;
      t_en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
      /* this should give a directory as target. */
    }
    if ((!(t_en->type & FT_DIR)) || (t_en->type & FT_DIR_UP) || (!io_is_valid (t_en->label))|| (EN_IS_DIRUP (t_en)))
    {
      break;
      /*gtk_drag_finish (context, FALSE, (mode == TR_MOVE), time);
      uri_free_list (list);
      return;*/
    }
    /*fprintf(stderr,"dbg:at dnd 3\n");*/
    
    // tmpfile ==NULL means drop cancelled
    u = list->data;
    s_en = entry_new_by_path (u->url);
    if (strcmp(s_en->path,t_en->path)==0){
	    /*fprintf(stderr,"dbg:nonsense input\n");*/
	    break;
    }
    /*fprintf(stderr,"dbg:at dnd 4\n");*/
    tmpfile=CreateTmpList(win->top,list,t_en);
    /*fprintf(stderr,"dbg:at dnd 5\n");*/
    if (!tmpfile) break;
    // acording to tmpfile name, do a direct move, here, and break.
    cursor_wait (GTK_WIDGET (ctree));
    /* FIXME: links on same device should also be a DirectTransfer() */
    if (on_same_device() && (mode == TR_MOVE)) DirectTransfer(win->top,mode,tmpfile);
    else IndirectTransfer(win->top,mode,tmpfile);
    
    unlink(tmpfile);
    
    uri_free_list (list);
    cursor_reset (GTK_WIDGET (ctree));    
    
    break;
  default:
    break;
  }
  /* set cpy over*/
  /*fprintf(stderr,"dbg:parent:runOver\n");*/
  gtk_drag_finish (context, TRUE, TRUE, time);
  update_tree (GTK_CTREE (ctree), node);
}

/*
 * DND sender: prepare data for the remote
 * event: drag_data_get
 */
void
on_drag_data_get (GtkWidget * widget, GdkDragContext * context, GtkSelectionData * selection_data, guint info, guint time, gpointer data)
{
  GtkCTreeNode *node = NULL;
  GtkCTree *ctree = GTK_CTREE (widget);
  GList *selection;
  entry *en;
  int num, i, len, slen;
  gchar *files;
  cfg *win = (cfg *) data;

  if (!ctree)
    return;

  num = g_list_length (GTK_CLIST (ctree)->selection);
  if (!num)
    node = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
  else
    node = GTK_CTREE_NODE (GTK_CLIST (ctree)->selection->data);

  /* prepare data for the receiver
   */
  switch (info)
  {
  case TARGET_ROOTWIN:
    /* not implemented */
    win->dnd_data = NULL;
    break;
  default:
    selection = GTK_CLIST (ctree)->selection;
    for (len = 0, i = 0; i < num; i++)
    {
      node = selection->data;
      en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
      len += strlen (en->path) + 5 + 2;
      selection = selection->next;
    }
    win->dnd_data = files = g_malloc (len + 1);
    files[0] = '\0';
    selection = GTK_CLIST (ctree)->selection;
    for (i = 0; i < num; i++)
    {
      node = selection->data;
      en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
      slen = strlen (en->path);
      sprintf (files, "file:%s\r\n", en->path);
      files += slen + 5 + 2;
      selection = selection->next;
    }
    gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) win->dnd_data, len);
    break;
  }
}

gboolean
on_drag_motion (GtkWidget * widget, GdkDragContext * dc, gint x, gint y, guint t, gpointer data)
{
  gboolean same;
  GtkWidget *source_widget;
  GdkDragAction action;

  /* Get source widget and check if it is the same as the
   * destination widget. 
   */
  source_widget = gtk_drag_get_source_widget (dc);
  same = ((source_widget == widget) ? TRUE : FALSE);

  /* Insert code to get our default action here. */
  if (preferences & DRAG_DOES_COPY) action = GDK_ACTION_COPY;
  else action = GDK_ACTION_MOVE;

  /* Respond with default drag action (status). First we check
   * the dc's list of actions. If the list only contains
   * move or copy then we select just that, otherwise we return
   * with our default suggested action.
   * If no valid actions are listed then we respond with 0.
   */

  /* Only move? */
  if (dc->actions == GDK_ACTION_MOVE)
    gdk_drag_status (dc, GDK_ACTION_MOVE, t);
  else if (dc->actions == GDK_ACTION_COPY)
    gdk_drag_status (dc, GDK_ACTION_COPY, t);
  else if (dc->actions == GDK_ACTION_LINK)
    gdk_drag_status (dc, GDK_ACTION_LINK, t);
  else if (dc->actions & action)
    gdk_drag_status (dc, action, t);
  else
    gdk_drag_status (dc, 0, t);

  return (TRUE);
}

void
on_drag_end (GtkWidget * widget, GdkDragContext * context, gpointer data)
{
  GtkCTreeNode *node = NULL;
  GtkCTree *ctree = GTK_CTREE (widget);
  int num;

  if (!ctree)
    return;
  num = g_list_length (GTK_CLIST (ctree)->selection);
  if (!num)
    node = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
  else
  {
    node = GTK_CTREE_ROW (GTK_CLIST (ctree)->selection->data)->parent;
  }
  update_tree (GTK_CTREE (ctree), node);
  return;
}
