/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
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

/* functions to use tubo.c for mkdir */

/*******SMBmkdir******************/
static GtkWidget *dialog;

static char *new_dir = NULL;
static GtkWidget *dir_name_entry;

/* function executed after all pipes
*  timeouts and inputs have been set up */
static void
SMBmkdirFork (void)
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

static void
proceed_dir_name (GtkWidget * widget, gpointer data)
{
  int ok;
  ok = (int) ((long) data);
  if (ok)
  {
    void SMBmkdir_with_name (void);
    char *fileO;
    fileO = gtk_entry_get_text (GTK_ENTRY (dir_name_entry));
    if (fileO)
    {
      if (new_dir)
      {
	free (new_dir);
      }
      new_dir = (char *) malloc (strlen (fileO) + 1);
      strcpy (new_dir, fileO);
    }

    SMBmkdir_with_name ();
  }
  else
    new_dir = NULL;
  gtk_widget_destroy (dialog);
}

static void
dir_name_entry_keypress (GtkWidget * entry, GdkEventKey * event, gpointer data)
{
  if (event->keyval == GDK_Return)
    proceed_dir_name (NULL, (gpointer) ((long) 1));
  return;
}

static GtkWidget *
mkdir_name (char *remote_share, char *remote_dir)
{
  GtkWidget *button, *hbox, *label;
  char *ask;
  ask = _("Please provide a name for the new directory at");

  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_widget_realize (dialog);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (ask);
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (" //");
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (thisN->server);
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new ("/");
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (remote_share);
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (remote_dir);
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);


  gtk_widget_show (hbox);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  dir_name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), dir_name_entry, EXPAND, NOFILL, 0);
  gtk_entry_set_visibility ((GtkEntry *) dir_name_entry, TRUE);
  gtk_signal_connect (GTK_OBJECT (dir_name_entry), "key-press-event", GTK_SIGNAL_FUNC (dir_name_entry_keypress), NULL);
  gtk_widget_show (dir_name_entry);


  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (proceed_dir_name), (gpointer) ((long) 1));
  button = gtk_button_new_with_label ("Cancel");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (proceed_dir_name), (gpointer) ((long) 0));
  gtk_widget_show (dialog);
  gtk_widget_grab_focus (dir_name_entry);
  return dialog;
}




/* function to process stdout produced by child */
static int
SMBmkdirStdout (int n, void *data)
{
  char *line;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  if (strstr (line, "ERRDOS"))
  {				/* server has died */
    SMBResult = CHALLENGED;
  }
  print_status (line);

  return TRUE;
}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBmkdirForkOver (void)
{
  GtkCTreeNode *node;
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Directory creation failed. See diagnostics for details."));
    break;
  default:
    /* directory creation was successful: */
    {
      char *textos[SHARE_COLUMNS];
      int i;
      time_t fecha;
      fecha = time (NULL);
      print_status (_("Directory created."));
      for (i = 0; i < SHARE_COLUMNS; i++)
	textos[i] = "";
      textos[SHARE_NAME_COLUMN] = new_dir;
      textos[SHARE_SIZE_COLUMN] = "0";
      textos[SHARE_DATE_COLUMN] = ctime (&fecha);
      textos[COMMENT_COLUMN] = (char *) malloc (1 + strlen (selected.share) + strlen (selected.dirname) + 1 + strlen (new_dir) + 1);
      if (strcmp (selected.dirname, "/") == 0)
      {
	sprintf (textos[COMMENT_COLUMN], "/%s/%s", selected.share, new_dir);
      }
      else
      {
	sprintf (textos[COMMENT_COLUMN], "/%s%s/%s", selected.share, selected.dirname, new_dir);
      }
      node = gtk_ctree_insert_node ((GtkCTree *) shares, (GtkCTreeNode *) selected.node, NULL, textos, SHARE_COLUMNS, gPIX_dir_close, gPIM_dir_close, gPIX_dir_open, gPIM_dir_open, FALSE, FALSE);
      free (textos[COMMENT_COLUMN]);
      {
	int *data;
	data = (int *) malloc (2 * sizeof (int));
	data[0] = data[1] = 0;
	gtk_ctree_node_set_row_data_full ((GtkCTree *) shares, node, data, node_destroy);
      }

    }
    break;
  }
}


void
SMBmkdir_with_name (void)
{
  int i;
  animation (TRUE);
  cursor_wait (GTK_WIDGET (smb_nav));
  stopcleanup = FALSE;


  if (!new_dir)
  {
    print_status (_("Create directory cancelled."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;
  }

/* let's allow subdirectories, if user knows his way around */

  if (strlen (new_dir) + strlen (selected.dirname) + strlen ("mkdir") + 5 > XFSAMBA_MAX_STRING)
  {
    print_diagnostics ("DBG: Max string exceeded!");
    print_status (_("Create directory failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;

  }
  sprintf (NMBcommand, "mkdir \\\"%s\\%s\\\"", selected.dirname, new_dir);
  for (i = 0; i < strlen (NMBcommand); i++)
    if (NMBcommand[i] == '/')
      NMBcommand[i] = '\\';
  print_diagnostics (NMBcommand);
  print_diagnostics ("\n");


  fork_obj = Tubo (SMBmkdirFork, SMBmkdirForkOver, TRUE, SMBmkdirStdout, parse_stderr);
}

void
SMBmkdir (void)
{				/* data is a pointer to the share */

  if (!selected.dirname)
  {
    return;
  }

  if (not_unique (fork_obj))
  {
    return;
  }

  print_status (_("Creating dir..."));
  stopcleanup = FALSE;

  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  /* here, dialog to ask new dir name */
  gtk_window_set_transient_for (GTK_WINDOW (mkdir_name (NMBshare, selected.dirname)), GTK_WINDOW (smb_nav));

/* gtk_main() keeps rolling here. SMBmkdir_with_name() is
*  called to finish off */

  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  return;
}
