/*  dnd modules for xfsamba
 *  
 *  Copyright (C) 2002 Edscott Wilson Garcia under GNU GPL
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*  FIXME: upload not working if directory must be listed, broken pipe on many 
 *         uploads at once, no error message on failed up loads, tree is not being updated */

/* FIXME: put in dummy entry to get expanders for folders */
/* FIXME: directories cannot be uploaded by dnd */
/* FIXME: change all meesages for xf_dlg routines. */
/* FIXME: part 2... get drag src to send to xftree */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* for _( definition, it also includes config.h : */
#include "my_intl.h"
#include "constant.h"

/* local xfsamba includes : */
#undef XFSAMBA_MAIN
#include "xfsamba.h"
#include "xfsamba_dnd.h"
#include "uri.h"
#include "gtk_dlg.h"

extern void
select_share (GtkCTree * ctree, GList * node, gint column, gpointer user_data);
extern void
SMBDropFile (char *tmpfile);

/* this function is repeated in ../xftree/xtree_cpy.c */
char *randomTmpName(char *ext){
    static char *fname=NULL;
    int fnamelen;
    if (fname) g_free(fname);
    if (ext==NULL) fnamelen=strlen("/tmp/xfsamba.XXXXXX")+1;
    else fnamelen=strlen("/tmp/xfsamba.XXXXXX")+strlen(ext)+2;
    fname = (char *)malloc(sizeof(char)*(fnamelen));
    if (!fname) return NULL;
    sprintf(fname,"/tmp/xfsamba.XXXXXX");
    close(mkstemp(fname));
    if (ext) {
	    unlink(fname);
	    strcat(fname,"."); strcat(fname,ext);
    }
    return fname;
}

/* this function is almost repeated in ../xftree/xtree_cpy.c */

static int ok_input(GtkWidget *parent,char *source){
  struct stat t_stat;
  
  if (lstat (source, &t_stat) < 0) {
	if (errno != ENOENT) return xf_dlg_error_continue (parent,source, strerror (errno));
  }
  if (S_ISDIR(t_stat.st_mode)){ /* FIXME: do a recursive glob to get all files to upload */
        /*fprintf(stderr,"dbg:at okinput 1 \n");*/
	  return DLG_RC_SKIP;
  }
 
  /*fprintf(stderr,"dbg:%s->%s\n",source,target);*/
   if (S_ISFIFO(t_stat.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy FIFO") );
   }
   if (S_ISCHR(t_stat.st_mode)||S_ISBLK(t_stat.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy device file") );
   }
   if (S_ISSOCK(t_stat.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy socket") );
   }
 
  return DLG_RC_OK;
}


char  *CreateTmpList(GtkWidget *parent,GList *list,char *target){
    FILE *tmpfile;
    uri *u;
    char *w;
    static char *fname=NULL;
    int i,nitems=0;
    
    nitems=0;
    if ((fname=randomTmpName(NULL))==NULL) return NULL;
    if ((tmpfile=fopen(fname,"w"))==NULL) return NULL;
    for (;list!=NULL;list=list->next){
        u = list->data;
	/*src=u->url;*/

	/*fprintf(stderr,"dbg:target=%s\n",target);*/
	switch (ok_input(parent,u->url)){
		case DLG_RC_SKIP:
			/*fprintf(stderr,"dbg:skipping %s\n",s_en->path);*/		      		
			  break;
		case DLG_RC_CANCEL:  /* dnd cancelled */
			/*fprintf(stderr,"dbg:cancelled %s\n",s_en->path);*/
    			 fclose (tmpfile);
			 unlink (fname);
			 return NULL;
		default: 
			 nitems++;
			 /*fprintf(tmpfile,"%d:%s:%s\n",u->type,u->url,target);*/ 
			 if (!strchr(u->url,'/')) {
				 fclose(tmpfile);
				 unlink (fname);
				 return NULL;
			 }
			 w=strrchr(u->url,'/')+1;
			 for (i = 0; i < strlen (selected.dirname); i++) {
				 if (selected.dirname[i] == '/') selected.dirname[i] = '\\';
			 }
  			 fprintf(tmpfile,"put \"%s\" \"%s\\%s\";\n",u->url,selected.dirname,w);
			 /*fprintf(stderr,"put \"%s\" \"%s\\%s\";\n",u->url,selected.dirname,w);*/
			 fflush(NULL);
			 break;
	} 
    }
    fclose (tmpfile);
    /*fprintf(stderr,"dbg:nitems = %d\n",nitems);*/
    if (!nitems) {
       /*fprintf(stderr,"dbg:nitems = %d\n",nitems);*/
	    unlink(fname);
	    return NULL;
    }
    /*fprintf(stderr,"dbg: same device=%d\n",same_device);*/
    return fname;
}




/*
 * called if drop data will be received
 * signal: drag_data_received
 */
void
on_drag_data (GtkWidget * ctree, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, void *client)
{
  int mode = 0;
  uri *u;
  int row, col;
  int nitems, action;
  GtkCList *clist;
  char *tmpfile=NULL;
  GList *list=NULL;

#define DISABLE_DND
#ifdef DISABLE_DND
  return;
#endif
  
  while (gtk_events_pending()) gtk_main_iteration();
  if ((!ctree) || (data->length < 0) || (data->format != 8))
  {
    gtk_drag_finish (context, FALSE, FALSE, time);
    return;
  }
  clist = GTK_CLIST (ctree);
  action = context->action <= GDK_ACTION_DEFAULT ? GDK_ACTION_COPY : context->action;
  row = col = -1;
  y -= clist->column_title_area.height;
  gtk_clist_get_selection_info (clist, x, y, &row, &col);
  fprintf(stderr,"dbg: drop received. row=%d\n",row);
  
  switch (info)
  {
    case TARGET_XTREE_WIDGET:
    case TARGET_XTREE_WINDOW:
    case TARGET_STRING:
    case TARGET_URI_LIST:
    /*fprintf(stderr,"dbg:at dnd 1\n");*/
    if (action == GDK_ACTION_MOVE) mode = TR_MOVE;
    else if (action == GDK_ACTION_COPY) mode = TR_COPY;
    else return;
    /* do an unselect */
    gtk_ctree_unselect_recursive ((GtkCTree *)ctree, NULL);
    select_share ((GtkCTree *)ctree, (GList *) gtk_ctree_node_nth ((GtkCTree *)ctree,row), col, NULL);
    /* FIXME:if share could not be open, then abort drop */
 
    fprintf(stderr,"dbg: share=%s\n",selected.share);
    fprintf(stderr,"dbg: dir=%s\n",selected.dirname);
   
    nitems = uri_parse_list ((const char *) data->data, &list);
    if (!nitems) break; /* of course */
    uri_remove_file_prefix_from_list (list);
    /* tmpfile ==NULL means drop cancelled*/
    u = list->data;
    tmpfile=CreateTmpList(smb_nav,list,"FIXME(use smb tgt here)");
    if (!tmpfile) {
         /*fprintf(stderr,"dbg:null tmpfile\n");*/
	 break;
    }
    else fprintf(stderr,"dbg:tmpfile=%s\n",tmpfile);
    /*cursor_wait (GTK_WIDGET (ctree));*/
    SMBDropFile (tmpfile);
/*	    DirectTransfer(ctree,mode,tmpfile);*/
    
    list=uri_free_list (list);
    /*cursor_reset (GTK_WIDGET (ctree));    */
    
    break;
  default:
    break;
  }
  /*fprintf(stderr,"dbg:parent:runOver\n");*/
  gtk_drag_finish (context, TRUE, TRUE, time);
    
}

gboolean
on_drag_motion (GtkWidget * ctree, GdkDragContext * dc, gint x, gint y, guint t, gpointer data)
{
  GdkDragAction action;
   
#if 0
  /* enable this when drag get is enabled */
  {
  gboolean same;
  GtkWidget *source_widget;
  source_widget = gtk_drag_get_source_widget (dc);
  same = ((source_widget == ctree) ? TRUE : FALSE);
  if (same){
	  printf("same widget for dnd motion\n");
	  //return TRUE;
  } else {
	  printf("different widget for dnd motion\n");
	  //return FALSE;
  }
 }
#endif
 
 action = GDK_ACTION_COPY;
 /*printf("dbg:dc->actions=%d\n",dc->actions);*/


  if (dc->actions == GDK_ACTION_MOVE)			gdk_drag_status (dc, GDK_ACTION_MOVE, t);
  else if (dc->actions == GDK_ACTION_COPY)		gdk_drag_status (dc, GDK_ACTION_COPY, t);
  else if (dc->actions == GDK_ACTION_LINK)		gdk_drag_status (dc, GDK_ACTION_LINK, t); 
#if 0
 /* these two dont work in gtk12 */ 
  else if (dc->actions == GDK_ACTION_PRIVATE)		gdk_drag_status (dc, GDK_ACTION_ASK, t);  
  else if (dc->actions == GDK_ACTION_ASK)		gdk_drag_status (dc, GDK_ACTION_PRIVATE, t);  
#endif
  else if (dc->actions & action)			gdk_drag_status (dc, action, t);
  else							gdk_drag_status (dc, 0, t);
  /*fprintf(stderr,"dbg: drag motion done...\n");*/
 
 return (TRUE);
}

