/*
 * xtree_gui.h
 *
 * Copyright (C) 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia 2001 for Xfce project
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
#include "xtree_cfg.h"

#define SPACING 	5
/* moved to reg.h: #define DEF_APP		"netscape"*/
#define TIMERVAL 	6000
#define MAXBUF		8192

#define WINCFG		1
#define TOPWIN		2

#define yes		1
#define no		0
#define ERROR 		-1

typedef struct
{
  int x;
  int y;
  int width;
  int height;
}
wgeo_t;

typedef struct autotype_t{
	char *extension;
	char *command;
}autotype_t;

enum
{
  MN_NONE = 0,
  MN_DIR = 1,
  MN_FILE = 2,
  MN_MIXED = 3,
  MN_HLP = 4,
  MN_TARCHILD = 5,
  MENUS
};

enum
{
  COL_NAME,
  COL_SIZE,
  COL_DATE,
  COL_UID,
  COL_GID,
  COL_MODE,
  COLUMNS			/* number of columns */
};

enum
{
  PIX_DIR_OPEN=0, PIX_DIR_OPEN_LNK,
  PIX_DIR_CLOSE,PIX_DIR_CLOSE_LNK,PIX_DIR_UP,
  PIX_DIR_PD,
  PIX_PAGE,PIX_PAGE_C,PIX_PAGE_F,PIX_PAGE_O,
  	PIX_PAGE_H,PIX_PAGE_LNK,PIX_CORE,PIX_TAR,
	PIX_COMPRESSED,PIX_IMAGE,PIX_TEXT,PIX_MAIL,
	PIX_BAK,PIX_DUP,PIX_TAR_TABLE,PIX_TAR_EXP,
	PIX_TAR_TABLE_R,PIX_TAR_EXP_R,PIX_PS,
	PIX_ADOBE,
  PIX_PAGE_AUDIO,
  PIX_PACKAGE,
  PIX_LINKFLAG, 
  PIX_PAGE_HTML, 
  PIX_CHAR_DEV,
  PIX_FIFO,
  PIX_SOCKET,
  PIX_BLOCK_DEV,
  PIX_STALE_LNK,
  PIX_EXE,PIX_EXE_SCRIPT,PIX_EXE_LINK,
  LAST_PIX
};
/* don't repeat masks that already exist */
enum
{
  PIM_DIR_OPEN=0,
  PIM_DIR_CLOSE,
  PIM_PACKAGE,
  PIM_LINKFLAG, 
  PIM_DIR_PD,
  PIM_PAGE,
  PIM_PAGE_HTML, 
  PIM_CHAR_DEV,
  PIM_FIFO,
  PIM_SOCKET,
  PIM_BLOCK_DEV,
  PIM_STALE_LNK,
  PIM_EXE,
  LAST_PIM
};

#ifdef __XFTREE_GUI_MAIN__
#define EXTERN
#else 
#define EXTERN extern
#endif
EXTERN GdkPixmap *gPIX[LAST_PIX];
EXTERN GdkPixmap *gPIM[LAST_PIM];

void gui_main (char *path, char *xap, char *trash, char *reg, wgeo_t *, int);
char *mode_txt(mode_t mode);
cfg *new_top (char *path, char *xap, char *trash, GList * reg, int width, int height, int flags);

void create_pixmaps(int h,GtkWidget * ctree);
#endif
