## Process this file with automake to produce Makefile.in

PACKAGE = @PACKAGE@
VERSION = @VERSION@

CLEANFILES=*~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = -I../ -I../libs/ @X_CFLAGS@ @IMLIB_CFLAGS@ \
-DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
-DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfwm

xfwm_SOURCES = \
	default_icon.h \
	menus.h \
	misc.h \
	parse.h \
	screen.h \
	xfwm.h \
	add_window.c \
	bindings.c \
	borders.c \
	builtins.c \
	colormaps.c \
	colors.c \
	complex.c \
	decorations.c \
	events.c \
	focus.c \
	functions.c \
	icons.c \
	menus.c \
	misc.c \
	module.c \
	move.c \
	placement.c \
	read.c \
	resize.c \
	session.c \
        session.h \
	stack.c \
        stack.h \
	style.c \
	themes.c \
        themes.h \
	virtual.c \
	windows.c \
	xfwm.c \
	xinerama.h \
	xinerama.c

xfwm_LDADD = ../libs/libxfwmcore.la @INTLLIBS@ @LIBS@ @IMLIB_LIBS@ \
@DMALLOC_LIB@ ${X_LIBS} ${X_PRE_LIBS} -lXext -lX11 -lXpm @XINERAMA_LIBS@ \
@XFT_LIBS@

xfwm_DEPENDENCIES = ../libs/libxfwmcore.la 

man_MANS=xfwm.1
