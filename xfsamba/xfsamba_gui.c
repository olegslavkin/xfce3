/*  gui modules for xfsamba
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* for _( definition, it also includes config.h : */
#include "my_intl.h"
#include "constant.h"
/* for pixmap creation routines : */
#include "xfce-common.h"
#include "xpmext.h"

/* local xfsamba includes : */
#undef XFSAMBA_MAIN
#include "xfsamba.h"
#include "tubo.h"

static GtkWidget *vpaned, *hpaned, *vpaned2, *show_links, *hide_links, *show_diag, *hide_diag;
/* diagnostics=0x01, links=0x10 :*/
int view_toggle = 0x11;
static GtkWidget * user, *passwd, *dialog;
static nmb_cache *current_cache;

void
node_destroy (gpointer p)
{
  int *data = (int *) p;
  free (data);
}

static gint
compare (GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
  GtkCTreeRow *row1 = (GtkCTreeRow *) ptr1;
  GtkCTreeRow *row2 = (GtkCTreeRow *) ptr2;
  int i, *i1, *i2;

  i1 = row1->row.data;
  i2 = row2->row.data;
  switch (clist->sort_column)
  {
  case 2:
    i = 0;
    break;
  case 3:
    i = 1;
    break;
  default:
    i = 0;
  }

  return i1[i] - i2[i];
}

static gint
on_click_column (GtkCList * clist, gint column, gpointer data)
{
  GtkCTreeNode *node;
  GList *selection;

  if (clist != GTK_CLIST (shares))
    gtk_clist_set_compare_func (clist, NULL);
  else
  {
    if ((column == 2) || (column == 3))
    {
      gtk_clist_set_compare_func (clist, compare);
    }
    else
    {
      gtk_clist_set_compare_func (clist, NULL);
    }
  }

  if (column != clist->sort_column)
    gtk_clist_set_sort_column (clist, column);
  else
  {
    if (clist->sort_type == GTK_SORT_ASCENDING)
      clist->sort_type = GTK_SORT_DESCENDING;
    else
      clist->sort_type = GTK_SORT_ASCENDING;
  }
  selection = clist->selection;
  if (selection)
  {
    do
    {
      node = selection->data;
      if (!GTK_CTREE_ROW (node)->children || (!GTK_CTREE_ROW (node)->expanded))
      {
	node = GTK_CTREE_ROW (node)->parent;
      }
      gtk_ctree_sort_node (GTK_CTREE (clist), node);
      selection = selection->next;
    }
    while (selection);
  }
  else
  {
    gtk_clist_sort (clist);
  }
  return TRUE;
}


static void
cb_master (GtkWidget * item, GtkWidget * ctree)
{
  my_show_message (_("If you have win95 nodes on your network, xfsamba might not find\n" "a master browser. If you start smb services on your linux box,\n" "making it a samba-server, the problem will be fixed as long as\n" "the win95 box(es) are reset. You know the routine, reset wind*ws\n" "for changes to take effect. Otherwise,\n" "you must type in the node to be browsed and hit RETURN.\n"));
}

static void
cb_about (GtkWidget * item, GtkWidget * ctree)
{
  my_show_message (_("This is XFSamba " XFSAMBA_VERSION "\n(c) Edscott Wilson Garcia under GNU GPL" "\nXFCE modules are (c) Olivier Fourdan http://www.xfce.org/"));
}

#ifdef OBSOLETE
static void
configure (void)
{
  static int call = 0;
  GdkEvent event;
  GdkEventConfigure *configure;
  configure = (GdkEventConfigure *) (&event);
  event.type = GDK_CONFIGURE;
  configure->width = smb_nav->allocation.width;
  configure->height = smb_nav->allocation.height;
  if ((call++) % 2)
    configure->height++;
  else
    configure->height--;	/*hack */
  gtk_propagate_event (smb_nav, &event);
}
#endif

static void
cb_view (GtkWidget * widget, gpointer data)
{
  int caso;
  caso = (int) ((long) data);
  if (caso & 0xf00)
  {
    switch (caso)
    {
    case 0x100:
      view_toggle = 0x0;
      break;
    case 0x200:
      view_toggle = 0x10;
      break;
    case 0x400:
      view_toggle = 0x11;
      break;
    }
  }
  else
    view_toggle ^= caso;

  if (view_toggle & 0x01)
  {
    gtk_widget_hide (show_diag);
    gtk_widget_show (hide_diag);
    gtk_paned_set_position (GTK_PANED (vpaned), vpaned->allocation.height * 0.75);

  }
  else
  {
    gtk_widget_hide (hide_diag);
    gtk_widget_show (show_diag);
    gtk_paned_set_position (GTK_PANED (vpaned), vpaned->allocation.height);
  }

  if (view_toggle & 0x10)
  {
    gtk_widget_hide (show_links);
    gtk_widget_show (hide_links);
    gtk_paned_set_position (GTK_PANED (vpaned2), vpaned2->allocation.height / 2);
    gtk_paned_set_position (GTK_PANED (hpaned), hpaned->allocation.width / 2);
  }
  else
  {
    gtk_widget_hide (hide_links);
    gtk_widget_show (show_links);
    gtk_paned_set_position (GTK_PANED (vpaned2), vpaned2->allocation.height);
  }
}



static void
destroy_dialog (GtkWidget * widget, gpointer data)
{
  if (SMBResult == CHALLENGED)
  {
#ifdef DBG_XFSAMBA
    print_diagnostics ("DBG:Unflagging entry in primary cache\n");
#endif
    if (current_cache)
      current_cache->visited = 0;
    /*      
       pop_cache(thisN->shares); 
     */
    SMBResult = SUCCESS;
  }
  if (SMBResult == CHALLENGED)
  {
    SMBResult = SUCCESS;
  }
  gtk_widget_destroy ((GtkWidget *) data);
}

