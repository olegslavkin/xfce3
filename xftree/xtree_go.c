
/*
 * xtree_go.c: go routines for xftree
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

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern GdkPixmap  *gPIX_dir_close,  *gPIX_dir_open;
extern GdkBitmap *gPIM_dir_close, *gPIM_dir_open;

typedef struct golist {
	struct golist *previous;
	char *path;
} golist;

/* gogo will be duplicate on fork, so child must reset to NULL. 
 * The heap allocated memory will not be duplicated in child
 * if TFM is right about dynamic memory. 
 * Best is to fork, and then exec a new instance: 
 *   - New window
 *   - Open in new
 *   */

static GList *
free_list (GList * list)
{
  g_list_free (list);
  return NULL;
}


void *pushgo(char *path,void *vgo){
	golist *lastgo,*gogo;
	gogo=(golist *)vgo;	
	/*fprintf(stderr,"dbg: 1gogo path=%s\n",path);*/
	lastgo=gogo;
	gogo=(golist *)malloc(sizeof(golist));
	if (!gogo){
		gogo=lastgo;
		return (void *)gogo;
	}
	gogo->previous=lastgo;
	gogo->path=(char *)malloc(strlen(path)+1);
	if (!(gogo->path)){
		free(gogo);
		gogo=lastgo;
		return (void *)gogo;
	}
	strcpy(gogo->path,path);
	/*fprintf(stderr,"dbg: 2gogo path=%s\n",path);*/
	return (void *)gogo;
}

static void *popgo(void *vgo){
	golist *thisgo,*gogo;
	gogo=(golist *)vgo;	
	if (!gogo) return (void *)gogo;
	thisgo=gogo->previous;
	if (gogo->path) free(gogo->path);
	free(gogo);
	gogo=thisgo;
	return (void *)gogo;
}

void go_to (GtkCTree * ctree, GtkCTreeNode * root, char *path, int flags)
{
  int i;
  char *label[COLUMNS];
  entry *en;
  char *icon_name;
  cfg *win;
  
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

	/*fprintf(stderr,"dbg: go_to path=%s\n",path);*/
	
	
  if (strstr(path,"/..")) {
     if (strlen(path)<=3) return; /* no higher than root */
	*(strrchr(path,'/'))=0;

	*(strrchr(path,'/'))=0;
	if (path[0]==0) strcpy(path,"/");
   }
       	
	
  en = entry_new_by_path (path);
  if (!en)
  {
    xf_dlg_error(win->top,_("Cannot find path"),path);
    /*fprintf (stderr,"dbg:Can't find row data at go_to()\n");*/
    return;
  }

  if (!io_is_valid (en->label)) return;
  en->flags = flags;

	/*fprintf(stderr,"dbg: 2go_to path=%s\n",path);*/

  for (i = 0; i < COLUMNS; i++)
  {
    if (i == COL_NAME)
      label[i] = uri_clear_path (en->path);
    else
      label[i] = "";
  }
  ctree_freeze (ctree);
  
	/*fprintf(stderr,"dbg: 3go_to path=%s\n",path);*/
  
  win->gogo=pushgo(path,win->gogo);
  gtk_ctree_remove_node (ctree, root);
  

  root = gtk_ctree_insert_node (ctree, NULL, NULL, label, 8, gPIX_dir_close, gPIM_dir_close, gPIX_dir_open, gPIM_dir_open, FALSE, TRUE);
  gtk_ctree_node_set_row_data_full (ctree, root, en, node_destroy);
  add_subtree (ctree, root, uri_clear_path (en->path), 2, en->flags);
  ctree_thaw (ctree);
  set_title (GTK_WIDGET (ctree), uri_clear_path (en->path));
  icon_name = strrchr (en->path, '/');
  if ((icon_name) && (!(*(++icon_name))))
    icon_name = NULL;
  gdk_window_set_icon_name (gtk_widget_get_toplevel (GTK_WIDGET (ctree))->window, (icon_name ? icon_name : "/"));

}

    /* A big bug was found and fixed at cb_go_to: Freeing memory twice
     * and appending glist elements to an already freed glist. */
void
cb_go_to (GtkWidget * item, GtkCTree * ctree)
{
  GtkCTreeNode *node, *root;
  entry *en;
  static char *path=NULL;
  char *p;
  static GList *list=NULL;
  int count;
  cfg *win;
  golist *thisgo;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (list != NULL) list = free_list (list);
  
  root = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
  /* count selection returns root-node when count==0 ;-) */
  count = count_selection (ctree, &node); 
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
  /* therefore, en != NULL */

  if (path) free(path);
  if (!(path=(char *)malloc(strlen(en->path)+1) ) ) return;
  
  strcpy (path, en->path);
  if ((count != 1) || !(en->type & FT_DIR)) { /* make combo box */
    p = path;
#if 0
    if (preferences&SOMEOPTION) while ((p = strrchr (path, '/')) != NULL) {
	if (p == path) { /* this puts / on the combo */
	  *(p + 1) = '\0';
	  list = g_list_append (list, g_strdup (path));
	  break;
	}
	*p = '\0';
	list = g_list_append (list, g_strdup (path));
    } 
    else 
#endif
    if (win->gogo) for (thisgo=((golist *)(win->gogo))->previous; thisgo!=NULL; thisgo=thisgo->previous)
	       list = g_list_append (list,thisgo->path);
  
    if (xf_dlg_combo (win->top,_("Go to"), path, list) != DLG_RC_OK) {
      return;
    }
  } else strcpy(path,en->path);
  go_to (ctree, root, path, en->flags);
}

void cb_go_back (GtkWidget * item, GtkCTree * ctree){
  GtkCTreeNode *root;
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->gogo=popgo(win->gogo);
  if (!(win->gogo)) return;
  root = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
  go_to (ctree, root, ((golist *)(win->gogo))->path, IGNORE_HIDDEN);
  if ((win->gogo) && (((golist *)(win->gogo))->previous)) 
	  win->gogo=popgo ((golist *)(win->gogo)); /* current dir */
}


/*
 */
void
cb_go_home (GtkWidget * item, GtkCTree * ctree)
{
  GtkCTreeNode *root;
  root = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
  go_to (ctree, root, getenv ("HOME"), IGNORE_HIDDEN);
}


/*
 * change root and go one directory up
 */
void
cb_go_up (GtkWidget * item, GtkCTree * ctree)
{
  entry *en;
  static char *path=NULL;;
  char *p;
  GtkCTreeNode *root;

  root = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), root);
  if (!en) return;
  if (path) free(path);
  if (!(path=(char *)malloc(strlen(en->path)+1) ) ) return;
  strcpy (path, en->path);
	/*fprintf(stderr,"dbg: go_up=%s\n",path);*/
  p = strrchr (path, '/');
  if (p == path)
  {
    if (!*(p + 1)) return;
    *(p + 1) = '\0';
  }
  else *p = '\0';
  go_to (ctree, root, path, en->flags);
}


