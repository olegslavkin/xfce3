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

#include "xfce-common.h"
#include "usermenu.h"
#include "clistmenu.h"
#include "interface.h"

#include "my_intl.h"


void
clist_from_menu (GtkCList * clist, RootMenu * menu)
{
  GList *item_in_list = menu->entries;
  gchar *status_fmt = g_strdup (_("   Menu: %s   Parent: %s"));
  gchar status_text[MAXSTRLEN];

  gtk_clist_freeze (GTK_CLIST (clist));

  /* set status bar */
  if (menu->parent == NULL)
  {
    sprintf (status_text, status_fmt, _("Toplevel"), _("None"));
  }
  else
  {
    RootMenu *parent_menu = menu_find_by_name (menu->parent);

    if (parent_menu->parent == NULL)
    {
      gchar *caption = menu_get_caption (menu);

      sprintf (status_text, status_fmt, caption, _("Toplevel"));
      g_free (caption);
    }
    else
    {
      gchar *caption = menu_get_caption (menu);
      gchar *parent_caption = menu_get_caption (parent_menu);

      sprintf (status_text, status_fmt, caption, parent_caption);
      g_free (caption);
      g_free (parent_caption);
    }
  }
  gtk_label_set_text (GTK_LABEL (label_status), status_text);
  g_free (status_fmt);

  /* Fill the clist */
  while (item_in_list != NULL)
  {
    gchar *text[3];
    ItemData *item = (ItemData *) item_in_list->data;

    if (item == NULL)
      break;

    switch (item->type)
    {
    case NOP:
      text[0] = g_strdup (TYPE_NOP_TEXT);
      text[1] = NULL;
      text[2] = NULL;
      break;
    case SUBMENU:
      text[0] = g_strdup (TYPE_SUBMENU_TEXT);
      text[1] = g_strdup (item->caption);
      text[2] = NULL;
      break;
    default:
      text[0] = g_strdup (TYPE_ITEM_TEXT);
      text[1] = g_strdup (item->caption);
      text[2] = g_strdup (item->command);
      break;
    }

    lastRow = gtk_clist_append (GTK_CLIST (clist_menuitems), text);
    item_in_list = g_list_next (item_in_list);
  }

  gtk_clist_thaw (GTK_CLIST (clist));
}

