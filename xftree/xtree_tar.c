
/*
 * xtree_tar.c: general tar functions.
 *
 * Copyright (C) 
 *
 * Edscott Wilson Garcia 2002, for xfce project
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
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
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
#include "xtree_icons.h"
#include "tubo.h"

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

typedef struct tar_dir{
	GtkCTreeNode *node;
	char *name;
	struct tar_dir *next;
}tar_dir;

static tar_dir *headTar=NULL;

static tar_dir *push_tar_dir(GtkCTreeNode *node,char *name){
	struct tar_dir *p,*l=NULL;

		/*fprintf(stderr,"dbg:pushing %s\n",name);*/
	p=headTar;
	while (p){l=p; p=p->next;}
	p=(tar_dir *)malloc(sizeof(tar_dir));
	if (!p) return NULL;
	p->name=g_strdup (name);
	p->next=NULL;
	p->node=node;
	if (l) l->next=p; else headTar=p;
	return headTar;	
}

static GtkCTreeNode *find_tar_dir(char *name){
	struct tar_dir *p;
	p=headTar;
		/*fprintf(stderr,"dbg:looking for %s\n",name);*/
	if (!name) return NULL;
	while (p){
	       if (strcmp(p->name,name)==0) break;
		/*fprintf(stderr,"dbg:%s <-> %s\n",p->name,name);*/
       	       p=p->next;
	}
	if (p) {
		/*fprintf(stderr,"dbg:found parent at %s\n",p->name);*/
		return p->node;
	}
	return NULL;
}

static tar_dir *clean_tar_dir(void){
	struct tar_dir *p,*l;
	p=headTar;
	while (p){
	       l=p;	
       	       p=p->next;
	       g_free(l->name);
	       g_free(l);
	}
	return NULL;
}

/* dummy entry to get expander without expanding */
GtkCTreeNode *add_tar_dummy(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en){
   GtkCTreeNode *item;
   icon_pix pix;
   entry *en;
   gchar *text[COLUMNS];
   text[COL_NAME]=text[COL_DATE]=text[COL_SIZE]="";
   text[COL_MODE]=text[COL_UID]=text[COL_GID]="";
   if ((en = entry_new ())==NULL) return NULL;
   en->type =  FT_TARCHILD;
   en->label=g_strdup(".");
   en->path=g_strdup("tar:.");
   memcpy((void *)(&(en->st)),(void *)(&(p_en->st)),sizeof(struct stat));
   set_icon_pix(&pix,en->type,en->label);   
   item=gtk_ctree_insert_node (ctree,parent, NULL, text, SPACING, 
		  pix.pixmap,pix.pixmask,
		  pix.open,pix.openmask,
		  TRUE,FALSE);
   gtk_ctree_node_set_row_data_full (ctree,item,en,node_destroy);
   return (item);   
}

