/*
 * xtree_cpy.c: these are the copy routines for xftree. The design
 *              is quite different from Rasca's originals, but 
 *              some of Rasca's ideas remain, and the input/output is
 *              enhanced (ewg).
 * 
 * copywrite 1999-2001 under GNU/GPL
 * Edscott Wilson Garcia, 
 * Olivier Fourdan, 
 * Rasca, Berlin
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
#include <signal.h>
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
#include <sys/types.h>
#include <fcntl.h>
#include <glob.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "top.h"
#include "reg.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_get.h"
#include "gtk_prop.h"
#include "gtk_dnd.h"
#include "xfcolor.h"
#include "xfce-common.h"
#include "../xfsamba/tubo.h"
#include "xtree_cfg.h"
#include "xtree_dnd.h"
#include "xtree_misc.h"
#include "xtree_gui.h"
#include "xtree_cpy.h"
#include "xfce-common.h"
#include "xtree_mess.h"
#include "tubo.h"


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define X_OPT GTK_FILL|GTK_EXPAND|GTK_SHRINK

#define FORK_CHAR_LEN 255

#ifndef GLOB_TILDE
#define GLOB_TILDE 0x0
#endif
#ifndef GLOB_ONLYDIR
#define GLOB_ONLYDIR 0x0
#endif

static gboolean same_device=FALSE;
static void *rw_fork_obj;
static GtkWidget *cat,*info[3],*progress;
static gboolean I_am_child=FALSE,incomplete_target=FALSE;
static char *fork_target,*fork_source;


static int ok_input(GtkWidget *parent,char *target,entry *s_en);
static gboolean force_override=FALSE;
static int internal_rw_file(char *target,char *source,long int size);
static int rwStderr (int n, void *data);
static int rwStdout (int n, void *data);
static void rwForkOver (void);
static void set_innerloop(gboolean state);
static int process_error(int code);
static void cb_cancel (GtkWidget * w, void *data);

gboolean on_same_device(void){return same_device;}

/*
 * process_error sends the appropriate error text from
 * child process to parent for dialog creation.
 * code = return value from internal_rw_file()
 */
static int process_error(int code){
	char *message="runOver",*txt=NULL;
	switch (code){
	  case RW_ERRNO:
		  message=strerror(errno);
		  break;	
	  case RW_ERROR_MALLOC:
		  message=_("Insufficient system memory"); 
		  break;
	  case RW_ERROR_READING_SRC:
	  case RW_ERROR_OPENING_SRC:
	  case RW_ERROR_WRITING_SRC:
		  txt=fork_source; 
		  message=strerror(errno);
		  break;
	  case RW_ERROR_STAT_WRITE_TGT:
	  case RW_ERROR_WRITING_TGT:
	  case RW_ERROR_OPENING_TGT:
		  txt=fork_target;
		  message=strerror(errno); 
		  break;
	  case RW_ERROR_TOO_FEW:
		  txt=fork_target;
		  message=_("Too few bytes transferred ! Device full ?"); 
		  break;
	  case RW_ERROR_TOO_MANY:
		  txt=fork_target;
		  message=_("Too many bytes transferred !?"); 
		  break;
	  case RW_ERROR_STAT_READ_SRC:
		  txt=fork_source; 
		  message=_("Can't stat() file"); 
		  break;
	  case RW_ERROR_STAT_READ_TGT:
		  txt=fork_source; 
		  message=_("Can't stat() file"); 
		  break;
	  case RW_ERROR_CLOSE_TGT:
		  txt=fork_target;
		  message=_("Error closing file"); 
		  break;		
	  case RW_ERROR_CLOSE_SRC:
		  txt=fork_source;
		  message=_("Error closing file"); 
		  break;
	  case RW_ERROR_UNLINK_SRC:
		  txt=fork_source;
		  message=_("Error deleting"); 
		  break;
	  case RW_ERROR_FIFO:
		  txt=fork_source;
		  message=_("Can't copy FIFO"); 
		  break;
	  case RW_ERROR_DEVICE:
		  txt=fork_source;
		  message=_("Can't copy device file"); 
		  break;
	  case RW_ERROR_SOCKET:
		  txt=fork_source;
		  message=_("Can't copy socket"); 
		  break;
	}
	if (strstr(message,"\n")) strtok(message,"\n");
	if (I_am_child){
		fprintf(stdout,"child:%s %s\n",message,(txt)?txt:"*");
		fflush(NULL);
	} 
	return code;
}