void
clist_row_selected (GtkCList * clist, gint row)
{
  gchar *type, *caption, *command;

  selectedRow = row;

  type = g_malloc (MAXSTRLEN);
  gtk_clist_get_text (clist, row, 0, &type);
  if (type == NULL)
    type = g_strdup ("");

  caption = g_malloc (MAXSTRLEN);
  gtk_clist_get_text (clist, row, 1, &caption);
  if (caption == NULL)
    caption = g_strdup ("");

  command = g_malloc (MAXSTRLEN);
  gtk_clist_get_text (clist, row, 2, &command);
  if (command == NULL)
    command = g_strdup ("");

  entries_set_text (type, caption, command);

  if (gtk_clist_row_is_visible (clist, row) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto (clist, row, 0, 0, 0);
}

void
clist_row_unselected (GtkCList * clist)
{
  selectedRow = -1;

  entries_set_text (g_strdup (TYPE_ITEM_TEXT), g_strdup (""), g_strdup (""));
}

void
clist_row_moveup (GtkCList * clist, gint row)
{
  ItemData *item;

  if (selectedRow <= 0)
    return;

  selectedRow--;
  gtk_clist_row_move (clist, row, selectedRow);

  if (gtk_clist_row_is_visible (clist, selectedRow) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto (clist, selectedRow, 0, 0, 0);

  item = item_nth (menu_clist, row);

  if (item != NULL)
  {
    menu_clist->entries = g_list_remove (menu_clist->entries, (gpointer) item);
    menu_clist->entries = g_list_insert (menu_clist->entries, (gpointer) item, row - 1);
  }

  file_saved = 0;
}

void
clist_row_movedown (GtkCList * clist, gint row)
{
  ItemData *item;

  if (selectedRow == -1 || selectedRow == lastRow)
    return;

  selectedRow++;
  gtk_clist_row_move (clist, row, selectedRow);

  if (gtk_clist_row_is_visible (clist, row) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto (clist, selectedRow, 0, 1, 0);

  item = item_nth (menu_clist, row);

  if (item != NULL)
  {
    menu_clist->entries = g_list_remove (menu_clist->entries, (gpointer) item);
    menu_clist->entries = g_list_insert (menu_clist->entries, (gpointer) item, row + 1);
  }

  file_saved = 0;
}


void
clist_to_sub (GtkCList * clist, gint row)
{
  RootMenu *menu;
  ItemData *item = (ItemData *) g_list_nth_data (menu_clist->entries, row);

  if (item == NULL || item->type != SUBMENU)
    return;

  if ((menu = menu_find_by_name (item->name)) == NULL)
    return;

  menu_clist = menu;

  gtk_clist_clear (clist);
  clist_from_menu (clist, menu_clist);

  clist_row_unselected (clist);
}

void
clist_to_parent (GtkCList * clist, RootMenu * current_root)
{
  if (current_root->parent == NULL || (menu_clist = menu_find_by_name (current_root->parent)) == NULL)
    return;

  gtk_clist_clear (clist);
  clist_from_menu (clist, menu_clist);
  clist_row_unselected (clist);
}

gchar **
entries_get_text (void)
{
  gchar *text;

  gchar **result = g_malloc (3 * sizeof (char *));

  text = gtk_editable_get_chars (GTK_EDITABLE (combo_entry_type), 0, -1);
  result[0] = (text == NULL) ? g_strdup ("") : g_strdup (text);
  g_free (text);

  if (g_strcasecmp (result[0], TYPE_NOP_TEXT) == 0)
  {
    result[1] = g_strdup ("");
    result[2] = g_strdup ("");

    return result;
  }

  text = gtk_editable_get_chars (GTK_EDITABLE (entry_caption), 0, -1);
  result[1] = (text == NULL) ? g_strdup ("") : g_strdup (text);
  g_free (text);

  if (g_strcasecmp (result[0], TYPE_SUBMENU_TEXT) == 0)
  {
    result[2] = g_strdup ("");

    return result;
  }

  text = gtk_editable_get_chars (GTK_EDITABLE (entry_command), 0, -1);
  result[2] = (text == NULL) ? g_strdup ("") : g_strdup (text);
  g_free (text);

  return result;
}

void
entries_set_text (gchar * type, gchar * caption, gchar * command)
{
  gchar *text;

  if (g_strcasecmp (type, TYPE_NOP_TEXT) == 0)
  {
    gtk_entry_set_text (GTK_ENTRY (combo_entry_type), TYPE_NOP_TEXT);
    gtk_entry_set_text (GTK_ENTRY (entry_caption), g_strdup (""));
    gtk_entry_set_text (GTK_ENTRY (entry_command), g_strdup (""));

    return;
  }
  else if (g_strcasecmp (type, TYPE_SUBMENU_TEXT) == 0)
  {
    gtk_entry_set_text (GTK_ENTRY (combo_entry_type), TYPE_SUBMENU_TEXT);

    if (caption == NULL)
      text = g_strdup ("");
    else
      text = g_strdup (caption);
    gtk_entry_set_text (GTK_ENTRY (entry_caption), g_strdup (text));
    g_free (text);

    gtk_entry_set_text (GTK_ENTRY (entry_command), g_strdup (""));

    return;
  }
  else if (g_strcasecmp (type, TYPE_ITEM_TEXT) == 0)
  {
    gtk_entry_set_text (GTK_ENTRY (combo_entry_type), TYPE_ITEM_TEXT);

    if (caption == NULL)
      text = g_strdup ("");
    else
      text = g_strdup (caption);
    gtk_entry_set_text (GTK_ENTRY (entry_caption), g_strdup (text));
    g_free (text);

    if (command == NULL)
      text = g_strdup ("");
    else
      text = g_strdup (command);
    gtk_entry_set_text (GTK_ENTRY (entry_command), g_strdup (text));
    g_free (text);

    gtk_editable_set_position (GTK_EDITABLE (entry_command), -1);

    return;
  }
  else
  {
    gtk_entry_set_text (GTK_ENTRY (combo_entry_type), TYPE_ITEM_TEXT);
    gtk_entry_set_text (GTK_ENTRY (entry_caption), g_strdup (""));
    gtk_entry_set_text (GTK_ENTRY (entry_command), g_strdup (""));

    return;
  }
}


void
add (GtkCList * clist, RootMenu * menu, gchar * text[])
{
  ItemData *item;

  if (text[0] == NULL || strlen (text[0]) == 0)
    return;

  if (strcmp (text[0], TYPE_NOP_TEXT) == 0)
  {
    item = item_new ();
    item->type = NOP;
    item->name = NULL;
    item->caption = NULL;
    item->command = NULL;

    item_add (menu, item);
  }

  if (strcmp (text[0], TYPE_SUBMENU_TEXT) == 0)
  {
    if (text[1] == NULL || strlen (text[1]) == 0)
      return;

    item = item_new ();
    item->type = SUBMENU;
    item->name = name_from_caption (text[1]);

    /* menu name must be unique */
    if (menu_find_by_name (item->name) != NULL)
      return;

    item->caption = g_strdup (text[1]);
    item->command = NULL;

    item_add (menu, item);
  }

  if (strcmp (text[0], TYPE_ITEM_TEXT) == 0)
  {
    if (text[1] == NULL || strlen (text[1]) == 0)
      return;

    item = item_new ();
    item->type = ITEM;
    item->name = NULL;
    item->caption = g_strdup (text[1]);
    item->command = g_strdup (text[2]);

    item_add (menu, item);
  }

  lastRow = gtk_clist_append (clist, text);
  gtk_clist_select_row (clist, lastRow, 0);

  file_saved = 0;
}

gchar *
name_from_caption (gchar * caption)
{
  gchar *name = g_strdup (caption);
  g_strdown (g_strstrip (name));
  name = g_strdelimit (name, " ", '_');

  return name;
}

void
update_nth (GtkCList * clist, RootMenu * menu, gint n, gchar * text[])
{
  ItemData *olditem, *item;

  olditem = item_nth (menu, n);

  /* separator can not be updated (no data!) */
  if (olditem == NULL || olditem->type == NOP)
    return;

  /* if item types of old and new entry are not the same
   * we can not update
   */
  if (olditem->type == SUBMENU && g_strcasecmp (text[0], TYPE_SUBMENU_TEXT) == 0)
  {
    if (text[1] == NULL || strlen (text[1]) == 0)
      return;

    item = item_new ();
    item->type = SUBMENU;
    item->name = name_from_caption (text[1]);
    item->caption = g_strdup (text[1]);
    item->command = NULL;

    item_update_nth (menu, item, n);
    gtk_clist_set_text (clist, n, 1, g_strdup (text[1]));

    file_saved = 0;
  }

  if (olditem->type == ITEM && g_strcasecmp (text[0], TYPE_ITEM_TEXT) == 0)
  {
    if (text[1] == NULL || strlen (text[1]) == 0)
      return;

    item = item_new ();
    item->type = ITEM;
    item->name = NULL;
    item->caption = g_strdup (text[1]);
    item->command = g_strdup (text[2]);

    item_update_nth (menu, item, n);
    gtk_clist_set_text (clist, n, 1, g_strdup (text[1]));
    gtk_clist_set_text (clist, n, 2, g_strdup (text[2]));

    file_saved = 0;
  }
}

void
remove_nth (GtkCList * clist, RootMenu * menu, gint n)
{
  gtk_clist_remove (clist, n);
  lastRow--;
  item_remove_nth (menu, n);
  if (n == 0)
    gtk_clist_select_row (clist, n, 0);
  else
    gtk_clist_select_row (clist, n - 1, 0);

  file_saved = 0;
}

void
save ()
{
  if (file_saved != 1)
  {
    write_menu (list_allmenus);
    file_saved = 1;
    my_show_message (_("Your changes will take effect on next startup"));
  }
}

void
reset ()
{
  if (lastRow >= 0)
    gtk_clist_clear (GTK_CLIST (clist_menuitems));

  if (list_allmenus != NULL)
    list_allmenus = clear_menulist (list_allmenus);

  selectedRow = -1;
  lastRow = -1;

  read_menu ();
  clist_from_menu (GTK_CLIST (clist_menuitems), menu_clist);

  file_saved = 1;
}

void
quit ()
{
  if (file_saved == 1)
    gtk_main_quit ();
  else
  {
    GtkWidget *dialog = create_dialog_quit ();
    gtk_widget_show (dialog);
  }
}
