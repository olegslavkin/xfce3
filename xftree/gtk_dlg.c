/*
 * gtk_dlg.c
 *
 * Copyright (C) 1999 Rasca, Berlin
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
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "gtk_dlg.h"
#include "gtk_dnd.h"
#include "icons/warning.xpm"
#include "icons/info.xpm"
#include "icons/question.xpm"
#include "icons/error.xpm"
#include "xpmext.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define B_WIDTH		80
#define B_HEIGHT	35
#define E_WIDTH		260

typedef struct
{
  GtkWidget *top;
  GtkWidget *entry;
  void *data;
  int result;
  int type;
}
dlg;

static dlg dl;

static GtkTargetEntry target_table[] = {
  {"STRING", 0, TARGET_STRING},
#	define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))
};

/*
 * called if user presses cancel button or ESC
 */
static void
on_cancel (GtkWidget * btn, gpointer * data)
{
  if ((int) ((long) data) != DLG_RC_DESTROY)
  {
    gtk_widget_destroy (dl.top);
  }
  dl.result = DLG_RC_CANCEL;
  gtk_main_quit ();
}

/*
 * called if user presses ok button
 */
static void
on_ok (GtkWidget * ok, gpointer * data)
{
  if (dl.entry)
  {
    if (dl.data)
      sprintf (dl.data, "%s", gtk_entry_get_text (GTK_ENTRY (dl.entry)));
  }
  gtk_widget_destroy (dl.top);

  dl.result = (int) ((long) data);
  gtk_main_quit ();
}

/*
 * call on_ok if user presses RETURN in entry widget
 */
static gint
on_key_press (GtkWidget * entry, GdkEventKey * event, gpointer cancel)
{
  if (event->keyval == GDK_Escape)
  {
    on_cancel ((GtkWidget *) cancel, (gpointer) ((long) DLG_RC_CANCEL));
    return (TRUE);
  }
  return (FALSE);
}

/*
 * user drops some data on the entry
 */
static void
on_drag_data_received (GtkWidget * entry, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, void *client)
{
  int len;
  char *text;

  if ((data->length == 0) || (data->length > DLG_MAX) || (data->format != 8) || (info != TARGET_STRING))
  {
    gtk_drag_finish (context, FALSE, TRUE, time);
  }
  /* ensure that we have a terminating null byte
   */
  len = data->length;
  text = g_malloc (len + 1);
  text[len] = '\0';
  strncpy (text, (gchar *) data->data, len);
  if (text[len - 1] == '\n')
  {
    text[len - 1] = '\0';
  }
  gtk_entry_set_text (GTK_ENTRY (entry), text);
  gtk_drag_finish (context, TRUE, TRUE, time);
  g_free (text);
}

/* -----------------------------------------------------------------------------
 * -----------------------------------------------------------------------------
 * Changes for dialog button accelerators
 * SJB
 */
 
/* Type used to pass additional args to the callback */
typedef struct
{
	GtkWidget 	  *bttn;    /* the button we are adding the accelerator	to. */
	GtkAccelGroup *accelgrp;    /* the accelerator group for the new accelerator. */
} AccelBttnInfo;

/* -----------------------------------------------------------------------------
 * Adds an accelerator to the supplied button using the text in the label widget.
 * If the label text contains an underscore, the subsequent character in the label
 * becomes the accelerator key for the button. The label is altered so that the
 * accelerator is underlined.
 */
 
static void make_accel_callback(GtkWidget *labelwidget, gpointer bttndata)
{
	AccelBttnInfo *data = bttndata;
	gchar *label;
	guint key;

	gtk_label_get(GTK_LABEL(labelwidget), &label);

	key = gtk_label_parse_uline(GTK_LABEL(labelwidget), label);
	if(key != GDK_VoidSymbol)
		gtk_widget_add_accelerator(data->bttn, "clicked", data->accelgrp, key, 0, 0);
}

/* -----------------------------------------------------------------------------
 * Create a button with an associated accelerator key, generated from the text
 * in the label. The accelerator is added to the given accelgrp.
 */
 
static GtkWidget *make_button_with_accel(gchar *label, GtkAccelGroup *accelgrp)
{
	AccelBttnInfo bttninfo;

	GtkWidget *bttn = NULL;

	bttn = gtk_button_new_with_label(_(label));
  if(bttn != NULL)
  {
	  bttninfo.bttn = bttn;
	  bttninfo.accelgrp = accelgrp;

	  gtk_container_foreach(GTK_CONTAINER(bttn), make_accel_callback, &bttninfo);
  }

	return bttn;
}
/* SJB
 * -----------------------------------------------------------------------------
 */

