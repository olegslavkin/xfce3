/* copywrite 2001 edscott wilson garcia under GNU/GPL 
 * 
 * xtree_pasteboard.c: pasteboard routines for xftree
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
#include "xtree_pasteboard.h"
#include "xtree_cpy.h"
#include "xtree_cb.h"
#include "xtree_functions.h"
#include "xtree_mess.h"


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif


extern gboolean tar_extraction;

static char *pasteboard=NULL;

static char *define_pasteboard(GtkWidget *parent){
  static char *homedir=NULL;
  int len;
  struct stat h_stat;

  if (homedir) free(homedir);
  
  len = strlen ((char *) getenv ("HOME")) + 
	  strlen ("/.xfce/") + strlen (XFTREE_PASTEBOARD) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
failed:
    if (parent) xf_dlg_error(parent,strerror(errno),
		    _("Cannot create pasteboard\n"));
    return NULL;
  }
  /* if .xfce directory isnot there, create it. */
  snprintf (homedir, len, "%s/.xfce", (char *) getenv ("HOME"));
  if (stat(homedir,&h_stat) < 0){
	if (errno!=ENOENT) goto failed;
	if (mkdir(homedir,0770) < 0) goto failed;
  }

  snprintf (homedir, len, "%s/.xfce/%s", 
		  (char *) getenv ("HOME"), XFTREE_PASTEBOARD);
  return homedir;
}

void cb_clean_pasteboard(GtkWidget * widget, GtkCTree * ctree){
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (!pasteboard) pasteboard=define_pasteboard(win->top);
  if (pasteboard) unlink(pasteboard);
  return;
}

static void copy_cut(GtkWidget * widget, GtkCTree * ctree,gboolean cut)
{
  GtkCTreeNode *node;
  GList *selection;
  entry *en;
  FILE *pastefile;
  int i=0;

  cb_clean_pasteboard(widget, ctree);
  if (!pasteboard){
	  fprintf(stderr,"dbg: unable to define pasteboard\n");
	  return;
  }
  
  if ((pastefile=fopen(pasteboard,"w"))==NULL) {
       cfg *win;
       win = gtk_object_get_user_data (GTK_OBJECT (ctree));
       xf_dlg_error(win->top,strerror(errno),pasteboard);
       return;
  }
  
  fprintf(pastefile,"*** %s ***\n",(cut)?"cut":"copy");
 
  if (!(g_list_length (GTK_CLIST (ctree)->selection))) return;
  
  for (selection=GTK_CLIST (ctree)->selection;selection;selection=selection->next){
    node = selection->data;
    en = gtk_ctree_node_get_row_data (ctree, node);
    if (!io_is_valid (en->label) || (en->type & FT_DIR_UP)) continue;
    i++;
    /*fprintf(pastefile,"file:%s\r\n",en->path);*/
    fprintf(pastefile,"%s\n",en->path);
  }
  fclose(pastefile);
  if (!i) unlink(pasteboard);
  gtk_ctree_unselect_recursive (GTK_CTREE (ctree), NULL);
}

void cb_copy(GtkWidget * widget, GtkCTree * ctree){
   copy_cut(widget,ctree,FALSE);
}

void cb_cut(GtkWidget * widget, GtkCTree * ctree){
   copy_cut(widget,ctree,TRUE);
}

/* this is equivalent to copying by dnd */
void cb_paste(GtkWidget * widget, GtkCTree * ctree){
  FILE *pastefile;
  char *tmpfile,*texto,*word,*path;
  cfg *win;
  struct stat f_stat;
  entry *t_en;
  int i,num;
  GList *list;
  gboolean cut;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (!pasteboard) pasteboard=define_pasteboard(win->top);
  if (!pasteboard) {
	  fprintf(stderr,"dbg: unable to define pasteboard\n");
	  return;
  }

  path=valid_path(ctree,TRUE);
  
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))>1) 
  
  if (!(pastefile=fopen(pasteboard,"r"))) {
pasteboard_is_empty:
	  xf_dlg_info(win->top,_("The pasteboard is currently empty."));
	  return;
  }

  /* stat pastefile to get size.*/ 
  if (stat(pasteboard,&f_stat)<0) {
  	fprintf(stderr,"dbg:cb_paste(), this shouldn't happen\n");
	return; 
  }
  
  if (!(pastefile=fopen(pasteboard,"r"))) return;

  texto=(char *)malloc((long long)f_stat.st_size+1);
  if (fread(texto,(size_t)f_stat.st_size,1,pastefile)!=1){
  	fprintf(stderr,"dbg:error reading pasteboard\n");
	fclose(pastefile);
	return;
  }
  fclose(pastefile);
  
  texto[(long long)f_stat.st_size]=0;
  word=strtok(texto,"\n");
  if (!word) {
  	/*fprintf(stderr,"dbg:cb_paste(), this shouldn't happen\n");*/
	return; 
  }
  if (strstr(word,"*** cut ***")) cut=TRUE; else cut=FALSE;
  word=word+strlen(word)+1;
  if (!strlen(word)){
	 free(texto);
	 goto pasteboard_is_empty;
  }
	 
  /* create list to send to CreateTmpList */
  i = uri_parse_list (word, &list);
  free(texto);
  if (!i) return;
  /* create a tmpfile */
  t_en = entry_new_by_path(path);
  tmpfile=CreateTmpList(win->top,list,t_en);
  entry_free(t_en);
  
  /*fprintf(stderr,"dbg:tmpfile=%s\n",tmpfile);*/

   if (tmpfile) {
     if (tar_extraction){
	    DirectTransfer((GtkWidget *)ctree,(cut)?TR_MOVE:TR_COPY,tmpfile);
     } else {
	  if ((cut)&&(on_same_device())) DirectTransfer((GtkWidget *)ctree,TR_MOVE,tmpfile);
	  else IndirectTransfer((GtkWidget *)ctree,(cut)?TR_MOVE:TR_COPY,tmpfile);
	  unlink(tmpfile); 
     } 
  }
  if (cut) unlink(pasteboard);
  update_timer(ctree);
  return;
}

void cb_paste_show(GtkWidget * widget, GtkCTree * ctree){
  FILE *pastefile;
  char *texto;
  cfg *win;
  struct stat f_stat;
  int i;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (!pasteboard) pasteboard=define_pasteboard(win->top);
  if (!pasteboard) {
	  fprintf(stderr,"dbg: unable to define pasteboard\n");
	  return;
  }
  
  if (!(pastefile=fopen(pasteboard,"r"))) {
	  xf_dlg_info(win->top,_("The pasteboard is currently empty."));
	  return;
  }

  /* stat pastefile to get size.*/ 
  if (stat(pasteboard,&f_stat)<0) {
  	fprintf(stderr,"dbg:cb_paste(), this shouldn't happen\n");
	return; 
  }
  
  texto=(char *)malloc((long long)f_stat.st_size+1);
  if (fread(texto,(long long)f_stat.st_size,1,pastefile)!=1){
  	fprintf(stderr,"dbg:error reading pasteboard\n");
	fclose(pastefile);
	return;
  }
  fclose(pastefile);
  texto[(long long)f_stat.st_size]=0;
  for (i=0;i<strlen(texto);i++) if (texto[i]=='\r') texto[i]=' ';

  clear_cat(widget,ctree);
  show_cat( _("Pasteboard contents:\n"));
  show_cat(texto);
  /*xf_dlg_new(win->top,_("Pasteboard contents:\n"),
                  texto,NULL,DLG_INFO|DLG_OK);*/
  free(texto);
  return;
}