/*FIXME: make it a char *, char * with a stat call (KISS)*/
char *mktgpath(entry *ten,entry *sen){
  static char *target=NULL;
  if (target) free(target);
  target=(char *)malloc((strlen(ten->path)+strlen(sen->label)+2)*sizeof(char));
  if (!target) target="malloc error: mktgpath()\n";
  if (EN_IS_DIR (ten))
  {
    /* if target is a directory add the filename to the new path */
    if (io_is_root (ten->path))
    {
      /* do not add a slash */
      sprintf (target, "%s%s", ten->path, sen->label);
    }
    else
    {
      sprintf (target, "%s/%s", ten->path, sen->label);
    }
  }
  else
  {
    sprintf (target, "%s", ten->path);
  }
  return target;
}


void set_override(gboolean state)
{force_override=state;return;}

static int nitems;

char  *CreateTmpList(GtkWidget *parent,GList *list,entry *t_en){
    static char *fname=NULL;
    char *target;
    int fnamelen;
    FILE *tmpfile;
    uri *u;
    entry *s_en;
    
    nitems=0;
    if (fname) free(fname);
    fnamelen=strlen("/tmp/xftree.9999.tmp")+1;
    srandom(time(NULL));
    fname = (char *)malloc(sizeof(char)*(fnamelen));
    if (!fname) return NULL;
    sprintf(fname,"/tmp/xftree.%d.tmp",(int)((9999.0/RAND_MAX)*random()));
    if ((tmpfile=fopen(fname,"w"))==NULL) return NULL;
	/* create tmp file, unique name*/
    
/*    same_device=FALSE;*/
    same_device=TRUE;
    for (;list!=NULL;list=list->next){
	struct stat s_stat,t_stat;
        u = list->data;
	/*fprintf(stderr,"dbg:url=%s\n",u->url);*/
	s_en = entry_new_by_path (u->url);
	if (!s_en) {
		/*fprintf(stderr,"dbg:s_en is NULL\n");*/
		continue;
	}
	target=mktgpath(t_en,s_en);
	
	if (stat(target,&t_stat)<0){  /* follow link stat() */
		char *route,*end;
		route=(char *)malloc(strlen(target)+1);
		if (route) {
		  strcpy(route,target);
		  end=strrchr(route,'/');
		  if (end) {
			if (end==route) end[1]=0; /* root directory */
			else end[0]=0;
		  	stat(route,&t_stat);
		  }
		  free(route);  
		}
	}
	lstat(s_en->path,&s_stat); /* stat() the link itself */

	/*fprintf(stderr,"dbg:target=%s\n",target);*/
	switch (ok_input(parent,target,s_en)){
		case DLG_RC_SKIP:
			/*fprintf(stderr,"dbg:skipping %s\n",s_en->path);*/
		      		
			  break;
		case DLG_RC_CANCEL:  /* dnd cancelled */
			/*fprintf(stderr,"dbg:cancelled %s\n",s_en->path);*/
			 entry_free(s_en); 
			 //cancel_drop(TRUE); 
    			 fclose (tmpfile);
			 unlink (fname);
			 return NULL;
		default: 
			 if (s_stat.st_dev != t_stat.st_dev) same_device=FALSE;
			 nitems++;
			 fprintf(tmpfile,"%d:%s:%s\n",u->type,s_en->path,target);
			 fflush(NULL);
			 break;
	} 
	entry_free(s_en); 	
    }
    fclose (tmpfile);
    /*fprintf(stderr,"dbg:nitems = %d\n",nitems);*/
    if (!nitems) {
	    unlink(fname);
	    return NULL;
    }
    /*fprintf(stderr,"dbg: same device=%d\n",same_device);*/
    return fname;
}