static int passwd_caso;

static void
ok_dialog (GtkWidget * widget, gpointer data)
{
  char *s, *t;
  int caso;

  caso = (int) ((long) data);

  s = gtk_entry_get_text (GTK_ENTRY (user));
  t = gtk_entry_get_text (GTK_ENTRY (passwd));

  if (thisN)
  {
    if (thisN->password)
      free (thisN->password);
    thisN->password = (unsigned char *) malloc (strlen (s) + strlen (t) + 2);
    if (strlen (t) > 0)
      sprintf (thisN->password, "%s%%%s", s, t);
    else
      sprintf (thisN->password, "%s", s);
  }

  if (passwd_caso == 1)
  {
    gtk_widget_destroy (dialog);
    if (SMBResult == CHALLENGED)
    {
      SMBResult = SUCCESS;
      if (selected.directory)
      {
	SMBList ();
      }
      else
      {				/* browsing has been done at netbios: */
	SMBrefresh (thisN->netbios, FORCERELOAD);
      }
    }
  }
  else
  {
    if (default_user)
      free (default_user);
    default_user = (char *) malloc (strlen (s) + strlen (t) + 2);
    if (strlen (t) > 0)
      sprintf (default_user, "%s%%%s", s, t);
    else
      sprintf (default_user, "%s", s);
    gtk_widget_destroy (dialog);
  }
}
static void
entry_keypress (GtkWidget * entry, GdkEventKey * event, gpointer data)
{
  if (event->keyval == GDK_Return)
  {
    if (entry == user)
      gtk_widget_grab_focus (passwd);
    if (entry == passwd)
      ok_dialog (NULL, NULL);
  }
  return;

}


GtkWidget *
passwd_dialog (int caso)
{
  GtkWidget *button, *hbox, *label;

  passwd_caso = caso;

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

  if (caso == 1)
    label = gtk_label_new (_("Please provide information for server "));
  else
    label = gtk_label_new (_("Please provide browsing preferences "));
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  if (caso == 1)
  {
    label = gtk_label_new (thisN->server);
    gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
    gtk_widget_show (label);
  }

  gtk_widget_show (hbox);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Username : "));
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  user = gtk_entry_new ();
  if ((thisN) && (thisN->password))
  {
    strtok (thisN->password, "\%");
    if (strstr (thisN->password, "Guest") == NULL)
      gtk_entry_set_text ((GtkEntry *) user, thisN->password);
  }
  gtk_box_pack_start (GTK_BOX (hbox), user, EXPAND, NOFILL, 0);
  gtk_signal_connect (GTK_OBJECT (user), "key-press-event", GTK_SIGNAL_FUNC (entry_keypress), NULL);
  gtk_widget_show (user);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Password  : "));
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  passwd = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), passwd, EXPAND, NOFILL, 0);
  gtk_entry_set_visibility ((GtkEntry *) passwd, FALSE);
  gtk_signal_connect (GTK_OBJECT (passwd), "key-press-event", GTK_SIGNAL_FUNC (entry_keypress), NULL);
  gtk_widget_show (passwd);


  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (ok_dialog), (gpointer) ((long) caso));
  button = gtk_button_new_with_label ("Cancel");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (destroy_dialog), (gpointer) dialog);
  gtk_widget_show (dialog);
  gtk_widget_grab_focus (user);
  return dialog;
}

static void
select_user (void)
{
  gtk_window_set_transient_for (GTK_WINDOW (passwd_dialog (0)), GTK_WINDOW (smb_nav));

}


void
cb_download (GtkWidget * widget, gpointer data)
{
  if (selected.file)
    SMBGetFile ();
  else
    my_show_message (_("A file must be selected!"));
}

void
cb_upload (GtkWidget * widget, gpointer data)
{
  if (selected.directory)
    SMBPutFile ();
  else
    my_show_message (_("A directory must be selected!"));
}

void
cb_new_dir (GtkWidget * widget, gpointer data)
{
  if (selected.directory)
    SMBmkdir ();
  else
    my_show_message (_("A directory must be selected!"));
}

void
cb_delete (GtkWidget * widget, gpointer data)
{
  if ((selected.directory) || (selected.file))
    SMBrm ();
  else
  {
    my_show_message (_("Something to be deleted must be selected!"));
  }
}

void
cb_tar (GtkWidget * widget, gpointer data)
{
  if (selected.directory)
  {
    SMBtar ();
  }
  else
  {
    my_show_message (_("A directory must be selected!"));
  }
}


void
select_share (GtkCTree * ctree, GList * node, gint column, gpointer user_data)
{
  nmb_cache *cache;
  char *line[3];

  if (!gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) node, SHARE_NAME_COLUMN, line))
    return;
  if (!gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) node, COMMENT_COLUMN, line + 1))
    return;


  selected.directory = selected.file = FALSE;
  if (selected.share)
    free (selected.share);
  selected.share = NULL;
  if (selected.dirname)
    free (selected.dirname);
  selected.dirname = NULL;
  if (selected.filename)
    free (selected.filename);
  selected.filename = NULL;
  if (selected.comment)
    free (selected.comment);
  selected.comment = NULL;

  selected.comment = (char *) malloc (strlen (line[1]) + 1);
  strcpy (selected.comment, line[1]);

  selected.node = node;		/* this is leaf or node */
  selected.parent_node = NULL;

  if (!strncmp (selected.comment, "Disk", strlen ("Disk")))
  {
    selected.directory = TRUE;
  }

  if ((selected.comment[0] == '/') && (selected.comment[1]))
  {
    selected.directory = TRUE;
    selected.parent_node = node;
  }

  if ((GTK_CTREE_ROW (node)->parent) && (line[1][0] != '/'))
  {
    selected.file = TRUE;
    selected.parent_node = (GList *) (GTK_CTREE_ROW (node)->parent);
  }

