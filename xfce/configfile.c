/*  gxfce
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
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

#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <X11/Xlib.h>
#include "constant.h"
#include "configfile.h"
#include "my_string.h"
#include "xfce-common.h"
#include "xfce_main.h"
#include "selects.h"
#include "popup.h"
#include "xfce.h"
#include "xfwm.h"
#include "fileutil.h"
#include "my_intl.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

char *rcfile = "xfce3rc";
int nl = 0;
int buffersize = 127;

void
syntax_error (char *s)
{
  fprintf (stderr, _("XFce : Syntax error in configuration file\n(%s)\n"), s);
  my_alert (_("Syntax error in configuration file\nAborting"));
  end_XFCE (2);
}

void
data_error (char *s)
{
  fprintf (stderr, _("XFce : Data mismatch error in config file\n(%s)\n"), s);
  my_alert (_("Data mismatch error in configuration file\nAborting"));
  end_XFCE (3);
}

char *
nextline (FILE * f, char *lineread)
{
  char *p;
  do
    {
      nl++;
      if (!fgets (lineread, MAXSTRLEN + 1, f))
        {
          return (NULL);
	}
      if (strlen (lineread))
        {
	  lineread [strlen (lineread) - 1] = '\0';
	}
      p = skiphead (lineread);
    }
  while (!strlen (p) && !feof (f));
  if (strlen (p))
    skiptail (p);
  return ((!feof (f)) ? p : NULL);
}

config *
initconfig (config * newconf)
{
  if (!newconf)
    newconf = (config *) g_malloc (sizeof (config) + 1);
  newconf->panel_x = -1;
  newconf->panel_y = -1;
  newconf->wm = 0;
  newconf->visible_screen = 4;
  newconf->visible_popup = 6;
  newconf->select_icon_size = 0; /* Small size */
  newconf->popup_icon_size = 0;  /* Small size */
  newconf->colorize_root = FALSE;
  newconf->gradient_root = FALSE;
  newconf->detach_menu = TRUE;
  newconf->gradient_active_title = FALSE;
  newconf->gradient_inactive_title = TRUE;
  newconf->clicktofocus = TRUE;
  newconf->opaquemove = TRUE;
  newconf->opaqueresize = TRUE;
  newconf->show_diagnostic = FALSE;
  newconf->apply_xcolors = TRUE;
  newconf->mapfocus = TRUE;
  newconf->snapsize = 10;
  newconf->autoraise = 0;
  newconf->tooltipsdelay = 250;
  newconf->startup_flags = (F_SOUNDMODULE | F_MOUSEMODULE |
			    F_BACKDROPMODULE | F_PAGERMODULE | 
                            F_GNOMEMODULE | F_GNOMEMENUMODULE |
                            F_KDEMENUMODULE);
  newconf->iconpos = 0;		/* Top of screen */
  newconf->fonts[0] =
    (char *) g_malloc (sizeof (char) * (strlen (XFWM_TITLEFONT) + 1));
  newconf->fonts[1] =
    (char *) g_malloc (sizeof (char) * (strlen (XFWM_MENUFONT) + 1));
  newconf->fonts[2] =
    (char *) g_malloc (sizeof (char) * (strlen (XFWM_ICONFONT) + 1));
  strcpy (newconf->fonts[0], XFWM_TITLEFONT);
  strcpy (newconf->fonts[1], XFWM_MENUFONT);
  strcpy (newconf->fonts[2], XFWM_ICONFONT);
  newconf->digital_clock = 0;
  newconf->hrs_mode = 1;
  newconf->panel_layer = DEFAULT_LAYER;
  newconf->xfwm_engine = XFCE_ENGINE;

  return newconf;
}

