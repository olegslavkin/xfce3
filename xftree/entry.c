/*
 * entry.c
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
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <glib.h>
#include "entry.h"
#include "io.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/*
 */
entry *
entry_dupe (entry * en)
{
  entry *new_en = NULL;

  if (!en)
    return (NULL);
  new_en = g_malloc (sizeof (entry));
  new_en->path = g_strdup (en->path);
  new_en->label = g_strdup (en->label);
  new_en->size = en->size;
  new_en->type = en->type;
  new_en->flags = en->flags;
  new_en->mtime = en->mtime;
  new_en->inode = en->inode;
  new_en->mode = en->mode;
  new_en->date = en->date;
  new_en->org_mem = en->org_mem;
  return (new_en);
}


/*
 */
void
entry_free (entry * en)
{
  if (en)
    {
      g_free (en->path);
      g_free (en->label);
      g_free (en);
    }
}

/*
 */
entry *
entry_new (void)
{
  entry *en = g_malloc (sizeof (entry));
  if (!en)
    return (NULL);
  en->path = en->label = NULL;
  en->type = en->flags = en->size = en->mtime = en->inode = 0;
  en->org_mem = NULL;
  return (en);
}

/*
 */
entry *
entry_new_by_path_and_label (char *path, char *label)
{
  struct stat s;
  entry *en;
  struct tm *t;

  if (lstat (path, &s) == -1)
    {
      return (NULL);
    }
  en = entry_new ();
  if (!en)
    return (NULL);
  en->path = g_strdup (path);
  en->label = g_strdup (label);

  en->size = s.st_size;
  en->inode = s.st_ino;
  en->mtime = s.st_mtime;
  t = localtime (&s.st_mtime);
  en->date.year = 1900 + t->tm_year;
  en->date.month = t->tm_mon + 1;
  en->date.day = t->tm_mday;
  en->date.hour = t->tm_hour;
  en->date.min = t->tm_min;

  if (S_ISLNK (s.st_mode))
    {
      en->type |= FT_LINK;
      if (stat (path, &s) == -1)
	{
	  en->type |= FT_STALE_LINK;
	  en->size = 0;
	  return (en);
	}
    }
  en->mode = s.st_mode;

  if (io_is_dirup (en->label))
    {
      en->type = FT_DIR_UP | FT_DIR;
      return (en);
    }

  if (S_ISDIR (s.st_mode))
    {
      en->type |= FT_DIR;
      if (access (path, R_OK | X_OK) != 0)
	en->type |= FT_DIR_PD;
    }
  else if (S_ISREG (s.st_mode))
    {
      en->type |= FT_FILE;
      if ((s.st_mode & S_IXUSR) ||
	  (s.st_mode & S_IXGRP) || (s.st_mode & S_IXOTH))
	en->type |= FT_EXE;
    }
  else if (S_ISCHR (s.st_mode))
    {
      en->type |= FT_CHAR_DEV;
    }
  else if (S_ISBLK (s.st_mode))
    {
      en->type |= FT_BLOCK_DEV;
    }
  else if (S_ISFIFO (s.st_mode))
    {
      en->type |= FT_FIFO;
    }
  else if (S_ISSOCK (s.st_mode))
    {
      en->type |= FT_SOCKET;
    }
  else
    {
      en->type |= FT_UNKNOWN;
    }
  return (en);
}

/*
 * extract label from path and call the function above
 */
entry *
entry_new_by_path (char *path)
{
  char *label, *p;
  entry *en;

  p = label = g_strdup (path);
  p = strrchr (label, '/');
  if (p)
    {
      if (p != label)
	{
	  if (*(p + 1) == '\0')
	    {
	      /* remove slash at the end */
	      *p = '\0';
	      /* search again */
	      p = strrchr (label, '/');
	      if (!p)
		{
		  /* give up */
		  p = label;
		}
	    }
	  else
	    {
	      p++;
	    }
	}
    }
  else
    p = label;
  en = entry_new_by_path_and_label (path, p);
  g_free (label);
  return en;
}

