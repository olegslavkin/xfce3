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

#ifdef __XFTREE_GUI_MAIN__
GdkPixmap  *gPIX_dir_close=NULL,  *gPIX_dir_open=NULL,
        * gPIX_page=NULL,*gPIX_core=NULL, 
	*gPIX_pageC=NULL, *gPIX_pageH=NULL,*gPIX_pageF=NULL,
	*gPIX_tar=NULL,*gPIX_compressed=NULL,*gPIX_image=NULL,*gPIX_text=NULL,
	*gPIX_page_lnk=NULL, *gPIX_dir_pd=NULL, 
	*gPIX_dir_close_lnk=NULL, *gPIX_dir_open_lnk=NULL, *gPIX_dir_up=NULL, 
	*gPIX_char_dev=NULL, *gPIX_fifo=NULL, *gPIX_socket=NULL, 
	*gPIX_block_dev=NULL, *gPIX_exe=NULL, *gPIX_stale_lnk=NULL, *gPIX_exe_lnk=NULL;
/* don't repeat masks that already exist */
GdkBitmap * gPIM_page=NULL,*gPIM_char_dev=NULL, *gPIM_fifo=NULL, *gPIM_socket=NULL, 
	*gPIM_block_dev=NULL, *gPIM_exe=NULL, *gPIM_stale_lnk=NULL,
	*gPIM_dir_close=NULL, *gPIM_dir_open=NULL;

#else
extern GdkPixmap  *gPIX_dir_close,  *gPIX_dir_open,
        * gPIX_page,*gPIX_core, 
	*gPIX_pageC, *gPIX_pageH,*gPIX_pageF,
	*gPIX_tar,*gPIX_compressed,*gPIX_image,*gPIX_text,
	*gPIX_page_lnk, *gPIX_dir_pd, 
	*gPIX_dir_close_lnk, *gPIX_dir_open_lnk, *gPIX_dir_up, 
	*gPIX_char_dev, *gPIX_fifo, *gPIX_socket, 
	*gPIX_block_dev, *gPIX_exe, *gPIX_stale_lnk, *gPIX_exe_lnk;
/* don't repeat masks that already exist */
extern GdkBitmap * gPIM_page,*gPIM_char_dev, *gPIM_fifo, *gPIM_socket, 
	*gPIM_block_dev, *gPIM_exe, *gPIM_stale_lnk,
	*gPIM_dir_close, *gPIM_dir_open;
#endif

void gui_main (char *path, char *xap, char *trash, char *reg, wgeo_t *, int);

cfg *new_top (char *path, char *xap, char *trash, GList * reg, int width, int height, int flags);

void create_pixmaps(int h,GtkWidget * ctree);
#endif