void
backupconfig (char *extension)
{
  char homedir[MAXSTRLEN + 1];
  char buffer[MAXSTRLEN + 1];
  char backname[MAXSTRLEN + 1];
  FILE *copyfile;
  FILE *backfile;
  int nb_read;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  /*
     Backup any existing config file before creating a new one 
   */
  if (existfile (homedir))
    {
      snprintf (backname, MAXSTRLEN, "%s%s", homedir, extension);
      backfile = fopen (backname, "w");
      copyfile = fopen (homedir, "r");
      if ((backfile) && (copyfile))
	{
	  while ((nb_read = fread (buffer, 1, MAXSTRLEN, copyfile)) > 0)
	    {
	      fwrite (buffer, 1, nb_read, backfile);
	    }
	  fflush (backfile);
	  fclose (backfile);
	  fclose (copyfile);
	}
    }
}

void
writeconfig (void)
{
  char homedir[MAXSTRLEN + 1];
  FILE *configfile = NULL;
  int i, j;
  int x, y;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  /*
     Backup any existing config file before creating a new one 
   */
  if (existfile (homedir))
    backupconfig (BACKUPEXT);

  configfile = fopen (homedir, "w");

  if (!configfile)
    my_alert (_("Cannot create file"));
  else
    {
      fprintf (configfile, "%s\n", XFCE3SIG);
      fprintf (configfile, "[Coords]\n");
      gdk_window_get_root_origin (gxfce->window, &x, &y);
      fprintf (configfile, "\t%i\n", x);
      fprintf (configfile, "\t%i\n", y);
      fprintf (configfile, "[ButtonLabels]\n");
      for (i = 0; i < NBSCREENS; i++)
	fprintf (configfile, "\t%s\n", get_gxfce_screen_label (i));
      fprintf (configfile, "[External_Icons]\n");
      for (i = 0; i < NBSELECTS; i++)
	if (get_exticon_str (i) && (strlen (get_exticon_str (i))))
	  fprintf (configfile, "\t%s\n", get_exticon_str (i));
	else
	  fprintf (configfile, "\tNone\n");
      fprintf (configfile, "[Popups]\n");
      fprintf (configfile, "\t%i\n", current_config.visible_popup);
      fprintf (configfile, "[Icons]\n");
      fprintf (configfile, "\t%s\n", save_icon_str ());
      fprintf (configfile, "[WorkSpace]\n");
      fprintf (configfile,
	       current_config.colorize_root ? "\tRepaint\n" :
	       "\tNoRepaint\n");
      fprintf (configfile,
	       current_config.gradient_root ? "\tGradient\n" : "\tSolid\n");
      fprintf (configfile, "[Lock]\n");
      fprintf (configfile, "\t%s\n", get_command (NBSELECTS));
      fprintf (configfile, "[MenuOption]\n");
      fprintf (configfile,
	       current_config.detach_menu ? "\tDetach\n" : "\tNoDetach\n");
      fprintf (configfile, "[XFwmOption]\n");
      fprintf (configfile,
	       current_config.clicktofocus ? "\tClickToFocus\n" :
	       "\tFollowMouse\n");
      fprintf (configfile,
	       current_config.opaquemove ? "\tOpaqueMove\n" :
	       "\tNoOpaqueMove\n");
      fprintf (configfile,
	       current_config.opaqueresize ? "\tOpaqueResize\n" :
	       "\tNoOpaqueResize\n");
      fprintf (configfile, "\t%i\n", current_config.snapsize);
      fprintf (configfile, "\t%i\n", current_config.startup_flags);
      fprintf (configfile,
	       current_config.autoraise ? "\tAutoraise\n" :
	       "\tNoAutoraise\n");
      fprintf (configfile,
	       current_config.gradient_active_title ? "\tGradientActive\n" :
	       "\tOpaqueActive\n");
      fprintf (configfile,
	       current_config.gradient_inactive_title ? "\tGradientInactive\n"
	       : "\tOpaqueInactive\n");
      switch (current_config.iconpos)
	{
	case 1:
	  fprintf (configfile, "\tIconsOnLeft\n");
	  break;
	case 2:
	  fprintf (configfile, "\tIconsOnBottom\n");
	  break;
	case 3:
	  fprintf (configfile, "\tIconsOnRight\n");
	  break;
	default:
	  fprintf (configfile, "\tIconsOnTop\n");
	}
      fprintf (configfile, "\t%s\n", current_config.fonts[0]);
      fprintf (configfile, "\t%s\n", current_config.fonts[1]);
      fprintf (configfile, "\t%s\n", current_config.fonts[2]);
      fprintf (configfile,
	       current_config.mapfocus ? "\tMapFocus\n" :
	       "\tNoMapFocus\n");
      switch (current_config.xfwm_engine)
	{
	case MOFIT_ENGINE:
	  fprintf (configfile, "\tMofit_engine\n");
	  break;
	case TRENCH_ENGINE:
	  fprintf (configfile, "\tTrench_engine\n");
	  break;
	case GTK_ENGINE:
	  fprintf (configfile, "\tGtk_engine\n");
	  break;
	default:
	  fprintf (configfile, "\tXfce_engine\n");
	  break;
	}
      fprintf (configfile, "[Screens]\n");
      fprintf (configfile, "\t%i\n", current_config.visible_screen);
      fprintf (configfile, "[Tooltips]\n");
      fprintf (configfile, "\t%i\n", current_config.tooltipsdelay);
      fprintf (configfile, "[Clock]\n");
      fprintf (configfile,
	       current_config.digital_clock ? "\tDigital\n" : "\tAnalog\n");
      fprintf (configfile,
	       current_config.hrs_mode ? "\t24hrs\n" : "\t12hrs\n");
      fprintf (configfile, "[Sizes]\n");
      switch (current_config.select_icon_size)
	{
	case 0:
	  fprintf (configfile, "\tSmallPanelIcons\n");
	  break;
	case 2:
	  fprintf (configfile, "\tLargePanelIcons\n");
	  break;
	default:
	  fprintf (configfile, "\tMediumPanelIcons\n");
	}
      switch (current_config.popup_icon_size)
	{
	case 0:
	  fprintf (configfile, "\tSmallMenuIcons\n");
	  break;
	case 2:
	  fprintf (configfile, "\tLargeMenuIcons\n");
	  break;
	default:
	  fprintf (configfile, "\tMediumMenuIcons\n");
	}
      fprintf (configfile, "[XColors]\n");
      fprintf (configfile,
	       current_config.apply_xcolors ? "\tApply\n" : "\tIgnore\n");
      fprintf (configfile, "[Diagnostic]\n");
      fprintf (configfile,
	       current_config.show_diagnostic ? "\tShow\n" : "\tIgnore\n");
      fprintf (configfile, "[Layer]\n");
      fprintf (configfile, "\t%d\n", current_config.panel_layer);
      fprintf (configfile, "[Commands]\n");
      for (i = 0; i < NBSELECTS; i++)
	if (strlen (selects[i].command))
	  fprintf (configfile, "\t%s\n", get_command (i));
	else
	  fprintf (configfile, "\tNone\n");
      for (i = 0; i < NBPOPUPS; i++)
	{
	  fprintf (configfile, "[Menu%u]\n", i + 1);
	  for (j = 0; j < get_popup_menu_entries (i); j++)
	    {


	      fprintf (configfile, "\t%s\n", get_popup_entry_label (i, j));
	      fprintf (configfile, "\t%s\n", get_popup_entry_icon (i, j));
	      fprintf (configfile, "\t%s\n", get_popup_entry_command (i, j));
	    }

	}
      fflush (configfile);
      fclose (configfile);
    }
}

