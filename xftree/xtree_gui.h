/*
 * xtree_gui.h
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

#ifndef __XTREE_GUI_H__
#define __XTREE_GUI_H__
#include <gtk/gtk.h>

typedef struct
{
  int x;
  int y;
  int width;
  int height;
}
wgeo_t;

enum
{
  MN_NONE = 0,
  MN_DIR = 1,
  MN_FILE = 2,
  MN_MIXED = 3,
  MN_HLP = 4,
  MENUS
};

enum
{
  COL_NAME,
  COL_SIZE,
  COL_DATE,
  COLUMNS			/* number of columns */
};

int count_selection (GtkCTree * ctree, GtkCTreeNode ** first);
void node_destroy (gpointer p);
void ctree_thaw (GtkCTree * ctree);
void ctree_freeze (GtkCTree * ctree);
void add_subtree (GtkCTree * ctree, GtkCTreeNode * root, char *path, int depth, int flags);
void set_title (GtkWidget * w, const char *path);


void gui_main (char *path, char *xap, char *trash, char *reg, wgeo_t *, int);
#endif