GtkCTreeNode *add_tar_tree(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en){
      GtkCTreeNode *s_item=NULL,*p_node;
      entry *d_en;
      FILE *pipe;
      char *cmd;
      gchar *text[COLUMNS],date[32],size[32],mode[12],uid[32],gid[32];
      cfg *win;

      win = gtk_object_get_user_data (GTK_OBJECT (ctree));
      text[COL_DATE]=date;
      text[COL_SIZE]=size;
      text[COL_MODE]=mode;
      text[COL_UID]=uid;
      text[COL_GID]=gid;

      cmd=(char *)malloc(strlen("tar -vtzZf --use-compress-program bunzip2 ")+strlen(p_en->path)+1);
      if (!cmd) return NULL;
      if (p_en->type & FT_GZ) sprintf (cmd, "tar -vtzf %s", p_en->path);
      else if (p_en->type & FT_COMPRESS ) sprintf (cmd, "tar -vtZf %s", p_en->path);
      else if (p_en->type & FT_BZ2 ) sprintf (cmd, "tar --use-compress-program bunzip2 -vtf %s", p_en->path);
      else sprintf (cmd, "tar -vtf %s", p_en->path);
      /*printf("dbg:%s\n",cmd);*/
      pipe = popen (cmd, "r");
      if (pipe) {
              icon_pix pix;  
	      char *p,*d,*u;
	      char line[256];
	      while (!feof(pipe) && fgets (line, 255, pipe)){
      		      if ((d_en = entry_new ())==NULL) {
			      pclose (pipe);
			      return NULL;
		      }
		      d_en->type =  FT_TARCHILD;
		      /*fprintf(stderr,"dbg:%s",line);*/
		      /* mode */
		      strcpy(text[COL_MODE],strtok(line," "));
		      d_en->st.st_mode=0;
		      if (strlen(line)>8) {
		       if (line[0]=='r')d_en->st.st_mode |= 0400; 
		       if (line[1]=='w')d_en->st.st_mode |= 0200; 
		       switch (*(line+2)){
			      case 'x':  d_en->st.st_mode |= 0100;break;
			      case 's':  d_en->st.st_mode |= 04100;break;
			      case 'S':  d_en->st.st_mode |= 04000;break;
		       }
		       if (line[3]=='r')d_en->st.st_mode |= 040; 
		       if (line[4]=='w')d_en->st.st_mode |= 020; 
		       switch (*(line+5)){
			      case 'x':  d_en->st.st_mode |= 010;break;
			      case 's':  d_en->st.st_mode |= 02010;break;
			      case 'S':  d_en->st.st_mode |= 02000;break;
		       }
		       if (line[6]=='r')d_en->st.st_mode |= 04; 
		       if (line[7]=='w')d_en->st.st_mode |= 02; 
		       switch (*(line+8)){
			      case 'x':  d_en->st.st_mode |= 01;break;
			      case 't':  d_en->st.st_mode |= 01001;break;
			      case 'T':  d_en->st.st_mode |= 01000;break;
		       }
		      }
		      /* uid/gid */
		      u=strtok(NULL," ");
		      /* size */
		      strcpy(text[COL_SIZE],strtok(NULL," "));
		      /* date */
		      strcpy(text[COL_DATE],strtok(NULL," "));
		      strcat(text[COL_DATE]," ");
		      strcat(text[COL_DATE],strtok(NULL," "));
		      /* name */
		      p=strtok(NULL,"\n");
		      if ((p[strlen(p)-1]=='/')||(p[strlen(p)-1]=='\\'))  d_en->type |=  FT_DIR;
		      else d_en->type |=  FT_FILE;
		      d_en->path =(char *)malloc(strlen("tar:")+strlen(p_en->path)+1+strlen(p)+1);
		      if (!d_en->path) {
			      pclose (pipe);
			      return NULL;
		      }
		      sprintf(d_en->path,"tar:%s:%s",p_en->path,p);
		      /* use either / or \ */
		      d=strrchr(d_en->path,':')+1;
		      if ((!(d_en->type &  FT_DIR))&&(strstr(d,"/")||strstr(d,"\\"))){
			      if (strstr(d,"/")) d=strrchr(d_en->path,'/')+1;
			      else if (strstr(d,"\\")) d=strrchr(d_en->path,'\\')+1;
		      } 				   
		      d_en->label=g_strdup (d);
		      if ((d_en->type &  FT_DIR)&&(strlen(d_en->label)>1)) 
			      d_en->label[strlen(d_en->label)-1]=0;
		      
		      if (win->preferences&ABREVIATE_PATHS) 
			      text[COL_NAME] = (d_en->type &  FT_DIR)?
				      abreviate(d_en->label):abreviateP(d_en->label);
		      else  text[COL_NAME] = d_en->label;
			      
		      strcpy(text[COL_GID],u);
		      strcpy(text[COL_UID],strtok(u,"/"));
		      if (strstr(text[COL_GID],"/"))text[COL_GID]=strrchr(text[COL_GID],'/')+1;	      

		      {
			      char *P_path,*d=NULL;
			      P_path=g_strdup (p);
			      if (d_en->type &  FT_DIR) {
			         if (P_path[strlen(P_path)-1]=='/') P_path[strlen(P_path)-1]=0;
				 else if (P_path[strlen(P_path)-1]=='\\') P_path[strlen(P_path)-1]=0;
			      }
			      if (strstr(P_path,"/")) d=strrchr(P_path,'/');
			      else if (strstr(P_path,"\\")) d=strrchr(P_path,'\\');
			      if (d) d[1]=0;
			      if (d_en->type &  FT_DIR){
			      }
			      p_node=find_tar_dir(P_path);
			      if (!p_node) p_node=parent;
			      g_free(P_path);
			      
		      }
	      	      set_icon_pix(&pix,d_en->type,d_en->label);
		      s_item=gtk_ctree_insert_node (ctree,p_node, NULL, text, SPACING, 
		  		pix.pixmap,pix.pixmask,pix.open,pix.openmask,
				(d_en->type &  FT_DIR)?FALSE:TRUE,FALSE);
		      if (s_item) {
		          if (d_en->type &  FT_DIR) headTar=push_tar_dir(s_item,p);
		          gtk_ctree_node_set_row_data_full (ctree, s_item, d_en, node_destroy);
			  /*fprintf(stderr,"subitem inserted\n");*/
		      }
	      }
	      pclose (pipe);
      }
      /*fprintf(stderr,"dbg:done inserting tar stuff\n");*/
      free(cmd);
      headTar=clean_tar_dir();
      gtk_ctree_sort_node (ctree, parent);
      return s_item;
}

