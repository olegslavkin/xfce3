#define SAVE_GEOMETRY 		0x01
#define DOUBLE_CLICK_GOTO 	0x02
#define SHORT_TITLES	 	0x04
#define DRAG_DOES_COPY		0x08
#define CUSTOM_COLORS		0x10
#define LARGE_TOOLBAR		0x20
#define HIDE_TOOLBAR		0x40
#define CUSTOM_FONT		0x80
#define HIDE_SIZE		0x100
#define HIDE_DATE		0x200
#define HIDE_MENU		0x400
#define HIDE_TITLES		0x800
#define SUBSORT_BY_FILETYPE	0x1000
#define FILTER_OPTION		0x2000
#define ABREVIATE_PATHS		0x4000

#define XFTREE_CONFIG_FILE "xftreerc"

#ifdef XTREE_MESS_MAIN  
unsigned int preferences,stateTB[2];
int geometryX,geometryY;
GtkWidget *Awin;
char *Apath;
GdkColor ctree_color;
#else /* XTREE_MESS_MAIN */
extern unsigned int preferences,stateTB[2];
extern int geometryX,geometryY;
extern GtkWidget *Awin;
extern char *Apath;
extern GdkColor ctree_color;
#endif /* XTREE_MESS_MAIN */

#ifndef __XTREE_MESS_H__
#define __XTREE_MESS_H__
#include <sys/types.h>
#include <dirent.h>
#include <glob.h>
typedef struct xf_dirent {
	DIR *dir;
	char *globstring;
	glob_t dirlist;
	int glob_count;
}xf_dirent;


void read_defaults(void);
void save_defaults(GtkWidget *parent);
char * override_txt(char *new_file,char *old_file);
GtkWidget *
 shortcut_menu (GtkWidget * parent, char *txt, gpointer func, gpointer data);
void 
 cb_toggle_preferences (GtkWidget * widget, gpointer data);
void cb_custom_SCK(GtkWidget * item, GtkWidget * ctree);
void cb_default_SCK(GtkWidget * item, GtkWidget * ctree );
void show_cat (char *message);
void cb_select_colors (GtkWidget * widget, GtkWidget * ctree);
void cb_select_font (GtkWidget * widget, GtkWidget * ctree);
void cb_hide_date (GtkWidget * widget, GtkWidget * ctree);
void cb_hide_size (GtkWidget * widget, GtkWidget * ctree);
void set_colors(GtkWidget * ctree);
int set_fontT(GtkWidget * ctree);
void cb_dnd_help(GtkWidget * item, GtkWidget * ctree);
void cb_hide_menu (GtkWidget * widget, GtkWidget *ctree);
void cb_hide_titles (GtkWidget * widget, GtkWidget *ctree);
void cb_subsort(GtkWidget * widget, GtkWidget *ctree);
void cb_filter(GtkWidget * widget, GtkWidget *ctree);
void cb_abreviate(GtkWidget * widget, GtkWidget *ctree);
xf_dirent *xf_opendir(char *path,GtkWidget *ctree);
xf_dirent *xf_closedir(xf_dirent *diren);
char *xf_readdir(xf_dirent *diren);
char *abreviate(char *path);

#endif
