/*   xfsamba.c */

/*  an smb navigator for xfce desktop: requires the samba suite programs:
 *  smbclient and nmblookup (included in most linux distributions).
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
 *
 *  xfce modules used by xfsamba are copyright by Olivier Fourdan
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

#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <glob.h>
#include <time.h>
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
/* for pixmap creation routines : */
#include "xfce-common.h"
#include "fileselect.h"

/* local xfsamba includes : */
#define XFSAMBA_MAIN
#include "tubo.h"
#include "xfsamba.h"

/*#define DBG_XFSAMBA*/

#define LOCATION_SHARES      1
#define LOCATION_SERVERS     2
#define LOCATION_WORKGROUPS  3

/* global memory data, not to be jeopardized on forks: */
static unsigned char NMBpassword[XFSAMBA_MAX_STRING + 1];
static unsigned char NMBnetbios[XFSAMBA_MAX_STRING + 1];
static unsigned char NMBshare[XFSAMBA_MAX_STRING + 1];
static unsigned char NMBcommand[XFSAMBA_MAX_STRING + 1];
static char NMBserverIP[XFSAMBA_MAX_STRING + 1];

/* other private variables : */
static GList *items = NULL;
static int cual_chingao;
static int NMBfirst = 1;
static GtkCTreeNode *LastNode;
static GtkWidget *dialog, *dir_name_entry;

static void SMBLookup (unsigned char *servidor, int reload);
static void SMBprintTitles (void);
static void SMBForkOver (void);



void
xfsamba_abort (int why)
{
  gtk_main_quit ();
}

void
print_diagnostics (char *message)
{
  if (!message)
    return;
  gtk_text_insert (GTK_TEXT (diagnostics), NULL, NULL, NULL,
		   message, strlen (message));
}

void
print_status (char *mess)
{
  static char message[256];
  strncpy (message, mess, 255);
  message[255] = 0;
  if (strstr (message, "\n"))
    strtok (message, "\n");	/* chop chop */
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:");
  print_diagnostics (message);
  print_diagnostics ("\n");
#endif
  gtk_label_set_text ((GtkLabel *) statusline, message);
}

/* parse stderr sent by child for diagnostics: */
static int
parse_stderr (int n, void *data)
{
  char *line;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);
  return TRUE;
}

static gboolean
not_unique (void *object)
{
  if (object)
    {
      print_diagnostics ("DBG:Fork object not null!\n");
      return 1;
    }
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:comment=");
  print_diagnostics (selected.comment);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:share=");
  print_diagnostics (selected.share);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:dirname=");
  print_diagnostics (selected.dirname);
  print_diagnostics ("\n");
  if (selected.file)
    {
      print_diagnostics ("DBG:filename=");
      print_diagnostics (selected.filename);
      print_diagnostics ("\n");
    }
#endif
  SMBResult = SUCCESS;
  return 0;
}

