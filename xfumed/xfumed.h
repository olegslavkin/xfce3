#ifndef _XFUMED_H
#define _XFUMED_H

#include "my_intl.h"

#define RCFILE ".xfce/xfwm.user_menu"

#define INTROTEXT "# This file is generated by XFumed - XFce User Menu Editor"

#define DLG_ITEMTEXT    _("Item")
#define DLG_SUBMENUTEXT _("Submenu")
#define DLG_NOPTEXT     _("Separator")

#define TEMPNODETEXT    "temporary node"
#define NOP_CAPTION     "   "

#define SPACING 2

typedef struct _xfdlg
{
  GtkWidget *window;
  GtkWidget *entry_type_combo;
  GtkWidget *entry_type_entry;
  GtkWidget *caption_entry;
  GtkWidget *cmd_entry;
  char *entry;
  char *caption;
  char *cmd;
  GtkWidget *fileselect_button;
  gboolean update;
}
XFDLG;

typedef struct _xfmenu
{
  GtkWidget *window;
  GtkWidget *ctree;
  GList *menulist;
  GtkCTreeNode *selected_node;
  GtkCTreeNode *first_node;
  gboolean saved;
  XFDLG *dlg;

}
XFMENU;

XFDLG *xfdlg_new (void);
XFMENU *xfmenu_new (void);
void xfumed_init (int *argc, char ***argv);
GtkWidget *read_menu (XFMENU * xfmenu);
void remove_node (XFMENU * xfmenu, GtkCTreeNode * node);

void load_ctree_images (void);
char *get_next_word (char **remainder);
void entry_from_line (XFMENU * xfmenu, GtkCTreeNode * parent, char *string);
gboolean find_node_by_name (XFMENU * xfmenu, char *name, GtkCTreeNode ** node);
GtkCTreeNode *update_node (XFMENU * xfmenu, char **text);
GtkCTreeNode *insert_node (XFMENU * xfmenu, GtkCTreeNode * parent, GtkCTreeNode * sibling, char **text, gboolean is_leaf);
GtkCTreeNode *new_temp_node (XFMENU * xfmenu, char *menuname);
void write_menu (XFMENU * xfmenu);
void write_node (GtkCTree * ctree, GtkCTreeNode * node, gpointer data);

#endif
