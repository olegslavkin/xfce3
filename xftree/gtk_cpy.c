/*
 * gtk_cpy.c
 *
 * Copyright (C) 1998,1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include "my_intl.h"
#include "uri.h"
#include "top.h"
#include "entry.h"
#include "io.h"
#include "gtk_cpy.h"
#include "gtk_dlg.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* #define X_OPT 0 */
#define X_OPT GTK_FILL|GTK_EXPAND|GTK_SHRINK

#define ST_CANCEL	2
#define ST_OK		1

#define ERROR		-1
#define BUFLEN 8192

typedef struct
{
  GtkWidget *w;
  int *status;
}
state;

/*
 * callback if source ctree is destroyed
 * so we do not have to update the labels
 */
void
transfer_view (GtkWidget * s_ctree, int *status)
{
  printf (_("transfer_view() %d\n"), *status);
  *status = 0;
}


/*
 * callback if cancel button was pressed
 */
static void
cb_cancel (GtkWidget * w, void *data)
{
  state *s = (state *) data;
  if (s->w)
  {
    gtk_widget_destroy (s->w);
  }
  *s->status = ST_CANCEL;
}

/*
 * copy/move a file and update the dialog box
 */
int
trans_file (entry * sen, entry * ten, int mode, GtkWidget ** info, state * st)
{
  char target[PATH_MAX + 1];
  char lnk[PATH_MAX + 1];
  FILE *ofp, *nfp;
  int len, all, rc;
  char buff[BUFLEN], transfered[32];
  char *string;
  struct stat s_stat, t_stat;

  if (EN_IS_DIRUP (sen) || (!io_is_valid (sen->label)) || (sen->type & FT_DIR_UP))
    return (FALSE);
  if (EN_IS_DIRUP (ten) || (!io_is_valid (ten->label)) || (ten->type & FT_DIR_UP))
    return (FALSE);

  if (mode == TR_MOVE)
  {
    string = _("move");
  }
  else if (mode == TR_COPY)
  {
    string = _("copy");
  }
  else if (mode == TR_LINK)
  {
    string = _("link");
  }
  else
  {
    string = "?";
  }


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

  gtk_entry_set_text (GTK_ENTRY (info[0]), sen->path);
  gtk_entry_set_text (GTK_ENTRY (info[1]), target);
  sprintf (transfered, _("%d bytes"), 0);
  gtk_label_set_text (GTK_LABEL (info[2]), transfered);

  if (lstat (target, &t_stat) != ERROR)
  {
    /* check if they are the same files */
    if (t_stat.st_ino != sen->inode)
    {
      rc = dlg_ok_skip (_("Override file?"), target);
      if (rc == DLG_RC_SKIP)
	return (TRUE);
      if (rc == DLG_RC_CANCEL)
	return (FALSE);
    }
  }
  else if (lstat (ten->path, &t_stat) == ERROR)
  {
    return dlg_continue (_("Can't stat() file"), ten->path);
  }

  if (lstat (sen->path, &s_stat) == ERROR)
  {
    return dlg_continue (_("Can't stat() file"), sen->path);
  }
#if 0
  /* If src and dest aren't on the same device, ask user */
  if (s_stat.st_dev != t_stat.st_dev)
  {
    char msg[DLG_MAX];
    sprintf (msg, _("Do you want to %s the item(s) to"), string);
    if (dlg_question (msg, ten->path) != DLG_RC_OK)
    {
      return (TRUE);
    }
  }
#endif
  if (mode & TR_MOVE)
  {
    /* first check if the files are on the
     * same device - this would be a simple situation :)
     */

    /* we copy/move symbolic links as links, so stat() the link
     * itself
     */
    if (s_stat.st_dev == t_stat.st_dev)
    {
      /* are on the same device, so we have just to rename
       */
      if (rename (sen->path, target) == ERROR)
      {
	return dlg_continue (sen->path, strerror (errno));
      }
      return (TRUE);
    }
  }
  else if (mode & TR_LINK)
  {
    if (symlink (sen->path, target) == -1)
    {
      return dlg_continue (lnk, strerror (errno));
    }
    return (TRUE);
  }
  /* symbolic links are copied as they are
   */
  if (EN_IS_LINK (sen))
  {
    len = readlink (sen->path, lnk, PATH_MAX);
    if (len <= 0)
    {
      perror (_("readlink()"));
      return (0);
    }
    lnk[len] = '\0';
    if (symlink (lnk, target) == -1)
    {
      return dlg_continue (lnk, strerror (errno));
    }
    if (mode & TR_MOVE)
    {
      if (unlink (sen->path) == ERROR)
      {
	return dlg_continue (sen->path, strerror (errno));
      }
    }
    return (TRUE);
  }
  /* we can't copy device files */
  if (EN_IS_DEVICE (sen))
  {
    return dlg_continue (sen->path, _("Can't copy device file"));
  }
  /* we can't copy fifo files */
  if (EN_IS_FIFO (sen))
  {
    return dlg_continue (sen->path, _("Can't copy FIFO"));
  }
  if (EN_IS_SOCKET (sen))
  {
    return dlg_continue (sen->path, _("Can't copy socket"));
  }

  /* we have to copy the data by reading/writing
   */
  ofp = fopen (sen->path, "rb");
  if (!ofp)
  {
    return dlg_error_continue (sen->path, strerror (errno));
  }
  nfp = fopen (target, "wb");
  if (!nfp)
  {
    fclose (ofp);
    return dlg_error_continue (target, strerror (errno));
  }
  all = 0;
  while ((len = fread (buff, 1, BUFLEN, ofp)) > 0)
  {
    fwrite (buff, 1, len, nfp);
    all += len;
    sprintf (transfered, _("%d bytes"), all);
    gtk_label_set_text (GTK_LABEL (info[2]), transfered);
    if (*st->status == ST_CANCEL)
    {
      fclose (nfp);
      fclose (ofp);
      unlink (target);
      return (FALSE);
    }
  }
  fclose (nfp);
  fclose (ofp);
  if (all < sen->size)
  {
    dlg_error (_("Too few bytes transferred ! Device full ?"), target);
  }
  else if (all > sen->size)
  {
    dlg_error (_("Too many bytes transferred !?"), target);
  }
  if (stat (sen->path, &s_stat) != ERROR)
  {
    chmod (target, s_stat.st_mode);
  }
  if (mode & TR_MOVE)
  {
    if (unlink (sen->path) == ERROR)
    {
      return dlg_error_continue (sen->path, strerror (errno));
    }
  }
  return (TRUE);
}

