/*  xfumed
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "callbacks.h"
#include "interface.h"

#include "my_intl.h"
#include "xfce-common.h"
#include "fileselect.h"

gboolean on_xfumed_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
  quit ();
  return FALSE;
}

gboolean on_xfumed_destroy_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
  gtk_main_quit ();
  return TRUE;
}

void
on_button_to_parent_clicked (GtkButton * button, gpointer user_data)
{
  clist_to_parent (GTK_CLIST (clist_menuitems), menu_clist);
}

void
on_button_to_sub_clicked (GtkButton * button, gpointer user_data)
{
  if (selectedRow == -1)
    return;

  clist_to_sub (GTK_CLIST (clist_menuitems), selectedRow);
}

void
on_clist_menuitems_select_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
  clist_row_selected (GTK_CLIST (clist_menuitems), row);
}

void
on_clist_menuitems_unselect_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
  clist_row_unselected (GTK_CLIST (clist_menuitems));
}

void
on_button_up_clicked (GtkButton * button, gpointer user_data)
{
  clist_row_moveup (GTK_CLIST (clist_menuitems), selectedRow);
}

void
on_button_down_clicked (GtkButton * button, gpointer user_data)
{
  clist_row_movedown (GTK_CLIST (clist_menuitems), selectedRow);
}

void
on_combo_entry1_changed (GtkEditable * editable, gpointer user_data)
{
  gchar *type = gtk_editable_get_chars (GTK_EDITABLE (combo_entry_type), 0, -1);

  if (g_strcasecmp (type, TYPE_NOP_TEXT) == 0)
  {
    gtk_entry_set_text (GTK_ENTRY (entry_caption), "");
    gtk_entry_set_text (GTK_ENTRY (entry_command), "");
    gtk_entry_set_editable (GTK_ENTRY (entry_caption), FALSE);
    gtk_entry_set_editable (GTK_ENTRY (entry_command), FALSE);

    /* block button to select command */
    if (fileselect_blocked != 1)
    {
      fileselect_blocked = 1;
      gtk_signal_handler_block_by_func (GTK_OBJECT (button_fileselect), GTK_SIGNAL_FUNC (on_button_fileselect_clicked), NULL);
    }

    g_free (type);
    return;
  }
  else if (g_strcasecmp (type, TYPE_SUBMENU_TEXT) == 0)
  {
    gtk_entry_set_text (GTK_ENTRY (entry_command), "");
    gtk_entry_set_editable (GTK_ENTRY (entry_caption), TRUE);
    gtk_entry_set_editable (GTK_ENTRY (entry_command), FALSE);

    /* block button to select command */
    if (fileselect_blocked != 1)
    {
      fileselect_blocked = 1;
      gtk_signal_handler_block_by_func (GTK_OBJECT (button_fileselect), GTK_SIGNAL_FUNC (on_button_fileselect_clicked), NULL);
    }

    g_free (type);
    return;
  }

  gtk_entry_set_editable (GTK_ENTRY (entry_caption), TRUE);
  gtk_entry_set_editable (GTK_ENTRY (entry_command), TRUE);

  /* unblock button to select command */
  if (fileselect_blocked == 1)
  {
    fileselect_blocked = 0;
    gtk_signal_handler_unblock_by_func (GTK_OBJECT (button_fileselect), GTK_SIGNAL_FUNC (on_button_fileselect_clicked), NULL);
  }

  g_free (type);
}

void
on_button_fileselect_clicked (GtkButton * button, gpointer user_data)
{
  gchar *selected_command = open_fileselect (NULL);

  if (selected_command)
  {
    gtk_entry_set_text (GTK_ENTRY (entry_command), selected_command);
    gtk_editable_set_position (GTK_EDITABLE (entry_command), -1);
  }
}

void
on_button_add_clicked (GtkButton * button, gpointer user_data)
{
  gchar **text = entries_get_text ();

  add (GTK_CLIST (clist_menuitems), menu_clist, text);
  g_free (text);
}

void
on_button_update_clicked (GtkButton * button, gpointer user_data)
{
  gchar **text;

  if (selectedRow == -1)
    return;

  text = entries_get_text ();
  update_nth (GTK_CLIST (clist_menuitems), menu_clist, selectedRow, text);
  g_free (text);
}

void
on_button_remove_clicked (GtkButton * button, gpointer user_data)
{
  if (selectedRow == -1)
    return;

  remove_nth (GTK_CLIST (clist_menuitems), menu_clist, selectedRow);
}

void
on_button_save_clicked (GtkButton * button, gpointer user_data)
{
  save ();
}

void
on_button_reset_clicked (GtkButton * button, gpointer user_data)
{
  reset ();
}

void
on_button_quit_clicked (GtkButton * button, gpointer user_data)
{
  quit ();
}

void
on_button_cancel_quit_clicked (GtkButton * button, gpointer dialog)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
  return;
}

void
on_button_continue_quit_clicked (GtkButton * button, gpointer dialog)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
  gtk_main_quit ();
}

gboolean on_quit_dialog_delete (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  return TRUE;
}
