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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtkmain.h>
#include "mygtkclock.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define MY_CLOCK_DEFAULT_SIZE 100

#ifndef M_PI
#define M_PI 3.141592654
#endif

/* Forward declarations */

static void my_gtk_clock_class_init (MyGtkClockClass * klass);
static void my_gtk_clock_init (MyGtkClock * clock);
static void my_gtk_clock_destroy (GtkObject * object);

static void my_gtk_clock_realize (GtkWidget * widget);
static void my_gtk_clock_size_request (GtkWidget * widget,
				       GtkRequisition * requisition);
static void my_gtk_clock_size_allocate (GtkWidget * widget,
					GtkAllocation * allocation);
static gint my_gtk_clock_expose (GtkWidget * widget, GdkEventExpose * event);
static void my_gtk_clock_draw (GtkWidget * widget, GdkRectangle * area);
static gint my_gtk_clock_timer (MyGtkClock * clock);
static void my_gtk_clock_draw_internal (GtkWidget * widget);


/* Local data */

static GtkWidgetClass *parent_class = NULL;

guint my_gtk_clock_get_type ()
{
  static guint clock_type = 0;

  if (!clock_type)
    {
      GtkTypeInfo clock_info = {
	"MyGtkClock",
	sizeof (MyGtkClock),
	sizeof (MyGtkClockClass),
	(GtkClassInitFunc) my_gtk_clock_class_init,
	(GtkObjectInitFunc) my_gtk_clock_init,
	(gpointer) NULL,
	(gpointer) NULL,
      };

      clock_type = gtk_type_unique (gtk_widget_get_type (), &clock_info);
    }

  return clock_type;
}

static void
my_gtk_clock_class_init (MyGtkClockClass * class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  object_class->destroy = my_gtk_clock_destroy;

  widget_class->realize = my_gtk_clock_realize;
  widget_class->expose_event = my_gtk_clock_expose;
  widget_class->draw = my_gtk_clock_draw;
  widget_class->size_request = my_gtk_clock_size_request;
  widget_class->size_allocate = my_gtk_clock_size_allocate;
}




static void
my_gtk_clock_realize (GtkWidget * widget)
{
  MyGtkClock *clock;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  clock = MY_GTK_CLOCK (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window =
    gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->parent->style, widget->window,
			    GTK_STATE_NORMAL);

  if (!(clock->timer))
    clock->timer = gtk_timeout_add (clock->interval,
				    (GtkFunction) my_gtk_clock_timer,
				    (gpointer) clock);
}

static void
my_gtk_clock_init (MyGtkClock * clock)
{
  time_t ticks;
  struct tm *tm;
  gint h, m, s;

  ticks = time (0);
  tm = localtime (&ticks);
  h = tm->tm_hour;
  m = tm->tm_min;
  s = tm->tm_sec;
  clock->hrs_angle = 2.5 * M_PI - (h % 12) * M_PI / 6 - m * M_PI / 360;
  clock->min_angle = 2.5 * M_PI - m * M_PI / 30;
  clock->sec_angle = 2.5 * M_PI - s * M_PI / 30;

  clock->timer = 0;
  clock->radius = 0;
  clock->pointer_width = 0;
  clock->relief = TRUE;
  clock->shadow_type = GTK_SHADOW_IN;
  clock->parent_style = NULL;

  clock->mode = MY_GTK_CLOCK_ANALOG;
  clock->military_time = 0;	/* use 12 hour mode by default */
  clock->display_am_pm = 1;	/* display am or pm by default. */

  clock->interval = 100;	/* 1/10 seconds update interval by default */
}

GtkWidget *
my_gtk_clock_new (void)
{
  MyGtkClock *clock;

  clock = gtk_type_new (my_gtk_clock_get_type ());

  return GTK_WIDGET (clock);
}

static void
my_gtk_clock_destroy (GtkObject * object)
{
  MyGtkClock *clock;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (object));

  clock = MY_GTK_CLOCK (object);

  if (clock->timer)
    gtk_timeout_remove (clock->timer);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void
my_gtk_clock_show_ampm (MyGtkClock * clock, gboolean show)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  clock->display_am_pm = show;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