void
resetconfig (void)
{
  char homedir[MAXSTRLEN + 1];
  FILE *configfile;
  int i;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  configfile = fopen (homedir, "w");
  if (!configfile)
    my_alert (_("Cannot reset configuration file"));
  else
    {
      fprintf (stderr, _("Creating new config file...\n"));
      fprintf (configfile, "%s\n", XFCE3SIG);
      fprintf (configfile, "[Coords]\n");
      fprintf (configfile, "\t%i\n", -1);
      fprintf (configfile, "\t%i\n", -1);
      fprintf (configfile, "[ButtonLabels]\n");
      for (i = 0; i < NBSCREENS; i++)
	fprintf (configfile, "\t%s\n", screen_names[i]);
      fprintf (configfile, "[External_Icons]\n");
      for (i = 0; i < NBSELECTS; i++)
	fprintf (configfile, "\tNone\n");
      fprintf (configfile, "[Popups]\n");
      fprintf (configfile, "\t6\n");
      fprintf (configfile, "[Icons]\n");
      fprintf (configfile, "\t%s\n", DEFAULT_ICON_SEQ);
      fprintf (configfile, "[WorkSpace]\n");
      fprintf (configfile, "\tRepaint\n");
      fprintf (configfile, "\tGradient\n");
      fprintf (configfile, "[Lock]\n");
      fprintf (configfile, "\tNone\n");
      fprintf (configfile, "[MenuOption]\n");
      fprintf (configfile, "\tDetach\n");
      fprintf (configfile, "[XFwmOption]\n");
      fprintf (configfile, "\tClickToFocus\n");
      fprintf (configfile, "\tOpaqueMove\n");
      fprintf (configfile, "\tOpaqueResize\n");
      fprintf (configfile, "\t10\n");
      fprintf (configfile, "\t%i\n",
	       (F_SOUNDMODULE | F_MOUSEMODULE | F_BACKDROPMODULE |
		F_PAGERMODULE));
      fprintf (configfile, "\tNoAutoRaise\n");
      fprintf (configfile, "\tGradientActive\n");
      fprintf (configfile, "\tGradientInactive\n");
      fprintf (configfile, "\tIconsOnTop\n");
      fprintf (configfile, "\t%s\n", XFWM_TITLEFONT);
      fprintf (configfile, "\t%s\n", XFWM_MENUFONT);
      fprintf (configfile, "\t%s\n", XFWM_ICONFONT);
      fprintf (configfile, "\tMapFocus\n");
      fprintf (configfile, "\tXfce_engine\n");
      fprintf (configfile, "[Screens]\n");
      fprintf (configfile, "\t4\n");
      fprintf (configfile, "[Tooltips]\n");
      fprintf (configfile, "\t250\n");
      fprintf (configfile, "[Clock]\n");
      fprintf (configfile, "\tAnalog\n");
      fprintf (configfile, "\t24hrs\n");
      fprintf (configfile, "[Sizes]\n");
      fprintf (configfile, "\tMediumPanelIcons\n");
      fprintf (configfile, "\tSmallMenuIcons\n");
      fprintf (configfile, "[XColors]\n");
      fprintf (configfile, "\tApply\n");
      fprintf (configfile, "[Diagnostic]\n");
      fprintf (configfile, "\tIgnore\n");
      fprintf (configfile, "[Layer]\n");
      fprintf (configfile, "\t%d\n", DEFAULT_LAYER);
      fprintf (configfile, "[Commands]\n");
      for (i = 0; i < NBSELECTS; i++)
	fprintf (configfile, "\tNone\n");
      for (i = 0; i < NBPOPUPS; i++)
	{
	  fprintf (configfile, "[Menu%u]\n", i + 1);
	}
      fflush (configfile);
      fclose (configfile);
    }
}

