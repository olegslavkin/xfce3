/*
 * gtk_dlg.h
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

#ifndef __GTK_DLG_H__
#define __GTK_DLG_H__

#define DLG_OK			(1<<0)
#define DLG_CANCEL		(1<<1)
#define DLG_YES			(1<<2)
#define DLG_NO			(1<<3)
#define DLG_CONTINUE	(1<<4)
#define DLG_CLOSE		(1<<5)
#define DLG_ALL			(1<<6)
#define DLG_SKIP		(1<<7)
/* */
#define DLG_OK_CANCEL	(DLG_OK|DLG_CANCEL)
#define DLG_YES_NO		(DLG_YES|DLG_NO)
/* */
#define DLG_ENTRY_VIEW	(1<<8)
#define DLG_ENTRY_EDIT	(1<<9)
#define DLG_COMBO		(1<<10)
/* */
#define DLG_INFO		(1<<11)
#define DLG_WARN		(1<<12)
#define DLG_ERROR		(1<<13)
#define DLG_QUESTION	(1<<14)

#define DLG_RC_CANCEL	0
#define DLG_RC_OK		1
#define DLG_RC_ALL		2
#define DLG_RC_CONTINUE	3
#define DLG_RC_SKIP		4
#define DLG_RC_DESTROY	5

#define DLG_MAX		1024

/* true modal macros: (requiere toplevel widget address, p) */
#define xf_dlg_string(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_OK_CANCEL|DLG_ENTRY_EDIT|DLG_QUESTION)
#define xf_dlg_continue(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_CONTINUE|DLG_CANCEL)
#define xf_dlg_error(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_ERROR|DLG_CLOSE)
#define xf_dlg_error_continue(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_ERROR|DLG_CONTINUE|DLG_CANCEL)
#define xf_dlg_warning(p,a) \
			xf_dlg_new(p,a,NULL,NULL,DLG_WARN|DLG_CLOSE)
#define xf_dlg_info(p,a) \
			xf_dlg_new(p,a,NULL,NULL,DLG_INFO|DLG_CLOSE)
#define xf_dlg_ask(p,a) \
			xf_dlg_new(p,a,NULL,NULL,DLG_QUESTION|DLG_YES_NO)
#define xf_dlg_skip(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_QUESTION|DLG_SKIP|DLG_CANCEL)
#define xf_dlg_ok_skip(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_QUESTION|DLG_OK|DLG_SKIP|DLG_CANCEL)
#define xf_dlg_question(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_YES_NO|DLG_ENTRY_VIEW|DLG_QUESTION)
#define xf_dlg_question_l(p,a,b,c) \
			xf_dlg_new(p,a,b,NULL,c|DLG_OK_CANCEL|DLG_ENTRY_VIEW|DLG_QUESTION)
#define xf_dlg_combo(p,a,b,c) \
			xf_dlg_new(p,a,b,c,DLG_OK_CANCEL|DLG_COMBO)

int xf_dlg_new (GtkWidget *parent,char *label, char *defval, void *data, int flags);


/* deprecated calls (because they are not transient): */

#define dlg_string(a,b) \
			dlg_new(a,b,NULL,DLG_OK_CANCEL|DLG_ENTRY_EDIT|DLG_QUESTION)
#define dlg_continue(a,b) \
			dlg_new(a,b,NULL,DLG_CONTINUE|DLG_CANCEL)
#define dlg_error(a,b) \
			dlg_new(a,b,NULL,DLG_ERROR|DLG_CLOSE)
#define dlg_error_continue(a,b) \
			dlg_new(a,b,NULL,DLG_ERROR|DLG_CONTINUE|DLG_CANCEL)
#define dlg_warning(a) \
			dlg_new(a,NULL,NULL,DLG_WARN|DLG_CLOSE)
#define dlg_info(a) \
			dlg_new(a,NULL,NULL,DLG_INFO|DLG_CLOSE)
#define dlg_ask(a) \
			dlg_new(a,NULL,NULL,DLG_QUESTION|DLG_YES_NO)
#define dlg_skip(a,b) \
			dlg_new(a,b,NULL,DLG_QUESTION|DLG_SKIP|DLG_CANCEL)
#define dlg_ok_skip(a,b) \
			dlg_new(a,b,NULL,DLG_QUESTION|DLG_OK|DLG_SKIP|DLG_CANCEL)
#define dlg_question(a,b) \
			dlg_new(a,b,NULL,DLG_YES_NO|DLG_ENTRY_VIEW|DLG_QUESTION)
#define dlg_question_l(a,b,c) \
			dlg_new(a,b,NULL,c|DLG_OK_CANCEL|DLG_ENTRY_VIEW|DLG_QUESTION)
#define dlg_combo(a,b,c) \
			dlg_new(a,b,c,DLG_OK_CANCEL|DLG_COMBO)

/* deprecated: */
int dlg_new (char *label, char *defval, void *data, int flags);

#endif
