/*  gxfce
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


/****************************************************************************
 * This module is from original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


#ifndef __THEMES_H__
#define __THEMES_H__

#include <X11/Xlib.h>

#include "xfwm.h"
#include "screen.h"

#define GetButtonState(window) (onoroff ? Active : Inactive)

/* macro to change window background color/pixmap */
#define ChangeWindowColor(window,valuemask) {				\
        if(NewColor)							\
        {								\
          XChangeWindowAttributes(dpy,window,valuemask, &attributes);	\
          XClearWindow(dpy,window);					\
        }								\
      }

#define NewFontAndColor(newfont,color,backcolor) {\
   Globalgcv.font = newfont;\
   Globalgcv.foreground = color;\
   Globalgcv.background = backcolor;\
   Globalgcm = (newfont ? GCFont : 0) | GCForeground | GCBackground; \
   XChangeGC(dpy,Scr.ScratchGC3,Globalgcm,&Globalgcv); \
}


void RelieveRoundedRectangle (Window, int, int, int, int, GC, GC);
void DrawLinePattern (Window, GC, GC, struct vector_coords *, int, int);
void RelieveRectangle (Window, int, int, int, int, GC, GC);
void DrawUnderline (Window, GC, int, int, char *, int);
void DrawSeparator (Window, GC, GC, int, int, int, int, int);
void DrawIconWindow (XfwmWindow *);
void RedoIconName (XfwmWindow *);
void DrawIconWindow (XfwmWindow *);
void PaintEntry (MenuRoot *, MenuItem *);

void SetInnerBorder (XfwmWindow *, Bool);
void DrawButton (XfwmWindow *, Window, int, int, ButtonFace *, GC, GC, Bool, int);
void SetTitleBar (XfwmWindow *, Bool);
void RelieveWindow (XfwmWindow *, Window, int, int, int, int, GC, GC, int);
void RelieveIconTitle (Window, int, int, GC, GC);
void RelieveIconPixmap (Window, int, int, GC, GC);
void RelieveHalfRectangle (Window, int, int, int, int, GC, GC, int);
void DrawSelectedEntry (Window, int, int, int, int, GC *);
void DrawTopMenu (Window, int, GC, GC);
void DrawBottomMenu (Window, int, int, int, int, GC, GC);
void DrawTrianglePattern (Window, GC, GC, GC, int, int, int, int, short);

void SetInnerBorder_xfce (XfwmWindow *, Bool);
void DrawButton_xfce (XfwmWindow *, Window, int, int, ButtonFace *, GC, GC, Bool, int);
void SetTitleBar_xfce (XfwmWindow *, Bool);
void RelieveWindow_xfce (XfwmWindow *, Window, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_xfce (Window, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_xfce (Window, int, int, int, int, GC *);
void DrawTopMenu_xfce (Window, int, GC, GC);
void DrawBottomMenu_xfce (Window, int, int, int, int, GC, GC);
void DrawTrianglePattern_xfce (Window, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_xfce (Window, int, int, GC, GC);
void RelieveIconPixmap_xfce (Window, int, int, GC, GC);

void SetInnerBorder_mofit (XfwmWindow *, Bool);
void DrawButton_mofit (XfwmWindow *, Window, int, int, ButtonFace *, GC, GC, Bool, int);
void SetTitleBar_mofit (XfwmWindow *, Bool);
void RelieveWindow_mofit (XfwmWindow *, Window, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_mofit (Window, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_mofit (Window, int, int, int, int, GC *);
void DrawTopMenu_mofit (Window, int, GC, GC);
void DrawBottomMenu_mofit (Window, int, int, int, int, GC, GC);
void DrawTrianglePattern_mofit (Window, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_mofit (Window, int, int, GC, GC);
void RelieveIconPixmap_mofit (Window, int, int, GC, GC);

void SetInnerBorder_trench (XfwmWindow *, Bool);
void DrawButton_trench (XfwmWindow *, Window, int, int, ButtonFace *, GC, GC, Bool, int);
void DrawStripes_trench (XfwmWindow *, Window, int, int, int, int, Bool);
void SetTitleBar_trench (XfwmWindow *, Bool);
void RelieveWindow_trench (XfwmWindow *, Window, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_trench (Window, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_trench (Window, int, int, int, int, GC *);
void DrawTopMenu_trench (Window, int, GC, GC);
void DrawBottomMenu_trench (Window, int, int, int, int, GC, GC);
void DrawTrianglePattern_trench (Window, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_trench (Window, int, int, GC, GC);
void RelieveIconPixmap_trench (Window, int, int, GC, GC);

void SetInnerBorder_gtk (XfwmWindow *, Bool);
void DrawButton_gtk (XfwmWindow *, Window, int, int, ButtonFace *, GC, GC, Bool, int);
void DrawStripes_gtk (XfwmWindow *, Window, int, int, int, int, Bool);
void SetTitleBar_gtk (XfwmWindow *, Bool);
void RelieveWindow_gtk (XfwmWindow *, Window, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_gtk (Window, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_gtk (Window, int, int, int, int, GC *);
void DrawTopMenu_gtk (Window, int, GC, GC);
void DrawBottomMenu_gtk (Window, int, int, int, int, GC, GC);
void DrawTrianglePattern_gtk (Window, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_gtk (Window, int, int, GC, GC);
void RelieveIconPixmap_gtk (Window, int, int, GC, GC);

#endif
