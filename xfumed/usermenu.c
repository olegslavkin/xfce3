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

#include "usermenu.h"
#include "my_intl.h"

RootMenu *
menu_new (gchar * name)
{
  RootMenu *menu = g_malloc (sizeof (RootMenu));

  menu->parent = NULL;
  menu->name = g_strdup (name);
  menu->entries = NULL;

  return menu;
}

RootMenu *
menu_find_by_name (gchar * name)
{
  GList *menu_in_list = g_list_find_custom (list_allmenus, (gpointer) name,
					    (GCompareFunc) compare_menuname);

  if (menu_in_list == NULL)
    return NULL;

  return (RootMenu *) menu_in_list->data;
}

gchar *
menu_get_caption (RootMenu * menu)
{
  RootMenu *parent;
  ItemData *item;
  gint i;

  if (menu->parent == NULL || (parent = menu_find_by_name (menu->parent)) == NULL)
    return g_strdup (_("Toplevel"));

  for (i = 0; i < g_list_length (parent->entries); ++i)
  {
    item = item_nth (parent, i);

    if (item->type == SUBMENU && strcmp (item->name, menu->name) == 0)
      return g_strdup (item->caption);
  }

  /* nothing found */
  return g_strdup (_("Toplevel"));
}


gint
compare_menuname (gpointer * menu, gpointer * name)
{
  RootMenu *root = (RootMenu *) menu;

  if (root == NULL)
    return -1;
  else
    return strcmp (root->name, (gchar *) name);
}

GList *
menu_add (RootMenu * menu)
{
  list_allmenus = g_list_append (list_allmenus, (gpointer) menu);

  return list_allmenus;
}

GList *
menu_remove (RootMenu * menu)
{
  list_allmenus = g_list_remove (list_allmenus, (gpointer) menu);

  g_free (menu->name);
  g_free (menu->parent);

  while (menu->entries != NULL)
    item_remove_nth (menu, 0);

  g_list_free (menu->entries);
  g_free (menu);

  return list_allmenus;
}


ItemData *
item_new (void)
{
  ItemData *item = g_malloc (sizeof (ItemData));

  item->type = NOP;
  item->name = NULL;
  item->caption = NULL;
  item->command = NULL;

  return item;
}

ItemData *
item_nth (RootMenu * menu, gint n)
{
  return (ItemData *) g_list_nth_data (menu->entries, n);
}

void
item_add (RootMenu * menu, ItemData * item)
{
  menu->entries = g_list_append (menu->entries, (gpointer) item);

  if (item->type == SUBMENU)
  {
    RootMenu *newmenu;
    newmenu = menu_find_by_name (item->name);

    if (newmenu == NULL)
    {
      newmenu = menu_new (item->name);
      newmenu->parent = g_strdup (menu->name);
      newmenu->entries = NULL;

      list_allmenus = menu_add (newmenu);
    }
    else
      newmenu->parent = g_strdup (menu->name);
  }
}

void
item_update_nth (RootMenu * menu, ItemData * item, gint n)
{
  GList *item_in_list = g_list_nth (menu->entries, n);
  ItemData *old_item = (ItemData *) item_in_list->data;

  /* The type can not be updated. That would not be an update, but an
   * addition and a removal in one step.
   */
  if (old_item == NULL || old_item->type != item->type)
    return;

  if (item->type == SUBMENU)
  {
    RootMenu *menu = menu_find_by_name (old_item->name);

    g_free (menu->name);
    menu->name = g_strdup (item->name);
  }

  g_free (old_item->name);
  g_free (old_item->caption);
  g_free (old_item->command);
  g_free (old_item);

  item_in_list->data = (gpointer) item;
}

