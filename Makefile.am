## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.4

@SET_MAKE@

CLEANFILES=*~ *.bak core *.core *.o .depend .deps/* *.cache
USE_INCLUDED_LIBINTL = @USE_INCLUDED_LIBINTL@
USE_NLS = @USE_NLS@
USE_DT = @USE_DT@
IMLIB_LIBS = @IMLIB_LIBS@
GDK_IMLIB_LIBS = @GDK_IMLIB_LIBS@
GDK_PIXBUF_LIBS = @GDK_PIXBUF_LIBS@
DMALLOC_LIB = @DMALLOC_LIB@
GTK_ENGINE_DIR = @GTK_ENGINE_DIR@

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

SUBDIRS = intl libs xfce_theme xfce xfwm xftree xfpager xfmouse xfbd xfclock \
	xfsound xfgnome xfrun xfmenu xfdiff xfglob xfsamba xfumed scripts \
	other backdrops icons palettes fonts textures sounds docs po dt