static void
localize_rcfilename (char *rcfile)
{
  char charset_code[MAXSTRLEN + 1];
  char area_code[MAXSTRLEN + 1];
  char country_code[MAXSTRLEN + 1];
  char temp[MAXSTRLEN + 1];
  int bottom_ptr = 0;

  if (strcmp (rcfile, "") == 0)
    return;
  if (!getenv ("LANG"))
    return;
  if (strcmp (getenv ("LANG"), "") == 0)
    return;
  strncpy (charset_code, getenv ("LANG"), MAXSTRLEN);
  bottom_ptr = strlen (charset_code) - 1;

  /* Try Charset Code */
  snprintf (temp, MAXSTRLEN, "%s.%s", rcfile, charset_code);
  if (existfile (temp))
    {
      strcpy (rcfile, temp);
      return;
    }

  /* Try Area Code */
  while (charset_code[bottom_ptr] != '.')
    {
      if (bottom_ptr <= 0)
	{
	  bottom_ptr = strlen (charset_code);
	  break;
	}
      bottom_ptr--;
    }
  strncpy (area_code, charset_code, bottom_ptr);
  area_code[bottom_ptr] = '\0';
  snprintf (temp, MAXSTRLEN, "%s.%s", rcfile, charset_code);
  if (existfile (temp))
    {
      strcpy (rcfile, temp);
      return;
    }

  /* Try Country Code */
  while (charset_code[bottom_ptr] != '_')
    {
      if (bottom_ptr <= 0)
	{
	  bottom_ptr = strlen (charset_code);
	  break;
	}
      bottom_ptr--;
    }
  strncpy (country_code, charset_code, bottom_ptr);
  area_code[bottom_ptr] = '\0';
  snprintf (temp, MAXSTRLEN, "%s.%s", rcfile, country_code);
  if (existfile (temp))
    {
      strcpy (rcfile, temp);
      return;
    }
}