void
my_gtk_clock_ampm_toggle (MyGtkClock * clock)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  if (clock->display_am_pm)
    clock->display_am_pm = 0;
  else
    clock->display_am_pm = 1;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

gboolean my_gtk_clock_ampm_shown (MyGtkClock * clock)
{
  g_return_val_if_fail (clock != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MY_CLOCK (clock), FALSE);
  return (clock->display_am_pm);
}

void
my_gtk_clock_show_secs (MyGtkClock * clock, gboolean show)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));
  clock->display_secs = show;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

void
my_gtk_clock_secs_toggle (MyGtkClock * clock)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  if (clock->display_secs)
    clock->display_secs = 0;
  else
    clock->display_secs = 1;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

gboolean my_gtk_clock_secs_shown (MyGtkClock * clock)
{
  g_return_val_if_fail (clock != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MY_CLOCK (clock), FALSE);
  return (clock->display_secs);
}

void
my_gtk_clock_show_military (MyGtkClock * clock, gboolean show)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));
  clock->military_time = show;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

void
my_gtk_clock_military_toggle (MyGtkClock * clock)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  if (clock->military_time)
    clock->military_time = 0;
  else
    clock->military_time = 1;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

gboolean my_gtk_clock_military_shown (MyGtkClock * clock)
{
  g_return_val_if_fail (clock != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MY_CLOCK (clock), FALSE);
  return (clock->military_time);
}

void
my_gtk_clock_set_interval (MyGtkClock * clock, guint interval)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));
  clock->interval = interval;

  if (clock->timer)
    {
      gtk_timeout_remove (clock->timer);
      clock->timer = gtk_timeout_add (clock->interval,
				      (GtkFunction) my_gtk_clock_timer,
				      (gpointer) clock);
    }
}

guint my_gtk_clock_get_interval (MyGtkClock * clock)
{
  g_return_val_if_fail (clock != NULL, 0);
  g_return_val_if_fail (GTK_IS_MY_CLOCK (clock), 0);
  return (clock->interval);
}

void
my_gtk_clock_suspend (MyGtkClock * clock)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  if (clock->timer)
    {
      gtk_timeout_remove (clock->timer);
      clock->timer = 0;
    }
}

void
my_gtk_clock_resume (MyGtkClock * clock)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  if (clock->timer)
    return;
  if (!(clock->interval))
    return;

  clock->timer = gtk_timeout_add (clock->interval,
				  (GtkFunction) my_gtk_clock_timer,
				  (gpointer) clock);
}

void
my_gtk_clock_set_mode (MyGtkClock * clock, MyGtkClockMode mode)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));
  clock->mode = mode;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

void
my_gtk_clock_toggle_mode (MyGtkClock * clock)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));
  if (clock->mode == MY_GTK_CLOCK_ANALOG)
    clock->mode = MY_GTK_CLOCK_DIGITAL;
  else
    clock->mode = MY_GTK_CLOCK_ANALOG;
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw (GTK_WIDGET (clock), NULL);
}

MyGtkClockMode
my_gtk_clock_get_mode (MyGtkClock * clock)
{
  g_return_val_if_fail (clock != NULL, 0);
  g_return_val_if_fail (GTK_IS_MY_CLOCK (clock), 0);
  return (clock->mode);
}

static void
my_gtk_clock_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
  requisition->width = MY_CLOCK_DEFAULT_SIZE;
  requisition->height = MY_CLOCK_DEFAULT_SIZE;
}

static void
my_gtk_clock_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  MyGtkClock *clock;
  gint size;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  clock = MY_GTK_CLOCK (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {

      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

    }
  size = MIN (allocation->width, allocation->height);
  clock->radius = size * 0.45;
  clock->internal = size * 0.49;
  clock->pointer_width = MAX (clock->radius / 5, 3);
}