/*
 * create a modal dialog and handle it
 */
gint dlg_new (char *labelval, char *defval, void *data, int type)
{
  GtkWidget *ok = NULL, *cancel = NULL, *all = NULL, *skip = NULL, *close = NULL, *icon = NULL, *combo = NULL, *label, *box, *button_box;
  char title[DLG_MAX];
  char *longlabel = NULL;
  GdkPixmap *pix = NULL, *pim;
  GtkAccelGroup *accelgrp;

  dl.result = 0;
  dl.type = type;
  dl.entry = NULL;
  dl.data = defval;

  dl.top = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dl.top), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (dl.top), TRUE);
  box = gtk_hbox_new (FALSE, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->vbox), box, TRUE, TRUE, 0);
  gtk_widget_realize (dl.top);

  /* SJB */
	accelgrp = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(dl.top), accelgrp);

  /* what kind of pixmap do we want to use..?
   */
  if (type & DLG_QUESTION)
  {
    pix = MyCreateGdkPixmapFromData (question_xpm, dl.top, &pim, FALSE);
    sprintf (title, _("Question"));
  }
  else if (type & DLG_INFO)
  {
    pix = MyCreateGdkPixmapFromData (info_xpm, dl.top, &pim, FALSE);
    sprintf (title, _("Information"));
  }
  else if (type & DLG_ERROR)
  {
    pix = MyCreateGdkPixmapFromData (error_xpm, dl.top, &pim, FALSE);
    sprintf (title, _("Error"));
  }
  else if (type & DLG_WARN)
  {
    pix = MyCreateGdkPixmapFromData (warning_xpm, dl.top, &pim, FALSE);
    sprintf (title, _("Warning"));
  }
  else
  {
    sprintf (title, _("Dialog"));
  }
  if (pix)
  {
    icon = gtk_pixmap_new (pix, pim);
    gtk_box_pack_start (GTK_BOX (box), icon, FALSE, FALSE, 0);
  }
  gtk_window_set_title (GTK_WINDOW (dl.top), title);

  button_box = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), button_box, TRUE, FALSE, 0);

  /* create the requested buttons..
   */
  if (type & (DLG_OK | DLG_YES | DLG_CONTINUE))
  {
    if (type & DLG_OK)
    {
      /* SJB ok = gtk_button_new_with_label (_("Ok")); */
      ok = make_button_with_accel(_("_Ok"), accelgrp);
    }
    else if (type & DLG_YES)
    {
      /* SJB ok = gtk_button_new_with_label (_("Yes")); */
      ok = make_button_with_accel(_("_Yes"), accelgrp);
    }
    else
    {
      /* SJB ok = gtk_button_new_with_label (_("Continue")); */
      ok = make_button_with_accel(_("_Continue"), accelgrp);
    }
    GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (button_box), ok, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_OK));
    gtk_widget_set_usize (ok, B_WIDTH, B_HEIGHT);
    gtk_signal_connect (GTK_OBJECT (ok), "key_press_event", GTK_SIGNAL_FUNC (on_key_press), (gpointer) cancel);
  }
  if (type & DLG_SKIP)
  {
    skip = gtk_button_new_with_label (_("Skip"));
    GTK_WIDGET_SET_FLAGS (skip, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (button_box), skip, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (skip), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_SKIP));
    gtk_widget_set_usize (skip, B_WIDTH, B_HEIGHT);
  }
  if (type & (DLG_CANCEL | DLG_NO))
  {
    if (type & DLG_CANCEL)
    {
      /* SJB cancel = gtk_button_new_with_label (_("Cancel")); */
      cancel = make_button_with_accel(_("_Cancel"), accelgrp);
    }
    else
    {
      /* SJB cancel = gtk_button_new_with_label (_("No")); */
      cancel = make_button_with_accel(_("_No"), accelgrp);
    }
    GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (button_box), cancel, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (on_cancel), (gpointer) ((long) DLG_RC_CANCEL));
    gtk_widget_set_usize (cancel, B_WIDTH, B_HEIGHT);
    gtk_widget_grab_default (cancel);
  }
  if (type & DLG_ALL)
  {
    /* SJB all = gtk_button_new_with_label (_("All")); */
    all = make_button_with_accel(_("_All"), accelgrp);
    
    GTK_WIDGET_SET_FLAGS (all, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (button_box), all, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (all), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_ALL));
    gtk_widget_set_usize (all, B_WIDTH, B_HEIGHT);
  }
  if (type & DLG_CLOSE)
  {
    /* SJB close = gtk_button_new_with_label (_("Close")); */
    close = make_button_with_accel(_("_Close"), accelgrp);
    
    GTK_WIDGET_SET_FLAGS (close, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (button_box), close, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (close), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_OK));
  }

  if (type & DLG_ENTRY_VIEW)
  {
    dl.entry = gtk_entry_new_with_max_length (DLG_MAX);
    gtk_widget_set_usize (dl.entry, E_WIDTH, -1);
    gtk_entry_set_editable (GTK_ENTRY (dl.entry), FALSE);
    gtk_widget_set_sensitive (dl.entry, FALSE);
  }
  else if (type & DLG_ENTRY_EDIT)
  {
    dl.entry = gtk_entry_new_with_max_length (DLG_MAX);
    gtk_widget_set_usize (dl.entry, E_WIDTH, -1);
    GTK_WIDGET_SET_FLAGS (dl.entry, GTK_CAN_DEFAULT);
    gtk_drag_dest_set (dl.entry, GTK_DEST_DEFAULT_ALL, target_table, NUM_TARGETS, GDK_ACTION_COPY);
    gtk_signal_connect (GTK_OBJECT (dl.entry), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data_received), NULL);
  }
  else if (type & DLG_COMBO)
  {
    combo = gtk_combo_new ();
    gtk_editable_select_region (GTK_EDITABLE (GTK_COMBO (combo)->entry), 0, -1);
    gtk_combo_disable_activate (GTK_COMBO (combo));
    dl.entry = GTK_COMBO (combo)->entry;
    gtk_drag_dest_set (dl.entry, GTK_DEST_DEFAULT_ALL, target_table, NUM_TARGETS, GDK_ACTION_COPY);
    gtk_signal_connect (GTK_OBJECT (dl.entry), "activate", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_OK));
    gtk_signal_connect (GTK_OBJECT (dl.entry), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data_received), NULL);
  }
  else
  {
    if (defval)
    {
      longlabel = g_malloc (strlen (labelval) + strlen (defval) + 5);
      sprintf (longlabel, "%s: %s", labelval, (char *) defval);
      labelval = longlabel;
    }
  }

  label = gtk_label_new (labelval);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 3);

  if (dl.entry)
  {
    if (type & DLG_COMBO)
    {
      if (data)
      {
	gtk_combo_set_popdown_strings (GTK_COMBO (combo), (GList *) data);
      }
      gtk_box_pack_start (GTK_BOX (box), combo, TRUE, TRUE, 3);
      gtk_signal_connect (GTK_OBJECT (dl.entry), "key_press_event", GTK_SIGNAL_FUNC (on_key_press), (gpointer) cancel);
    }
    else
    {
      gtk_box_pack_start (GTK_BOX (box), dl.entry, TRUE, TRUE, 3);
      gtk_signal_connect_object (GTK_OBJECT (dl.entry), "activate", GTK_SIGNAL_FUNC (gtk_button_clicked), GTK_OBJECT (ok));
      gtk_widget_grab_focus (dl.entry);
      gtk_signal_connect (GTK_OBJECT (dl.entry), "key_press_event", GTK_SIGNAL_FUNC (on_key_press), (gpointer) cancel);
    }
    if (defval)
      gtk_entry_set_text (GTK_ENTRY (dl.entry), defval);
    if (type & DLG_ENTRY_EDIT)
      gtk_entry_select_region (GTK_ENTRY (dl.entry), 0, -1);
  }
  gtk_signal_connect (GTK_OBJECT (dl.top), "destroy", GTK_SIGNAL_FUNC (on_cancel), (gpointer) ((long) DLG_RC_DESTROY));
  gtk_signal_connect (GTK_OBJECT (dl.top), "key_press_event", GTK_SIGNAL_FUNC (on_key_press), (gpointer) cancel);
  gtk_widget_show_all (dl.top);
  gtk_main ();
  if (longlabel)
    g_free (longlabel);
  return (dl.result);
}