/* function to check input before
 * packing off to copy/move/link
 * */
static int ok_input(GtkWidget *parent,char *target,entry *s_en){
  struct stat t_stat,s_stat;
  char *source;
  gboolean target_exists=TRUE;
  
  source=s_en->path;
	/* check for valid source */
       /* fprintf(stderr,"dbg:at okinput %s->%s\n",s_en->path,target);*/
  if (EN_IS_DIRUP (s_en) || !io_is_valid (s_en->label)) return DLG_RC_CANCEL;
        /*fprintf(stderr,"dbg:at okinput 1 \n");*/
  if (stat (target, &t_stat) < 0) {
	if (errno != ENOENT) return xf_dlg_error_continue (parent,target, strerror (errno));
	else target_exists=FALSE;
  }
  if (lstat (source, &s_stat) < 0) {
	return xf_dlg_error_continue (parent,source, strerror (errno));
  }

  /*fprintf(stderr,"dbg:%s->%s\n",source,target);*/
  
  /*fprintf(stderr,"dbg:at okinput 2 \n");*/
  /* target and source are the same */
  if ((target_exists) && (t_stat.st_ino == s_stat.st_ino)) {
	  /*fprintf(stderr,"dbg:nonsense imput\n");*/
	  return DLG_RC_CANCEL;  
  }

  if (S_ISFIFO(s_stat.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy FIFO") );
  }
  if (S_ISCHR(s_stat.st_mode)||S_ISBLK(s_stat.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy device file") );
  }
  if (S_ISBLK(s_stat.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy socket") );
  }
 
  /*fprintf(stderr,"dbg:at okinput 3 \n");*/
  /* check for overwrite, both files and directories */
  if ((target_exists) && (!force_override)) {
	int rc;
        rc=xf_dlg_new(parent,override_txt(target,s_en->path),NULL,NULL,DLG_CANCEL|DLG_OK|DLG_SKIP|DLG_ALL);
        if (rc == DLG_RC_SKIP) 	 return DLG_RC_SKIP;
        if (rc == DLG_RC_CANCEL) return DLG_RC_CANCEL;
        if (rc == DLG_RC_ALL)	 {
		force_override=TRUE;
		return DLG_RC_OK;
	}
  }
   
  return DLG_RC_OK;
}

#define CHILD_FILE_LENGTH 64
#define MAX_LINE_SIZE (sizeof(char)*256)
static char child_file[CHILD_FILE_LENGTH];
static int  child_mode;

static gboolean SubChildTransfer(char *target,char *source){
	struct stat s_stat,t_stat;
	int i,rc;
	
	if (stat(target,&t_stat)<0){  /* follow link stat() */
		char *route,*end;
		route=(char *)malloc(strlen(target+1));
		if (route) {
		  strcpy(route,target);
		  end=strrchr(route,'/');
		  if (end==route) end[1]=0; /* root directory */
		  else end[0]=0;
		  stat(route,&t_stat);
		  free(route);
		}
	}
	lstat(source,&s_stat); /* stat() the link itself */
	
	/* recursivity, if src is directory:
	 * 1- mkdir tgt/src
	 * 2- glob src
	 * 3 foreach glob in globlist recall function.
	 * */

	if (S_ISDIR(s_stat.st_mode)) { /* directories fall in here, going recursive */
  		glob_t dirlist;
		char *globstring,*newtarget,*src;
		
		
		globstring = (char *)malloc(strlen(source)+3);
		if (!globstring) return FALSE;
		sprintf(globstring,"%s/*",source);
		/* create target dir */
		if (mkdir(target,(s_stat.st_mode|0700))<0){
			fprintf(stdout,"child:%s %s\n",strerror(errno),target);
			return FALSE;
		}
	  	/*fprintf(stderr,"dbg:dir created: %s\n",target);*/
		/* glob source dir */
		if (glob (globstring, GLOB_ERR | GLOB_TILDE, NULL, &dirlist) != 0) {
		  /*fprintf (stderr, "dbg:%s: no match\n", globstring);*/
			return TRUE;
		}
  		else for (i = 0; i < dirlist.gl_pathc; i++) {
		  if (strstr(dirlist.gl_pathv[i],"/")) src=strrchr(dirlist.gl_pathv[i],'/')+1;
		  else src = dirlist.gl_pathv[i];
		  newtarget=(char *)malloc(strlen(target)+strlen(src)+3);
		  if (!newtarget) {
			  free(globstring);
			  return FALSE;
		  }
		  sprintf(newtarget,"%s/%s",target,src);
		  /*fprintf(stderr,"dbg:dirlist: %s\n",dirlist.gl_pathv[i]);*/
		  if (!SubChildTransfer(newtarget,dirlist.gl_pathv[i])){
			/*fprintf(stderr,"dbg:dirlisterror: %s\n",dirlist.gl_pathv[i]);*/
	  		free(globstring);
		 	free(newtarget);
			return FALSE;
		  }
		  free(newtarget);
		}
		free(globstring);
		
		/* remove old directory */
		if ((child_mode & TR_MOVE) && (rmdir(source)<0)){
			process_error(RW_ERROR_WRITING_TGT);
			return FALSE;
		}
		return TRUE;		
	}
	
	if ((child_mode & TR_MOVE) && (s_stat.st_dev == t_stat.st_dev) ){
	   if (rename (source, target) < 0){
	     process_error(RW_ERROR_WRITING_TGT);
	     return FALSE;
	   } else return TRUE;
	}
	if (child_mode & TR_LINK){ 
		/*FIXME:never tested link function. 
		 * how the hell do you call this? 
		 * put it in help or something*/
	  if (symlink (source, target) < 0) {
	     process_error(RW_ERROR_WRITING_TGT);
	     return FALSE;
	  } else return TRUE;
	}
	if (S_ISFIFO(s_stat.st_mode)){
	   process_error(RW_ERROR_FIFO);
	   return FALSE;
	}
	if (S_ISCHR(s_stat.st_mode)||S_ISBLK(s_stat.st_mode)){
	   process_error(RW_ERROR_DEVICE);
	   return FALSE;
	}
	if (S_ISSOCK(s_stat.st_mode)){
	   process_error(RW_ERROR_SOCKET);
	   return FALSE;
	}	
        /* we have to copy the data by reading/writing  */
 	rc=process_error(internal_rw_file(target,source,s_stat.st_size));
        /* on any error, cancel whole operation:
	 * (which means the continue and cancel buttons
	 *  in the popup are the same thing for the time being) */	
        if (rc != RW_OK) return FALSE; 	
	if ((child_mode & TR_MOVE) && (unlink(source) < 0)) {
	   process_error(RW_ERROR_WRITING_SRC);
	   return FALSE;
	}
	
	if (chmod (target, s_stat.st_mode) < 0){
		return (RW_ERROR_STAT_WRITE_TGT);
	}
	/* wow. everything went ok. */
	return TRUE;
}

void finish (int sig)
{
  if (incomplete_target) unlink(fork_target);
  unlink(child_file);
  _exit(123);
}

static void ChildTransfer(void){
	FILE *tfile;
	char *line,*source,*target;
	int type,i;
	
	I_am_child=TRUE;
	signal(SIGTERM,finish);
	incomplete_target=FALSE;
	line=(char *)malloc(MAX_LINE_SIZE);
	if (!line) {
		process_error(RW_ERROR_MALLOC);
		_exit(123);
	}
	tfile=fopen(child_file,"r");
	if (!tfile) {
		process_error(RW_ERRNO);
		_exit(123);
	}
	i=0;
	while (fgets(line,MAX_LINE_SIZE-1,tfile) && !feof(tfile)){
		/*fprintf(stderr,"dbg:%s\n",line);*/
		type=atoi(strtok(line,":"));
		source=strtok(NULL,":");
		target=strtok(NULL,"\n");
		
		/*fprintf(stderr,"dbg:(%d)%s->%s\n",type,source,target);*/
		fprintf(stdout,"child:tgt-src:%s:%s\n",target,source);
		fprintf(stdout,"child:item:%d\n",i++);
		if (!SubChildTransfer(target,source)) break;

		/* child version of copy will cancel operation if
		 * any error occurs. This might happen 
		 * when the offending file is within a directory that has
		 * been dndped. te get around this, child must exec a xftree
		 * to do all copying, handle progress dialog, query on errors,
		 * and finally exit quietly witout ever opening a ctree 
		 * itself. Another way to make it work would be to fix the
		 * stdin function in tubo.c so that it works and establish 
		 * a two way communication bewteen parent and child. I prefer
		 * the first way because it would allow simultaneous copy operations
		 * from two or more independent drag and drops, and it would free
		 * the xftree window for the user before the copy finishes. (ewg)
		 * */
	}
	fclose(tfile);
	free (line);
	fflush(NULL);
	_exit(123);
}

gboolean IndirectTransfer(GtkWidget *parent,int mode,char *tmpfile) {
    GtkWidget *cpy_dlg=NULL;
    if (CHILD_FILE_LENGTH < strlen("/tmp/xftree.9999.tmp")+1){
		fprintf(stderr,"dbg:This is a serious mistake, I need %d bytes\n",
				strlen("/tmp/xftree.9999.tmp")+1);
	}		
	strncpy(child_file,tmpfile,CHILD_FILE_LENGTH);
	child_file[CHILD_FILE_LENGTH-1]=(char)0;
	child_mode = mode;
  	if (!cpy_dlg) cpy_dlg=show_cpy(parent,TRUE,mode);
        set_show_cpy_bar(0,nitems);
        /*fprintf(stderr,"dbg:about to fork with %s\n",tmpfile);*/
        rw_fork_obj = Tubo (ChildTransfer, rwForkOver, TRUE, rwStdout, rwStderr);
	/*fprintf(stderr,"dbg:call to innerloop from IndirectTransfer()\n");*/
	set_innerloop(TRUE);
       if (cpy_dlg) set_show_cpy_bar(nitems,nitems);
       show_cpy(parent,FALSE,mode);
       return TRUE;
}

/* function for non forked move on same device */
gboolean DirectTransfer(GtkWidget *parent,int mode,char *tmpfile) {
	FILE *tfile;
	char *line,*source,*target;
	int type,i;
	
    	/*fprintf(stderr,"dbg: at DirectTransfer\n");*/
	
	line=(char *)malloc(MAX_LINE_SIZE);
	if (!line) {
		xf_dlg_error(parent,strerror(errno),NULL);
		return FALSE;
	}
	tfile=fopen(tmpfile,"r");
	if (!tfile) {
		xf_dlg_error(parent,strerror(errno),tmpfile);
		return FALSE;
	}
	i=0;
	while (fgets(line,MAX_LINE_SIZE-1,tfile) && !feof(tfile)){
		/*fprintf(stderr,"dbg:%s\n",line);*/
		type=atoi(strtok(line,":"));
		source=strtok(NULL,":");
		target=strtok(NULL,"\n");
		
		/* moveit */
	        if (rename (source, target) < 0){
		  if (xf_dlg_error_continue(parent,strerror(errno),target)==DLG_RC_CANCEL)
	          	return FALSE;
		}
	}
	fclose(tfile);
	free (line);
	return TRUE; 
}

/* now using a dynamic buffer instead of static. */
/* function called by rw_file() 
 *
 * recursive function for directories
 * */
#define BUFFER_SIZE (8192)
static int internal_rw_file(char *target,char *source,long int size){
	int i,j=0,source_file,target_file,total_size=0;
	char *buffer;
	gboolean too_few=FALSE,too_many=FALSE;

	fork_target=target;
	fork_source=source;
	buffer=(char *)malloc(BUFFER_SIZE);
	/* allocate buffer */
	if (!buffer) return (RW_ERROR_MALLOC);
	/* open source */
	source_file=open(source,O_RDONLY);
	if (source_file < 0) {
		free(buffer);
		return (RW_ERROR_OPENING_SRC);
	}
	/* open target */
	target_file=open(target,O_WRONLY|O_TRUNC|O_CREAT);
	if (target_file < 0) {
		close (source_file);
		free(buffer);
		return (RW_ERROR_OPENING_TGT);
	}
	incomplete_target=TRUE;
	/* read/write loop */
	/* 
	 * interruption occurs with signal SIGTERM
	 * sent from parent process via the dialog
	 * in which case parent proceeds with cpyForkCleanup()
	 * Closing the source file is not necesary 
	 * because the child process will terminate, closing it then. 
	 * */
	while ((i = read (source_file,buffer,BUFFER_SIZE)) > 0){
		if ((j=write(target_file,buffer,i)) < 0) break;
		if (i > j){ too_few=TRUE; break;}
		if (i < j){ too_many=TRUE; break;}
		total_size += j;
		fprintf(stdout,"child:bytes:%d bytes\n",total_size);fflush(NULL);
		/* don't hog cpu: */
		usleep(5000);
	}
	free(buffer);
	if (close(source_file)<0){
		close(target_file);
		return (RW_ERROR_CLOSE_SRC);
	}
	if (close(target_file)<0){
		return (RW_ERROR_CLOSE_TGT);
	}
	incomplete_target=FALSE;

	if ((i<0) || (j<0) || (too_few) || (too_many))
	{
		if (unlink(source))return (RW_ERROR_UNLINK_SRC); 
		if (too_few) return (RW_ERROR_TOO_FEW);
		if (too_many) return (RW_ERROR_TOO_MANY);
		if (i<0) return (RW_ERROR_READING_SRC);
		if (j<0) return (RW_ERROR_WRITING_TGT);
	}
	
	if (total_size < size){
		return (RW_ERROR_TOO_FEW);
	}
	/* everything went OK */
	return (RW_OK);
}

/* FUNCTION start/stop inner gtk_main() */
static void set_innerloop(gboolean state){
	static gboolean innerloop=FALSE;
	if (state==innerloop) return;
	innerloop=state;
	/*fprintf(stderr,"dbg:innerloop %s\n",(state)?"started":"terminated");fflush(NULL);*/
	if (state) gtk_main();
	else gtk_main_quit();
}

/* function to process stderr produced by child */
static int rwStderr (int n, void *data){
  char *line;
  
  if (n) return TRUE; /* this would mean binary data */
  line = (char *) data;
  /*fprintf(stderr,"dbg (child):%s\n",line);*/
  return TRUE;
}


/* function to process stdout produced by child */
static int rwStdout (int n, void *data){
  char *line,*texto;
  int rc;
  
  if (n) return TRUE; /* this would mean binary data */
  line = (char *) data;
  /*fprintf(stderr,"dbg(rwStdout):%s\n",line);fflush(NULL);*/

  /* only process child specific lines: */  
  if (strncmp(line,"child:",strlen("child:"))!=0){
         /*fprintf(stderr,"dbg:discarding line\n");fflush(NULL);*/
	 return TRUE;
  }
/* clear to process next file */
  if (strncmp(line,"child:runOver",strlen("child:runOver"))==0) {
  	/*set_innerloop(FALSE);*/
	return TRUE;
  }
/* transferred bytes progress report */
  if (strncmp(line,"child:bytes:",strlen("child:bytes:"))==0) {
  	strtok(line,":");
  	strtok(NULL,":");
  	texto=strtok(NULL,"\n");
	if (texto) gtk_label_set_text (GTK_LABEL (info[2]),texto );
	return TRUE;
  }
 /* src-tgt update */
  if (strncmp(line,"child:tgt-src:",strlen("child:tgt-src:"))==0) {
	char *src,*tgt;
  	strtok(line,":");
  	strtok(NULL,":");
	tgt=strtok(NULL,":");
	src=strtok(NULL,"\n");
        if (tgt && src) set_show_cpy(tgt,src);
	return TRUE;
  }
 /* nitems update */
  if (strncmp(line,"child:item:",strlen("child:item:"))==0) {
  	strtok(line,":");
  	strtok(NULL,":");
        set_show_cpy_bar(atoi(strtok(NULL,"\n")),0);
	return TRUE;
  }
  
 /* anything else is a process_error() output: */
  strtok(line,":");
  texto=strtok(NULL,"\n");
  /*fprintf(stderr,"dbg(rwStdout error):%s\n",texto);fflush(NULL);*/
  /*set_dnd_status sets static variable in xtree_dnd to break loop;*/
/*  rc=xf_dlg_error_continue (cat,texto,NULL);*/
  rc=xf_dlg_error (cat,texto,NULL);
  if (rc==DLG_RC_CANCEL) cb_cancel(NULL,NULL);
  /*set_innerloop(FALSE);*/
  return TRUE;
}

/* function called when child is dead */
static void rwForkOver (void)
{
/*  cursor_reset (GTK_WIDGET (smb_nav));*/
  rw_fork_obj = NULL;
  /*fprintf(stderr,"dbg: call to innerloop from forkover()\n");fflush(NULL);*/
  set_innerloop(FALSE);
}

#if 0
/* function to be executed by the child process */
static void rwFork(void){
	process_error(internal_rw_file(fork_target,fork_source));
	fflush(NULL);
/*	sleep(1);*/
	_exit(123);
}

/* function to set up child fork.
 * target and source strings are copied
 * to global memory to avoid it being lost
 * */
int rw_file(char *target,char *source){
	/* strings copied to global memory for proper fork */
	strncpy(fork_source,source,FORK_CHAR_LEN);
	fork_source[FORK_CHAR_LEN]=(char)0;
	strncpy(fork_target,target,FORK_CHAR_LEN);
	fork_target[FORK_CHAR_LEN]=(char)0;
fprintf(stderr,"dbg:about to fork with %s->%s\n",fork_source,fork_target);
        rw_fork_obj = Tubo (rwFork, rwForkOver, TRUE, rwStdout, rwStderr);
	/* process keyboard events until copy is done: */
fprintf(stderr,"dbg:call to innerloop from rw_file()\n");
	set_innerloop(TRUE);
	return TRUE;
}
#endif

/* function to set source and target strings
 * in the dialog box.
 * */
void set_show_cpy(char *target,char *source){
	char line[256];
	if (!cat) return; /* abort if show_cpy not called yet */
	gtk_entry_set_text (GTK_ENTRY (info[0]), source);
	gtk_entry_set_text (GTK_ENTRY (info[1]), target);
	sprintf (line, _("%d bytes"), 0);
	gtk_label_set_text (GTK_LABEL (info[2]),line );
}

/* function to set the state of the progress bar
 * can send 0 for nitems if it's just an update */
void set_show_cpy_bar(int item,int nitems){
	float p;
	static int n;
	if (nitems) n=nitems;
	if (n < item) n=item;
	if (!cat) return; /* abort if show_cpy not called yet */
	/* this optionally sets the progress bar */
	p = ((float)item)/((float)n);
	gtk_progress_set_percentage((GtkProgress *)progress,p);	
}

/* function to clean up the mess after child process
 * has been sent a SIGTERM. (Might a unlink target be 
 * good here?)
 * */
static void cpyForkCleanup(void)
{
	/* break inner gtk_main() loop */
	/*fprintf(stderr,"dbg:call to innerloop from forkcleanup\n");*/
	
  	set_innerloop(FALSE);
	/* unlink(fork_target); */
}

/* this cancel will interrupt the copy in progress
 * and the copy loop. (status is what breaks the loop)
 * */
static void
cb_cancel (GtkWidget * w, void *data)
{
	/* kill the current copy process */
	rw_fork_obj = TuboCancel (rw_fork_obj, cpyForkCleanup);
	gtk_widget_destroy(cat);
	cat=NULL;
}

/* This creates the dialog progress box for copy/move
 * operations. 
 * FIXME: it is still bugged in the respect that the
 * source and target paths don't always fit into the
 * respective entry boxes
 * */
GtkWidget *show_cpy(GtkWidget *parent,gboolean show,int mode)
{
  static char *title=NULL,*tit="";
  GtkWidget *cancel, *label[2], *box, *table;
  
  if (!show) {
     if (cat) gtk_widget_destroy(cat);
     cat=NULL;
     return cat;     
  }

  if (cat) {
	  fprintf(stderr,"This never happens-- show_cat(): cat != NULL");
	  return cat;
  }
  
 
  cat = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT (cat), "destroy", GTK_SIGNAL_FUNC (cb_cancel), NULL);
 
  box = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->vbox), box, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (box), table, TRUE, TRUE, 0);
 
  label[0] = gtk_label_new (_("Source: "));
  gtk_table_attach (GTK_TABLE (table), label[0], 0, 1, 0, 1, X_OPT, 0, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label[0]), GTK_JUSTIFY_RIGHT);

  info[0] = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), info[0], 1, 2, 0, 1, X_OPT, 0, 0, 0);

  label[1] = gtk_label_new (_("Target: "));
  gtk_table_attach (GTK_TABLE (table), label[1], 0, 1, 1, 2, X_OPT, 0, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label[1]), GTK_JUSTIFY_RIGHT);

  info[1] = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), info[1], 1, 2, 1, 2, X_OPT, 0, 0, 0);

  info[2] = gtk_label_new (_("file x of x"));
  gtk_box_pack_start (GTK_BOX (box), info[2], TRUE, TRUE, 0);

  progress = gtk_progress_bar_new();
  gtk_progress_bar_set_bar_style  ((GtkProgressBar *)progress,GTK_PROGRESS_CONTINUOUS);  
  gtk_progress_bar_set_orientation((GtkProgressBar *)progress,GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_box_pack_start (GTK_BOX (box), progress, TRUE, TRUE, 0);

  cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->action_area), cancel, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (cb_cancel),NULL);

  gtk_widget_set_usize(cat,300,150);
  gtk_widget_show_all (cat);

  if (mode == TR_COPY)      tit = _("XFTree: Copy");	  
  else if (mode == TR_MOVE) tit = _("XFTree: Move");
  else if (mode == TR_LINK) tit = _("XFTree: Link");
  else printf("This never happens either.\n");
  title = (char *)malloc(sizeof(char)*(strlen (tit)+1));	  
  if (title) {
	sprintf (title, tit);
	gtk_window_set_title (GTK_WINDOW (cat), title);
        free(title);
  }

  
  gtk_widget_show (cat);
  gtk_widget_realize(cat);  
  /*gdk_flush();*/
  gtk_window_set_modal (GTK_WINDOW (cat), TRUE);
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (cat), GTK_WINDOW (parent));
  else fprintf(stderr,"This should not happen show_cat(): parent is null!!\n");

  return cat;

}