static void
draw_round_frame (GtkWidget * widget)
{
  MyGtkClock *clock;
  GtkStyle *style;
  gint x, y, w, h;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  style = gtk_widget_get_style (widget);

  w = h = 2 * clock->internal;
  x = widget->allocation.width / 2 - clock->internal;
  y = widget->allocation.height / 2 - clock->internal;

  gdk_draw_arc (widget->window, style->base_gc[GTK_STATE_NORMAL],
		TRUE, x, y, w, h, 0, 360 * 64);

  gdk_draw_arc (widget->window, style->black_gc,
		FALSE, x + 1, y + 1, w - 1, h - 1, 30 * 64, 180 * 64);
  gdk_draw_arc (widget->window, style->mid_gc[GTK_STATE_NORMAL],
		FALSE, x, y, w, h, 30 * 64, 180 * 64);

  gdk_draw_arc (widget->window, style->bg_gc[GTK_STATE_NORMAL],
		FALSE, x + 1, y + 1, w - 1, h - 1, 210 * 64, 180 * 64);
  gdk_draw_arc (widget->window, style->light_gc[GTK_STATE_NORMAL],
		FALSE, x, y, w, h, 210 * 64, 180 * 64);
}


static void
draw_ticks (GtkWidget * widget)
{
  MyGtkClock *clock;
  gint xc, yc;
  gint i;
  gdouble theta;
  gdouble s, c;
  GdkPoint points[5];

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  xc = widget->allocation.width / 2;
  yc = widget->allocation.height / 2;

  for (i = 0; i < 12; i++)
    {
      theta = (i * M_PI / 6.);
      s = sin (theta);
      c = cos (theta);

      points[0].x =
	xc + s * (clock->radius - clock->pointer_width / 2) -
	clock->pointer_width / 4;
      points[0].y =
	yc + c * (clock->radius - clock->pointer_width / 2) -
	clock->pointer_width / 4;
      points[1].x =
	xc + s * (clock->radius - clock->pointer_width / 2) -
	clock->pointer_width / 4;
      points[1].y =
	yc + c * (clock->radius - clock->pointer_width / 2) +
	clock->pointer_width / 4;
      points[2].x =
	xc + s * (clock->radius - clock->pointer_width / 2) +
	clock->pointer_width / 4;
      points[2].y =
	yc + c * (clock->radius - clock->pointer_width / 2) +
	clock->pointer_width / 4;
      points[3].x =
	xc + s * (clock->radius - clock->pointer_width / 2) +
	clock->pointer_width / 4;
      points[3].y =
	yc + c * (clock->radius - clock->pointer_width / 2) -
	clock->pointer_width / 4;
      points[4].x =
	xc + s * (clock->radius - clock->pointer_width / 2) -
	clock->pointer_width / 4;
      points[4].y =
	yc + c * (clock->radius - clock->pointer_width / 2) -
	clock->pointer_width / 4;

      if (clock->relief)
	gtk_draw_polygon (widget->style,
			  widget->window,
			  GTK_STATE_NORMAL, GTK_SHADOW_OUT, points, 5, TRUE);
      else
	gdk_draw_polygon (widget->window,
			  widget->style->text_gc[GTK_STATE_NORMAL],
			  TRUE, points, 5);
    }

}

static void
draw_sec_pointer (GtkWidget * widget)
{
  MyGtkClock *clock;
  GdkPoint points[5];
  gint xc, yc;
  gdouble s, c;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  xc = widget->allocation.width / 2;
  yc = widget->allocation.height / 2;

  s = sin (clock->sec_angle);
  c = cos (clock->sec_angle);


  points[0].x = xc + s * clock->pointer_width / 4;
  points[0].y = yc + c * clock->pointer_width / 4;
  points[1].x = xc + c * clock->radius;
  points[1].y = yc - s * clock->radius;
  points[2].x = xc - s * clock->pointer_width / 4;
  points[2].y = yc - c * clock->pointer_width / 4;
  points[3].x = xc - c * clock->pointer_width / 4;
  points[3].y = yc + s * clock->pointer_width / 4;
  points[4].x = xc + s * clock->pointer_width / 4;
  points[4].y = yc + c * clock->pointer_width / 4;

  if (clock->relief)
    gtk_draw_polygon (widget->style,
		      widget->window,
		      GTK_STATE_NORMAL, GTK_SHADOW_OUT, points, 5, TRUE);
  else
    gdk_draw_polygon (widget->window,
		      widget->style->text_gc[GTK_STATE_NORMAL],
		      TRUE, points, 5);

}