/*
 * move/copy a directory to the named target
 * args: e.g. /opt/src -> /usr will move/copy to /usr/src
 */
int
trans_dir (entry * sen, entry * ten, int mode, GtkWidget ** info, state * st)
{
  char target[PATH_MAX + 1];
  char source[PATH_MAX + 1];
  DIR *dir;
  entry *new_sen, *new_ten;
  struct dirent *de;
  struct stat s_stat, t_stat;
  int rc = 1;

  if (!io_is_valid (sen->label) || (sen->type & FT_DIR_UP))
    return (FALSE);

  if (!io_is_valid (ten->label) || (ten->type & FT_DIR_UP))
    return (FALSE);

  /* update labels
   */
  gtk_entry_set_text (GTK_ENTRY (info[0]), sen->path);
  gtk_entry_set_text (GTK_ENTRY (info[1]), ten->path);

  sprintf (target, "%s/%s", ten->path, sen->label);

  if (strcmp (sen->path, target) == 0)
  {
    dlg_error (_("Source and Target directories are the same"), target);
    return (FALSE);
  }
  if (mode == TR_MOVE)
  {
    /* first check if the directories are on the
     * same device - this would be a simple situation :)
     */

    /* we copy/move symbolic links as links, so stat() the link
     * itself
     */
    if (lstat (sen->path, &s_stat) == ERROR)
    {
      return dlg_error_continue (_("Can't stat() file"), sen->path);
    }
    if (stat (ten->path, &t_stat) == ERROR)
    {
      return dlg_error_continue (_("Can't stat() file"), ten->path);
    }
    if (s_stat.st_dev == t_stat.st_dev)
    {
      /* are on the same device, so we have just to rename
       */
      if (rename (sen->path, target) == ERROR)
      {
	return dlg_error_continue (sen->path, strerror (errno));
      }
      return (TRUE);
    }
  }
  else if (mode & TR_LINK)
  {
    if (symlink (sen->path, target) == -1)
    {
      return dlg_continue (sen->path, strerror (errno));
    }
    return (TRUE);
  }
  dir = opendir (sen->path);
  if (!dir)
  {
    return dlg_error_continue (sen->path, strerror (errno));
  }
  if (mkdir (target, 0xFFFF) == ERROR)
  {
    if (errno == EEXIST)
    {
      if (!io_is_directory (target))
      {
	closedir (dir);
	return dlg_skip (target, _("exists and is not a directory"));
      }
      /* else: silent ignore that the directory is still there
       */
    }
    else
    {
      dlg_error (target, strerror (errno));
      closedir (dir);
      return (0);
    }
  }
  while ((de = readdir (dir)) != NULL)
  {
    if (*st->status == ST_CANCEL)
    {
      closedir (dir);
      return (0);
    }
    if (!io_is_valid (de->d_name))
      continue;

    sprintf (source, "%s/%s", sen->path, de->d_name);
    new_sen = entry_new_by_path (source);
    if (!new_sen)
      return (0);
    new_ten = entry_new_by_path (target);
    if (EN_IS_DIR (new_sen) && (!EN_IS_LINK (new_sen)))
    {
      /* do not follow links */
      rc = trans_dir (new_sen, new_ten, mode, info, st);
    }
    else
    {
      rc = trans_file (new_sen, new_ten, mode, info, st);
    }
    entry_free (new_sen);
    entry_free (new_ten);
    if (!rc)
    {
      closedir (dir);
      return (FALSE);
    }
  }
  closedir (dir);
  if (mode & TR_MOVE)
  {
    if (rmdir (sen->path) == ERROR)
    {
      return dlg_error_continue (sen->path, strerror (errno));
    }
  }
  return (TRUE);
}

