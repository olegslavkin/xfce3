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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "usermenu.h"

#include "my_intl.h"
#include "xfce-common.h"

GtkWidget*
create_xfumed (void)
{
/* commented out variables are defined globally in xfumed.h,
 * which is included by all other headers */
  GtkWidget *xfumed;
  GtkWidget *vbox1;
  GtkWidget *hbox1;
  GtkWidget *vbox2;
  GtkWidget *frame_status;
/*  GtkWidget *label_status; */
  GtkWidget *hbox_status;
  GtkWidget *hbuttonbox2;
  GtkWidget *button_to_parent;
  GtkWidget *button_to_sub;
  GtkWidget *scrolledwindow1;
/*  GtkWidget *clist_menuitems; */
  GtkWidget *label_col1;
  GtkWidget *label_col2;
  GtkWidget *label_col3;
  GtkWidget *hbuttonbox3;
	GtkWidget *arrow_up;
	GtkWidget *arrow_down;
  GtkWidget *button_up;
  GtkWidget *button_down;
  GtkWidget *vseparator1;
  GtkWidget *vbox3;
  GtkWidget *label_edit_section;
  GtkWidget *table_entries;
  GtkWidget *label_type;
  GtkWidget *label_caption;
  GtkWidget *label_command;
  GtkWidget *combo_type;
  GList *combo_type_items = NULL;
/*  GtkWidget *combo_entry_type; */
/*  GtkWidget *entry_caption; */
  GtkWidget *hbox_command;
/*  GtkWidget *entry_command; */
/*  GtkWidget *button_fileselect; */
  GtkWidget *vbuttonbox1;
  GtkWidget *button_add;
  GtkWidget *button_update;
  GtkWidget *button_remove;
  GtkWidget *hseparator1;
  GtkWidget *hbuttonbox1;
  GtkWidget *button_save;
  GtkWidget *button_reset;
  GtkWidget *button_quit;

  xfumed = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (xfumed), "xfumed", xfumed);
  gtk_container_set_border_width (GTK_CONTAINER (xfumed), 2);
  gtk_window_set_title (GTK_WINDOW (xfumed), _("XFce User Menu Editor"));

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (xfumed), vbox1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "vbox2", vbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

  frame_status = gtk_frame_new (NULL);
  gtk_widget_ref (frame_status);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "frame_status", frame_status,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_status);
  gtk_box_pack_start (GTK_BOX (vbox2), frame_status, FALSE, TRUE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (frame_status), 4);

  hbox_status = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_status);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "hbox_status", hbox_status,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_status);
  gtk_container_add (GTK_CONTAINER (frame_status), hbox_status); 

  label_status = gtk_label_new (NULL);
  gtk_widget_ref (label_status);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_status", 
                            label_status, 
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_status);
  gtk_box_pack_start (GTK_BOX (hbox_status), label_status, FALSE, TRUE, 2);
  gtk_misc_set_padding (GTK_MISC (label_status), 0, 2);
  gtk_label_set_justify(GTK_LABEL(label_status),GTK_JUSTIFY_LEFT);

  hbuttonbox2 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox2);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "hbuttonbox2", hbuttonbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbuttonbox2, FALSE, TRUE, 2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox2), GTK_BUTTONBOX_SPREAD);

  button_to_parent = gtk_button_new_with_label (_("Go to parent menu"));
  gtk_widget_ref (button_to_parent);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_to_parent", button_to_parent,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_to_parent);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), button_to_parent);
  GTK_WIDGET_SET_FLAGS (button_to_parent, GTK_CAN_DEFAULT);

  button_to_sub = gtk_button_new_with_label (_("Go to submenu"));
  gtk_widget_ref (button_to_sub);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_to_sub", button_to_sub,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_to_sub);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), button_to_sub);
  GTK_WIDGET_SET_FLAGS (button_to_sub, GTK_CAN_DEFAULT);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox2), scrolledwindow1, TRUE, TRUE, 10);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1),
                                  GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);

  clist_menuitems = gtk_clist_new (3);
  gtk_widget_ref (clist_menuitems);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "clist_menuitems", clist_menuitems,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (clist_menuitems);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist_menuitems);
  gtk_clist_set_column_width (GTK_CLIST (clist_menuitems), 0, 100);
  gtk_clist_set_column_width (GTK_CLIST (clist_menuitems), 1, 150);
  gtk_clist_set_column_width (GTK_CLIST (clist_menuitems), 2, 150);
  gtk_clist_column_titles_show (GTK_CLIST (clist_menuitems));
  gtk_clist_column_titles_passive(GTK_CLIST(clist_menuitems));

  label_col1 = gtk_label_new (_("Type"));
  gtk_widget_ref (label_col1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_col1", label_col1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_col1);
  gtk_clist_set_column_widget (GTK_CLIST (clist_menuitems), 0, label_col1);

  label_col2 = gtk_label_new (_("Caption"));
  gtk_widget_ref (label_col2);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_col2", label_col2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_col2);
  gtk_clist_set_column_widget (GTK_CLIST (clist_menuitems), 1, label_col2);

  label_col3 = gtk_label_new (_("Command"));
  gtk_widget_ref (label_col3);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_col3", label_col3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_col3);
  gtk_clist_set_column_widget (GTK_CLIST (clist_menuitems), 2, label_col3);

  hbuttonbox3 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox3);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "hbuttonbox3", hbuttonbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbuttonbox3, FALSE, TRUE, 2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox3), GTK_BUTTONBOX_SPREAD);

  button_up = gtk_button_new();/*_with_label ("/\\")*/
  gtk_widget_ref (button_up);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_up", button_up,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_up);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), button_up);
  GTK_WIDGET_SET_FLAGS (button_up, GTK_CAN_DEFAULT);
  
  arrow_up = gtk_arrow_new(GTK_ARROW_UP,GTK_SHADOW_IN);
  gtk_widget_ref (arrow_up);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "arrow_up", arrow_up,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show(arrow_up);
  gtk_container_add (GTK_CONTAINER (button_up), arrow_up);

  button_down = gtk_button_new();
  gtk_widget_ref (button_down);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_down", button_down,
                            (GtkDestroyNotify) gtk_widget_unref);

  arrow_down = gtk_arrow_new(GTK_ARROW_DOWN,GTK_SHADOW_IN);
  gtk_widget_ref (arrow_down);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "arrow_down", arrow_down,
                            (GtkDestroyNotify) gtk_widget_unref);
							
  gtk_widget_show(arrow_down);
  gtk_container_add (GTK_CONTAINER (button_down), arrow_down);

  gtk_widget_show (button_down);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), button_down);
  GTK_WIDGET_SET_FLAGS (button_down, GTK_CAN_DEFAULT);

  vseparator1 = gtk_vseparator_new ();
  gtk_widget_ref (vseparator1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "vseparator1", vseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vseparator1);
  gtk_box_pack_start (GTK_BOX (hbox1), vseparator1, FALSE, TRUE, 4);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox3, FALSE, TRUE, 0);

  label_edit_section = gtk_label_new (_("Edit item"));
  gtk_widget_ref (label_edit_section);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_edit_section", label_edit_section,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_edit_section);
  gtk_box_pack_start (GTK_BOX (vbox3), label_edit_section, FALSE, FALSE, 10);

  table_entries = gtk_table_new (3, 2, FALSE);
  gtk_widget_ref (table_entries);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "table_entries", table_entries,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table_entries);
  gtk_box_pack_start (GTK_BOX (vbox3), table_entries, FALSE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table_entries), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table_entries), 2);

  label_type = gtk_label_new (_("Type : "));
  gtk_widget_ref (label_type);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_type", label_type,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_type);
  gtk_table_attach (GTK_TABLE (table_entries), label_type, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label_type), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label_type), 0, 0.5);

  label_caption = gtk_label_new (_("Caption : "));
  gtk_widget_ref (label_caption);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_caption", label_caption,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_caption);
  gtk_table_attach (GTK_TABLE (table_entries), label_caption, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label_caption), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label_caption), 0, 0.5);

  label_command = gtk_label_new (_("Command : "));
  gtk_widget_ref (label_command);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "label_command", label_command,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_command);
  gtk_table_attach (GTK_TABLE (table_entries), label_command, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label_command), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label_command), 0, 0.5);

  combo_type = gtk_combo_new ();
  gtk_widget_ref (combo_type);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "combo_type", combo_type,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (combo_type);
  gtk_table_attach (GTK_TABLE (table_entries), combo_type, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_combo_set_value_in_list (GTK_COMBO (combo_type), TRUE, FALSE);
  combo_type_items = g_list_append (combo_type_items, 
                                        (gpointer) TYPE_ITEM_TEXT);
  combo_type_items = g_list_append (combo_type_items, 
                                        (gpointer)  TYPE_SUBMENU_TEXT);
  combo_type_items = g_list_append (combo_type_items, 
                                        (gpointer)  TYPE_NOP_TEXT);
  gtk_combo_set_popdown_strings (GTK_COMBO (combo_type), combo_type_items);
  g_list_free (combo_type_items);

  combo_entry_type = GTK_COMBO (combo_type)->entry;
  gtk_widget_ref (combo_entry_type);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "combo_entry_type", combo_entry_type,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (combo_entry_type);
  gtk_entry_set_editable (GTK_ENTRY (combo_entry_type), FALSE);
  gtk_entry_set_text (GTK_ENTRY (combo_entry_type), TYPE_ITEM_TEXT);

  entry_caption = gtk_entry_new ();
  gtk_widget_ref (entry_caption);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "entry_caption", entry_caption,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry_caption);
  gtk_table_attach (GTK_TABLE (table_entries), entry_caption, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  hbox_command = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_command);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "hbox_command", hbox_command,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_command);
  gtk_table_attach (GTK_TABLE (table_entries), hbox_command, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  entry_command = gtk_entry_new ();
  gtk_widget_ref (entry_command);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "entry_command", entry_command,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry_command);
  gtk_box_pack_start (GTK_BOX (hbox_command), entry_command, TRUE, TRUE, 0);

  button_fileselect = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (button_fileselect);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_fileselect", button_fileselect,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_fileselect);
  gtk_box_pack_start (GTK_BOX (hbox_command), button_fileselect, FALSE, FALSE, 0);

  vbuttonbox1 = gtk_vbutton_box_new ();
  gtk_widget_ref (vbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "vbuttonbox1", vbuttonbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox3), vbuttonbox1, TRUE, TRUE, 10);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox1), GTK_BUTTONBOX_START);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (vbuttonbox1), 80, 0);

  button_add = gtk_button_new_with_label (_("Add"));
  gtk_widget_ref (button_add);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_add", button_add,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_add);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), button_add);
  GTK_WIDGET_SET_FLAGS (button_add, GTK_CAN_DEFAULT);

  button_update = gtk_button_new_with_label (_("Update"));
  gtk_widget_ref (button_update);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_update", button_update,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_update);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), button_update);
  GTK_WIDGET_SET_FLAGS (button_update, GTK_CAN_DEFAULT);

  button_remove = gtk_button_new_with_label (_("Remove"));
  gtk_widget_ref (button_remove);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_remove", button_remove,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_remove);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), button_remove);
  GTK_WIDGET_SET_FLAGS (button_remove, GTK_CAN_DEFAULT);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, TRUE, 4);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "hbuttonbox1", hbuttonbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, TRUE, 2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_SPREAD);

  button_save = gtk_button_new_with_label (_("Save"));
  gtk_widget_ref (button_save);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_save", button_save,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_save);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_save);
  GTK_WIDGET_SET_FLAGS (button_save, GTK_CAN_DEFAULT);

  button_reset = gtk_button_new_with_label (_("Reset"));
  gtk_widget_ref (button_reset);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_reset", button_reset,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_reset);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_reset);
  GTK_WIDGET_SET_FLAGS (button_reset, GTK_CAN_DEFAULT);

  button_quit = gtk_button_new_with_label (_("Quit"));
  gtk_widget_ref (button_quit);
  gtk_object_set_data_full (GTK_OBJECT (xfumed), "button_quit", button_quit,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_quit);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_quit);
  GTK_WIDGET_SET_FLAGS (button_quit, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (xfumed), "delete_event",
                      GTK_SIGNAL_FUNC (on_xfumed_delete_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (xfumed), "destroy_event",
                      GTK_SIGNAL_FUNC (on_xfumed_destroy_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_to_parent), "clicked",
                      GTK_SIGNAL_FUNC (on_button_to_parent_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_to_sub), "clicked",
                      GTK_SIGNAL_FUNC (on_button_to_sub_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (clist_menuitems), "select_row",
                      GTK_SIGNAL_FUNC (on_clist_menuitems_select_row),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (clist_menuitems), "unselect_row",
                      GTK_SIGNAL_FUNC (on_clist_menuitems_unselect_row),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_up), "clicked",
                      GTK_SIGNAL_FUNC (on_button_up_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_down), "clicked",
                      GTK_SIGNAL_FUNC (on_button_down_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (combo_entry_type), "changed",
                      GTK_SIGNAL_FUNC (on_combo_entry1_changed),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_fileselect), "clicked",
                      GTK_SIGNAL_FUNC (on_button_fileselect_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_add), "clicked",
                      GTK_SIGNAL_FUNC (on_button_add_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_update), "clicked",
                      GTK_SIGNAL_FUNC (on_button_update_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_remove), "clicked",
                      GTK_SIGNAL_FUNC (on_button_remove_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_save), "clicked",
                      GTK_SIGNAL_FUNC (on_button_save_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_reset), "clicked",
                      GTK_SIGNAL_FUNC (on_button_reset_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_quit), "clicked",
                      GTK_SIGNAL_FUNC (on_button_quit_clicked),
                      NULL);

  return xfumed;
}

GtkWidget* create_dialog_quit(void)
{
  GtkWidget *quit_dialog;
  gchar * message = _("There are unsaved changes to the menu.\n\n"
                      "Do you want to quit and lose the changes ?");
  gint nbuttons=2;
  gchar * buttons[] = { _("Don't Quit") , _("Quit, don't save") };
  gint default_button = 1;
  GtkSignalFunc signal_handlers[ ] = { 
                GTK_SIGNAL_FUNC(on_button_cancel_quit_clicked),
                GTK_SIGNAL_FUNC(on_button_continue_quit_clicked) };
  
  quit_dialog =  my_show_dialog (message, nbuttons, buttons, default_button,
                          signal_handlers, NULL);
  gtk_signal_connect ( GTK_OBJECT(quit_dialog),"delete_event",
                          GTK_SIGNAL_FUNC(on_quit_dialog_delete),
                          GTK_OBJECT(quit_dialog) );
  return quit_dialog;
}