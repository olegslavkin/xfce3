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

#include <gtk/gtk.h>
#include <gmodule.h>

#define SCROLLBAR_WIDTH 15

static gint range_slider_width;
static gint range_min_slider_size;
static gint range_stepper_size;
static gint range_stepper_slider_spacing;

/* Theme functions to export */
void theme_init (GtkThemeEngine * engine);
void theme_exit (void);

/* Exported vtable from th_draw */

extern GtkStyleClass xfce_default_class;

/* internals */

static guint
theme_parse_rc_style (GScanner * scanner, GtkRcStyle * rc_style)
{
  guint token;

  token = g_scanner_peek_next_token (scanner);
  while (token != G_TOKEN_RIGHT_CURLY)
  {
    switch (token)
    {
    default:
      g_scanner_get_next_token (scanner);
      token = G_TOKEN_RIGHT_CURLY;
      break;
    }

    if (token != G_TOKEN_NONE)
      return token;

    token = g_scanner_peek_next_token (scanner);
  }

  g_scanner_get_next_token (scanner);

  rc_style->engine_data = NULL;

  return G_TOKEN_NONE;
}

static void
theme_merge_rc_style (GtkRcStyle * dest, GtkRcStyle * src)
{
}

static void
theme_rc_style_to_style (GtkStyle * style, GtkRcStyle * rc_style)
{
  style->klass = &xfce_default_class;
}

static void
theme_duplicate_style (GtkStyle * dest, GtkStyle * src)
{
}

static void
theme_realize_style (GtkStyle * style)
{
}

static void
theme_unrealize_style (GtkStyle * style)
{
}

static void
theme_destroy_rc_style (GtkRcStyle * rc_style)
{
}

static void
theme_destroy_style (GtkStyle * style)
{
}

void
theme_init (GtkThemeEngine * engine)
{
  GtkRangeClass *rangeclass;

  engine->parse_rc_style = theme_parse_rc_style;
  engine->merge_rc_style = theme_merge_rc_style;
  engine->rc_style_to_style = theme_rc_style_to_style;
  engine->duplicate_style = theme_duplicate_style;
  engine->realize_style = theme_realize_style;
  engine->unrealize_style = theme_unrealize_style;
  engine->destroy_rc_style = theme_destroy_rc_style;
  engine->destroy_style = theme_destroy_style;
  engine->set_background = NULL;

  /* Make scrollbars wider */
  rangeclass = (GtkRangeClass *) gtk_type_class (gtk_range_get_type ());
 
  /* Save old values */
  range_slider_width = rangeclass->slider_width;
  range_min_slider_size = rangeclass->min_slider_size;
  range_stepper_size = rangeclass->stepper_size;
  range_stepper_slider_spacing = rangeclass->stepper_slider_spacing;

  /* Set new values */
  rangeclass->slider_width = SCROLLBAR_WIDTH;
  rangeclass->min_slider_size = SCROLLBAR_WIDTH;
  rangeclass->stepper_size = SCROLLBAR_WIDTH;
  rangeclass->stepper_slider_spacing = 0;
}

void
theme_exit (void)
{
   GtkRangeClass *rangeclass;
   GtkScaleClass *scaleclass;

   rangeclass = (GtkRangeClass *)gtk_type_class(gtk_range_get_type());
   scaleclass = (GtkScaleClass *)gtk_type_class(gtk_scale_get_type());

   rangeclass->slider_width = range_slider_width;
   rangeclass->min_slider_size = range_min_slider_size;
   rangeclass->stepper_size = range_stepper_size;
   rangeclass->stepper_slider_spacing = range_stepper_slider_spacing;
}

/* The following function will be called by GTK+ when the module
 * is loaded and checks to see if we are compatible with the
 * version of GTK+ that loads us.
 */
G_MODULE_EXPORT const gchar *g_module_check_init (GModule * module);
const gchar *
g_module_check_init (GModule * module)
{
  return gtk_check_version (GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION - GTK_INTERFACE_AGE);
}