char *
get_first_ok (char *p)
{
  char *tok;
  static char ret[MAXSTRLEN + 1];
  int nb;
  if (p == NULL)
    return (p);
  if (p == "")
    return (p);
  tok = strtok (p, ",");
  while (tok != NULL)
    {
      tok = skiphead (tok);
      if (strstr (tok, "Module"))
	{
	  tok += 7;
	  nb = (int) (strcspn (tok, " "));
	  strncpy (ret, tok, nb);
	  ret[nb] = '\0';
	  if (existfile (ret))
	    {
	      tok -= 7;
	      strcpy (ret, tok);
	      skiptail (ret);
	      return (ret);
	    }
	}
      if (strstr (tok, "Term"))
	{
	  tok += 5;
	  nb = (int) (strcspn (tok, " "));
	  strncpy (ret, tok, nb);
	  ret[nb] = '\0';
	  if (existfile (ret))
	    {
	      tok -= 5;
	      strcpy (ret, tok);
	      skiptail (ret);
	      return (ret);
	    }
	}
      nb = (int) (strcspn (tok, " "));
      strncpy (ret, tok, nb);
      ret[nb] = '\0';
      if (existfile (ret))
	{
	  strcpy (ret, tok);
	  skiptail (ret);
	  return (ret);
	}
      tok = strtok (NULL, ",");
    }
  return ("None");
}

