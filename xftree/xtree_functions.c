/*
 * xtree_functions.c: general gui functions.
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
#include <glob.h>
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

#ifndef GLOB_TILDE
#define GLOB_TILDE 0
#endif

int update_tree (GtkCTree * ctree, GtkCTreeNode * node);
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
  gboolean manage_timeout;
  cfg *win;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  manage_timeout = (win->timer != 0);

  if (manage_timeout)
  {
    gtk_timeout_remove (win->timer);
    win->timer = 0;
  }


  /* get a list of directories we have to check
   */
  gtk_ctree_post_recursive (ctree, 
		  GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list), 
		  get_visible_or_parent, &list);

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
 * what should we do here?
 */
void menu_detach (void)
{
  /*printf (_("dbg:menu_detach()\n"));*/
}

void
tree_unselect  (GtkCTree *ctree,GList *node,gpointer user_data)
{
  gtk_ctree_unselect_recursive (ctree, NULL);
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

typedef struct icon_pix {
	GdkPixmap *pixmap;
	GdkBitmap *pixmask;
	GdkPixmap *open;
	GdkBitmap *openmask;
} icon_pix;

static gboolean set_icon_pix(icon_pix *pix,entry *en) {
  gboolean isleaf=TRUE;
  pix->open=pix->openmask=NULL;
  if (en->type & FT_EXE) {
    if (en->type & FT_LINK) pix->pixmap=gPIX_exe_lnk; 
    else pix->pixmap=gPIX_exe;
    pix->pixmask=gPIM_exe;    
  }
  else if (en->type & FT_FILE)/* letter modified here */
  {
    if (en->type & FT_LINK) pix->pixmap=gPIX_page_lnk; 
    else {
      char *loc;
      pix->pixmap=gPIX_page; /* default */
      if (strcmp(en->label,"core")==0) pix->pixmap=gPIX_core;
      if ( (loc=strrchr(en->path,'.')) != NULL ){
	      if (strlen(loc)==2) switch (loc[1]){
		      case 'c': pix->pixmap=gPIX_pageC; break;
		      case 'h': pix->pixmap=gPIX_pageH; break;
		      case 'f': pix->pixmap=gPIX_pageF; break;
		      default: break;				      
	      }
	      else if ((strcmp(loc,".txt")==0) ||
	               (strcmp(loc,".doc")==0) ||
	               (strcmp(loc,".tex")==0)) pix->pixmap=gPIX_text;
	      
	      else if ((strcmp(loc,".jpg")==0) ||
	               (strcmp(loc,".gif")==0) ||
	       	       (strcmp(loc,".png")==0) ||
	               (strcmp(loc,".jpeg")==0)||
	               (strcmp(loc,".tiff")==0)||
	               (strcmp(loc,".xbm")==0) ||
	               (strcmp(loc,".xpm")==0) ||
	               (strcmp(loc,".xcf")==0) ||
	               (strcmp(loc,".pcx")==0)) pix->pixmap=gPIX_image;
	      
	      else if ((strcmp(loc,".gz")==0)  || 
	               (strcmp(loc,".tgz")==0) || 
	               (strcmp(loc,".bz2")==0) || 
	               (strcmp(loc,".zip")==0)) pix->pixmap=gPIX_compressed;
	      
	      else if (strcmp(loc,".tar")==0) pix->pixmap=gPIX_tar;
      }
    }  
    pix->pixmask=gPIM_page;
  }
  else if (en->type & FT_DIR_UP) {pix->pixmap=gPIX_dir_up,pix->pixmask=gPIM_dir_close;}
  else if (en->type & FT_DIR_PD) {
	  isleaf=FALSE;
	  pix->pixmap=gPIX_dir_pd,pix->pixmask=gPIM_dir_close;
	  pix->open=gPIX_dir_open,pix->openmask=gPIM_dir_open;
  }
  else if (en->type & FT_DIR){
    isleaf=FALSE;
    if (en->type & FT_LINK) pix->pixmap=gPIX_dir_close_lnk;
    else pix->pixmap=gPIX_dir_close;
    pix->pixmask=gPIM_dir_close;
    pix->open=gPIX_dir_open,pix->openmask=gPIM_dir_open;
  }
  else if (en->type & FT_CHAR_DEV){pix->pixmap=gPIX_char_dev,pix->pixmask=gPIM_char_dev;}
  else if (en->type & FT_BLOCK_DEV){pix->pixmap=gPIX_block_dev,pix->pixmask=gPIM_block_dev;}
  else if (en->type & FT_FIFO){pix->pixmap=gPIX_fifo,pix->pixmask=gPIM_fifo;}
  else if (en->type & FT_SOCKET){pix->pixmap=gPIX_socket,pix->pixmask=gPIM_socket;}
  else {pix->pixmap=gPIX_stale_lnk,pix->pixmask=gPIM_stale_lnk;}

  return isleaf;
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
  icon_pix pix;  
  gboolean isleaf;

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

  isleaf=set_icon_pix(&pix,en);

/**************************/

  item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING, 
		  pix.pixmap,pix.pixmask, pix.open,pix.openmask, isleaf, FALSE);
  if (item)
    gtk_ctree_node_set_row_data_full (ctree, item, en, node_destroy);
  *type = en->type;
  return (item);
}

void
update_node (GtkCTree * ctree, GtkCTreeNode * node, int type, char *label)
{
  icon_pix pix;  
  gboolean isleaf;
  entry *en;
  
  if (!ctree || !node || !label)
  {
    return;
  }
  en = gtk_ctree_node_get_row_data (ctree, node); 
  isleaf=set_icon_pix(&pix,en);
  gtk_ctree_set_node_info (ctree, node, label, SPACING, 
		  pix.pixmap,pix.pixmask, pix.open, pix.openmask, isleaf, FALSE);
  
}

