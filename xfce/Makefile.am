## Process this file with automake to produce Makefile.in

CLEANFILES=*~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ -I../ -I../libs/ \
-DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
-DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfce

xfce_SOURCES = \
	action.c action.h \
	action_cb.c action_cb.h \
	configfile.c configfile.h \
	diagnostic.c diagnostic.h \
	info.c info.h \
	modify.c modify.h \
	modify_cb.c modify_cb.h \
	move.c move.h \
	popup.c popup.h \
	popup_cb.c popup_cb.h \
	screen.c screen.h \
	screen_cb.c screen_cb.h \
	selects.c selects.h \
	session.c session.h \
	setup.c setup.h \
	setup_cb.c setup_cb.h \
	startup.c startup.h \
	xfce.c xfce.h \
	xfce_cb.c xfce_cb.h \
	xfce_main.c xfce_main.h \
	xfwm.c xfwm.h

xfce_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ @GDK_IMLIB_LIBS@ \
@GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfce_DEPENDENCIES = ../libs/libxfcecore.la

man_MANS=xfce.1
