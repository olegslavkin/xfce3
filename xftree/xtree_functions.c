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
#include "xtree_functions.h"

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

static void update_status(GtkCTreeNode * node,GtkCTree * ctree);
static int update_tree (GtkCTree * ctree, GtkCTreeNode * node);
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
  GtkCTreeNode *node,*status_node;
  gboolean manage_timeout;
  cfg *win;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  status_node=win->status_node;

  /* has root directory vanished ? */
  {
    entry *en;
    glob_t dirlist;
    //root = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
    en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), 
		  GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list));
    if(glob (en->path, GLOB_ERR, NULL, &dirlist)){
	    globfree(&dirlist);
	    /* clean history (what else has vanished?)*/
	    while (win->gogo) win->gogo=popgo(win->gogo);
	    /* go home */
	    cb_go_home (NULL, ctree);
	    return (TRUE);
    } else globfree(&dirlist);
  }

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
    if (update_tree (ctree, node) != TRUE) {
      break;
    }
    tmp = tmp->next;
  }
  g_list_free (list);
  if (manage_timeout)
  {
    win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
  }
  update_status(status_node,ctree);
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

static gboolean checkif_type(char **Type,char *loc){
  int i;
  for (i=0;Type[i]!=NULL;i++) 
	  if (strcmp(loc,Type[i])==0) return TRUE;
  return FALSE;
}