void
item_remove_nth (RootMenu * menu, gint n)
{
  GList *item_in_list = g_list_nth (menu->entries, n);
  ItemData *item;

  if (item_in_list == NULL)
    return;

  menu->entries = g_list_remove_link (menu->entries, item_in_list);
  item = (ItemData *) item_in_list->data;

  if (item == NULL)
    return;

  if (item->type == SUBMENU)
  {
    RootMenu *menu = menu_find_by_name (item->name);

    if (menu != NULL)
      list_allmenus = menu_remove (menu);
  }

  g_free (item->name);
  g_free (item->caption);
  g_free (item->command);
  g_free (item);
  g_list_free_1 (item_in_list);
}

GList *
clear_menulist ()
{
  while (list_allmenus != NULL && list_allmenus->data != NULL)
  {
    RootMenu *menu = (RootMenu *) g_list_last (list_allmenus)->data;

    list_allmenus = menu_remove (menu);
  }

  return NULL;
}


void
read_menu (void)
{
  FILE *configFile;
  gchar *configName;
  gchar line[MAXSTRLEN];
  gchar *word;
  RootMenu *current_menu = NULL;
  ItemData *item;

  list_allmenus = NULL;

  configName = g_strconcat (g_getenv ("HOME"), "/.xfce/xfwm.user_menu", NULL);

  /* at least add toplevel menu and set 
   * as rootmenu for clist 
   */
  menu_toplevel = menu_new ("user_menu");
  list_allmenus = menu_add (menu_toplevel);
  menu_clist = menu_toplevel;

  if ((configFile = fopen (configName, "r")) == NULL)
    return;

  /* process file line by line */
  while (get_line (configFile, line, MAXSTRLEN) != 0)
  {
    gchar *remainder;

    if (line[0] == '#')
      continue;

    word = get_next_word (line);
    remainder = strstr (line, word) + strlen (word);
    if (remainder[0] != '\0')
      remainder++;

    if (g_strcasecmp (word, "AddToMenu") == 0)
    {
      g_free (word);

      /* next word is an menu name */
      if ((word = get_next_word (remainder)) == NULL || strlen (word) == 0)
	continue;


      remainder = strstr (remainder, word) + strlen (word);
      if (remainder[0] != '\0')
	remainder++;

      /* if menu is not in menu list, add it */
      if ((current_menu = menu_find_by_name (word)) == NULL)
      {
	current_menu = menu_new (word);
	list_allmenus = menu_add (current_menu);
      }

      g_free (word);

      /* there can be an item definition on this line */
      if ((item = item_from_menuline (remainder)) != NULL)
	item_add (current_menu, item);

      continue;
    }

    if (current_menu != NULL && g_strcasecmp (word, "+") == 0)
    {
      g_free (word);

      if ((item = item_from_menuline (remainder)) != NULL)
	item_add (current_menu, item);

      continue;
    }

    /* nothing found: reset current_menu */
    g_free (word);
    current_menu = NULL;
  }

  fclose (configFile);
  file_saved = 1;

  return;
}


ItemData *
item_from_menuline (gchar * line)
{
  ItemData *item = item_new ();
  gchar *word;
  gchar *string = line;

  word = get_next_word (string);
  if (strlen (word) == 0 && string[0] != '\0')
    string++;
  else
  {
    item->caption = g_strdup (word);
    string = strstr (string, word) + strlen (word);
  }

  while (string[0] != '\0')
  {
    g_free (word);
    word = get_next_word (string + 1);

    if (strlen (word) == 0 || g_strcasecmp (word, "Nop") == 0)
    {
      g_free (word);
      g_free (item->caption);
      item->type = NOP;
      item->caption = NULL;

      return item;
    }
    else
    {
      string = strstr (string, word) + strlen (word);

      if (string[0] != '\0' && g_strcasecmp (word, "Exec") == 0)
      {
	g_free (word);

	item->type = ITEM;
	item->command = get_eol (string + 1);

	return item;
      }

      if (string[0] != '\0' && g_strcasecmp (word, "PopUp") == 0)
      {
	g_free (word);

	item->type = SUBMENU;
	item->name = get_next_word (string + 1);

	return item;
      }
    }
  }

  /* nothing useful found */
  return NULL;
}

