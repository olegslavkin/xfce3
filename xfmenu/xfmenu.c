/*  xfmenu
 *  Copyright (C) 2000 Jasper Huijsmans (j.b.huijsmans@chem.rug.nl)
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

/* ---------------------------------------------------------------------------
 *  xfmenu
 * 
 *  GNOME or KDE menu for xfwm
 * ---------------------------------------------------------------------------
 *
 *  This program tries to read one or more gnome or kde menus and translate 
 *  them to xfwm menus. It looks for the menus in a couple of standard 
 *  locations.
 *  For all menuitems it's checked if they are executable for the user. The 
 *  menus are added to the standard 'user_menu'.
 *  One of the nicest features is that if the LANG variable is set xfmenu
 *  tries to find translated entries.
 *  
 * ---------------------------------------------------------------------------
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <glib.h>

#include "utils.h"
#include "module.h"
#include "constant.h"

#ifndef HAVE_SCANDIR
#include "my_scandir.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define GMENUPATH "/share/gnome/apps"
#define KMENUPATH "/share/applnk"
#define MENUNAME  "__%s_%s_level%i__"

 /* pipe file descriptor */
int fd[2];

 /* types of menu */
typedef enum menutypes
{ GNOME, KDE }
menutype;

 /* types of menu items */
typedef enum itemtypes
{ SUB, ENTRY }
itemtype;

 /*  One type for both submenus and menu entries.
  *  For a submenu the 'cmd' item is unused, and for
  *  a menu entry the 'shortname' is not needed 
  */
typedef struct
{
  itemtype type;		/*  SUB or ENTRY               */
  char *shortname;		/*  Internal name for submenu  */
  char *fullname;		/*  Translated name            */
  char *cmd;			/*  NULL for submenu           */
}
ITEM;


/* get_line reads 1 line of a file.
 * 'line' must be big enough to hold the data.
 * This must be given by 'limit' */

/* NOTE: Isn't there some library function I could use? (Jasper) */

static int
get_line (FILE * fp, char *line, const int limit)
{
  int i;
  char chr = '\0';

  for (i = 0; i < limit && (chr = getc (fp)) != EOF && chr != '\n'; i++)
    line[i] = chr;

  line[i] = '\0';

  /* if the line is longer than limit we have to find the
   * end of the line (or the file) without adding characters
   * to 'line' */
  if (i >= limit && !(chr == EOF || chr == '\n'))
    while ((chr = getc (fp)) != EOF && chr != '\n');

  if (i > 0)
    return i;
  else
    {
      if (chr == '\n')
	return 1;
      else
	return 0;
    }
}

/* select_dir takes a directory entry as argument and returns
 * 1 if it is a directory, but not '.' or '..'. Otherwise it 
 * returns 0.
 * Used as an argument for scandir. 
 */

static int
select_dir (const struct dirent *direntry)
{
  struct stat filestat;
  char *name = direntry->d_name;

  if (strcmp (name, ".") == 0 ||
      strcmp (name, "..") == 0 || stat (name, &filestat) == -1)
    return 0;

  /* check if entry is a directory. If directory
   * name starts with a '.' assume that it should 
   * remain hidden and return 0 anyway (I added this,
   * because my KDE menu has a '.hidden' directory).
   */
  if (S_ISDIR (filestat.st_mode) && name[0] != '.')
    return 1;
  else
    return 0;
}

/* select_entry takes a directory entry as argument
 * and returns 1 if it is a '*.desktop' or '*.kdelnk'
 * file. Otherwise it returns 0.
 * Used as argument for scandir. 
 */

static int
select_entry (const struct dirent *direntry)
{
  struct stat filestat;

  if (stat (direntry->d_name, &filestat) == -1)
    return 0;

  if (S_ISREG (filestat.st_mode) &&
      (strstr (direntry->d_name, ".desktop") ||
       strstr (direntry->d_name, ".kdelnk")))
    return 1;
  else
    return 0;
}

/* 'get_subs_or_entries' finds directory entries in 'dir' and if a 
 * '.order' file is present it will use this file to arrange the 
 * directory entries.
 * The 'type' argument determines whether subdirs or menu entries
 * are returned (by using the functions select_dir or select_entry).
 */

