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

#ifndef _XFUMED_H
#define _XFUMED_H

#ifndef MAXSTRLEN
#define MAXSTRLEN 1024
#endif

/* Typedefs */

typedef enum
{ ITEM, SUBMENU, NOP }
ItemType;
typedef struct _root_menu
{
  gchar *parent;
  gchar *name;
  GList *entries;
}
RootMenu;

typedef struct _item_data
{
  ItemType type;
  gchar *name;
  gchar *caption;
  gchar *command;
}
ItemData;

/* global variables */

GList *list_allmenus;
RootMenu *menu_toplevel;
RootMenu *menu_clist;
GtkWidget *file_selector;
GtkWidget *xfumed;
GtkWidget *label_status;
GtkWidget *clist_menuitems;
gint selectedRow;
gint lastRow;
GtkWidget *combo_entry_type;
GtkWidget *entry_caption;
GtkWidget *entry_command;
GtkWidget *button_fileselect;
gint file_saved;
gint fileselect_blocked;

#endif