#define TAR_CMD_LEN 1024
static char tar_cmd[TAR_CMD_LEN];
static char tar_tgt[TAR_CMD_LEN];
static gboolean tar_cmd_error;
static GtkWidget *tar_parent;
static GtkCTree *tar_ctree;
static void *tar_fork_obj=NULL;
static GtkCTreeNode *tar_node;

static void tubo_cmd(void){
	char *args[10];
	int i;
	int status;
	args[0]=strtok(tar_cmd," ");
	if (args[0]) for (i=1;i<10;i++){
		args[i]=strtok(NULL," ");
		if (!args[i]) break;
	}
	i=fork();
	if (!i){
	   execvp(args[0],args);
	   _exit(123);
	}
	wait(&status);
	_exit(123);
}

int tar_output;
static void tubo_cmdE(void){
	  char *args[10];
	  int i;
	  if (tar_output) {close(tar_output);tar_output=0;}
	  /*fprintf(stdout,"forked:%s\n",tar_cmd);*/
	  args[0]=strtok(tar_cmd," ");
	  if (args[0]) for (i=1;i<10;i++){
		args[i]=strtok(NULL," ");
		if (!args[i]) break;
  	  }
	  /*fprintf(stdout,"args:\n");*/
	  for (i=0;i<10;i++){
		  if (!args[i]) break;
		  fprintf(stdout,"%s ",args[i]);
	  }
	  /*fprintf(stdout,"\n");*/
	  if (args[0]) execvp(args[0],args);
	  _exit(123);
}

/* function to process stderr produced by child */
static int rwStderr (int n, void *data){
  char *line;
  
  if (n) return TRUE; /* this would mean binary data */
  line = (char *) data;
  /*fprintf(stderr,"dbg (child):%s\n",line);*/
  tar_cmd_error=TRUE;
  xf_dlg_error(tar_parent,line,NULL);
  return TRUE;
}
/* function to process stdout produced by child */
static int rwStdout (int n, void *data){
  char *line;
  if (!tar_output) return TRUE;
  if (n) {/* this would mean binary data */
	  write(tar_output,data, n);
	  /*write(1,data, n);*/
  } else{
	  line = (char *) data;
	  write(tar_output,data, strlen(line));
	  /*write(1,data, strlen(line));*/
  }
  return TRUE;
}

/* function called when child is dead */
static void rwForkOver (void)
{
  if (tar_cmd_error) {
    /*fprintf(stderr,"dbg fork is over with error\n");*/
  } else {
    /*fprintf(stderr,"dbg fork is over OK\n");*/
    if (tar_node) {
	gtk_ctree_unselect(tar_ctree,(GtkCTreeNode *)tar_node);
        gtk_ctree_remove_node (tar_ctree,(GtkCTreeNode *)tar_node);
        while (gtk_events_pending()) gtk_main_iteration();
    }
  }
  if (tar_output) {
	  close(tar_output);
	  tar_output=0;
  }
  tar_fork_obj=NULL;
}


static int
compare_node_path (gconstpointer ptr1, gconstpointer ptr2)
{
  entry *en1 = (entry *) ptr1, *en2 = (entry *) ptr2;

  return strcmp (en1->path, en2->path);
}

static int inner_tar_delete(GtkCTree *ctree,char *path){
	char *tar_file,*tar_entry;
	cfg *win;
	entry check;
	int type=0;

	/*fprintf(stderr,"dbg:del path=%s\n",path);*/
	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
	check.path=g_strdup(path);
	tar_node=gtk_ctree_find_by_row_data_custom (ctree, 
			GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list), 
			&check, compare_node_path);

	if (strncmp(check.path,"tar:",strlen("tar:"))!=0) {errno=EFAULT;return -1;}
	strtok(check.path,":");
	tar_file=strtok(NULL,":"); if (!tar_file){errno=EFAULT;return -1;}
	tar_entry=strtok(NULL,"\n"); if (!tar_entry){errno=EFAULT;return -1;}
	if (strlen("tar -zZ --use-compress-program bunzip2 --delete  -f ")+strlen(tar_file)+1+strlen(tar_entry)+1 >TAR_CMD_LEN){
		errno=EIO;return -1;
	}
	
	{
	  char *w;
	  w=strrchr(tar_file,'.');
      	  if ( (strcmp(w,".tgz")==0)||(strcmp(w,".gz")==0) ) type = FT_GZ;
	  else if (strcmp(w,".Z")==0)  type = FT_COMPRESS;
	  else if (strcmp(w,".bz2")==0) type = FT_BZ2; 
	}
	
	if (type & FT_GZ) sprintf (tar_cmd, "tar -zf %s --delete %s",tar_file,tar_entry);
	else if (type & FT_COMPRESS ) sprintf (tar_cmd, "tar -Zf %s --delete %s",tar_file,tar_entry);
	else if (type & FT_BZ2 ) sprintf (tar_cmd, "tar --use-compress-program bunzip2 -f %s --delete %s",tar_file,tar_entry);
        else  sprintf (tar_cmd, "tar -f %s --delete %s",tar_file,tar_entry);
        /*printf("dbg:%s\n",tar_cmd);*/

	tar_cmd_error=FALSE;
	tar_parent=win->top;
	tar_ctree=ctree;
	tar_fork_obj=Tubo (tubo_cmd, rwForkOver, 0, rwStderr, rwStderr);
	usleep(50000);
	g_free(check.path);
        return 0;
	
}