static GList *
get_subs_or_entries (const char *dir, const itemtype type)
{
  char *cwd = g_get_current_dir ();
  char *orderfile = ".order";
  GList *list = NULL;
  GList *order, *list2;
  FILE *fp;
  struct dirent **direntrylist;
  int i = 0, n = 0;

  /* read directory entries 
   * select subdirs or menuentries */
  chdir (dir);
  if (type == SUB)
    n = scandir (".", &direntrylist, select_dir, alphasort);
  else
    n = scandir (".", &direntrylist, select_entry, alphasort);
  if (n >= 0)
    /* construct the list */
    for (i = 0; i < n; i++)
      {
	list = g_list_append (list, g_strdup (direntrylist[i]->d_name));
      }
  else
    {
      /* no appropriate directory entries found */
      chdir (cwd);
      return NULL;
    }

  /* If there is a .order file use it to reorder the list */
  if ((fp = fopen (orderfile, "r")) == NULL)
    {
      /* '.order' file doesn't exist or could not be opened. */
      chdir (cwd);
      return list;
    }
  else
    {
      char line[MAXSTRLEN];
      order = NULL;
      while (get_line (fp, line, MAXSTRLEN - 1))
	{
	  order = g_list_append (order, g_strdup (line));
	}
      fclose (fp);
    }

  /* order list might still be empty */
  if (order == NULL)
    {
      chdir (cwd);
      return list;
    }

  /* put entries of 'order' in 'list2' if they are also in 'list' */
  list2 = NULL;
  while (order != NULL)
    {
      if (g_list_find_custom (list, order->data, (GCompareFunc) strcmp) != NULL)
	{
	  list2 = g_list_append (list2, order->data);
	}
      order = order->next;
    }

  /* put additional items of list in list2 */
  while (list != NULL)
    {
      if (g_list_find_custom (list2, list->data, (GCompareFunc) strcmp) == NULL)
	{
	  list2 = g_list_append (list2, list->data);
	}
      list = list->next;
    }
  chdir (cwd);
  return list2;
}

/* 'read_dentry' reads 'file' and returns
 * a pointer to an ITEM structure. 
 */

static ITEM *
read_dentry (const char *file, const itemtype type)
{
  FILE *fp;
  char line[MAXSTRLEN], *string, *lang;
  char *fullname = NULL, *shortname = NULL, *cmd = NULL, *term = NULL;
  char *fullcmd = NULL, *name_lang = NULL, *eng_name = NULL;
  ITEM *dentry;

  /* NLS stuff */
  if ((lang = g_getenv ("LANG")) != NULL)
    name_lang = g_strconcat ("Name[", lang, NULL);

  /* try to open the file */
  if ((fp = fopen (file, "r")) == NULL)
    {
      if (type != SUB)
	{
	  return NULL;
	}
      else
	{
	  /* No .directory file found. Use dirname instead */
	  char *dir, *base;
	  dir = g_dirname (file);
	  base = g_basename (dir);
	  if (strcmp (base, ".") != 0)
	    fullname = g_strdup (base);
	  else
	    fullname = g_strdup ((type == GNOME) ?
				 "__gnome_menu__" : "__kde_menu__");
	  g_free (dir);

	  shortname = g_strdup (fullname);
	  /* make lowercase and replace spaces with underscores */
	  g_strdown (shortname);
	  shortname = g_strdelimit (shortname, " ", '_');

	  dentry = (ITEM *) g_malloc (sizeof (ITEM));
	  dentry->type = SUB;
	  dentry->shortname = shortname;
	  dentry->fullname = fullname;
	  dentry->cmd = cmd;
	  return dentry;
	}
    }
  else
    {
      /* read file line by line */
      while (get_line (fp, line, MAXSTRLEN - 1) != 0)
	{

	  if (shortname == NULL)
	    if ((string = strstr (line, "Name=")) != NULL)
	      {
		string += 5;	/* 'string' now points to remainder of line */
		eng_name = g_strdup (string);
		g_strstrip (eng_name);
                if (shortname)
                  g_free (shortname);
		shortname = g_strdup (string);

		/* make lowercase and replace spaces with underscores */
		g_strdown (g_strstrip (shortname));
		shortname = g_strdelimit (shortname, " ", '_');

                if (fullname)
                  g_free (fullname);
		fullname = g_strdup (string);
		g_strstrip (fullname);
		continue;
	      }

	  if (name_lang != NULL)
	    if ((string = strstr (line, name_lang)) != NULL)
	      {
                char *p;
                if ((p = strchr (string, '=')))
                  string = ++p;
                if (fullname)
                  g_free (fullname);
		fullname = g_strdup (string);
                if (shortname == NULL)
                  {
		    shortname = g_strdup (string);
		    /* make lowercase and replace spaces with underscores */
		    g_strdown (g_strstrip (shortname));
		    shortname = g_strdelimit (shortname, " ", '_');
                  }
		g_strstrip (fullname);
		continue;
	      }
            

	  if (type == ENTRY && cmd == NULL)
	    if ((string = strstr (line, "TryExec=")) == NULL &&
				(string = strstr (line, "SwallowExec=")) == NULL &&
				(string = strstr (line, "Exec=")) != NULL)
	      {
		string += 5;
		cmd = g_strdup (string);
		g_strstrip (cmd);
		continue;
	      }

	  if (type == ENTRY && term == NULL)
	    if ((string = strstr (line, "Terminal=")) != NULL)
	      {
		string += 9;
		term = g_strdup (string);
		g_strstrip (term);
		continue;
	      }

	  if (shortname != NULL && fullname != NULL)
	    {
	      if (type == SUB)
		{
		  fclose (fp);
		  if (fullname != eng_name)
		    g_free (eng_name);
		  dentry = (ITEM *) g_malloc (sizeof (ITEM));
		  dentry->type = SUB;
		  dentry->shortname = shortname;
		  dentry->fullname = fullname;
		  dentry->cmd = cmd;
		  return dentry;
		}
	      else if (term != NULL && cmd != NULL)
		break;
	    }
	}

      /* If we got here it means we're not dealing with a submenu
       * or we couldn't find a name.
       * We now need to find the command to execute.
       */

      fclose (fp);

      if ((shortname == NULL) || ((cmd == NULL) && (type == ENTRY)) || ((fullname == NULL) && (eng_name == NULL)))
	{
	  return NULL;
	}

      if (fullname == NULL)
	fullname = eng_name;
      else
	g_free (eng_name);

      dentry = (ITEM *) g_malloc (sizeof (ITEM));
      dentry->type = type;
      dentry->shortname = shortname;
      dentry->fullname = fullname;

      if (term != NULL &&
	  ((strcmp (term, "1") == 0) || (mystrcasecmp (term, "True") == 0)))
	{
	  fullcmd = g_strconcat ("xfterm -e \"", cmd, "\"", NULL);
	  dentry->cmd = fullcmd;
	}
      else
	{
	  dentry->cmd = cmd;
	}

      return dentry;
    }
}