/*
 * move or copy files
 */
int
transfer (GtkCTree * s_ctree, GList * list, char *path, int mode, int *alive)
{
  GtkWidget *dialog, *cancel, *label[2], *box, *table, *info[3];
  GtkCTreeNode *node;
  char title[64];
  entry *en, *target_en;
  int nitems, rc;
  state st[2];
  int status;

  status = ST_OK;
  st[0].status = st[1].status = &status;

  if (mode == TR_COPY)
  {
    sprintf (title, _("XFTree: Copy"));
  }
  else if (mode == TR_MOVE)
  {
    sprintf (title, _("XFTree: Move"));
  }
  else if (mode == TR_LINK)
  {
    sprintf (title, _("XFTree: Link"));
  }
  else
  {
    printf (_("Unknown transfer mode: %d\n"), mode);
    return (0);
  }
  nitems = g_list_length (list);
  if ((!nitems) || (!list))
  {
    dlg_error (_("transfer()"), _("fatal error"));
    return (FALSE);
  }
  target_en = entry_new_by_path (path);
  if (!target_en)
  {
    dlg_error (_("Error getting info from"), path);
    return (FALSE);
  }
  if ((nitems > 1) && (!(EN_IS_DIR (target_en))))
  {
    dlg_error (_("Fatal Error"), _("Can't transfer items to a file"));
    return (FALSE);
  }

  dialog = gtk_dialog_new ();
  st[0].w = dialog;
  st[1].w = NULL;
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy", GTK_SIGNAL_FUNC (cb_cancel), &st[1]);
  top_register (dialog);

  box = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), box, TRUE, TRUE, 0);
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

  info[2] = gtk_label_new (_("0 bytes transferred"));
  gtk_box_pack_start (GTK_BOX (box), info[2], TRUE, TRUE, 0);

  cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), cancel, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (cb_cancel), &st[0]);

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_widget_show_all (dialog);

  while (list)
  {
    if (status == ST_CANCEL)
    {
      entry_free (target_en);
      top_delete (dialog);
      return (0);
    }
    en = (entry *) list->data;
    if (EN_IS_DIR (en) && (!EN_IS_DIR (target_en)))
    {
      dlg_error (_("Error"), _("Can't transfer a directory to a file"));
      entry_free (target_en);
      goto END;
    }
    if (!EN_IS_DIRUP (en) && io_is_valid (en->label) && !(en->type & FT_DIR_UP))
    {
      if (EN_IS_DIR (en) && (!EN_IS_LINK (en)))
      {
	rc = trans_dir (en, target_en, mode, info, &st[0]);
      }
      else
      {
	rc = trans_file (en, target_en, mode, info, &st[0]);
      }
      if (*alive && (mode == TR_MOVE) && rc && s_ctree)
      {
	/* remove old nodes
	 */
	node = gtk_ctree_find_by_row_data (s_ctree, GTK_CTREE_NODE (GTK_CLIST (s_ctree)->row_list), en->org_mem);
	if (node)
	{
	  gtk_ctree_remove_node (s_ctree, node);
	}
      }
      else if (!rc)
      {
	entry_free (target_en);
	goto END;
      }
    }
    list = list->next;
  }

  /* release the toplevel from the list of all toplevels
   */
END:
  top_delete (dialog);
  if (status == ST_OK)
    gtk_widget_destroy (dialog);
  return (ST_OK);
}