/* function executed by several children after all pipes
*  timeouts and inputs have been set up */
static void
SMBClientFork (void)
{
  char *the_netbios;
  the_netbios =
    (char *) malloc (strlen (NMBnetbios) + strlen (NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);
#ifdef DBG_XFSAMBA
  fprintf (stderr, "DBG:smbclient %s -c \"%s\"\n", the_netbios, NMBcommand);
  fflush(NULL);sleep(1);
#endif

  execlp ("smbclient", "smbclient", the_netbios, "-U", NMBpassword, "-c",
	  NMBcommand, (char *) 0);
}


/* smb lookup particulars:**/

void
SMBCleanLevel2 (void)
{
  nmb_cache *cache;
  if (!thisN)
    return;
  if (!thisN->shares)
    return;
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:Cleaning second level cache\n");
#endif

  cache = thisN->shares;
  while (cache)
    {
      cache->visited = 0;
      if (!cache->textos[SHARE_NAME_COLUMN])
	smoke_nmb_cache (cache);
      cache = cache->next;
    }

}

#ifdef _xxx_IN_PROGRESS
void
SMBreload (void)
{
  char *line[3];
  /* is variable LastNode used anywhere?? */
  /* function should be called only if node is in 
     *  second level cache. This will requiere some
     *  tweaking when multiple selections are enabled */
  if (!NMBselectedNode)
    return;
  if (!gtk_ctree_node_get_text (ctree,
				(GtkCTreeNode *) NMBselectedNode, COMMENT_COLUMN, line))
    return;
  if (line[0][0] != '/')
    return;			/* return !directory */
  while (line[0][strlen (line[0]) - 1] == ' ')
    line[0][strlen (line[0]) - 1] = 0;

  sprintf (the_share, "%s", line[0] + 1);

  /* delete tree entries */
  /* this is wrong, it should only remove children */
  gtk_ctree_remove_node ((GtkCTree *) shares, NMBselectedNode);
  /* */
  SMBTreeNode = NMBselectedNode;
  SMBList ((gpointer) the_share);

}
#endif
void
SMBrefresh (unsigned char *servidor, int reload)
{
  SMBResult = SUCCESS;
  if (!headN)
    {
      static gboolean NMBmastersLookup (gpointer data);
      
      stopcleanup=TRUE;
      if (servidor)
	headN = push_nmbName (servidor);
      else
	{
	  NMBmastersLookup (NULL);
	  return;
	}
    }

  switch (reload)
    {
    case REFRESH:
      if (!servidor)
	{
	  SMBLookup (headN->server, FALSE);
	}
      else
	{
	  SMBprintTitles ();
	  SMBForkOver ();
	}
      break;
    case FORCERELOAD:
      SMBLookup (servidor, TRUE);
      break;
    case RELOAD:
    default:
      SMBLookup (servidor, FALSE);
      break;
    }
}

static void
SMBprintTitles (void)
{
  char message[256];
  gtk_clist_clear ((GtkCList *) shares);
  gtk_clist_clear ((GtkCList *) servers);
  gtk_clist_clear ((GtkCList *) workgroups);

  {
    nmb_history *currentH;
    if (items != NULL)
      {
	g_list_free (items);
	items = NULL;
      }
    currentH = thisH;
    while (currentH)
      {
	if (currentH->record) {
	  if (currentH->record->server)
	    items = g_list_append (items, currentH->record->server);
	  if (currentH->record->serverIP)
	    gtk_label_set_text ((GtkLabel *) locationIP,
			      currentH->record->serverIP);
	  else
	    gtk_label_set_text ((GtkLabel *) locationIP, "-");
	}
	currentH = currentH->previous;
      }
    if (items)
      gtk_combo_set_popdown_strings (GTK_COMBO (location), items);
    if ((thisH)&&(thisH->record)&&(thisH->record->serverIP))
      gtk_label_set_text ((GtkLabel *) locationIP, thisH->record->serverIP);
    else
      gtk_label_set_text ((GtkLabel *) locationIP, "");
  }
  if (thisN->server){
    sprintf (message, "%s : %s", thisN->server, _("Shares"));
    gtk_label_set_text ((GtkLabel *) sharesL, message);
    sprintf (message, "%s : %s", thisN->server, _("Links to other servers"));
    gtk_label_set_text ((GtkLabel *) serversL, message);
    sprintf (message, "%s : %s", thisN->server, _("Links to other workgroups"));
    gtk_label_set_text ((GtkLabel *) workgroupsL, message);
  }
}

static void
SMBprint (nmb_list * currentN)
{
  nmb_cache *cache;
  GtkCTreeNode *node;
  cache = currentN->shares;
  while (cache)
    {
      GdkPixmap *gPIXo, *gPIXc;
      GdkBitmap *gPIMo, *gPIMc;
      if ((cache->textos[COMMENT_COLUMN]) &&
	  (strncmp (cache->textos[COMMENT_COLUMN], "Printer", strlen ("Printer")) == 0))
	{
	  gPIXc = gPIXo = gPIX_print;
	  gPIMc = gPIMo = gPIM_print;
	}
      else
	{
	  gPIXo = gPIX_dir_open_lnk;
	  gPIXc = gPIX_dir_close_lnk;
	  gPIMo = gPIM_dir_open_lnk;
	  gPIMc = gPIM_dir_close_lnk;
	}
      if (cache->textos[SHARE_NAME_COLUMN]){
	    int *data;      
	    node=gtk_ctree_insert_node ((GtkCTree *) shares,
			       NULL, NULL, cache->textos, SHARE_COLUMNS,
			       gPIXc, gPIMc, gPIXo, gPIMo, FALSE, FALSE);
	    data=(int *)malloc(2*sizeof(int));
	    data[0]=data[1]=0;
    	    gtk_ctree_node_set_row_data_full ((GtkCTree *)shares,node, 
					data, node_destroy);
       }

      cache = cache->next;
    }
  cache = currentN->servers;
  while (cache)
    {
      gint row;
      row = gtk_clist_append ((GtkCList *) servers, cache->textos);
      gtk_clist_set_pixmap ((GtkCList *) servers, row, 0,
			    (cache->visited) ? gPIX_comp2 : gPIX_comp1,
			    (cache->visited) ? gPIM_comp2 : gPIM_comp1);
      cache = cache->next;
    }
  cache = currentN->workgroups;
  while (cache)
    {
      gint row;
      row = gtk_clist_append ((GtkCList *) workgroups, cache->textos);
      gtk_clist_set_pixmap ((GtkCList *) workgroups, row, 0,
			    (cache->visited) ? gPIX_wg2 : gPIX_wg1,
			    (cache->visited) ? gPIM_wg2 : gPIM_wg1);
      cache = cache->next;
    }
  if (SMBResult == SUCCESS)
    print_status (_("Query done."));
  if (SMBResult == FAILED)
    print_status (_("Query failed. Machine may be down."));

}


/* smb lookup generals:**/

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBForkOver (void)
{
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  SMBprint (thisN);
  thisN->loaded = 1;
  if (SMBResult == CHALLENGED)
    {
      print_status (_("Query password has been requested."));
      gtk_window_set_transient_for (GTK_WINDOW (passwd_dialog (1)),
				    GTK_WINDOW (smb_nav));
    }
  fork_obj = NULL;
  nonstop=FALSE;
}

/* function to process stdout produced by child */
static int
SMBparseLookup (int n, void *data)
{
  char *line;
  gchar *textos[SHARE_COLUMNS];
  static char *position[2];
  
  /* data is a static memory location */
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  if (strstr (line, "Connection") && strstr (line, "failed"))
    {
      cual_chingao = LOCATION_SHARES;
      SMBResult = FAILED;
      position[0] = line;
      position[1] = NULL;
    }
  if (strstr (line, "Access") && strstr (line, "denied"))
    {
      cual_chingao = LOCATION_SHARES;
      SMBResult = CHALLENGED;
      position[0] = line;
      position[1] = NULL;
    }
  if (strstr (line, "--------"))
    {
      char *buf;
      position[0] = strstr (line, "---");
      buf = strtok (position[0], " ");

      if (buf)
	{
	  buf = strtok (NULL, "\n");
	  if (buf)
	    position[1] = strstr (buf, "---");
	}
      return TRUE;
    }
  if (strlen (line) < 3)
    return TRUE;
  if (strstr (line, "Sharename") && strstr (line, "Comment"))
    {
      cual_chingao = LOCATION_SHARES;
      position[0] = position[1] = NULL;
      return TRUE;
    }
  if (strstr (line, "Server") && strstr (line, "Comment"))
    {
      cual_chingao = LOCATION_SERVERS;
      position[0] = position[1] = NULL;
      return TRUE;
    }
  if (strstr (line, "Workgroup") && strstr (line, "Master"))
    {
      cual_chingao = LOCATION_WORKGROUPS;
      position[0] = position[1] = NULL;
      return TRUE;
    }
  if (!position[0])
    return TRUE;
  if (strstr (line, "\n"))
    strtok (line, "\n");	/* chop */
  latin_1_readable (line);

  {
	  int i;
	  for (i=0;i<SHARE_COLUMNS;i++) textos[i] = "";
  }
  textos[SHARE_NAME_COLUMN] = position[0];
  if (!position[1]){ 
    textos[COMMENT_COLUMN]="*"; 
    if (cual_chingao==LOCATION_WORKGROUPS)textos[WG_MASTER_COLUMN]="*"; 
    if (cual_chingao==LOCATION_SERVERS)textos[SERVER_COMMENT_COLUMN]="*"; 
  } 
  else
    {
      *(position[1] - 1) = 0;
     textos[COMMENT_COLUMN]= position[1]; 
     if (cual_chingao==LOCATION_WORKGROUPS)textos[WG_MASTER_COLUMN]= position[1]; 
     if (cual_chingao==LOCATION_SERVERS)textos[SERVER_COMMENT_COLUMN]= position[1]; 
    }


  switch (cual_chingao)
    {
    case LOCATION_SHARES:
      thisN->shares = push_nmb_cache (thisN->shares, textos);
      break;
    case LOCATION_SERVERS:
      thisN->servers = push_nmb_cache (thisN->servers, textos);
      break;
    case LOCATION_WORKGROUPS:
      thisN->workgroups = push_nmb_cache (thisN->workgroups, textos);
      break;
    default:
      return TRUE;
    }

  return TRUE;
}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
SMBFork (void)
{
  if (strlen (NMBpassword))
    execlp ("smbclient", "smbclient", "-N", "-L",
	    NMBnetbios, "-U", NMBpassword, (char *) 0);
  else
    execlp ("smbclient", "smbclient", "-N", "-L", NMBnetbios, (char *) 0);
}

static void
SMBLookup (unsigned char *servidor, int reload)
{
  char message[256];
  if (fork_obj)
    {
      print_diagnostics ("DBG:fork object not NULL!\n");
      return;
    }
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
  SMBCleanLevel2 ();
  cual_chingao = 0;
  SMBResult = SUCCESS;
  thisN = headN;
  if (servidor)
    {
      while (thisN)		/* push into first level cache (if not there) */
	{
	  if (strcmp ((char *) (thisN->server), (char *) servidor) == 0)
	    {
	      break;
	    }
	  thisN = thisN->next;
	}
      if (!thisN)
	thisN = push_nmbName (servidor);
    }

  if (!thisN->netbios)
    {				/* this will enter if nmblookup was stopped */
      static gboolean NMBmastersLookup (gpointer data);
      clean_nmb ();
      NMBmastersLookup (NULL);
      return;
    }

  sprintf (message, "%s %s (%s)", _("Querying"),
	   thisN->server, thisN->netbios);
  print_status (message);
  sprintf (NMBnetbios, "%s", thisN->netbios);

  if (!thisN->password) {
      thisN->password=(char *) malloc(strlen(default_user)+1);
      strcpy(thisN->password,default_user);
  }
  strncpy(NMBpassword,thisN->password,XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING]=0;
/*  if (thisN->password)
    {
      sprintf (NMBpassword, "%s", thisN->password);
    }
  else NMBpassword[0]=0;*/

  if ((!thisH) || ((thisH) && (thisN != thisH->record)))
    {
      smoke_history (thisH);
      thisH = push_nmb_history (thisN);
    }
  SMBprintTitles ();

  {
    if ((reload) || (!thisN->loaded))
      {
#ifdef DBG_XFSAMBA
	print_diagnostics ("DBG:Reloading server shares.\n");
#endif
	thisN->shares = clean_cache (thisN->shares);
	thisN->servers = clean_cache (thisN->servers);
	thisN->workgroups = clean_cache (thisN->workgroups);
	fork_obj = Tubo (SMBFork, SMBForkOver, FALSE,
			 SMBparseLookup, parse_stderr);
      }
    else
      SMBForkOver ();		/* load from cache  instead */
  }


  return;
}

/* resolve netbios name into IP */
#include "xfsamba_lookup.c"
/* resolve master browser IP into netbios name */
#include "xfsamba_masterresolve.c"
/* lookup master browsers on network */
#include "xfsamba_masterlookup.c"
/* list share's functions */
#include "xfsamba_list.c"
/* download files */
#include "xfsamba_download.c"
/* upload files */
#include "xfsamba_upload.c"
/* create directory */
#include "xfsamba_mkdir.c"
/* remove (directory or file) */
#include "xfsamba_rm.c"
/* tar (directory or file) */
#include "xfsamba_tar.c"


#include "xfce-common.h"
#include "xpmext.h"
#include "icons/warning.xpm"


static void
on_ok_abort (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
  exit (1);
}

GtkWidget *
abort_dialog (char *message)
{
  GtkWidget *label, *button, *dialog, *pixmapwid;
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;


  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_widget_realize (dialog);

  pixmap = MyCreateGdkPixmapFromData (warning, dialog, &mask, FALSE);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), pixmapwid, FALSE,
		      FALSE, 0);
  gtk_widget_show (pixmapwid);

  label = gtk_label_new (_("Samba failure! Xfsamba could not find file:"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, NOEXPAND,
		      NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, NOEXPAND,
		      NOFILL, 0);
  gtk_widget_show (label);

  label =
    gtk_label_new (_
		   ("(please install Samba or else correct the PATH environment variable)"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, NOEXPAND,
		      NOFILL, 0);
  gtk_widget_show (label);

  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
		      button, EXPAND, NOFILL, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_ok_abort), (gpointer) dialog);
  gtk_widget_show (button);

  gtk_widget_show (dialog);


  return dialog;
}