/*
 */
void
add_subtree (GtkCTree * ctree, GtkCTreeNode * root, char *path, int depth, int flags)
{
  xf_dirent *diren;
  GtkCTreeNode *item = NULL, *first = NULL;
  char *base;
  /* FIXME: use malloc() and free() calls instead of static memory size */
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
  if (!base){
    cfg * win;
    win = gtk_object_get_user_data (GTK_OBJECT (ctree));
    xf_dlg_error(win->top,_("internal malloc error:"),strerror(errno));
    exit(1);
  }
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
  {
   char *name;
   /* ../ is usually not filtered in, so add it */
   if (preferences&FILTER_OPTION) {
     type=FT_DIR_UP;
     sprintf (complete, "%s..", base);
     add_node (GTK_CTREE (ctree), root, NULL, "..",complete , &type, flags);
   }
   diren = xf_opendir (path,(GtkWidget *)ctree);
   if (!diren) {
       /*fprintf(stderr,"dbg:add_subtree() xf_opendir failed\n");*/
    g_free (base);
    return;
   }
   while ((name = xf_readdir (diren)) != NULL) {
    type = 0;
    item = NULL;
    d_len = strlen (name);
	/*fprintf(stderr,"dbg:%s\n",name);*/
    if (io_is_dirup (name)) type |= FT_DIR_UP | FT_DIR;
    else if ((d_len >= 1) && io_is_hidden (name) && ((flags & IGNORE_HIDDEN)))
      continue;
    sprintf (complete, "%s%s", base, name);
    strcpy (label, name);
    if ((!io_is_current (name))){
      if ((preferences&ABREVIATE_PATHS)&&(strlen(label)>16)){
	      int i;
	      label[8]='~';
	      for (i=9;i<=16;i++) label[i]=label[strlen(label)-(16-i)];
      }
      item = add_node (GTK_CTREE (ctree), root, first, label, complete, &type, flags);
    }
    if ((type & FT_DIR) && (!(type & FT_DIR_UP)) && (!(type & FT_DIR_PD)) && (io_is_valid (name)) && item){
      add_subtree (ctree, item, complete, depth - 1, flags);
    }
    else if (!first)
      first = item;
   }
   diren=xf_closedir (diren);
  } 
  g_free (base);
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
  en->flags ^= IGNORE_HIDDEN;
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
       /*fprintf(stderr,"dbg:nhc() %s <--> %s\n",en->label, label);fflush(NULL);*/
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
  char compl[PATH_MAX + 1];
  char label[NAME_MAX + 1];
  int type, p_len, changed, tree_updated, root_changed;
  gchar size[16];
  gchar date[32];
  cfg *win;
  gboolean manage_timeout;

  if (!ctree) return 0;

  if (!node) return 0;

  

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  /* simple case, abbreviate preference changed in another window */
  if ((win->preferences&ABREVIATE_PATHS) != (preferences&ABREVIATE_PATHS)) {
	  regen_ctree(ctree);
	  win->preferences=preferences;
	  return 0;
  }
  
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
  for (child=GTK_CTREE_ROW (node)->children;child!=NULL;child = GTK_CTREE_ROW (child)->sibling)
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
      sprintf (date, "%02d-%02d-%02d  %02d:%02d", 
		      child_en->date.year, child_en->date.month, 
		      child_en->date.day, child_en->date.hour, child_en->date.min);
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
  } /* end for child */

  if ((root_changed || tree_updated) && (en->type & FT_DIR))
  {
    if (GTK_CTREE_ROW (node)->expanded)
    {
      char *name;	    
      xf_dirent *diren;
      /* may be there are new files */
       /*fprintf(stderr,"dbg:update_tree()...\n");fflush(NULL);*/
       
       diren = xf_opendir (en->path,(GtkWidget *)ctree);
       if (!diren) {
	if (manage_timeout) {
	  win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
	}
	return TRUE;
       }
       p_len = strlen (en->path);
       while ((name = xf_readdir (diren)) != NULL) {
	strcpy (label, name);
	if ((preferences&ABREVIATE_PATHS)&&(strlen(label)>16)){
	      int i;
	      label[8]='~';
	      for (i=9;i<=16;i++) label[i]=label[strlen(label)-(16-i)];
	}
	if (io_is_hidden (name) && (en->flags & IGNORE_HIDDEN))  continue;
	if (io_is_current (name))  continue;
	if (io_is_current (label))  continue;
	if (!node_has_child (ctree, node, name))
	{
	  if (io_is_root (name)) sprintf (compl, "%s%s", en->path, name);
	  else   sprintf (compl, "%s/%s", en->path, name);
	  type = 0;
	  new_child = NULL;
	  if (name && !io_is_current (name) ){
	    new_child = add_node (ctree, node, NULL, label, compl, &type, en->flags);
	  }
	  
	  if ((type & FT_DIR) && (io_is_valid (name)) && !(type & FT_DIR_UP) && !(type & FT_DIR_PD) && new_child)
	    add_subtree (ctree, new_child, compl, 1, en->flags);
	  if (entry_type_update (en) == TRUE)
	    update_node (ctree, node, en->type, en->label);
	  entry_update (en);
	  tree_updated = TRUE;
	}
	/*else {fprintf(stderr,"dbg:update_tree()...node has child\n");fflush(NULL);}*/
       } /* end while */
       diren = xf_closedir (diren);  
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