int tar_delete(GtkCTree *ctree,char *path){
	while (1){
		if (!tar_fork_obj){
			return inner_tar_delete(ctree,path);
			break;
		}
                while (gtk_events_pending()) gtk_main_iteration();
		usleep(50000);
	}
	return 0;
}

int tar_extract(GtkCTree *ctree,char *tgt,char *src){
	char *tar_file,*tar_entry,*loc;
	entry *en;
	cfg *win;
	entry check;
	mode_t mode=S_IRUSR|S_IWUSR|S_IRGRP;

	tar_ctree=ctree;
	
	while (tar_fork_obj){
                while (gtk_events_pending()) gtk_main_iteration();
		usleep(50000);
	}
	if (src[strlen(src)-1]=='\n') src[strlen(src)-1]=0;
	if (src[strlen(src)-1]=='\r') src[strlen(src)-1]=0;
	/*fprintf(stderr,"tgt=%s\n",tgt);*/
	/*fprintf(stderr,"src=%s\n",src);*/

	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
/* FIXME: mode setting dont always work because the search might be done in the wrong xftree window. 
 *        this is not the case with a "delete" command, from where the code was taken */
	check.path=g_strdup(src);
	tar_node=gtk_ctree_find_by_row_data_custom (ctree, 
			GTK_CTREE_NODE (GTK_CLIST (ctree)->row_list), 
			&check, compare_node_path);
        en = gtk_ctree_node_get_row_data (GTK_CTREE (tar_ctree), tar_node);
	if (en) mode= en->st.st_mode;
	/*
	if (!en) fprintf(stderr,"dbg:entry not found\n");
	else fprintf(stderr,"dbg:mode=0%o\n",en->st.st_mode);*/

	tar_entry=strrchr(src,':');
	if (!tar_entry) return FALSE;
	*tar_entry='+';	
	tar_file=strrchr(src,':');
	if (!tar_file) return FALSE;
	tar_file++;
	*tar_entry=0;
	tar_entry++;
	
	if (strlen("tar --use-compress-program bunzip2 -OZ -f %% -x %%")+strlen(tar_file)+1+strlen(tar_entry)+1 >TAR_CMD_LEN){
		errno=EIO;return 0;
	}
        loc=strrchr(tar_file,'.');
	if (!loc) { 
		return FALSE;
	}
	
	if ( (strcmp(loc,".tgz")==0)||(strcmp(loc,".gz")==0) )
		sprintf(tar_cmd,"tar -Oz -f %s -x %s ",tar_file,tar_entry);
	else if (strcmp(loc,".Z")==0)	
		sprintf(tar_cmd,"tar -OZ -f %s -x %s ",tar_file,tar_entry);
	else if (strcmp(loc,".bz2")==0)	
		sprintf(tar_cmd,"tar -O --use-compress-program bunzip2 -f %s -x %s ",tar_file,tar_entry);
	else
		sprintf(tar_cmd,"tar -O -f %s -x %s ",tar_file,tar_entry);
	/*fprintf(stderr,"%s\n",tar_cmd);*/
	/*fprintf(stderr,"tgt=%s\n",tgt);*/
	
        if ((tar_output=creat(tgt,mode))<0){
		  fprintf(stderr,"open:%s(%s)\n",strerror(errno),tgt);
		  return FALSE;
	}	  
	strcpy(tar_tgt,tgt);	  
	tar_fork_obj=Tubo (tubo_cmdE, rwForkOver, 0, rwStdout, rwStderr);
	usleep(50000);
        return TRUE;
}



