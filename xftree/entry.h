/*
 * entry.h
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

#ifndef __ENTRY_H__
#define __ENTRY_H__

typedef struct
{
  int year;
  int month;
  int day;
  int hour;
  int min;
}
mdate;

typedef struct
{
  gchar *path;
  gchar *label;
  int size;
  int type;
  int flags;
  int mtime;
  int inode;
  unsigned int mode;
  mdate date;
  void *org_mem;
}
entry;


#define FT_LINK			(1<<0)
#define FT_DIR			(1<<1)
#define FT_FILE			(1<<2)
#define FT_CHAR_DEV		(1<<3)
#define FT_BLOCK_DEV	(1<<4)
#define FT_FIFO			(1<<5)
#define FT_SOCKET		(1<<6)
#define FT_EXE			(1<<7)
#define FT_HIDDEN		(1<<8)
#define FT_DIR_UP		(1<<9)
#define FT_DIR_PD		(1<<10)
#define FT_STALE_LINK	(1<<11)
#define FT_UNKNOWN		(1<<12)
#define FT_DUMMY		(1<<13)

#define EN_IS_DIR(en) (en->type & FT_DIR)
#define EN_IS_LINK(en) (en->type & FT_LINK)
#define EN_IS_DIRUP(en) (en->type & FT_DIR_UP)
#define EN_IS_DUMMY(en) (en->type & FT_DUMMY)
#define EN_IS_FIFO(en) (en->type & FT_FIFO)
#define EN_IS_SOCKET(en) (en->type & FT_SOCKET)
#define EN_IS_DEVICE(en) ((en->type & FT_CHAR_DEV)||(en->type & FT_BLOCK_DEV))
#ifndef ERROR
#define ERROR -1
#endif

void entry_free (entry *);
entry *entry_new (void);
entry *entry_dupe (entry *);
entry *entry_new_by_path (char *path);
entry *entry_new_by_path_and_label (char *path, char *label);
entry *entry_new_by_type (char *path, int type);
int entry_update (entry *);
int entry_type_update (entry *);

#endif
