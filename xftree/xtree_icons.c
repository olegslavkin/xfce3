/*
 * xtree_icons.c
 *
 * Copyright (C)2002 GNU-GPL
 * 
 * Edscott Wilson Garcia  for xfce project
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "icons.h"
#include "xtree_icons.h"
#include "entry.h"
#include "xpmext.h"

#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif


/* pixmap list */
typedef struct pixmap_list {
GdkPixmap  **pixmap;
GdkBitmap  **pixmask;
char **xpm;
} pixmap_list;

typedef struct gen_pixmap_list {
GdkPixmap  **pixmap;
GdkPixmap  **pixmask;
char **xpm;
char *c;
int kolor;
} gen_pixmap_list;

enum
{
  PIX_DIR_OPEN=0, PIX_DIR_OPEN_LNK,
  PIX_DIR_CLOSE,PIX_DIR_CLOSE_LNK,PIX_DIR_UP,
  PIX_DIR_PD,
  PIX_PAGE,PIX_PAGE_C,PIX_PAGE_F,PIX_PAGE_O,
  	PIX_PAGE_H,PIX_PAGE_LNK,PIX_CORE,PIX_TAR,
	PIX_COMPRESSED,PIX_IMAGE,PIX_TEXT,PIX_MAIL,
	PIX_BAK,PIX_DUP,PIX_TAR_TABLE,PIX_TAR_EXP,
	PIX_TAR_TABLE_R,PIX_TAR_EXP_R,PIX_PS,
	PIX_ADOBE,PIX_PO,PIX_WORD,
  PIX_PAGE_AUDIO,
  PIX_PACKAGE,
  PIX_LINKFLAG, 
  PIX_PAGE_HTML, 
  PIX_CHAR_DEV,
  PIX_FIFO,
  PIX_SOCKET,
  PIX_BLOCK_DEV,
  PIX_STALE_LNK,
  PIX_EXE,PIX_EXE_SCRIPT,PIX_EXE_LINK,
  LAST_PIX
};
/* don't repeat masks that already exist */
enum
{
  PIM_DIR_OPEN=0,
  PIM_DIR_CLOSE,
  PIM_PACKAGE,
  PIM_LINKFLAG, 
  PIM_DIR_PD,
  PIM_PAGE,
  PIM_PAGE_HTML, 
  PIM_CHAR_DEV,
  PIM_FIFO,
  PIM_SOCKET,
  PIM_BLOCK_DEV,
  PIM_STALE_LNK,
  PIM_EXE,
  LAST_PIM
};

static GdkPixmap *gPIX[LAST_PIX];
static GdkPixmap *gPIM[LAST_PIM];

/* masks that are duplicated elsewhere are initialized to NULL */
static pixmap_list pixmaps[]={
	{gPIX+PIX_PAGE,		gPIM+PIM_PAGE,		page_xpm},
	{gPIX+PIX_PS,		NULL,			page_ps_xpm},
	{gPIX+PIX_ADOBE,	NULL,			page_adobe_xpm},
	{gPIX+PIX_PACKAGE,	gPIM+PIM_PACKAGE,	package_green_xpm},
	{gPIX+PIX_LINKFLAG,	gPIM+PIM_LINKFLAG,	link_flag_xpm},
	{gPIX+PIX_PAGE_AUDIO,	NULL,			page_audio_xpm},
	{gPIX+PIX_TEXT,		NULL,			page_text_xpm},
	{gPIX+PIX_COMPRESSED,	NULL,			page_compressed_xpm},
	{gPIX+PIX_IMAGE,	NULL,			page_image_xpm},
	{gPIX+PIX_TAR,		NULL,			page_tar_xpm},
	{gPIX+PIX_PAGE_LNK,	NULL,			page_link_xpm},
	{gPIX+PIX_PO,		NULL,			page_po_xpm},
	{gPIX+PIX_BAK,		NULL,			page_backup_xpm},
	{gPIX+PIX_DIR_PD,	NULL,			dir_pd_xpm},
	{gPIX+PIX_DIR_OPEN,	gPIM+PIM_DIR_OPEN,	dir_open_xpm},
	{gPIX+PIX_DIR_OPEN_LNK,	NULL,			dir_open_lnk_xpm},
	{gPIX+PIX_DIR_CLOSE,	gPIM+PIM_DIR_CLOSE,	dir_close_xpm},
	{gPIX+PIX_DIR_CLOSE_LNK,NULL,			dir_close_lnk_xpm},
	{gPIX+PIX_DIR_UP,	NULL,			dir_up_xpm},
	{gPIX+PIX_EXE,		gPIM+PIM_EXE,		page_exe_xpm},
	{gPIX+PIX_EXE_LINK,	NULL,			page_exe_link_xpm},
	{gPIX+PIX_EXE_SCRIPT,	NULL,			page_exe_script_xpm},
	{gPIX+PIX_CORE,		NULL,			page_core_xpm},
	{gPIX+PIX_CHAR_DEV,	gPIM+PIM_CHAR_DEV,	char_dev_xpm},
	{gPIX+PIX_BLOCK_DEV,	gPIM+PIM_BLOCK_DEV,	block_dev_xpm},
	{gPIX+PIX_FIFO,		gPIM+PIM_FIFO,		fifo_xpm},
	{gPIX+PIX_SOCKET,	gPIM+PIM_SOCKET,	socket_xpm},
	{gPIX+PIX_STALE_LNK,	gPIM+PIM_STALE_LNK,	stale_lnk_xpm},
	{gPIX+PIX_PAGE_HTML,	gPIM+PIM_PAGE_HTML,	page_html_xpm},
	{NULL,NULL,NULL}
};

