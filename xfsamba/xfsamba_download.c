/* (c) 2001 Edscott Wilson Garcia GNU/GPL
* this file is included by xfsamba.c
* please see xfsamba.c for copyright notice 
* (touch xfsamba.c if modified) */

#define INCLUDED_BY_XFSAMBA_C
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
#endif
/* functions to use tubo.c for downloading SMB files */

/*******SMBGet******************/
/* function to process stdout produced by child */
static int
SMBGetStdout (int n, void *data)
{
  char *line;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  if (strstr (line, "ERRDOS"))
  {				/* server has died */
    SMBResult = CHALLENGED;
  }
  if (strstr (line, "Error opening local file"))
  {
    SMBResult = CHALLENGED;
  }
  print_diagnostics (line);

  return TRUE;
}


/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBGetForkOver (void)
{
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("File download failed. See diagnostics for reason."));
    break;
  default:
    /* upload was successful: */
    print_status (_("Download done."));
    break;

  }
  fork_obj = NULL;

}


void
SMBGetFile (void)
{
  char *fileS;
  static char *dataO = NULL;
  int i;

  stopcleanup = FALSE;
  if (!selected.filename)
  {
#ifdef DBG_XFSAMBA
    print_diagnostics ("DBG:No valid file selected\n");
#endif
    return;
  }
  if (not_unique (fork_obj))
  {
    return;
  }

  if (dataO)
  {
    free (dataO);
  }
  dataO = (char *) malloc (strlen (selected.filename) + strlen (selected.dirname) + 3);
  sprintf (dataO, "%s/%s", selected.dirname, selected.filename);

  print_status (_("Downloading file..."));

  for (i = 0; i < strlen (dataO); i++)
  {
    if (dataO[i] == '/')
    {
      dataO[i] = '\\';
    }
  }

  fileS = open_fileselect (selected.filename);
  if (!fileS)
  {
    print_status (_("File download cancelled."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;
  }
  if (strlen (dataO) + strlen (fileS) + strlen ("get") + 3 > XFSAMBA_MAX_STRING)
  {
    print_diagnostics ("DBG: Max string exceeded!");
    print_status (_("Download failed."));
    cursor_reset (GTK_WIDGET (smb_nav));
    animation (FALSE);
    return;

  }

/* done on file select:   while (the_dir[strlen (the_dir) - 1] == ' ')
	the_dir[strlen (the_dir) - 1] = 0;*/

  sprintf (NMBcommand, "get \\\"%s\\\" \"%s\"", dataO, fileS);

  print_diagnostics (NMBcommand);
  print_diagnostics ("\n");

  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  fork_obj = Tubo (SMBClientFork, SMBGetForkOver, TRUE, SMBGetStdout, parse_stderr);
  return;
}
