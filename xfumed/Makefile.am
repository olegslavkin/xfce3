## Process this file with automake to produce Makefile.in

CLEANFILES=*~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ -I../ -I../libs/ \
-DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
-DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfumed 

xfumed_SOURCES = main.c xfumed.h \
				interface.c interface.h \
				callbacks.c callbacks.h \
				usermenu.c usermenu.h \
				clistmenu.c clistmenu.h

xfumed_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
@GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
${X_LIBS} ${X_PRE_LIBS} -lXpm

xfumed_DEPENDENCIES = ../libs/libxfcecore.la