/* get_menu_name determines the name to use for the current submenu */
static char *
get_menu_name (const char *dir, menutype mtype)
{
  FILE *fp;
  char *file = g_strconcat (dir, "/.directory", NULL);
  char line[MAXSTRLEN], *string, *name, *dirname;

  if ((fp = fopen (file, "r")) != NULL)
    {
      while (get_line (fp, line, MAXSTRLEN - 1) != 0)
	{
	  if ((string = strstr (line, "Name=")) != NULL)
	    {
	      string += 5;
	      name = g_strdup (string);
	      g_strdown (g_strstrip (name));
	      name = g_strdelimit (name, " ", '_');
	      fclose (fp);
	      return name;
	    }
	}
      fclose (fp);
    }

  /* No .directory file or no name found. Use dirname instead */
  dirname = g_basename (dir);
  if (strcmp (dirname, ".") != 0)
    return g_strdup (dirname);
  else
    return g_strdup ((mtype == GNOME) ? "__gnome_menu__" : "__kde_menu__");
}

/* 'is_executable' tries to find executable 'cmd'. It
 * first looks if path is absolute and otherwise searches 
 * in PATH.
 */

static int
is_executable (const char *cmd)
{
  char *path, *dir, *name;
  char *bin = NULL;

  if (cmd == NULL || strlen(cmd)==0)
    return 0;

  /* remove command line switches */
  bin = g_strdup (cmd);
  bin = g_strdelimit (bin, " ", '\0');

  if (g_path_is_absolute (bin))
    {
      if (access (bin, F_OK) == -1)
	{
	  /* file doesn't exist */
	  g_free (bin);
	  return 0;
	}
      else if (access (bin, X_OK) == 0)
	{
	  /* file is executable */
	  g_free (bin);
	  return 1;
	}
      else
	{
	  g_free (bin);
	  return 0;
	}
    }
  else
    {
      /* look in PATH */
      path = g_strdup (g_getenv ("PATH"));
      dir = strtok (path, ":");

      if (dir == NULL)
	{
	  g_free (bin);
	  g_free (path);
	  return 0;
	}
      else
	{
	  do
	    {
	      name = g_strconcat (dir, "/", bin, NULL);
	      if (access (name, F_OK) == 0 && access (name, X_OK) == 0)
		{
		  g_free (path);
		  g_free (bin);
		  g_free (name);
		  return 1;
		}
	      g_free (name);
	    }
	  while ((dir = strtok (NULL, ":")) != NULL);

	  g_free (path);
	  g_free (bin);
	  return 0;
	}
    }
}