static gboolean image_type(char *loc){
  char *Type[]={
	  ".jpg",".JPG",".gif",".GIF",".png",".PNG",
	  ".JPEG",".jpeg",".TIFF",".tiff",".xbm","XBM",
	  ".XPM",".xpm",".XCF",".xcf",".PCX",".pcx",
	  ".BMP",".bmp",
	  NULL
  };
  return checkif_type(Type,loc);			  
}
static gboolean text_type(char *loc){
  char *Type[]={
	  ".txt",".TXT",".tex",".TEX",
	  ".doc",".DOC",
	  ".readme",".README",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean compressed_type(char *loc){
  char *Type[]={
	  ".gz",".tgz",".bz2",".zip",
	  ".ZIP",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean packed_type(char *loc){
  char *Type[]={
	  ".tar",".deb",".rpm",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean www_type(char *loc){
  char *Type[]={
	  ".html",".htm",".HTM",".HTML",
	  ".sgml",".SGML",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean audio_type(char *loc){
  char *Type[]={
	  ".wav",".mp3",".mid",".midi",
	  ".kar",".mpga",".mp2",
	  ".ra",".aif",".aiff",".ram",
	  ".rm",".au",".snd",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean script_type(char *loc){
  char *Type[]={
	  ".pl",".sh",".csh",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean mail_type(char *loc){
  char *Type[]={
	  "inbox","outbox","mbox","dead.letter",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean bak_type(char *loc){
  char *Type[]={
	  ".bak",".BAK",".old",".rpmsave",".rpmnew",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean dup_type(char *loc){
  char *l;
  for (l=loc+1;*l!=0;l++){
	  if ((*l > '9')||(*l < '0')) return FALSE;
  }
  return TRUE;			    
}



static gboolean set_icon_pix(icon_pix *pix,entry *en) {
  char *loc;
  gboolean isleaf=TRUE;
  pix->open=pix->openmask=NULL;
  if (en->type & FT_EXE) {
    if (en->type & FT_LINK) pix->pixmap=gPIX[PIX_EXE_LINK]; 
    else pix->pixmap=gPIX[PIX_EXE];
    if ( (loc=strrchr(en->path,'.')) != NULL ){
    	if (script_type(loc)) pix->pixmap=gPIX[PIX_EXE_SCRIPT];
    }
    pix->pixmask=gPIM[PIM_EXE];    
  }
  else if (en->type & FT_FILE)/* letter modified here */
  {
    pix->pixmask=gPIM[PIM_PAGE];
    if (en->type & FT_LINK) {
	    pix->pixmap=gPIX[PIX_PAGE_LNK]; 
    }
    else {
      pix->pixmap=gPIX[PIX_PAGE]; /* default */
      if (strcmp(en->label,"core")==0) pix->pixmap=gPIX[PIX_CORE];
      else if (mail_type(en->label))	pix->pixmap=gPIX[PIX_MAIL];	      
      else if ( (loc=strrchr(en->label,'-')) != NULL ){
         if (dup_type(loc)) pix->pixmap=gPIX[PIX_DUP];	      
      }
      if ( (loc=strrchr(en->label,'.')) != NULL ){
	      if (strlen(loc)==2) switch (loc[1]){
		      case 'c': pix->pixmap=gPIX[PIX_PAGE_C]; break;
		      case 'h': pix->pixmap=gPIX[PIX_PAGE_H]; break;
		      case 'f': pix->pixmap=gPIX[PIX_PAGE_F]; break;
		      case 'o': pix->pixmap=gPIX[PIX_PAGE_O]; break;
		      default: break;				      
	      }
	      else if (bak_type(loc)) 		pix->pixmap=gPIX[PIX_BAK];	      
	      else if (image_type(loc)) 	pix->pixmap=gPIX[PIX_IMAGE];
	      else if (text_type(loc))  	pix->pixmap=gPIX[PIX_TEXT];
	      else if (compressed_type(loc)) 	pix->pixmap=gPIX[PIX_COMPRESSED];
	      else if (www_type(loc)) {
		           pix->pixmap=gPIX[PIX_PAGE_HTML];
	                   pix->pixmask=gPIM[PIM_PAGE_HTML];
	      }
 	      else if (audio_type(loc)) {
		           pix->pixmap=gPIX[PIX_PAGE_AUDIO];
	                   pix->pixmask=gPIM[PIM_PAGE_AUDIO];
	      }
   	      else if (packed_type(loc)) pix->pixmap=gPIX[PIX_TAR];
      }
    }  
  }
  else if (en->type & FT_DIR_UP) {pix->pixmap=gPIX[PIX_DIR_UP],pix->pixmask=gPIM[PIM_DIR_CLOSE];}
  else if (en->type & FT_DIR_PD) {
	  isleaf=FALSE;
	  pix->pixmap=gPIX[PIX_DIR_PD],pix->pixmask=gPIM[PIM_DIR_CLOSE];
	  pix->open=gPIX[PIX_DIR_OPEN],pix->openmask=gPIM[PIM_DIR_OPEN];
  }
  else if (en->type & FT_DIR){
    isleaf=FALSE;
    if (en->type & FT_LINK) {
	    pix->pixmap=gPIX[PIX_DIR_CLOSE_LNK];
	    pix->open=gPIX[PIX_DIR_OPEN_LNK]; 
    }
    else {
	    pix->pixmap=gPIX[PIX_DIR_CLOSE];
            pix->open=gPIX[PIX_DIR_OPEN];
    }
    pix->pixmask=gPIM[PIM_DIR_CLOSE],pix->openmask=gPIM[PIM_DIR_OPEN];
  }
  else if (en->type & FT_CHAR_DEV){pix->pixmap=gPIX[PIX_CHAR_DEV],pix->pixmask=gPIM[PIM_CHAR_DEV];}
  else if (en->type & FT_BLOCK_DEV){pix->pixmap=gPIX[PIX_BLOCK_DEV],pix->pixmask=gPIM[PIM_BLOCK_DEV];}
  else if (en->type & FT_FIFO){pix->pixmap=gPIX[PIX_FIFO],pix->pixmask=gPIM[PIM_FIFO];}
  else if (en->type & FT_SOCKET){pix->pixmap=gPIX[PIX_SOCKET],pix->pixmask=gPIM[PIM_SOCKET];}
  else {pix->pixmap=gPIX[PIX_STALE_LNK],pix->pixmask=gPIM[PIM_STALE_LNK];}

  return isleaf;
}
/*
 */
GtkCTreeNode *
add_node (GtkCTree * ctree, GtkCTreeNode * parent, GtkCTreeNode * sibling, char *label, char *path, int *type, int flags)
{
  cfg *win;
  entry *en;
  GtkCTreeNode *item;
  gchar *text[COLUMNS];
  gchar size[16] = { "" };
  gchar date[32] = { "" };
  icon_pix pix;  
  gboolean isleaf;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
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
      sprintf (size, "?(ERR %lld)", -(long long)en->size);
    }
    else
    {
      sprintf (size, " %lld", (long long) en->size);
    }
  }
  if (win->preferences&ABREVIATE_PATHS) text[COL_NAME] = abreviateP(en->label); else 
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

static void update_status(GtkCTreeNode * node,GtkCTree * ctree){
   GtkCTreeNode *root,*child;
   entry *en,*p_en;
   status_info status_inf;
   cfg * win;
   char *texto;
   
   win = gtk_object_get_user_data (GTK_OBJECT (ctree));
   status_inf.howmany=0;
   status_inf.howmuch=0;
   /* is the status node still there? */
   root = GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list);
   if (!gtk_ctree_find (ctree,root,node)){
	   /*fprintf(stderr,"dbg:status node gone away.\n");*/
	   node=root;
	   win->status_node=root;
   }
 
   p_en = gtk_ctree_node_get_row_data (ctree, node); 
   
  for (child=GTK_CTREE_ROW (node)->children;child!=NULL;child = GTK_CTREE_ROW (child)->sibling){
      en = gtk_ctree_node_get_row_data (ctree, child); 
      status_inf.howmany++;
      status_inf.howmuch += en->size;
  }
   status_inf.howmuch *= 0.0009765625; /* value in Kb */ 
   texto=(char *)malloc(128+strlen(p_en->path));
   if (!texto) return;
   sprintf(texto,"%s: %lld %s, %lld Kb.",
		   p_en->path, /* FIXME: use abbreviated path if configured */
		   (long long)status_inf.howmany,
		   _("files"),
		   (long long)status_inf.howmuch);
   gtk_label_set_text ((GtkLabel *)win->status,texto);

   /*fprintf(stderr,"dbg:%s\n",texto);*/
   free(texto);
   win->status_node=node;
	
}


void add_subtree (GtkCTree * ctree, GtkCTreeNode * root, char *path, int depth, int flags)
{
  cfg * win;
  xf_dirent *diren;
  GtkCTreeNode *item = NULL, *first = NULL;
  char *base;
  char *complete;
  char *label;
  int add_slash = no, len, d_len;
  int type = 0;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  if (depth == 0) return ;

  if (!path) return ;
  len = strlen (path);
  if (!len)  return ;
  if (path[len - 1] != '/')
  {
    add_slash = yes;
    len++;
  }
  base = g_malloc (len + 1);
  if (!base){
    xf_dlg_error(win->top,_("internal malloc error:"),strerror(errno));
    exit(1);
  }
  strcpy (base, path);
  if (add_slash)
    strcat (base, "/");

      /* fprintf(stderr,"dbg:base=%s\n",base);*/
  if (depth == 1)
  {
    /* create dummy entry */
    if ((complete=(char *)malloc(strlen(base)+2))==NULL) return;
    sprintf (complete, "%s.", base);
    type = FT_DUMMY;
    add_node (GTK_CTREE (ctree), root, NULL, ".", complete, &type, flags);
    g_free (base);
    free(complete);
    return ;
  }
  {
   char *name;
   /* ../ is usually not filtered in, so add it */
   if (win->preferences&FILTER_OPTION) {
     type=FT_DIR_UP;
     if ((complete=(char *)malloc(strlen(base)+3))==NULL) return;
     sprintf (complete, "%s..", base);
     add_node (GTK_CTREE (ctree), root, NULL, "..",complete , &type, flags);
     free(complete);
   }
   diren = xf_opendir (path,(GtkWidget *)ctree);
   if (!diren) {
       /*fprintf(stderr,"dbg:add_subtree() xf_opendir failed\n");*/
    g_free (base);
    return ;
   }
   while ((name = xf_readdir (diren,(GtkWidget *)ctree)) != NULL) {
    type = 0;
    item = NULL;
    d_len = strlen (name);
	/*fprintf(stderr,"dbg:%s\n",name);*/
    if (io_is_dirup (name)) type |= FT_DIR_UP | FT_DIR;
    else if ((d_len >= 1) && io_is_hidden (name) && ((flags & IGNORE_HIDDEN)))
      continue;
    if ((complete=(char *)malloc(strlen(base)+strlen(name)+1))==NULL) continue;
    sprintf (complete, "%s%s", base, name);
    if ((label=(char *)malloc(strlen(name)+1))==NULL) continue;
    strcpy (label, name);
    if ((!io_is_current (name))){
      entry *en;
      item = add_node (GTK_CTREE (ctree), root, first, label, complete, &type, flags);
      en = gtk_ctree_node_get_row_data (ctree, item); 
    }
    if ((type & FT_DIR) && (!(type & FT_DIR_UP)) && (!(type & FT_DIR_PD)) && (io_is_valid (name)) && item){
	    /* this is just to get the necesary expanders on startup */
      add_subtree (ctree, item, complete, depth - 1, flags);
    }
    else {if (!first) {first = item;}}
    free(complete);
    free(label);
   }
   diren=xf_closedir (diren);
   update_status(root,ctree);
  } 
  g_free (base);
  gtk_ctree_sort_node (ctree, root);
  return ;
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
  cfg *win;
  entry *en;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
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
  if (win->preferences & STATUS_FOLLOWS_EXPAND){
   cfg *win;
   win = gtk_object_get_user_data (GTK_OBJECT (ctree));
   set_title_ctree((GtkWidget *)ctree,en->path);
  }
}

/*
 * unmark all marked on collapsion
 */
void
on_collapse (GtkCTree * ctree, GtkCTreeNode * node, char *path)
{
   cfg *win;
  GtkCTreeNode *child,*parent;
  /* unselect all children */
   win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  child = GTK_CTREE_NODE (GTK_CTREE_ROW (node)->children);
  if (node==GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list)) parent=node;
  else parent = GTK_CTREE_NODE (GTK_CTREE_ROW (node)->parent);
  while (child)
  {
    gtk_ctree_unselect (ctree, child);
    child = GTK_CTREE_ROW (child)->sibling;
  }
  if (win->preferences & STATUS_FOLLOWS_EXPAND){
   entry *en;
   en = gtk_ctree_node_get_row_data (ctree, parent);
   set_title_ctree((GtkWidget *)ctree,en->path);
  }
  update_status(parent,ctree);
}


void
set_title (GtkWidget * w, const char *path)
{
  char *title;
  title = (char *)malloc((strlen("XFTree: ")+strlen(path)+1)*sizeof(char));
  if (!title) return; 
  sprintf (title, "XFTree: %s", path);
  gtk_window_set_title (GTK_WINDOW (gtk_widget_get_toplevel (w)), title);
  free(title);

}
void
set_title_ctree (GtkWidget * ctree, const char *path)
{
  char *title;
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  title = (char *)malloc((strlen("XFTree: ")+strlen(path)+1)*sizeof(char));
  if (!title) return; 
  if (win->preferences&SHORT_TITLES) {
	  char *word;
	  word = strrchr (path,'/');
	  if (word) sprintf(title,"%s",(word[1]==0)?word:word+1);
	  else  sprintf(title,"XFTree");	  
  }
  else sprintf (title, "%s", path);
  gtk_window_set_title (GTK_WINDOW (gtk_widget_get_toplevel (win->top)), title);
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
  int type, p_len, changed, tree_updated, root_changed;
  gchar size[16];
  gchar date[32];
  cfg *win;
  gboolean manage_timeout;

  if (!ctree) return 0;

  if (!node) return 0;

  

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  /* simple case, global preference changed in another window */
  if ((win->preferences&FONT_STATE) != (preferences&FONT_STATE)) {
	/*printf("dbg: global prefs changed!\n");*/
	win->preferences &= (preferences&FONT_STATE);
        set_fontT((GtkWidget *)ctree);
	/*regen_ctree(ctree);*/
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
      sprintf (size, " %lld", (long long) child_en->size);
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
    /*fprintf(stderr,"dbg:rc=%d,tu=%d\n",root_changed,tree_updated);fflush(NULL);*/
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
       while ((name = xf_readdir (diren,(GtkWidget *)ctree)) != NULL) {
	if (io_is_hidden (name) && (en->flags & IGNORE_HIDDEN))  continue;
	if (io_is_current (name))  continue;
	if (!node_has_child (ctree, node, name))
	{
	  if (io_is_root (name)) sprintf (compl, "%s%s", en->path, name);
	  else   sprintf (compl, "%s/%s", en->path, name);
	  type = 0;
	  new_child = NULL;

	  /*fprintf(stderr,"dbg:adding %s (%s)\n",label,name);fflush(NULL);*/

	  new_child = add_node (ctree, node, NULL, name, compl, &type, en->flags);
	  
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
  return (TRUE);
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