static gen_pixmap_list gen_pixmaps[]={
	{gPIX+PIX_PAGE_C,	NULL,	page_xpm,	"c",	0},
	{gPIX+PIX_PAGE_H,	NULL,	page_xpm,	"h",	1},
	{gPIX+PIX_PAGE_F,	NULL,	page_xpm,	"f",	0},
	{gPIX+PIX_PAGE_O,	NULL,	page_xpm,	"o",	3},
	{gPIX+PIX_MAIL,		NULL,	page_xpm,	"@",	4},
	{gPIX+PIX_WORD,		NULL,	page_xpm,	"W",	1},
	{gPIX+PIX_DUP,		NULL,	page_xpm,	"*",	3},
	{gPIX+PIX_TAR_TABLE,	NULL,	dir_close_xpm,	".",	0},
	{gPIX+PIX_TAR_EXP,	NULL,	dir_open_xpm,	".",	0},
	{gPIX+PIX_TAR_TABLE_R,	NULL,	dir_close_xpm,	".",	2},
	{gPIX+PIX_TAR_EXP_R,	NULL,	dir_open_xpm,	".",	2},
	{NULL,NULL,NULL,0}
};



static gboolean checkif_type(char **Type,char *loc){
  int i;
  for (i=0;Type[i]!=NULL;i++) 
	  if (strcmp(loc,Type[i])==0) return TRUE;
  return FALSE;
}

