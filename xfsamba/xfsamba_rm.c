/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

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

/* functions to use tubo.c for rm and rmdir */

/*******SMBrmdir******************/

static GtkWidget *dialog;
/* function executed after all pipes
*  timeouts and inputs have been set up */
static void
SMBrmFork (void)
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


/* function to process stdout produced by child */
static int
SMBrmStdout (int n, void *data)
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
SMBrmForkOver (void)
{
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Remove failed. See diagnostics for details."));
    break;
  default:
    /* directory creation was successful: remove node from tree */
    eliminate2_cache (thisN->shares, selected.comment);
    gtk_ctree_remove_node ((GtkCTree *) shares, (GtkCTreeNode *) selected.node);
    print_status (_("Remove complete."));
    selected.directory = selected.file = FALSE;
    if (selected.share)
      free (selected.share);
    selected.share = NULL;
    if (selected.dirname)
      free (selected.dirname);
    selected.dirname = NULL;
    if (selected.filename)
      free (selected.filename);
    selected.filename = NULL;
    if (selected.comment)
      free (selected.comment);
    selected.comment = NULL;
    break;
  }
}


void
SMBrm (void)
{				/* data is a pointer to the share */
  int i;

  if ((!selected.filename) && (!selected.dirname))
  {
    return;
  }
  if (not_unique (fork_obj))
  {
    return;
  }
  stopcleanup = FALSE;

  print_status (_("Removing..."));


  if (!strncmp (selected.comment, "Disk", strlen ("Disk")))
  {
    my_show_message (_("Sorry, top level shares cannot be removed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    print_status (_("Remove cancelled."));
    return;
  }



  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  if (selected.directory)
  {
    sprintf (NMBcommand, "rmdir \\\"%s\\\"", selected.dirname);
  }
  else
  {				/* a file to remove */
    sprintf (NMBcommand, "cd \\\"%s\\\";del \\\"%s\\\"", selected.dirname, selected.filename);
  }

  for (i = 0; i < strlen (NMBcommand); i++)
    if (NMBcommand[i] == '/')
      NMBcommand[i] = '\\';

  print_diagnostics ("CMD: ");
  print_diagnostics (NMBcommand);
  print_diagnostics ("\n");

  {
    static GtkWidget *really_remove (void);
    gtk_window_set_transient_for (GTK_WINDOW (really_remove ()), GTK_WINDOW (smb_nav));
  }

  return;
}

static void
proceed_rm (GtkWidget * widget, gpointer data)
{
  int ok;
  ok = (int) ((long) data);
  if (ok)
  {
    fork_obj = Tubo (SMBrmFork, SMBrmForkOver, TRUE, SMBrmStdout, parse_stderr);
  }
  else
  {
    cursor_reset (GTK_WIDGET (smb_nav));
    animation (FALSE);
    print_status (_("Remove cancelled."));
  }
  gtk_widget_destroy (dialog);
}

static GtkWidget *
really_remove (void)
{
  GtkWidget *button, *hbox, *label;
  char *pathname;

  pathname = (char *) malloc (2 + strlen (thisN->server) + 1 + strlen (selected.share) + strlen (selected.dirname) + 1 + ((!selected.file) ? 0 : strlen (selected.filename)) + 1);

  sprintf (pathname, "//%s/%s", thisN->server, selected.share);
  if (strcmp (selected.dirname, "/") != 0)
  {
    strcat (pathname, selected.dirname);
  }
  if (selected.file)
  {
    strcat (pathname, "/");
    strcat (pathname, selected.filename);
  }

  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_widget_realize (dialog);



  label = gtk_label_new (_("Confirm remove?"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (pathname);
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);
  gtk_widget_show (hbox);


  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (proceed_rm), (gpointer) ((long) 1));

  button = gtk_button_new_with_label ("Cancel");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (proceed_rm), (gpointer) ((long) 0));
  gtk_widget_show (dialog);
  free (pathname);

  return dialog;
}