/* get the share and dirname: */
  if (selected.parent_node == NULL)
  {				/* top level node */
    selected.share = (char *) malloc (strlen (line[0]) + 1);
    sprintf (selected.share, "%s", line[0]);
    selected.dirname = (char *) malloc (3);
    sprintf (selected.dirname, "/");
  }
  else
  {				/* something not top level */
    char *word;
    int i;
    if (!gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) selected.parent_node, COMMENT_COLUMN, line + 1))
    {
      print_diagnostics ("DBG:unable to get parent information\n");
      return;
    }
    if (strncmp (line[1], "Disk", strlen ("Disk")) == 0)
    {
      gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) selected.parent_node, SHARE_NAME_COLUMN, line + 1);
      selected.share = (char *) malloc (strlen (line[1]) + 1);
      strcpy (selected.share, line[1]);
      selected.dirname = (char *) malloc (strlen ("/") + 1);
      sprintf (selected.dirname, "/");
    }
    else
    {
      word = line[1] + 1;
      selected.share = (char *) malloc (strlen (word) + 1);
      i = 0;
      while ((word[i] != '/') && (word[i]))
      {
	selected.share[i] = word[i];
	i++;
      }
      selected.share[i] = 0;

      if (strstr (word, "/"))
      {
	word = word + i + 1;
	while (word[strlen (word) - 1] == ' ')
	  word[strlen (word) - 1] = 0;

	selected.dirname = (char *) malloc (strlen (word) + 2);
	sprintf (selected.dirname, "/%s", word);
      }
      else
      {
	selected.dirname = (char *) malloc (strlen ("/") + 1);
	sprintf (selected.dirname, "/");
      }
    }
  }

#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:comment=");
  print_diagnostics (selected.comment);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:share=");
  print_diagnostics (selected.share);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:dirname=");
  print_diagnostics (selected.dirname);
  print_diagnostics ("\n");
#endif

  /* get file name, if applicable */
  if (selected.file)
  {
    char *word;
    word = line[0];
    while (word[strlen (word) - 1] == ' ')
      word[strlen (word) - 1] = 0;
    selected.filename = (char *) malloc (strlen (word) + 1);
    sprintf (selected.filename, "%s", word);
    /*      SMBGetFile(); */
#ifdef DBG_XFSAMBA
    print_diagnostics ("DBG:filename=");
    print_diagnostics (selected.filename);
    print_diagnostics ("\n");
#endif
    return;			/* nothing else to do for simple file */
  }
  else
  {
    selected.filename = NULL;
  }

/* determine whether or not to modify ctree: */

  cache = thisN->shares;

#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:Looking into cache for ");
  print_diagnostics (line[(GTK_CTREE_ROW (node)->parent) ? 1 : 0]);
  print_diagnostics ("\n");
#endif

  while (cache)
  {
    if (!(GTK_CTREE_ROW (node)->parent))
    {				/* look into first level cache */
      if (cache->textos[1])
      {
/*fprintf(stderr,"%s<->%s#1\n",cache->textos[1],line[0]); */
	if (strcmp (cache->textos[1], line[0]) == 0)
	{
	  if (cache->visited)
	  {
#ifdef DBG_XFSAMBA
	    print_diagnostics ("DBG:Found in cache. \n");
#endif
	    return;
	  }
	  cache->visited = 1;
	  current_cache = cache;
	  break;
	}
      }
    }
    else if (cache->textos[2])
    {				/* look into second level cache */
/* fprintf(stderr,"%s<->%s#2\n",cache->textos[2],line[1]); */
      if (strcmp (cache->textos[2], line[1]) == 0)
      {
#ifdef DBG_XFSAMBA
	print_diagnostics ("DBG:Found in cache. \n");
#endif
	current_cache = NULL;
	return;
      }
    }
    cache = cache->next;
  }
  if (!cache)
  {
    char *textos[3];
#ifdef DBG_XFSAMBA
    print_diagnostics ("DBG:Not found in cache.\n");
#endif
    textos[0] = textos[1] = NULL;
    textos[2] = line[1];
    push_nmb_cache (thisN->shares, textos);
    current_cache = NULL;
  }


  gtk_ctree_expand (ctree, (GtkCTreeNode *) node);
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:SMBList ");
#endif
  SMBList ();
}



void
select_server (GtkCList * clist, gint row, gint column, GdkEventButton * event, gpointer user_data)
{
  nmb_cache *cache;
  char *line[3];
  selected.directory = selected.file = FALSE;
  if (selected.share)
    free (selected.share);
  selected.share = NULL;
  if (selected.dirname)
    free (selected.dirname);
  selected.dirname = NULL;
  if (selected.filename)
    free (selected.filename);
  selected.filename = NULL;
  if (selected.comment)
    free (selected.comment);
  selected.comment = NULL;

  gtk_clist_get_text (clist, row, SERVER_NAME_COLUMN, line);
  if (line[0] == NULL)
    return;
  cache = thisN->servers;
  while (cache)
  {
    if (strcmp (cache->textos[SERVER_NAME_COLUMN], line[0]) == 0)
    {
      cache->visited = 1;
      break;
    }
    cache = cache->next;
  }
  SMBrefresh ((unsigned char *) (line[0]), RELOAD);
}

