/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

/* functions to use tubo.c for uploading SMB files */

/*******SMBPut******************/
/* function to process stdout produced by child */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <glob.h>
#include <time.h>
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

static char *fileUp;

/* function executed after all pipes
*  timeouts and inputs have been set up */
static void
SMBPutFork (void)
{
  char *the_netbios;
  the_netbios = (char *) malloc (strlen ((char *) NMBnetbios) + strlen ((char *) NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);
#ifdef DBG_XFSAMBA
  fprintf (stderr, "DBG:smbclient %s -c \"%s\"\n", the_netbios, NMBcommand);
  fflush (NULL);
  sleep (1);
#endif

  execlp ("smbclient", "smbclient", the_netbios, "-U", NMBpassword, "-c", NMBcommand, (char *) 0);
}


static int
SMBPutStdout (int n, void *data)
{
  char *line;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  if (strstr (line, "ERRDOS"))
  {				/* server has died */
    SMBResult = CHALLENGED;
  }
  print_diagnostics (line);

  return TRUE;
}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */

static void
SMBPutForkOver (void)
{
  GtkCTreeNode *node;
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("File upload failed. See diagnostics for reason."));
    break;
  default:
    /* upload was successful: */
    {
      time_t fecha;
      char sizeo[64];
      struct stat st;
      char *textos[SHARE_COLUMNS];
      off_t i, sizei = 0;
      print_status (_("Upload done."));
      for (i = 0; i < SHARE_COLUMNS; i++)
	textos[i] = "";

      textos[SHARE_NAME_COLUMN] = fileUp;

      if (lstat (fileUp, &st) == 0)
      {
	sprintf (sizeo, "%lld", (long long)st.st_size);
	textos[SHARE_SIZE_COLUMN] = sizeo;
	sizei = st.st_size;
      }
      else
      {
	textos[SHARE_SIZE_COLUMN] = "0";
      }
      textos[SHARE_DATE_COLUMN] = ctime (&fecha);

      textos[COMMENT_COLUMN] = _("Uploaded file.");
      node = gtk_ctree_insert_node ((GtkCTree *) shares, (GtkCTreeNode *) selected.node, NULL, textos, SHARE_COLUMNS, gPIX_page, gPIM_page, NULL, NULL, TRUE, FALSE);
      {
	off_t *data;
	data = (off_t *) malloc (2 * sizeof (off_t));
	data[0] = sizei;
	data[1] = 0;		/* for date. No problem here with time_t, but in list.c */
	gtk_ctree_node_set_row_data_full ((GtkCTree *) shares, node, data, node_destroy);
      }
    }
    break;

  }
  fork_obj = NULL;
}


void
SMBPutFile (void)
{
  glob_t dirlist;
  char *fileS, *dataO;
  int i;

  if (!selected.directory)
  {
    return;
  }
  if (not_unique (fork_obj))
  {
    return;
  }

  stopcleanup = FALSE;
  print_status (_("Uploading file..."));


  fileS = open_fileselect ("");
  if (!fileS)
  {
    print_status (_("File upload cancelled."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;
  }


  if (glob (fileS, GLOB_ERR, NULL, &dirlist) != 0)
  {
    globfree (&dirlist);
    my_show_message (_("Specified file does not exist"));
    print_status (_("Upload failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;
  }
  globfree (&dirlist);

  fileUp = fileS;
  while (strstr (fileUp, "/"))
    fileUp = strstr (fileUp, "/") + 1;

  if (strlen (fileUp) + strlen (selected.dirname) + strlen (fileS) + strlen ("put") + 5 > XFSAMBA_MAX_STRING)
  {
    print_diagnostics ("DBG: Max string exceeded!");
    print_status (_("Upload failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;

  }

  dataO = (char *) malloc (strlen (selected.dirname) + 1);

  strcpy (dataO, selected.dirname);
  for (i = 0; i < strlen (dataO); i++)
  {
    if (dataO[i] == '/')
    {
      dataO[i] = '\\';
    }
  }


  sprintf (NMBcommand, "put \"%s\" \\\"%s\\%s\\\"", fileS, dataO, fileUp);
  free (dataO);
  print_diagnostics (NMBcommand);
  print_diagnostics ("\n");


  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  fork_obj = Tubo (SMBPutFork, SMBPutForkOver, TRUE, SMBPutStdout, parse_stderr);
  return;
}