gint
get_line (FILE * fp, gchar line[], gint limit)
{
  gint i;
  gchar chr = '\0';

  for (i = 0; i < limit && (chr = getc (fp)) != EOF && chr != '\n'; i++)
    line[i] = chr;

  line[i] = '\0';

  /* if the line is longer than limit we have to find the
     * end of the line (or the file) without adding characters
     * to 'line' */
  if (i >= limit && !(chr == EOF || chr == '\n'))
    while ((chr = getc (fp)) != EOF && chr != '\n');

  if (i > 0)
    return i;
  else
  {
    if (chr == '\n')
      return 1;
    else
      return 0;
  }
}

gchar *
get_eol (gchar * text)
{
  gint len, i;
  gchar word[MAXSTRLEN];
  gchar *string = text;

  word[0] = '\0';

  while (string[0] == ' ')
    string++;

  len = strlen (string);
  for (i = 0; i < len && i < MAXSTRLEN; i++)
  {
    if ((string[i] == '\0') || (string[i] == '\n'))
      return g_strdup (word);
    else
    {
      word[i] = string[i];
      word[i + 1] = '\0';
    }
  }

  return g_strdup (word);
}

gchar *
get_next_word (gchar * text)
{
  gint len, i, delimiter;
  gchar word[MAXSTRLEN];
  gchar *string = text;

  word[0] = '\0';

  while (string[0] == ' ')
    string++;

  /* check if word is enclosed in quotes */
  if (string[0] == '\"')
  {
    string++;
    delimiter = '\"';
  }
  else
    delimiter = ' ';

  len = strlen (string);

  for (i = 0; i < len && i < MAXSTRLEN; i++)
  {
    if (string[i] == delimiter || string[i] == '\0')
      return g_strdup (word);
    else
    {
      word[i] = string[i];
      word[i + 1] = '\0';
    }
  }

  g_strstrip (word);

  return g_strdup (word);
}

void
write_menu (GList * menulist)
{
  FILE *configFile, *backupFile;
  gchar *configName, *backupName;
  GList *menu_in_list = menulist;

  configName = g_strconcat (g_getenv ("HOME"), "/.xfce/xfwm.user_menu", NULL);
  backupName = g_strconcat (configName, ".bak", NULL);

  if ((backupFile = fopen (backupName, "r")) != NULL)
  {
    /* There's a backup file already. We should keep it. */
    fclose (backupFile);
  }
  else if ((configFile = fopen (configName, "r")) != NULL && (backupFile = fopen (backupName, "w")) != NULL)
  {
    gint chr;
    while ((chr = getc (configFile)) != EOF)
      putc (chr, backupFile);
    fclose (configFile);
    fclose (backupFile);
  }

  if ((configFile = fopen (configName, "w")) == NULL)
  {
    perror ("xfumed");
    return;
  }

  fprintf (configFile, "%s\n", INTROTEXT);

  while (menu_in_list != NULL && menu_in_list->data != NULL)
  {
    RootMenu *menu = (RootMenu *) menu_in_list->data;
    GList *item_in_list = menu->entries;

    fprintf (configFile, "\nAddToMenu \"%s\"\n", menu->name);

    while (item_in_list != NULL && item_in_list->data != NULL)
    {
      ItemData *item = (ItemData *) item_in_list->data;

      switch (item->type)
      {
      case ITEM:
	fprintf (configFile, "AddToMenu \"%s\" \"%s\" Exec %s\n", menu->name, item->caption, item->command);
	break;
      case SUBMENU:
	fprintf (configFile, "AddToMenu \"%s\" \"%s\" PopUp \"%s\"\n", menu->name, item->caption, item->name);
	break;
      case NOP:
	fprintf (configFile, "AddToMenu \"%s\" \"\" Nop\n", menu->name);
	break;
      }

      item_in_list = g_list_next (item_in_list);
    }

    menu_in_list = g_list_next (menu_in_list);
  }

  fclose (configFile);
}