/*
 */
entry *
entry_new_by_type (char *path, int type)
{
  char *p;
  entry *en;

  en = entry_new ();
  if (!en)
    return (NULL);
  en->path = g_strdup (path);

  p = strrchr (en->path, '/');
  if (p)
    {
      if (p != en->path)
	{
	  if (*(p + 1) == '\0')
	    {
	      /* remove slash at the end */
	      *p = '\0';
	      /* search again */
	      p = strrchr (en->path, '/');
	      if (!p)
		{
		  /* give up */
		  p = en->path;
		}
	    }
	  else
	    {
	      p++;
	    }
	}
    }
  else
    p = en->path;
  en->label = g_strdup (p);
  en->type = type;
  return (en);
}

/*
 * update time, size, etc..
 * return 0 on failure
 * return 1 on nothing to do
 * return 2 on something has changed
 */
int
entry_update (entry * en)
{
  struct stat s;
  struct tm *t;
  int rc = FALSE;

  if (lstat (en->path, &s) == -1)
    {
      return (ERROR);
    }
  if (en->size != s.st_size)
    {
      rc = TRUE;
      en->size = s.st_size;
    }
  if (en->inode != s.st_ino)
    {
      rc = TRUE;
      en->inode = s.st_ino;
    }
  if (en->mtime != s.st_mtime)
    {
      rc = TRUE;
      en->mtime = s.st_mtime;
      t = localtime (&s.st_mtime);
      en->date.year = 1900 + t->tm_year;
      en->date.month = t->tm_mon + 1;
      en->date.day = t->tm_mday;
      en->date.hour = t->tm_hour;
      en->date.min = t->tm_min;
    }
  if (S_ISLNK (s.st_mode))
    {
      if (stat (en->path, &s) == -1)
	{
	  en->type |= FT_STALE_LINK;
	  return (TRUE);
	}
    }
  if (EN_IS_DIR (en) && (!S_ISDIR (s.st_mode)))
    return (ERROR);

  return (rc);
}


int
entry_type_update (entry * en)
{
  struct stat s;
  int rc = FALSE;

  if (lstat (en->path, &s) == -1)
    {
      return (ERROR);
    }

  if (S_ISLNK (s.st_mode))
    {
      if (stat (en->path, &s) == -1)
	{
	  en->type |= FT_STALE_LINK;
	  return (TRUE);
	}
    }
  if (EN_IS_DIR (en) && (!S_ISDIR (s.st_mode)))
    return (ERROR);

  if (en->mode != s.st_mode)
    {
      rc = TRUE;
      en->mode = s.st_mode;
      en->type = 0;
      if (S_ISDIR (s.st_mode))
	{
	  en->type |= FT_DIR;
	  if (access (en->path, R_OK | X_OK) != 0)
	    en->type |= FT_DIR_PD;
	}
      else if (S_ISREG (s.st_mode))
	{
	  en->type |= FT_FILE;
	  if ((s.st_mode & S_IXUSR) ||
	      (s.st_mode & S_IXGRP) || (s.st_mode & S_IXOTH))
	    en->type |= FT_EXE;
	}
      else if (S_ISCHR (s.st_mode))
	{
	  en->type |= FT_CHAR_DEV;
	}
      else if (S_ISBLK (s.st_mode))
	{
	  en->type |= FT_BLOCK_DEV;
	}
      else if (S_ISFIFO (s.st_mode))
	{
	  en->type |= FT_FIFO;
	}
      else if (S_ISSOCK (s.st_mode))
	{
	  en->type |= FT_SOCKET;
	}
      else
	{
	  en->type |= FT_UNKNOWN;
	}
      if (io_is_dirup (en->label))
	{
	  en->type = FT_DIR_UP | FT_DIR;
	}
    }

  return (rc);
}