/* function 'get_gnome_or_kde_dirs' returns a GList of 
 * directories. The argument is either GNOME or KDE.
 */

static GList *
get_gnome_or_kde_dirs (menutype mtype)
{
  char *path, *menupath;
  char *homedir = g_strdup (g_get_home_dir ());
  GList *dirs = NULL;

  if (mtype == GNOME)
    {
      path = g_getenv ("GNOMEDIR");	/* look if GNOMEDIR is set */
      menupath = GMENUPATH;
    }
  else
    {
      path = g_getenv ("KDEDIR");	/* idem for KDEDIR */
      menupath = KMENUPATH;
    }

  if (path == NULL)
    {
      /* GNOMEDIR / KDEDIR not defined. Supply some possible locations. */
      dirs = g_list_append (dirs, g_strconcat ("/usr", menupath, NULL));
      dirs = g_list_append (dirs, g_strconcat ("/usr/local", menupath, NULL));
      if (mtype == GNOME)
	dirs = g_list_append (dirs,
			      g_strconcat ("/opt/gnome", menupath, NULL));
      else
        {
	  dirs = g_list_append (dirs, g_strconcat ("/opt/kde", menupath, NULL));
	  dirs = g_list_append (dirs, g_strconcat ("/opt/kde2", menupath, NULL));
          dirs = g_list_append (dirs, "/usr/share/kde/applnk");
          dirs = g_list_append (dirs, "/usr/local/share/kde/applnk");
          dirs = g_list_append (dirs, "/usr/share/kde2/applnk");
          dirs = g_list_append (dirs, "/usr/local/share/kde2/applnk");
        }
    }
  else
    {
      /* GNOMEDIR / KDEDIR defined. */
      dirs = g_list_append (dirs, g_strconcat (path, menupath, NULL));
    }

  /* Add user's home directory */
  if (mtype == GNOME)
    dirs = g_list_append (dirs, g_strconcat (homedir, "/.gnome/apps", NULL));
  else
    {
      dirs = g_list_append (dirs, g_strconcat (homedir, "/.kde/applnk", NULL));
      dirs = g_list_append (dirs, g_strconcat (homedir, "/.kde2/applnk", NULL));
    }
  /* NOTE: This is a guess (Jasper) */
  return dirs;
}


/* make_gnome_menu recursively finds entries and submenus in 
 * 'rootdir' and every subdirectory. 
 * The 'level' argument is necessary to set the right name for 
 * the root menu to be able to add it to the user menu in 'main'.
 */