static void
draw_min_pointer (GtkWidget * widget)
{
  MyGtkClock *clock;
  GdkPoint points[5];
  gint xc, yc;
  gdouble s, c;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  xc = widget->allocation.width / 2;
  yc = widget->allocation.height / 2;

  s = sin (clock->min_angle);
  c = cos (clock->min_angle);


  points[0].x = xc + s * clock->pointer_width / 2;
  points[0].y = yc + c * clock->pointer_width / 2;
  points[1].x = xc + c * clock->radius;
  points[1].y = yc - s * clock->radius;
  points[2].x = xc - s * clock->pointer_width / 2;
  points[2].y = yc - c * clock->pointer_width / 2;
  points[3].x = xc - c * clock->pointer_width / 2;
  points[3].y = yc + s * clock->pointer_width / 2;
  points[4].x = xc + s * clock->pointer_width / 2;
  points[4].y = yc + c * clock->pointer_width / 2;

  if (clock->relief)
    gtk_draw_polygon (widget->style,
		      widget->window,
		      GTK_STATE_NORMAL, GTK_SHADOW_OUT, points, 5, TRUE);
  else
    gdk_draw_polygon (widget->window,
		      widget->style->text_gc[GTK_STATE_NORMAL],
		      TRUE, points, 5);

}

static void
draw_hrs_pointer (GtkWidget * widget)
{
  MyGtkClock *clock;
  GdkPoint points[5];
  gint xc, yc;
  gdouble s, c;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  xc = widget->allocation.width / 2;
  yc = widget->allocation.height / 2;

  s = sin (clock->hrs_angle);
  c = cos (clock->hrs_angle);


  points[0].x = xc + s * clock->pointer_width / 2;
  points[0].y = yc + c * clock->pointer_width / 2;
  points[1].x = xc + 3 * c * clock->radius / 5;
  points[1].y = yc - 3 * s * clock->radius / 5;
  points[2].x = xc - s * clock->pointer_width / 2;
  points[2].y = yc - c * clock->pointer_width / 2;
  points[3].x = xc - c * clock->pointer_width / 2;
  points[3].y = yc + s * clock->pointer_width / 2;
  points[4].x = xc + s * clock->pointer_width / 2;
  points[4].y = yc + c * clock->pointer_width / 2;


  if (clock->relief)
    gtk_draw_polygon (widget->style,
		      widget->window,
		      GTK_STATE_NORMAL, GTK_SHADOW_OUT, points, 5, TRUE);
  else
    gdk_draw_polygon (widget->window,
		      widget->style->text_gc[GTK_STATE_NORMAL],
		      TRUE, points, 5);
}


static void
my_gtk_clock_draw_digital (GtkWidget * widget)
{
  MyGtkClock *clock;
  time_t ticks;
  struct tm *tm;
  gint h, m, s;
  gint x, y;
  gchar ampm[3] = "AM";
  gchar time_buf[24];
  gint lbearing, rbearing, width, ascent, descent;	/* to measure out string. */

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  ticks = time (0);
  tm = localtime (&ticks);
  h = tm->tm_hour;
  m = tm->tm_min;
  s = tm->tm_sec;

  if (h >= 12)
    ampm[0] = 'P';

  if (!(clock->military_time))
    {
      if (h > 12)
	h -= 12;
    }

  if (clock->military_time)
    {
      if (clock->display_secs)
	sprintf (time_buf, "%d:%02d:%02d", h, m, s);
      else
	sprintf (time_buf, "%d:%02d", h, m);
    }
  else
    {
      if (clock->display_am_pm)
	{
	  if (clock->display_secs)
	    sprintf (time_buf, "%d:%02d:%02d %s", h, m, s, ampm);
	  else
	    sprintf (time_buf, "%d:%02d %s", h, m, ampm);
	}
      else
	{
	  if (clock->display_secs)
	    sprintf (time_buf, "%d:%02d:%02d", h, m, s);
	  else
	    sprintf (time_buf, "%d:%02d", h, m);
	}
    }

  gdk_string_extents (widget->style->font, time_buf, &lbearing, &rbearing,
		      &width, &ascent, &descent);

  /* Center in the widget */
  x = (widget->allocation.width - width) / 2;

  y = (widget->allocation.height + ascent - descent) / 2;

  gtk_draw_box (widget->style,
		widget->window,
		widget->state,
		clock->shadow_type,
		0, 0, widget->allocation.width, widget->allocation.height);
  gdk_draw_string (widget->window,
		   widget->style->font,
		   widget->style->text_gc[GTK_STATE_NORMAL], x, y, time_buf);
}