gboolean sane (char *bin)
{
  char *spath, *path, *globstring;
  glob_t dirlist;

  /* printf("getenv=%s\n",getenv("PATH")); */
  if (getenv ("PATH"))
    {
      path = (char *) malloc (strlen (getenv ("PATH")) + 2);
      strcpy (path, getenv ("PATH"));
      strcat (path, ":");
    }
  else
    {
      path = (char *) malloc (4);
      strcpy (path, "./:");
    }

  globstring = (char *) malloc (strlen (path) + strlen (bin) + 1);

/* printf("path=%s\n",path);*/

  if (strstr (path, ":"))
    spath = strtok (path, ":");
  else
    spath = path;

  while (spath)
    {
      sprintf (globstring, "%s/%s", spath, bin);
/*	 printf("checking for %s...\n",globstring);*/
      if (glob (globstring, GLOB_ERR, NULL, &dirlist) == 0)
	{
	  /*       printf("found at %s\n",globstring); */
	  free (globstring);
	  globfree (&dirlist);
	  free (path);
	  return TRUE;
	}
      globfree (&dirlist);
      spath = strtok (NULL, ":");
    }

  gtk_window_set_transient_for (GTK_WINDOW (abort_dialog (bin)),
				GTK_WINDOW (smb_nav));
  gtk_main ();
  /*printf("samba failure: %s not found in PATH\n",bin); */
  exit (1);
}

#include "icons/xfsamba.xpm"

int
main (int argc, char *argv[])
{
  headN = thisN = NULL;
  thisH = NULL;
  fork_obj = NULL;
  selected.parent_node = selected.node = NULL;
  selected.comment = selected.share =
    selected.dirname = selected.filename = NULL;
  stopcleanup=TRUE;
  default_user=(char *)malloc(strlen("Guest%")+1);
  strcpy(default_user,"Guest%");
  xfce_init (&argc, &argv);
  /*
     signal(SIGHUP,finish);
     signal(SIGSEGV,finish);
     signal(SIGKILL,finish);
     signal(SIGTERM,finish);
   */
  create_smb_window ();
  set_icon (smb_nav, "Xfsamba", xfsamba_xpm);
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
  sane ("nmblookup");
  sane ("smbclient");
  gtk_timeout_add (500, (GtkFunction) NMBmastersLookup, NULL);
  gtk_main ();
  return (0);
}