void
readconfig (void)
{
  char homedir[MAXSTRLEN + 1];
  char lineread[MAXSTRLEN + 1];
  char pixfile[MAXSTRLEN + 1];
  char command[MAXSTRLEN + 1];
  char label[256];
  char dummy[16];
  char *p;
  FILE *configfile = NULL;


  int i, j;
  int newconf = 0;
  nl = 0;
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  if (existfile (homedir))
    {
      configfile = fopen (homedir, "r");
    }
  else
    {
      fprintf (stderr, _("XFce : %s File not found.\n"), homedir);
      snprintf (homedir, MAXSTRLEN, "%s/%s", XFCE_CONFDIR, rcfile);
      if (existfile (homedir))
	{
	  configfile = fopen (homedir, "r");
	}
      else
	{
	  newconf = 1;
	  fprintf (stderr, _("XFce : %s File not found.\n"), homedir);
	  localize_rcfilename (homedir);
	  configfile = fopen (homedir, "r");
	}
    }
  if (!configfile)
    {
      my_alert (_("Cannot open configuration file"));
      fprintf (stderr, _("XFce : %s File not found.\n"), homedir);
      resetconfig ();
      snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
      configfile = fopen (homedir, "r");
    }
  if (!configfile)
    my_alert (_("Cannot open configuration file"));
  else
    {
      p = nextline (configfile, lineread);
      if (my_strncasecmp (p, XFCE3SIG, strlen (XFCE3SIG)))
	{
	  my_alert (_("Does not looks like an XFce 3 configuration file !"));
	  if (my_yesno_dialog
	      (_("Do you want to reset the configuration file ?\n \
	(The previous file will be saved with a \".orig\" extension)")))
	    {
	      backupconfig (".orig");
	      fclose (configfile);
	      resetconfig ();
              snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
	      configfile = fopen (homedir, "r");
	      if (!configfile)
		{
		  my_alert (_("Cannot open new config, Giving up..."));
		  data_error (_("Cannot load configuration file"));
		}
	      /* Skipping first line */
	      p = nextline (configfile, lineread);
	    }
	  else
	    syntax_error (_
			  ("Cannot use old version of XFce configuration files"));
	}
      p = nextline (configfile, lineread);
      if (my_strncasecmp (p, "[Coords]", strlen ("[Coords]")))
	syntax_error (p);
      p = nextline (configfile, lineread);
      current_config.panel_x = atoi (p);
      p = nextline (configfile, lineread);
      current_config.panel_y = atoi (p);
      p = nextline (configfile, lineread);
      if (my_strncasecmp (p, "[ButtonLabels]", strlen ("[ButtonLabels]")))
	syntax_error (p);
      i = 0;
      p = nextline (configfile, lineread);
      while ((i < NBSCREENS)
	     &&
	     (my_strncasecmp
	      (p, "[External_Icons]", strlen ("[External_Icons]"))))
	{
	  set_gxfce_screen_label (i, p);
	  p = nextline (configfile, lineread);
	  i++;
	}
      if (!my_strncasecmp
	  (p, "[External_Icons]", strlen ("[External_Icons]")))
	{
	  for (i = 0; i < NBSELECTS + 1; i++)
	    {
	      p = nextline (configfile, lineread);
	      if ((my_strncasecmp (p, "[Icons]", strlen ("[Icons]")))
		  && (my_strncasecmp (p, "[Popups]", strlen ("[Popups]"))))
		{
		  set_exticon_str (i, p);
		}
	      else
		{
		  break;
		}
	    }
	}
      if (!my_strncasecmp (p, "[Popups]", strlen ("[Popups]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.visible_popup = atoi (p);
	  if ((current_config.visible_popup) > NBPOPUPS)
	    current_config.visible_popup = NBPOPUPS;
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Icons]", strlen ("[Icons]")))
	{
	  p = nextline (configfile, lineread);
	  load_icon_str (p);
	  p = nextline (configfile, lineread);
	}
      else
	{
	  default_icon_str ();
	}
      if (!my_strncasecmp (p, "[WorkSpace]", strlen ("[WorkSpace]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.colorize_root =
	    (my_strncasecmp (p, "Repaint", strlen ("Repaint")) == 0);
	  p = nextline (configfile, lineread);
	  current_config.gradient_root =
	    (my_strncasecmp (p, "Gradient", strlen ("Gradient")) == 0);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Lock]", strlen ("[Lock]")))
	{
	  p = nextline (configfile, lineread);
	  set_command (NBSELECTS, p);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[MenuOption]", strlen ("[MenuOption]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.detach_menu =
	    (my_strncasecmp (p, "Detach", strlen ("Detach")) == 0);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[XFwmOption]", strlen ("[XFwmOption]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.clicktofocus =
	    (my_strncasecmp (p, "ClickToFocus", strlen ("ClickToFocus")) ==
	     0);
	  p = nextline (configfile, lineread);
	  current_config.opaquemove =
	    (my_strncasecmp (p, "OpaqueMove", strlen ("OpaqueMove")) == 0);
	  p = nextline (configfile, lineread);
	  if ((!my_strncasecmp (p, "OpaqueResize", strlen ("OpaqueResize")))
	      ||
	      (!my_strncasecmp
	       (p, "NoOpaqueResize", strlen ("NoOpaqueResize"))))
	    {
	      current_config.opaqueresize =
		(my_strncasecmp (p, "OpaqueResize", strlen ("OpaqueResize"))
		 == 0);
	      p = nextline (configfile, lineread);
	      current_config.snapsize = atoi (p);
	      p = nextline (configfile, lineread);
	      current_config.startup_flags = atoi (p);
	      p = nextline (configfile, lineread);
	    }
	  current_config.autoraise =
	    (my_strncasecmp (p, "AutoRaise", strlen ("AutoRaise")) == 0);
	  p = nextline (configfile, lineread);
	  current_config.gradient_active_title =
	    (my_strncasecmp (p, "GradientActive", strlen ("GradientActive"))
	     == 0);
	  p = nextline (configfile, lineread);
	  current_config.gradient_inactive_title =
	    (my_strncasecmp
	     (p, "GradientInactive", strlen ("GradientInactive")) == 0);
	  p = nextline (configfile, lineread);
	  if (!my_strncasecmp (p, "IconsOnLeft", strlen ("IconsOnLeft")))
	    current_config.iconpos = 1;
	  else
	    if (!my_strncasecmp
		(p, "IconsOnBottom", strlen ("IconsOnBottom")))
	    current_config.iconpos = 2;
	  else
	    if (!my_strncasecmp (p, "IconsOnRight", strlen ("IconsOnRight")))
	    current_config.iconpos = 3;
	  else
	    current_config.iconpos = 0;
	  p = nextline (configfile, lineread);
	  if (current_config.fonts[0])
	    g_free (current_config.fonts[0]);
	  current_config.fonts[0] =
	    (char *) g_malloc (sizeof (char) * (strlen (p) + 1));
	  strcpy (current_config.fonts[0], p);
	  p = nextline (configfile, lineread);
	  if (current_config.fonts[1])
	    g_free (current_config.fonts[1]);
	  current_config.fonts[1] =
	    (char *) g_malloc (sizeof (char) * (strlen (p) + 1));
	  strcpy (current_config.fonts[1], p);
	  p = nextline (configfile, lineread);
	  if (current_config.fonts[2])
	    g_free (current_config.fonts[2]);
	  current_config.fonts[2] =
	    (char *) g_malloc (sizeof (char) * (strlen (p) + 1));
	  strcpy (current_config.fonts[2], p);
	  p = nextline (configfile, lineread);
          if (!my_strncasecmp (p, "MapFocus", strlen ("MapFocus")))
            {
	      current_config.mapfocus=TRUE;
	      p = nextline (configfile, lineread);
            }
          else if (!my_strncasecmp (p, "NoMapFocus", strlen ("NoMapFocus")))
            {
	      current_config.mapfocus=FALSE;
	      p = nextline (configfile, lineread);
            }
          if (!my_strncasecmp (p, "X", strlen ("X")))
            {
	      current_config.xfwm_engine = XFCE_ENGINE;
	      p = nextline (configfile, lineread);
            }
          else if (!my_strncasecmp (p, "M", strlen ("M")))
            {
	      current_config.xfwm_engine = MOFIT_ENGINE;
	      p = nextline (configfile, lineread);
            }
          else if (!my_strncasecmp (p, "T", strlen ("T")))
            {
	      current_config.xfwm_engine = TRENCH_ENGINE;
	      p = nextline (configfile, lineread);
            }
          else if (!my_strncasecmp (p, "G", strlen ("G")))
            {
	      current_config.xfwm_engine = GTK_ENGINE;
	      p = nextline (configfile, lineread);
            }
          else if (my_strncasecmp (p, "[", strlen ("[")))
            {
              /*
               * Need to read a new line in case the theme is something else...
               */
  	      p = nextline (configfile, lineread);
            }
	}
      if (!my_strncasecmp (p, "[Screens]", strlen ("[Screens]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.visible_screen = atoi (p);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Tooltips]", strlen ("[Tooltips]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.tooltipsdelay = atoi (p);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Clock]", strlen ("[Clock]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.digital_clock =
	    (my_strncasecmp (p, "Digital", strlen ("Digital")) == 0);
	  p = nextline (configfile, lineread);
	  current_config.hrs_mode =
	    (my_strncasecmp (p, "24hrs", strlen ("24hrs")) == 0);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Sizes]", strlen ("[Sizes]")))
	{
	  p = nextline (configfile, lineread);
	  if (!my_strncasecmp
	      (p, "SmallPanelIcons", strlen ("SmallPanelIcons")))
	    current_config.select_icon_size = 0;
	  else
	    if (!my_strncasecmp
		(p, "LargePanelIcons", strlen ("LargePanelIcons")))
	    current_config.select_icon_size = 2;
	  else
	    current_config.select_icon_size = 1;
	  p = nextline (configfile, lineread);
	  if (!my_strncasecmp
	      (p, "SmallMenuIcons", strlen ("SmallMenuIcons")))
	    current_config.popup_icon_size = 0;
	  else
	    if (!my_strncasecmp
		(p, "LargeMenuIcons", strlen ("LargeMenuIcons")))
	    current_config.popup_icon_size = 2;
	  else
	    current_config.popup_icon_size = 1;
	  update_popup_size ();
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[XColors]", strlen ("[XColors]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.apply_xcolors =
	    (my_strncasecmp (p, "Apply", strlen ("Apply")) == 0);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Diagnostic]", strlen ("[Diagnostic]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.show_diagnostic =
	    (my_strncasecmp (p, "Show", strlen ("Show")) == 0);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Layer]", strlen ("[Layer]")))
	{
	  p = nextline (configfile, lineread);
	  current_config.panel_layer = atoi (p);
	  p = nextline (configfile, lineread);
	}
      if (!my_strncasecmp (p, "[Commands]", strlen ("[Commands]")))
	{
	  p = nextline (configfile, lineread);
          if (p)
            {
	      for (i = 0; i < NBSELECTS; i++)
	        {
	          if (newconf == 1)
		    set_command (i, get_first_ok (p));
	          else
		    set_command (i, p);
	          p = nextline (configfile, lineread);
	        }
            }
	}
      i = 0;
      sprintf (dummy, "[Menu%u]", i + 1);
      while ((p) && (!my_strncasecmp (p, dummy, strlen (dummy)))
	     && (i++ < NBPOPUPS))
	{
	  sprintf (dummy, "[Menu%u]", i + 1);
	  p = nextline (configfile, lineread);
	  j = 0;
	  while ((p) && (my_strncasecmp (p, dummy, strlen (dummy))))
	    {
	      strcpy (label, (p ? p : "None"));
	      p = nextline (configfile, lineread);
	      strcpy (pixfile, (p ? p : "Default icon"));
	      p = nextline (configfile, lineread);
	      if (newconf == 1)
		p = get_first_ok (p);
	      if (strcmp (p, "None"))
		{
		  strcpy (command, (p ? p : "None"));
		  if (j++ < NBMAXITEMS)
		    add_popup_entry (i - 1, label, pixfile, command);
		}
	      p = nextline (configfile, lineread);
	    }
	}
      fclose (configfile);
    }
}

void
update_config_screen_visible (int i)
{
  if (i <= NBSCREENS)
    current_config.visible_screen = i;
  else
    current_config.visible_screen = NBSCREENS;
  apply_wm_desk_names (current_config.visible_screen);
}
