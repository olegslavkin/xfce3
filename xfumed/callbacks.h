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

#include <gtk/gtk.h>

#include "xfumed.h"
#include "clistmenu.h"


gboolean on_xfumed_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data);

gboolean on_xfumed_destroy_event (GtkWidget * widget, GdkEvent * event, gpointer user_data);

void on_button_to_parent_clicked (GtkButton * button, gpointer user_data);

void on_button_to_sub_clicked (GtkButton * button, gpointer user_data);

void on_clist_menuitems_select_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data);

void on_clist_menuitems_unselect_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data);

void on_button_up_clicked (GtkButton * button, gpointer user_data);

void on_button_down_clicked (GtkButton * button, gpointer user_data);

void on_combo_entry1_changed (GtkEditable * editable, gpointer user_data);

void command_set_from_fileselection (GtkFileSelection * selector, gpointer user_data);
void on_button_fileselect_clicked (GtkButton * button, gpointer user_data);

void on_button_add_clicked (GtkButton * button, gpointer user_data);

void on_button_update_clicked (GtkButton * button, gpointer user_data);

void on_button_remove_clicked (GtkButton * button, gpointer user_data);

void on_button_save_clicked (GtkButton * button, gpointer user_data);

void on_button_reset_clicked (GtkButton * button, gpointer user_data);

void on_button_quit_clicked (GtkButton * button, gpointer user_data);

gboolean on_quit_dialog_delete (GtkWidget * widget, GdkEvent * event, gpointer dialog);

void on_button_cancel_quit_clicked (GtkButton * button, gpointer user_data);

void on_button_continue_quit_clicked (GtkButton * button, gpointer user_data);
