/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

/* functions to use tubo.c for listing contents of SMB shares */

/******* SMBlist (was SMDclient) */
#ifndef INCLUDED_BY_XFSAMBA_C

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "constant.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
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

#include "tubo.h"
#include "xfsamba.h"

#endif

static GtkCTreeNode *LastNode;

/* function to process stdout produced by child */
static int
SMBListStdout (int n, void *data)
{
  char *line;
  char *textos[SHARE_COLUMNS];
  char directorio[XFSAMBA_MAX_STRING];
  int i, filenamelen, caso = 0x01;
  char *pw;
  GdkPixmap *gPIX;
  GdkBitmap *gPIM;
  GtkCTreeNode *node;

  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);
  if (strstr (line, "ERRbadpw"))
  {				/* server has died */
    SMBResult = CHALLENGED;
    print_diagnostics ("DBG:");
    print_diagnostics (line);
/* here we must pop it from the cache!!!!! */
  }
  if (strlen (line) < 2)
    return TRUE;
  if (strstr (line, "  .   "))
    return TRUE;
  if (strstr (line, "  ..   "))
    return TRUE;
  if (strncmp (line, "  ", 2))
    return TRUE;
  /* ok. Now we have a line to process */
  /* client.c: "  %-30s%7.7s %8.0f  %s",filename,attr,size,asctime */
  /* asctime=25 */
  /* if (strlen(line) > 25+2+8+1+7) */

  pw = line + (strlen (line) - 1 - 25 - 2 - 8);
  while (pw[0] != ' ')
  {
    if (pw == line)
      break;
    pw--;
  }

  filenamelen = strlen (line) - strlen (pw) - 7;

  while (pw[0] == ' ')
  {
    if (pw[0] == 0)
      break;
    pw++;
  }


  /*filenamelen = strlen (line) - 25 - 2 - 8 - 1 - 7; */
  for (i = 0; i < SHARE_COLUMNS; i++)
    textos[i] = "";
  textos[SHARE_NAME_COLUMN] = line + 2;
  for (i = filenamelen + 1; i < filenamelen + 8; i++)
  {
    if (line[i] == 'D')
      caso ^= 0x08;
    if (line[i] == 'H')
      caso ^= 0x04;
    if (line[i] == 'R')
      caso ^= 0x02;
    line[i] = 0;
  }

  if (strstr (pw, "\n"))
    strtok (pw, "\n");		/* chop */

  /*textos[COMMENT_COLUMN] = pw; */
  if (strstr (pw, " "))
  {
    textos[SHARE_SIZE_COLUMN] = strtok (pw, " ");
    textos[SHARE_DATE_COLUMN] = pw + strlen (pw) + 1;
  }


  latin_1_readable (line);

  if (caso & 0x08)
  {
    if (strcmp (selected.dirname, "/") == 0)
    {
      sprintf (directorio, "/%s/%s", NMBshare, line + 2);
    }
    else
    {
      sprintf (directorio, "/%s%s/%s", NMBshare, selected.dirname, line + 2);
    }
    textos[COMMENT_COLUMN] = directorio;
    node = gtk_ctree_insert_node ((GtkCTree *) shares, (GtkCTreeNode *) selected.node, NULL, textos, SHARE_COLUMNS, gPIX_dir_close, gPIM_dir_close, gPIX_dir_open, gPIM_dir_open, FALSE, FALSE);
    /*return TRUE; */
  }
  else
  {
    /* here, use different icons or notify readonly or hidden... */
    gPIX = gPIX_page;
    gPIM = gPIM_page;		/* default */
    if (caso & 0x02)
    {
      gPIX = gPIX_rpage;
      gPIM = gPIM_rpage;
    }				/* readonly */
    if (caso & 0x04)
    {				/* hidden */
      if (caso & 0x02)
      {
	gPIX = gPIX_rdotfile;
	gPIM = gPIM_rdotfile;
      }				/* readonly */
      else
      {
	gPIX = gPIX_dotfile;
	gPIM = gPIM_dotfile;
      }
    }				/* hidden */
    node = gtk_ctree_insert_node ((GtkCTree *) shares, (GtkCTreeNode *) selected.node, NULL, textos, SHARE_COLUMNS, gPIX, gPIM, NULL, NULL, TRUE, FALSE);
  }

  {
    int *data;
    data = (int *) malloc (2 * sizeof (int));
    /*data[0]=data[1]=0; */
    data[0] = atoi (textos[SHARE_SIZE_COLUMN]);
    data[1] = 0;		/* to have date sorting work, must parse date into a time_t number */
    gtk_ctree_node_set_row_data_full ((GtkCTree *) shares, node, data, node_destroy);
  }

  return TRUE;
}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBListForkOver (void)
{
  /* no jalo para arreglar directorios: 
     gtk_ctree_sort_node ((GtkCTree *)shares,(GtkCTreeNode *)selected.node);
   */
  gtk_clist_thaw (GTK_CLIST (shares));
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  print_status (_("Retrieve done."));
  fork_obj = 0;
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Query password has been requested."));
    gtk_window_set_transient_for (GTK_WINDOW (passwd_dialog (1)), GTK_WINDOW (smb_nav));
    break;
  default:
    break;

  }
}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
SMBListFork (void)
{
  char *the_netbios;
  the_netbios = (char *) malloc (strlen (NMBnetbios) + strlen (NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);

  fprintf (stderr, "CMD: SMBclient fork: ");
  fprintf (stderr, "smbclient %s   %s   %s   %s   %s\n", the_netbios, "-U", "*******", "-c", NMBcommand);
  fflush (NULL);
  execlp ("smbclient", "smbclient", the_netbios, "-U", NMBpassword, "-c", NMBcommand, (char *) 0);
}


void
SMBList (void)
{
  stopcleanup = FALSE;
  if (not_unique (fork_obj))
  {
    return;
  }
  /* LastNode not used in this file! anywhere else? */
  LastNode = (GtkCTreeNode *) selected.node;

  if (strlen (selected.dirname) + strlen ("ls") + 4 > XFSAMBA_MAX_STRING)
  {
    print_diagnostics ("DBG: Max string exceeded!");
    print_status (_("List failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));

    return;

  }

  sprintf (NMBcommand, "ls \\\"%s\\\"*", selected.dirname);

  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  print_status (_("Retreiving..."));

  gtk_clist_freeze (GTK_CLIST (shares));
  fork_obj = Tubo (SMBListFork, SMBListForkOver, TRUE, SMBListStdout, parse_stderr);
  return;
}