static void
my_gtk_clock_draw_analog (GtkWidget * widget)
{
  MyGtkClock *clock;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  draw_round_frame (widget);
  draw_ticks (widget);
  draw_hrs_pointer (widget);
  draw_min_pointer (widget);
  if (clock->display_secs)
    draw_sec_pointer (widget);
}

static void
my_gtk_clock_draw_internal (GtkWidget * widget)
{
  MyGtkClock *clock;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));

  clock = MY_GTK_CLOCK (widget);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      switch (clock->mode)
	{
	case MY_GTK_CLOCK_ANALOG:
	  my_gtk_clock_draw_analog (widget);
	  break;
	case MY_GTK_CLOCK_DIGITAL:
	default:
	  my_gtk_clock_draw_digital (widget);
	  break;
	}
    }
}

static gint
my_gtk_clock_expose (GtkWidget * widget, GdkEventExpose * event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MY_CLOCK (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (GTK_WIDGET_DRAWABLE (widget), FALSE);
  g_return_val_if_fail (!GTK_WIDGET_NO_WINDOW (widget), FALSE);

  if (event->count > 0)
    return FALSE;

  my_gtk_clock_draw (widget, NULL);

  return FALSE;
}

static void
my_gtk_clock_draw (GtkWidget * widget, GdkRectangle * area)
{
  MyGtkClock *clock;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (widget));
  g_return_if_fail (GTK_WIDGET_DRAWABLE (widget));
  g_return_if_fail (!GTK_WIDGET_NO_WINDOW (widget));
  g_return_if_fail (GTK_IS_WIDGET (widget->parent));
  g_return_if_fail (GTK_WIDGET_DRAWABLE (widget->parent));

  clock = MY_GTK_CLOCK (widget);
  if (clock->parent_style != widget->parent->style)
    {
      if (clock->parent_style)
	gtk_style_unref (clock->parent_style);
      clock->parent_style = widget->parent->style;
      gtk_style_ref (clock->parent_style);
    }
  if (clock->mode == MY_GTK_CLOCK_ANALOG)
    gtk_draw_box (clock->parent_style,
		  widget->window,
		  widget->parent->state,
		  GTK_SHADOW_NONE,
		  0, 0, widget->allocation.width, widget->allocation.height);
  my_gtk_clock_draw_internal (widget);
}

static gint
my_gtk_clock_timer (MyGtkClock * clock)
{
  time_t ticks;
  struct tm *tm;
  static guint oh = 0, om = 0, os = 0;
  gint h, m, s;
  static gboolean sem = FALSE;

  g_return_val_if_fail (clock != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MY_CLOCK (clock), FALSE);

  if (sem)
    return TRUE;
  sem = TRUE;
  ticks = time (0);
  tm = localtime (&ticks);
  h = tm->tm_hour;
  m = tm->tm_min;
  s = tm->tm_sec;
  if (!(((!(clock->display_secs)) || (s == os)) && (m == om) && (h == oh)))
    {
      os = s;
      om = m;
      oh = h;
      clock->hrs_angle = 2.5 * M_PI - (h % 12) * M_PI / 6 - m * M_PI / 360;
      clock->min_angle = 2.5 * M_PI - m * M_PI / 30;
      clock->sec_angle = 2.5 * M_PI - s * M_PI / 30;
      my_gtk_clock_draw_internal (GTK_WIDGET (clock));
    }

  sem = FALSE;
  return TRUE;
}

void
my_gtk_clock_set_relief (MyGtkClock * clock, gboolean relief)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  clock->relief = relief;

  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw_internal (GTK_WIDGET (clock));
}

void
my_gtk_clock_set_shadow_type (MyGtkClock * clock, GtkShadowType type)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (GTK_IS_MY_CLOCK (clock));

  clock->shadow_type = type;

  if (GTK_WIDGET_REALIZED (GTK_WIDGET (clock)))
    my_gtk_clock_draw_internal (GTK_WIDGET (clock));
}
