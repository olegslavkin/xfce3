/* copywrite 2001 edscott wilson garcia under GNU/GPL 
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


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* this file is for processing certain common messages.
 * override warning to begin with */

#define BYTES "bytes"

char * override_txt(char *new_file,char *old_file)
{
  gboolean old_exists=FALSE;
  struct stat nst,ost;
  static char *message=NULL;
  char *ot=_("Override ?"),*otime=NULL,*ntime;
  char *with=_("with");
  int i,osize=0,nsize;
  
  if (message) {free (message);}
  if (lstat (new_file, &nst) == ERROR){
    fprintf(stderr,"this should never happen: override_txt()\n");
    return ot;
  }
  if (lstat (old_file, &ost) != ERROR){
    old_exists=TRUE;
    osize=1;
    i=ost.st_size;
    otime=(char *)malloc( strlen(ctime(&(ost.st_mtime))) + 1 );
    strcpy(otime,ctime(&(ost.st_mtime)) );
    while (i) {i = i/10; osize++;}
  }
  nsize=1;
  i=nst.st_size;
  while (i) {i = i/10; nsize++;}
  ntime=ctime(&(nst.st_mtime));
   
  i= 1 + strlen(ot) +1+ 
	  strlen(new_file) +1+ strlen(ntime) +1+ nsize +1+ strlen("bytes") + 1;
  if (old_exists) {
    i = i + 
	  + strlen(with) +1+
	  strlen(old_file) +1+ strlen(otime) +1+ osize +1+ strlen("bytes") + 1;
  }
  message=(char *)malloc(i);
  if (!message) {return ot;}
  if (old_exists){
	sprintf(message,"%s\n%s %s %ld %s\n%s\n%s %s %ld %s\n",ot,
			new_file,ntime,nst.st_size,BYTES,
			with,
			old_file,otime,ost.st_size,BYTES);
	free(otime);
  }
  else
	sprintf(message,"%s\n%s %s %ld %s\n",ot,
			new_file,ntime,nst.st_size,BYTES);
  return message;
}
			 
			  
			  
	  
  
  
  
     	   

	
