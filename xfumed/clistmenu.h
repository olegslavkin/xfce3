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

#ifndef _CLIST_MENU_H
#define _CLIST_MENU_H

#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "xfumed.h"
#include "usermenu.h"

#ifndef MAXSTRLEN
#define MAXSTRLEN 1024
#endif


void clist_from_menu (GtkCList * clist, RootMenu * menu);

void clist_row_selected (GtkCList * clist, gint row);

void clist_row_unselected (GtkCList * clist);

void clist_row_moveup (GtkCList * clist, gint row);

void clist_row_movedown (GtkCList * clist, gint row);

void clist_to_sub (GtkCList * clist, gint row);

void clist_to_parent (GtkCList * clist, RootMenu * current_root);

gchar **entries_get_text (void);

void entries_set_text (gchar * type, gchar * caption, gchar * command);

void add (GtkCList * clist, RootMenu * menu, gchar * text[]);

gchar *name_from_caption (gchar * caption);

void update_nth (GtkCList * clist, RootMenu * menu, gint n, gchar * text[]);

void remove_nth (GtkCList * clist, RootMenu * menu, gint n);

void save (void);

void reset (void);

void quit (void);

#endif