void
select_workgroup (GtkCList * clist, gint row, gint column, GdkEventButton * event, gpointer user_data)
{
  nmb_cache *cache;
  char *line[3];
  selected.directory = selected.file = FALSE;
  if (selected.share)
    free (selected.share);
  selected.share = NULL;
  if (selected.dirname)
    free (selected.dirname);
  selected.dirname = NULL;
  if (selected.filename)
    free (selected.filename);
  selected.filename = NULL;
  if (selected.comment)
    free (selected.comment);
  selected.comment = NULL;
  gtk_clist_get_text (clist, row, WG_MASTER_COLUMN, line);
  if (line[0] == NULL)
    return;
  cache = thisN->workgroups;
  while (cache)
  {
    if (strcmp (cache->textos[WG_MASTER_COLUMN], line[0]) == 0)
    {
      cache->visited = 1;
      break;
    }
    cache = cache->next;
  }
  SMBrefresh ((unsigned char *) (line[0]), RELOAD);
}

void
select_combo_server (GtkWidget * widget, gpointer data)
{
  char *s;
  s = gtk_entry_get_text (GTK_ENTRY (widget));
  /* is it in history list? */
  nonstop = TRUE;
  SMBrefresh ((unsigned char *) s, RELOAD);
}

void
go_reload (GtkWidget * widget, gpointer data)
{
  if (thisN)
    SMBrefresh (thisN->netbios, FORCERELOAD);
  else
    SMBrefresh (NULL, REFRESH);
}

void
go_home (GtkWidget * widget, gpointer data)
{
  print_status (_("Going to master browser..."));
  SMBCleanLevel2 ();
  SMBrefresh (NULL, REFRESH);
}

void
go_back (GtkWidget * widget, gpointer data)
{
  if ((thisH) && (thisH->previous))
  {
    print_status (_("Going back..."));
    SMBCleanLevel2 ();
    thisH = thisH->previous;
    thisN = thisH->record;
    SMBrefresh (thisN->netbios, REFRESH);
  }
}
void
go_forward (GtkWidget * widget, gpointer data)
{
  if ((thisH) && (thisH->next))
  {
    print_status (_("Going forward..."));
    SMBCleanLevel2 ();
    thisH = thisH->next;
    thisN = thisH->record;
    SMBrefresh (thisN->netbios, REFRESH);
  }
}

void
go_stop (GtkWidget * widget, gpointer data)
{

  if (fork_obj)
  {
    if (nonstop)
    {
      print_status (_("Attempting to stop query..."));	/* this is called atole */
      return;
    }

    print_status (_("Query stopped."));
    if (stopcleanup)
      TuboCancel (fork_obj, clean_nmb);
    else
      TuboCancel (fork_obj, NULL);
    cursor_reset (GTK_WIDGET (smb_nav));
    animation (FALSE);
    fork_obj = NULL;
  }				/* else print_status(_("Nothing to stop.")); */
  return;
}


static void
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  xfsamba_abort (1);
}

#define TOGGLENOT 4
#define TOGGLE 3
#define EMPTY_SUBMENU 2
#define SUBMENU 1
#define MENUBAR 0
#define RIGHT_MENU -1

static GtkWidget *
shortcut_menu (int submenu, GtkWidget * parent, char *txt, gpointer func, gpointer data)
{
  GtkWidget *menuitem;
  static GtkWidget *menu;
  int togglevalue;

  switch (submenu)
  {
  case TOGGLE:
  case TOGGLENOT:
    togglevalue = (int) data;
    menuitem = gtk_check_menu_item_new_with_label (txt);
    GTK_CHECK_MENU_ITEM (menuitem)->active = (submenu == TOGGLENOT) ? (!togglevalue) : togglevalue;
    gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
    break;
  case EMPTY_SUBMENU:
    menuitem = gtk_menu_item_new ();
    break;
  case RIGHT_MENU:
    menuitem = gtk_menu_item_new_with_label (txt);
    gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));
    break;
  case SUBMENU:
  case MENUBAR:
  default:
    menuitem = gtk_menu_item_new_with_label (txt);
    break;
  }
  if (submenu > 0)
  {
    gtk_menu_append (GTK_MENU (parent), menuitem);
    if ((submenu) && (submenu != EMPTY_SUBMENU) && (func))
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (func), (gpointer) data);
  }
  else
    gtk_menu_bar_append (GTK_MENU_BAR (parent), menuitem);
  gtk_widget_show (menuitem);

  if (submenu <= 0)
  {
    menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
    return menu;

  }
  return menuitem;
}

static GtkWidget *
newbox (gboolean pack, int vertical, GtkWidget * parent, gboolean expand, gboolean fill, int spacing)
{
  GtkWidget *box;
  switch (vertical)
  {
  case HORIZONTAL:
    box = gtk_hbox_new (FALSE, 0);
    break;
  case VERTICAL:
    box = gtk_vbox_new (FALSE, 0);
    break;
  case HANDLEBOX:
    box = gtk_handle_box_new ();
    break;
  default:
    box = NULL;
    return box;
  }

  if (pack)
    gtk_box_pack_start (GTK_BOX (parent), box, expand, fill, spacing);
  else
  {
    gtk_container_add (GTK_CONTAINER (parent), box);
    gtk_container_set_border_width (GTK_CONTAINER (parent), spacing);
  }

  gtk_widget_show (box);
  return box;
}

#include "icons/page.xpm"
#include "icons/rpage.xpm"
#include "icons/go_to.xpm"
#include "icons/go_back.xpm"
#include "icons/home.xpm"
#include "icons/stop.xpm"
#include "icons/dir_close_lnk.xpm"
#include "icons/dir_open_lnk.xpm"
#include "icons/dir_close.xpm"
#include "icons/dir_open.xpm"
#include "icons/comp1.xpm"
#include "icons/comp2.xpm"
#include "icons/wg1.xpm"
#include "icons/wg2.xpm"
#include "icons/reload.xpm"
#include "icons/dotfile.xpm"
#include "icons/rdotfile.xpm"
#include "icons/print.xpm"
#include "icons/help.xpm"
#include "icons/ip.xpm"
#include "icons/download.xpm"
#include "icons/upload.xpm"
#include "icons/view1.xpm"
#include "icons/view2.xpm"
#include "icons/view3.xpm"
#include "icons/delete.xpm"
#include "icons/new_dir.xpm"
#include "icons/tar.xpm"