void
make_menu (const char *rootdir, const int level, GList **entrynames, menutype mtype)
{
  char buffer[MAXSTRLEN];
  char *name = NULL;
  char *menuname = NULL;
  char *fullpath = NULL;
  GList *subdirs, *entries;
  ITEM *item;

  /* read the directory entries */
  subdirs = get_subs_or_entries (rootdir, SUB);
  entries = get_subs_or_entries (rootdir, ENTRY);

  if (level == 0)
    {
      if (mtype == GNOME)
	menuname = g_strdup ("__gnome_menu__");
      else
	menuname = g_strdup ("__kde_menu__");
    }
  else
    {
      name = get_menu_name (rootdir, mtype);
      menuname = (char *) g_malloc (MAXSTRLEN * sizeof (char));
      g_snprintf (menuname, MAXSTRLEN - 1, MENUNAME,
		  ((mtype == GNOME) ? "gnome" : "kde"), name, level);
      g_free (name);
    }

  /* Here comes the recursive part */

  while (subdirs != NULL)
    {
      char *next_name = NULL;
      int next_level;
      /* supply the path relative to rootdir 
       * for the directory-reading functions */
      fullpath = g_strconcat (rootdir, "/", subdirs->data, NULL);
      next_level = level + 1;
      make_menu (fullpath, next_level, entrynames, mtype);
      g_free (fullpath);

      /* information about the subdir is in the .directory file,
       * but code also works if this is not present */
      fullpath =
	g_strconcat (rootdir, "/", subdirs->data, "/.directory", NULL);
      item = read_dentry (fullpath, SUB);
      if (item == NULL)
	{
	  subdirs = subdirs->next;
	  continue;
	}

      next_name = (char *) g_malloc (MAXSTRLEN * sizeof (char));
      g_snprintf (next_name, MAXSTRLEN - 1, MENUNAME,
		  ((mtype == GNOME) ? "gnome" : "kde"),
		  item->shortname, next_level);
      g_free (item->shortname);
      item->shortname = next_name;

      if ((*entrynames == NULL) || 
          (g_list_find_custom (*entrynames, item->shortname, (GCompareFunc) strcmp) == NULL))
      {
        g_snprintf (buffer, MAXSTRLEN - 1,
		    "AddToMenu \"%s\" \"%s\" popup \"%s\"\n",
		    menuname, item->fullname, item->shortname);
        SendInfo (fd, buffer, 0);
        *entrynames = g_list_append (*entrynames, g_strdup(item->shortname));
      }
      g_free (fullpath);
      g_free (item);
      subdirs = subdirs->next;
    }


  /* ... and entries */
  while (entries != NULL)
    {
      fullpath = g_strconcat (rootdir, "/", entries->data, NULL);
      item = read_dentry (fullpath, ENTRY);
      if (item == NULL)
	{
	  g_free (fullpath);
	  entries = entries->next;
	  continue;
	}

      /* check if the command is executable by the user */
      if (is_executable (item->cmd) == 1)
	{
	  g_snprintf (buffer, MAXSTRLEN - 1,
		      "AddToMenu \"%s\" \"%s\" Exec %s\n",
		      menuname, item->fullname, item->cmd);
	  SendInfo (fd, buffer, 0);
	}
      g_free (fullpath);
      g_free (item);
      entries = entries->next;
    }

  g_free (menuname);
}

void
DeadPipe (int nonsense)
{
  exit (0);
}

/*  main program  */

int
main (int argc, char **argv)
{
  char *dir;
  GList *dirs;
  GList *entrynames;
  menutype mtype;
  /* for main loop */
  int i, start, stop;

  if (argc < 7)
    {
      fprintf (stderr, "This module should be executed by xfwm\n");
      exit (0);
    }

  fd[0] = atoi (argv[1]);
  fd[1] = atoi (argv[2]);
  SetMessageMask (fd, 0);

  entrynames = NULL;
  /* Find out what we have to do */
  if (argv[7] && argv[8] &&
      ((strcmp (argv[7], "-gnome") == 0 && strcmp (argv[8], "-kde") == 0) ||
       (strcmp (argv[7], "-kde") == 0 && strcmp (argv[8], "-gnome") == 0)))
    {
      /* both gnome and kde menus */
      start = 0;
      stop = 1;
    }
  else if (argv[7] && strcmp (argv[7], "-kde") == 0)
    {
      /* kde menu */
      start = 1;
      stop = 1;
    }
  else
    {
      /* gnome menu */
      start = 0;
      stop = 0;
    }

  /* main loop */
  for (i = start; i <= stop; ++i)
    {

      if (i == 1)
	mtype = KDE;
      else
	mtype = GNOME;

      /* get dirs */
      dirs = get_gnome_or_kde_dirs (mtype);

      /* make menus for each of the 'dirs' */
      while (dirs)
	{
	  dir = dirs->data;
	  if (chdir (dir) == 0)
	    {
	      make_menu (".", 0, &entrynames, mtype);
	    }
	  dirs = dirs->next;
	}

      /* add menu to user_menu */
      if (mtype == GNOME)
	{
	  fprintf (stderr, "Loading GNOME menus\n");
	  SendInfo (fd,
		    "AddToMenu \"user_menu\" GNOME popup \"__gnome_menu__\"\n",
		    0);
	}
      else
	{
	  fprintf (stderr, "Loading KDE menus\n");
	  SendInfo (fd,
		    "AddToMenu \"user_menu\" KDE popup \"__kde_menu__\"\n",
		    0);
	}
      g_list_free (entrynames);
      entrynames = NULL;
    }				/* end main loop */

  close (fd[0]);
  close (fd[1]);
  return 0;
}

/*  end main  */