static gboolean image_type(char *loc){
  char *Type[]={
	  ".jpg",".JPG",".gif",".GIF",".png",".PNG",
	  ".JPEG",".jpeg",".TIFF",".tiff",".xbm","XBM",
	  ".XPM",".xpm",".XCF",".xcf",".PCX",".pcx",
	  ".BMP",".bmp",
	  NULL
  };
  return checkif_type(Type,loc);			  
}
static gboolean text_type(char *loc){
  char *Type[]={
	  ".txt",".TXT",".tex",".TEX",
	  ".doc",".DOC",
	  ".readme",".README",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean word_type(char *loc){
  char *Type[]={
	  ".doc",".DOC",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean compressed_type(char *loc){
  char *Type[]={
	  ".gz",".tgz",".bz2",".Z",
	  ".zip",
	  ".ZIP",
	  ".arj",".ARJ",
	  ".lha",".LHA",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean www_type(char *loc){
  char *Type[]={
	  ".html",".htm",".HTM",".HTML",
	  ".sgml",".SGML",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean audio_type(char *loc){
  char *Type[]={
	  ".wav",".mp3",".mid",".midi",
	  ".kar",".mpga",".mp2",
	  ".ra",".aif",".aiff",".ram",
	  ".rm",".au",".snd",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean script_type(char *loc){
  char *Type[]={
	  ".pl",".sh",".csh",".py",".tsh",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean mail_type(char *loc){
  char *Type[]={
	  "inbox","outbox","mbox","dead.letter",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean bak_type(char *loc){
  char *Type[]={
	  ".bak",".BAK",".old",".rpmsave",".rpmnew",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean dup_type(char *loc){
  char *l;
  for (l=loc+1;*l!=0;l++){
	  if ((*l > '9')||(*l < '0')) return FALSE;
  }
  return TRUE;			    
}
static gboolean adobe_type(char *loc){
  char *Type[]={
	  ".pdf",".PDF",
	  NULL
  };
  return checkif_type(Type,loc);			  
}
static gboolean ps_type(char *loc){
  char *Type[]={
	  ".ps",".PS",
	  ".dvi",
	  NULL
  };
  return checkif_type(Type,loc);			  
}
static gboolean packed_type(char *loc){
  char *Type[]={
	  ".deb",".rpm",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

gboolean set_icon_pix(icon_pix *pix,int type,char *label) {
  char *loc;
  gboolean isleaf=TRUE;
  
  pix->pixmap=gPIX[PIX_PAGE];
  pix->pixmask=gPIM[PIM_PAGE];
  pix->open=pix->openmask=NULL;
  /* default link icon */
  if (type & FT_DIR){
      if (type & FT_TARCHILD) {
	if ((type & FT_GZ)||(type & FT_COMPRESS )||(type & FT_BZ2)){
	        pix->pixmap=gPIX[PIX_TAR_TABLE_R];
	        pix->pixmask=gPIM[PIM_DIR_CLOSE];
	        pix->open=gPIX[PIX_TAR_EXP_R];
	        pix->openmask=gPIM[PIM_DIR_OPEN];
	} else {
	        pix->pixmap=gPIX[PIX_TAR_TABLE];
	        pix->pixmask=gPIM[PIM_DIR_CLOSE];
	        pix->open=gPIX[PIX_TAR_EXP];
	        pix->openmask=gPIM[PIM_DIR_OPEN];
	}	
      } else {
        if (type & FT_LINK) {
		pix->pixmap=gPIX[PIX_DIR_CLOSE_LNK];
	 	pix->open=gPIX[PIX_DIR_OPEN_LNK]; 
        } else {
	 	pix->pixmap=gPIX[PIX_DIR_CLOSE];
	        pix->open=gPIX[PIX_DIR_OPEN];
       }
       pix->openmask=gPIM[PIM_DIR_OPEN];
       pix->pixmask=gPIM[PIM_DIR_CLOSE];
      }
      return FALSE; /* not leaf */  
  }
  if (type & FT_LINK){
	 pix->pixmap=gPIX[PIX_LINKFLAG];
         pix->pixmask=gPIM[PIM_LINKFLAG];    
  }
  if (type & FT_EXE) {
    if (type & FT_LINK) pix->pixmap=gPIX[PIX_EXE_LINK]; 
    else pix->pixmap=gPIX[PIX_EXE];
    if ( (loc=strrchr(label,'.')) != NULL ){
    	if (script_type(loc)) pix->pixmap=gPIX[PIX_EXE_SCRIPT];
    }
    pix->pixmask=gPIM[PIM_EXE];    
  }
  else if (type & FT_FILE)/* letter modified here */
  {
    pix->pixmask=gPIM[PIM_PAGE];
    if (type & FT_LINK) {
	    pix->pixmap=gPIX[PIX_PAGE_LNK]; 
    }
    else {
      pix->pixmap=gPIX[PIX_PAGE]; /* default */
      if (strcmp(label,"core")==0) pix->pixmap=gPIX[PIX_CORE];
      else if (mail_type(label))	pix->pixmap=gPIX[PIX_MAIL];	      
      else if ( (loc=strrchr(label,'-')) != NULL ){
         if (dup_type(loc)) pix->pixmap=gPIX[PIX_DUP];	      
      }
      if ( (loc=strrchr(label,'.')) != NULL ){
	      if (strlen(loc)==2) switch (loc[1]){
		      case 'c': pix->pixmap=gPIX[PIX_PAGE_C]; break;
		      case 'h': pix->pixmap=gPIX[PIX_PAGE_H]; break;
		      case 'f': pix->pixmap=gPIX[PIX_PAGE_F]; break;
		      case 'o': pix->pixmap=gPIX[PIX_PAGE_O]; break;
		      default: break;				      
	      }
	      else if (bak_type(loc)) 		pix->pixmap=gPIX[PIX_BAK];	      
	      else if (image_type(loc)) 	pix->pixmap=gPIX[PIX_IMAGE];
	      else if (text_type(loc))  	pix->pixmap=gPIX[PIX_TEXT];
	      else if (ps_type(loc))  		pix->pixmap=gPIX[PIX_PS];
	      else if (adobe_type(loc))  	pix->pixmap=gPIX[PIX_ADOBE];
	      else if (packed_type(loc)){
	      	      pix->pixmap=gPIX[PIX_PACKAGE];
	              pix->pixmask=gPIM[PIM_PACKAGE];
	      }
	      else if (compressed_type(loc)) {
	      	     pix->pixmap=gPIX[PIX_COMPRESSED];
  		     pix->open=gPIX[PIX_COMPRESSED];
		     pix->openmask=gPIM[PIM_PAGE];
	      }
	      else if (www_type(loc)) {
		           pix->pixmap=gPIX[PIX_PAGE_HTML];
	                   pix->pixmask=gPIM[PIM_PAGE_HTML];
	      }
 	      else if (audio_type(loc)) {
		           pix->pixmap=gPIX[PIX_PAGE_AUDIO];
	      }
   	      else if (strcmp(loc,".po")==0) pix->pixmap=gPIX[PIX_PO];
   	      else if (strcmp(loc,".tar")==0){
		     pix->pixmap=gPIX[PIX_TAR];
  		     pix->open=gPIX[PIX_TAR];
		     pix->openmask=gPIM[PIM_PAGE];
	      }
      }
    }  
  }
  else if (type & FT_DIR_UP) {pix->pixmap=gPIX[PIX_DIR_UP],pix->pixmask=gPIM[PIM_DIR_CLOSE];}
  else if (type & FT_DIR_PD) {
	  isleaf=FALSE;
	  pix->pixmap=gPIX[PIX_DIR_PD],pix->pixmask=gPIM[PIM_DIR_CLOSE];
	  pix->open=gPIX[PIX_DIR_OPEN],pix->openmask=gPIM[PIM_DIR_OPEN];
  }
  else if (type & FT_CHAR_DEV){pix->pixmap=gPIX[PIX_CHAR_DEV],pix->pixmask=gPIM[PIM_CHAR_DEV];}
  else if (type & FT_BLOCK_DEV){pix->pixmap=gPIX[PIX_BLOCK_DEV],pix->pixmask=gPIM[PIM_BLOCK_DEV];}
  else if (type & FT_FIFO){pix->pixmap=gPIX[PIX_FIFO],pix->pixmask=gPIM[PIM_FIFO];}
  else if (type & FT_SOCKET){pix->pixmap=gPIX[PIX_SOCKET],pix->pixmask=gPIM[PIM_SOCKET];}
  else if (type & FT_STALE_LINK){pix->pixmap=gPIX[PIX_STALE_LNK],pix->pixmask=gPIM[PIM_STALE_LNK];}

  return isleaf;
}


static void scale_pixmap(GtkWidget *hack,int h,GtkWidget *ctree,char **xpm,
		GdkPixmap **pixmap, GdkBitmap **pixmask){
#ifdef HAVE_GDK_PIXBUF
	if (h>0){
		GdkPixbuf *orig_pixbuf,*new_pixbuf;
		float r=0;
		int w,x,y;
	  	orig_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)(xpm));
		w=gdk_pixbuf_get_width (orig_pixbuf);
		if (w) r=(float)h/w; if (r<1.0) r=1.0;
		x=r*w;
		y=r*gdk_pixbuf_get_height (orig_pixbuf);

  		new_pixbuf  = gdk_pixbuf_scale_simple (orig_pixbuf,x,y,GDK_INTERP_NEAREST);
  		gdk_pixbuf_render_pixmap_and_mask (new_pixbuf,pixmap,pixmask,
				gdk_pixbuf_get_has_alpha (new_pixbuf));
  		gdk_pixbuf_unref (orig_pixbuf);
  		gdk_pixbuf_unref (new_pixbuf);
 	        gtk_clist_set_row_height ((GtkCList *)ctree,h);
	} else
#endif	
	{
	      	*pixmap = MyCreateGdkPixmapFromData(xpm,hack,pixmask,FALSE);
                gtk_clist_set_row_height ((GtkCList *)ctree,16);
	}


	return ;

}

void create_pixmaps(int h,GtkWidget *ctree){
  GtkStyle  *style;
  GdkColormap *colormap;
  GdkGC *gc; 

  int i;
  static GtkWidget *hack=NULL; 
  /* hack: to be able to use icons globally, independent of xftree window.*/
  if (!hack) {hack = gtk_window_new (GTK_WINDOW_POPUP); gtk_widget_realize (hack);}
  
#ifndef HAVE_GDK_PIXBUF
  else return; /* don't recreate pixmaps without gdk-pixbuf  */
#endif
  	
  for (i=0;pixmaps[i].pixmap != NULL; i++){ 
	  if (*(pixmaps[i].pixmap) != NULL) gdk_pixmap_unref(*(pixmaps[i].pixmap));
	  if ((pixmaps[i].pixmask)&&(*(pixmaps[i].pixmask) != NULL)) gdk_bitmap_unref(*(pixmaps[i].pixmask));
	  scale_pixmap(hack,h,ctree,pixmaps[i].xpm,pixmaps[i].pixmap,pixmaps[i].pixmask);
  }
 
  style=gtk_widget_get_style (ctree);
  for (i=0;gen_pixmaps[i].pixmap != NULL; i++){
	int x,y;
	GdkColor back;
	GdkColor kolor[5];
	gint lbearing, rbearing, width, ascent, descent;

	kolor[0].pixel=0, kolor[0].red= kolor[0].green=0;kolor[0].blue =65535;
	kolor[1].pixel=1, kolor[1].red= kolor[1].blue=0; kolor[1].green=40000;
	kolor[2].pixel=2, kolor[2].blue=kolor[2].green=0;kolor[2].red  =65535;
	kolor[3].pixel=3, kolor[3].red= kolor[3].green=  kolor[3].blue =42000;
	kolor[4].pixel=4, kolor[4].red= kolor[4].green=  kolor[4].blue =0;
	
	colormap = gdk_colormap_get_system();
	gdk_colormap_alloc_color (colormap,kolor+gen_pixmaps[i].kolor,FALSE,TRUE);  
	  	  
        if (!gdk_color_white (colormap,&back)) fprintf(stderr,"DBG: no white\n");
	gc = gdk_gc_new (hack->window);
	gdk_gc_set_foreground (gc,kolor+gen_pixmaps[i].kolor);
	gdk_gc_set_background (gc,&back);
  	

	
	if (*(gen_pixmaps[i].pixmap) != NULL) gdk_pixmap_unref(*(gen_pixmaps[i].pixmap));
	scale_pixmap(hack,h,ctree,gen_pixmaps[i].xpm,gen_pixmaps[i].pixmap,gen_pixmaps[i].pixmask);
        gdk_string_extents (style->font,gen_pixmaps[i].c,
			&lbearing,&rbearing,&width,&ascent,&descent);
#if 0	
  	fprintf(stderr,"dbg: drawing %s...lbearing=%d,rbearing=%d,width=%d,ascent=%d,descent=%d,measure=%d,width=%d,height=%d\n",
			gen_pixmaps[i].c,lbearing,rbearing,width,ascent,descent,
			gdk_char_measure(style->font,gen_pixmaps[i].c[0]),
			gdk_char_width(style->font,gen_pixmaps[i].c[0]),
			gdk_char_height (style->font,gen_pixmaps[i].c[0]));
#endif	


	/* numbers for page_xpm */
        x=h/4-lbearing+1;
        y=13*h/16-descent-1;
	gdk_draw_text ((GdkDrawable *)(*(gen_pixmaps[i].pixmap)),style->font,gc,
				x,y,gen_pixmaps[i].c,strlen(gen_pixmaps[i].c));
	gdk_gc_destroy (gc);
	  
  }
   
  return;
}

void init_pixmaps(void){
	int i;
  for (i=0;i<LAST_PIX;i++) gPIX[i]=NULL;
  for (i=0;i<LAST_PIM;i++) gPIM[i]=NULL;
}