static GtkWidget *
icon_button (char **data, char *tip)
{
  GtkWidget *button, *pixmap;
  GtkTooltips *tooltip;

  button = gtk_button_new ();
  gtk_button_set_relief ((GtkButton *) button, GTK_RELIEF_NONE);
  gtk_widget_set_usize (button, 30, 30);
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, tip, "ContextHelp/buttons/?");
  pixmap = MyCreateFromPixmapData (button, data);
  if (pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  else
  {
    gtk_widget_show (pixmap);
    gtk_container_add (GTK_CONTAINER (button), pixmap);
  }
  return button;
}

GtkWidget *
create_smb_window (void)
{
  int lineH;
  GtkWidget * vbox, *vbox1, *vbox2, *hbox, *widget, *handlebox, *scrolled, *button, *separator;


  smb_nav = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy (GTK_WINDOW (smb_nav), TRUE, TRUE, FALSE);

  gtk_signal_connect (GTK_OBJECT (smb_nav), "destroy", GTK_SIGNAL_FUNC (delete_event), (gpointer) GTK_WIDGET (smb_nav));
  gtk_signal_connect (GTK_OBJECT (smb_nav), "delete_event", GTK_SIGNAL_FUNC (delete_event), (gpointer) GTK_WIDGET (smb_nav));
  /* pixmaps */

  gPIX_page = MyCreateGdkPixmapFromData (page_xpm, smb_nav, &gPIM_page, FALSE);
  gPIX_rpage = MyCreateGdkPixmapFromData (rpage_xpm, smb_nav, &gPIM_rpage, FALSE);
  gPIX_dir_open = MyCreateGdkPixmapFromData (dir_open_xpm, smb_nav, &gPIM_dir_open, FALSE);
  gPIX_dir_close = MyCreateGdkPixmapFromData (dir_close_xpm, smb_nav, &gPIM_dir_close, FALSE);
  gPIX_dir_open_lnk = MyCreateGdkPixmapFromData (dir_open_lnk_xpm, smb_nav, &gPIM_dir_open_lnk, FALSE);
  gPIX_dir_close_lnk = MyCreateGdkPixmapFromData (dir_close_lnk_xpm, smb_nav, &gPIM_dir_close_lnk, FALSE);
  gPIX_comp1 = MyCreateGdkPixmapFromData (comp1_xpm, smb_nav, &gPIM_comp1, FALSE);
  gPIX_comp2 = MyCreateGdkPixmapFromData (comp2_xpm, smb_nav, &gPIM_comp2, FALSE);
  gPIX_wg1 = MyCreateGdkPixmapFromData (wg1_xpm, smb_nav, &gPIM_wg1, FALSE);
  gPIX_wg2 = MyCreateGdkPixmapFromData (wg2_xpm, smb_nav, &gPIM_wg2, FALSE);
  gPIX_reload = MyCreateGdkPixmapFromData (reload_xpm, smb_nav, &gPIM_reload, FALSE);
  gPIX_dotfile = MyCreateGdkPixmapFromData (dotfile_xpm, smb_nav, &gPIM_dotfile, FALSE);
  gPIX_rdotfile = MyCreateGdkPixmapFromData (rdotfile_xpm, smb_nav, &gPIM_rdotfile, FALSE);
  gPIX_print = MyCreateGdkPixmapFromData (print_xpm, smb_nav, &gPIM_print, FALSE);
  gPIX_help = MyCreateGdkPixmapFromData (help_xpm, smb_nav, &gPIM_help, FALSE);
  gPIX_ip = MyCreateGdkPixmapFromData (ip_xpm, smb_nav, &gPIM_ip, FALSE);
  gPIX_download = MyCreateGdkPixmapFromData (download_xpm, smb_nav, &gPIM_download, FALSE);
  gPIX_upload = MyCreateGdkPixmapFromData (upload_xpm, smb_nav, &gPIM_upload, FALSE);
  gPIX_tar = MyCreateGdkPixmapFromData (tar_xpm, smb_nav, &gPIM_tar, FALSE);
  gPIX_view1 = MyCreateGdkPixmapFromData (view1_xpm, smb_nav, &gPIM_view1, FALSE);
  gPIX_view2 = MyCreateGdkPixmapFromData (view2_xpm, smb_nav, &gPIM_view2, FALSE);
  gPIX_view3 = MyCreateGdkPixmapFromData (view3_xpm, smb_nav, &gPIM_view3, FALSE);
  gPIX_new_dir = MyCreateGdkPixmapFromData (new_dir_xpm, smb_nav, &gPIM_new_dir, FALSE);
  gPIX_delete = MyCreateGdkPixmapFromData (delete_xpm, smb_nav, &gPIM_delete, FALSE);

  /* boxes: */

  vbox = newbox (ADD, VERTICAL, smb_nav, FILL, EXPAND, 0);
  widget = gtk_label_new ("GET FONT INFO");
  lineH = widget->style->font->ascent + widget->style->font->descent + 5;

  {
    /* I think this way to structure boxcode is easier to picture */
    vbox1 = newbox (PACK, VERTICAL, vbox, NOEXPAND, NOFILL, 0);
    {
      handlebox = newbox (PACK, HANDLEBOX, vbox1, NOEXPAND, NOFILL, 0);
      {
	GtkWidget *menu, *submenu, *menubar;
	hbox = newbox (ADD, HORIZONTAL, handlebox, NOTUSED, NOTUSED, 3);
	/* menu bar */
	menubar = gtk_menu_bar_new ();
	gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menubar), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (hbox), menubar);
	gtk_widget_show (menubar);
	menu = shortcut_menu (MENUBAR, menubar, _("File"), NULL, NULL);
	/* (s): multiple file upload/download to be enabled in future */
	submenu = shortcut_menu (SUBMENU, menu, _("Download..."), GTK_SIGNAL_FUNC (cb_download), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Upload..."), GTK_SIGNAL_FUNC (cb_upload), NULL);

	submenu = shortcut_menu (SUBMENU, menu, _("New folder..."), GTK_SIGNAL_FUNC (cb_new_dir), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Delete..."), GTK_SIGNAL_FUNC (cb_delete), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Tar..."), GTK_SIGNAL_FUNC (cb_tar), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Exit"), GTK_SIGNAL_FUNC (delete_event), NULL);

	menu = shortcut_menu (MENUBAR, menubar, _("Preferences"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Browse as..."), GTK_SIGNAL_FUNC (select_user), NULL);

	menu = shortcut_menu (MENUBAR, menubar, _("View"), NULL, NULL);

	show_diag = submenu = shortcut_menu (SUBMENU, menu, _("Show diagnostics"), GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x01));
	hide_diag = submenu = shortcut_menu (SUBMENU, menu, _("Hide diagnostics"), GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x1));
	show_links = submenu = shortcut_menu (SUBMENU, menu, _("Show browser links"), GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x10));
	hide_links = submenu = shortcut_menu (SUBMENU, menu, _("Hide browser links"), GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x10));

	menu = shortcut_menu (MENUBAR, menubar, _("Go"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Home..."), GTK_SIGNAL_FUNC (go_home), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Reload..."), GTK_SIGNAL_FUNC (go_reload), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Forward..."), GTK_SIGNAL_FUNC (go_forward), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Back..."), GTK_SIGNAL_FUNC (go_back), NULL);
	menu = shortcut_menu (RIGHT_MENU, menubar, _("Help"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("About master browser..."), GTK_SIGNAL_FUNC (cb_master), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("About xfsamba..."), GTK_SIGNAL_FUNC (cb_about), NULL);
      }
      handlebox = newbox (PACK, HANDLEBOX, vbox1, NOEXPAND, NOFILL, 0);
      {
	hbox = newbox (ADD, HORIZONTAL, handlebox, NOTUSED, NOTUSED, 3);
	/*
	   hbox=newbox(PACK,HORIZONTAL,vbox1,EXPAND,FILL,0);{ 
	 */
	/* icon bar */
	button = icon_button (go_back_xpm, _("Back ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (go_back), NULL);
	gtk_widget_show (button);
	button = icon_button (go_to_xpm, _("Forward ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (go_forward), NULL);
	gtk_widget_show (button);
	button = icon_button (reload_xpm, _("Reload ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (go_reload), NULL);
	gtk_widget_show (button);

	button = icon_button (home_xpm, _("Home ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (go_home), NULL);
	gtk_widget_show (button);
	button = icon_button (stop_xpm, _("Stop ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (go_stop), NULL);
	gtk_widget_show (button);

	separator = gtk_vseparator_new ();
	gtk_widget_show (separator);
	gtk_box_pack_start (GTK_BOX (hbox), separator, FALSE, FALSE, 0);

	button = icon_button (new_dir_xpm, _("New folder ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_new_dir), NULL);
	gtk_widget_show (button);

	button = icon_button (delete_xpm, _("Delete ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_delete), NULL);
	gtk_widget_show (button);

	separator = gtk_vseparator_new ();
	gtk_widget_show (separator);
	gtk_box_pack_start (GTK_BOX (hbox), separator, FALSE, FALSE, 0);

	button = icon_button (download_xpm, _("Download ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_download), NULL);
	gtk_widget_show (button);

	button = icon_button (upload_xpm, _("Upload ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_upload), NULL);
	gtk_widget_show (button);


	button = icon_button (tar_xpm, _("Tar ..."));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_tar), NULL);
	gtk_widget_show (button);

	separator = gtk_vseparator_new ();
	gtk_widget_show (separator);
	gtk_box_pack_start (GTK_BOX (hbox), separator, FALSE, FALSE, 0);


	button = icon_button (help_xpm, _("Help ..."));
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_about), NULL);
	gtk_widget_show (button);

	separator = gtk_vseparator_new ();
	gtk_widget_show (separator);
	gtk_box_pack_end (GTK_BOX (hbox), separator, FALSE, FALSE, 0);

	button = icon_button (view3_xpm, _("Set View 3"));
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x400));
	gtk_widget_show (button);

	button = icon_button (view2_xpm, _("Set View 2"));
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x200));
	gtk_widget_show (button);

	button = icon_button (view1_xpm, _("Set View 1"));
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x100));
	gtk_widget_show (button);

      }
      handlebox = newbox (PACK, HANDLEBOX, vbox1, NOEXPAND, NOFILL, 0);
      {
	hbox = newbox (ADD, HORIZONTAL, handlebox, NOTUSED, NOTUSED, 3);
	/* location entry */
	gtk_widget_set_usize (hbox, WINDOW_WIDTH, lineH);
	widget = gtk_label_new (_("Location : "));
	gtk_box_pack_start (GTK_BOX (hbox), widget, NOEXPAND, NOFILL, 0);
	gtk_widget_show (widget);

	location = gtk_combo_new ();
	gtk_box_pack_start (GTK_BOX (hbox), location, EXPAND, FILL, 0);
	gtk_widget_show (location);

	gtk_combo_disable_activate ((GtkCombo *) location);
	/*
	   gtk_signal_connect (GTK_OBJECT (GTK_COMBO (location)->entry), "changed",
	 */
	gtk_signal_connect (GTK_OBJECT (GTK_COMBO (location)->entry), "activate", GTK_SIGNAL_FUNC (select_combo_server), NULL);

	button = icon_button (ip_xpm, _("Show IP"));
	gtk_box_pack_start (GTK_BOX (hbox), button, NOEXPAND, NOFILL, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (NMBLookup), NULL);
	gtk_widget_show (button);

	locationIP = gtk_label_new ("-");
	gtk_box_pack_start (GTK_BOX (hbox), locationIP, NOEXPAND, NOFILL, 0);
	gtk_widget_show (locationIP);
      }
    }
    vpaned = gtk_vpaned_new ();
    gtk_widget_ref (vpaned);
    gtk_object_set_data (GTK_OBJECT (smb_nav), "vpaned1", vpaned);
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
    vbox1 = gtk_vbox_new (FALSE, 0);	/* newbox (PACK, VERTICAL, vbox, EXPAND, FILL, 3); */
    gtk_paned_pack1 (GTK_PANED (vpaned), vbox1, TRUE, TRUE);
    gtk_widget_show (vbox1);
    {
      vpaned2 = gtk_vpaned_new ();
      gtk_object_set_data (GTK_OBJECT (smb_nav), "vpaned2", vpaned2);
      gtk_box_pack_start (GTK_BOX (vbox1), vpaned2, TRUE, TRUE, 0);
      vbox2 = gtk_vbox_new (FALSE, 0);	/* = newbox (PACK, VERTICAL, vbox1, EXPAND, FILL, 0); */
      gtk_paned_pack1 (GTK_PANED (vpaned2), vbox2, TRUE, TRUE);
      gtk_widget_show (vbox2);
      {
	/* location shares */
	sharesL = gtk_label_new (_("Location shares : "));
	gtk_box_pack_start (GTK_BOX (vbox2), sharesL, NOEXPAND, NOFILL, 0);
	gtk_widget_show (sharesL);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox2), scrolled, EXPAND, FILL, 0);
	gtk_widget_set_usize (scrolled, WINDOW_WIDTH, lineH * 6);
	gtk_widget_show (scrolled);
	{
	  int i;
	  gchar *titles[SHARE_COLUMNS];
	  for (i = 0; i < SHARE_COLUMNS; i++)
	    titles[i] = "";
	  titles[SHARE_NAME_COLUMN] = _("Name");
	  titles[SHARE_SIZE_COLUMN] = _("Size");
	  titles[SHARE_DATE_COLUMN] = _("Date");
	  titles[COMMENT_COLUMN] = _("Comment");
	  shares = gtk_ctree_new_with_titles (SHARE_COLUMNS, 0, titles);

	  /*
	     shares=gtk_ctree_new_with_titles(2,0,titles);
	   */
	  gtk_clist_set_auto_sort (GTK_CLIST (shares), FALSE);
	  gtk_clist_set_shadow_type (GTK_CLIST (shares), GTK_SHADOW_IN);
	  gtk_ctree_set_line_style (GTK_CTREE (shares), GTK_CTREE_LINES_NONE);
	  gtk_ctree_set_expander_style (GTK_CTREE (shares), GTK_CTREE_EXPANDER_TRIANGLE);
	  gtk_clist_set_reorderable (GTK_CLIST (shares), FALSE);
	  gtk_signal_connect (GTK_OBJECT (shares), "tree-select-row", GTK_SIGNAL_FUNC (select_share), (gpointer) GTK_WIDGET (shares));
	  gtk_signal_connect (GTK_OBJECT (shares), "click_column", GTK_SIGNAL_FUNC (on_click_column), NULL);

	  gtk_container_add (GTK_CONTAINER (scrolled), shares);
	  for (i = 0; i < SHARE_COLUMNS; i++)
	    gtk_clist_set_column_auto_resize ((GtkCList *) shares, i, TRUE);
	  gtk_clist_set_auto_sort ((GtkCList *) shares, TRUE);
	  gtk_widget_show (shares);
	}
      }

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_paned_pack2 (GTK_PANED (vpaned2), hbox, TRUE, TRUE);
      gtk_widget_show (hbox);

      /*hbox = newbox (PACK, HORIZONTAL, vbox3, EXPAND, FILL, 0); */

      {
	/* hbox=newbox(PACK,HORIZONTAL,vbox1,EXPAND,FILL,0);{ */
	hpaned = gtk_hpaned_new ();
	gtk_widget_ref (hpaned);
	gtk_object_set_data (GTK_OBJECT (smb_nav), "hpaned1", hpaned);
	gtk_box_pack_start (GTK_BOX (hbox), hpaned, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
				       /*= newbox (PACK, VERTICAL, hbox, EXPAND, FILL, 0);*/
	gtk_paned_pack1 (GTK_PANED (hpaned), vbox2, TRUE, TRUE);
	gtk_widget_show (vbox2);
	{
	  /* location computer links */
	  serversL = gtk_label_new (_("Location known servers : "));
	  gtk_box_pack_start (GTK_BOX (vbox2), serversL, NOEXPAND, NOFILL, 0);
	  gtk_widget_show (serversL);

	  scrolled = gtk_scrolled_window_new (NULL, NULL);
	  gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	  gtk_box_pack_start (GTK_BOX (vbox2), scrolled, EXPAND, FILL, 0);
	  gtk_widget_set_usize (scrolled, WINDOW_WIDTH / 2, lineH * 6);
	  gtk_widget_show (scrolled);

	  {
	    int i;
	    gchar *titles[SERVER_COLUMNS] = { "", "Server", "Comment" };
	    servers = gtk_clist_new_with_titles (SERVER_COLUMNS, titles);
	    gtk_container_add (GTK_CONTAINER (scrolled), servers);
	    for (i = 0; i < SERVER_COLUMNS; i++)
	      gtk_clist_set_column_auto_resize ((GtkCList *) servers, i, TRUE);
	    gtk_widget_show (servers);
	    gtk_signal_connect (GTK_OBJECT (servers), "select-row", GTK_SIGNAL_FUNC (select_server), (gpointer) GTK_WIDGET (servers));
	    gtk_signal_connect (GTK_OBJECT (servers), "click_column", GTK_SIGNAL_FUNC (on_click_column), NULL);
	  }
	}
	vbox2 = gtk_vbox_new (FALSE, 0);
				       /*= newbox (PACK, VERTICAL, hbox, EXPAND, FILL, 0);*/
	gtk_paned_pack2 (GTK_PANED (hpaned), vbox2, TRUE, TRUE);
	gtk_widget_show (vbox2);
	{
	  /* location workgroup links */
	  workgroupsL = gtk_label_new (_("Location known workgroups : "));
	  gtk_box_pack_start (GTK_BOX (vbox2), workgroupsL, NOEXPAND, NOFILL, 0);
	  gtk_widget_show (workgroupsL);

	  scrolled = gtk_scrolled_window_new (NULL, NULL);
	  gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	  gtk_box_pack_start (GTK_BOX (vbox2), scrolled, EXPAND, FILL, 0);
	  gtk_widget_show (scrolled);

	  {
	    int i;
	    gchar *titles[WG_COLUMNS] = { "", "Workgroup", "Master" };
	    workgroups = gtk_clist_new_with_titles (WG_COLUMNS, titles);
	    gtk_container_add (GTK_CONTAINER (scrolled), workgroups);
	    for (i = 0; i < WG_COLUMNS; i++)
	      gtk_clist_set_column_auto_resize ((GtkCList *) workgroups, i, TRUE);
	    gtk_widget_show (workgroups);
	    gtk_signal_connect (GTK_OBJECT (workgroups), "select-row", GTK_SIGNAL_FUNC (select_workgroup), (gpointer) GTK_WIDGET (workgroups));
	    gtk_signal_connect (GTK_OBJECT (workgroups), "click_column", GTK_SIGNAL_FUNC (on_click_column), NULL);
	  }
	}
	gtk_widget_show (hpaned);
      }
      gtk_widget_show (vpaned2);
    }
    vbox1 = gtk_vbox_new (FALSE, 0);	/*newbox (PACK, VERTICAL, vbox, NOEXPAND, NOFILL, 0); */
    gtk_paned_pack2 (GTK_PANED (vpaned), vbox1, TRUE, TRUE);

    {
      scrolled = gtk_scrolled_window_new (NULL, NULL);
      {
	/* diagnostics */
	gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolled, EXPAND, FILL, 3);
	diagnostics = gtk_text_new (NULL, NULL);
	gtk_text_set_editable (GTK_TEXT (diagnostics), FALSE);
	gtk_text_set_word_wrap (GTK_TEXT (diagnostics), TRUE);
	gtk_text_set_line_wrap (GTK_TEXT (diagnostics), TRUE);
	gtk_container_add (GTK_CONTAINER (scrolled), diagnostics);
	gtk_widget_set_usize (diagnostics, WINDOW_WIDTH, lineH * 5);
	gtk_widget_show (diagnostics);
	gtk_widget_show (scrolled);

      }
      gtk_widget_show (vbox1);
    }
    gtk_widget_show (vpaned);

    vbox1 = newbox (PACK, VERTICAL, vbox, NOEXPAND, NOFILL, 0);
    {
      hbox = newbox (PACK, HORIZONTAL, vbox1, EXPAND, FILL, 0);
      {
	/* status line */
	progress = gtk_progress_bar_new ();
	/* gtk_progress_bar_set_activity_step((GtkProgressBar *)progress,5); */
	gtk_box_pack_start (GTK_BOX (hbox), progress, NOEXPAND, NOFILL, 3);
	gtk_widget_show (progress);

	statusline = gtk_label_new (_("Welcome to xfsamba."));
	gtk_box_pack_start (GTK_BOX (hbox), statusline, NOEXPAND, NOFILL, 3);
	gtk_widget_show (statusline);
	gtk_widget_set_usize (statusline, WINDOW_WIDTH, lineH);
      }
    }
  }
  /* gtk_widget_set_usize (smb_nav, 640, 480); */
  gtk_widget_show (smb_nav);
  cb_view (NULL, (gpointer) ((long) 0x0));
  return smb_nav;
}

static gboolean anim;
static gboolean
animate_bar (gpointer data)
{
  static gboolean direction = TRUE;
  static gfloat fraction = 0.0, delta = 0.01;

  /* should be gtk_progress_set_fraction() */
  fraction += delta;
  if (fraction >= 1.0)
  {
    fraction = 1.0;
    if (direction)
      gtk_progress_bar_set_orientation ((GtkProgressBar *) progress, GTK_PROGRESS_RIGHT_TO_LEFT);
    else
      gtk_progress_bar_set_orientation ((GtkProgressBar *) progress, GTK_PROGRESS_LEFT_TO_RIGHT);
    direction = !direction;
    delta = -0.01;
  }
  if (fraction <= 0.0)
  {
    delta = 0.01;
    fraction = 0.0;
  }
  gtk_progress_set_percentage ((GtkProgress *) progress, fraction);
  /* if (!anim) gtk_progress_set_percentage((GtkProgress *)progress,1.0); */
  return anim;
}

void
animation (gboolean state)
{
  if (!state)
  {
    anim = FALSE;
  }
  else
  {
    anim = TRUE;
    gtk_timeout_add (100, (GtkFunction) animate_bar, (gpointer) ((long) anim));
  }
}
