#define SAVE_GEOMETRY 		0x01
#define DOUBLE_CLICK_GOTO 	0x02
#define SHORT_TITLES	 	0x04

#define XFTREE_CONFIG_FILE "xftreerc"

#ifdef XTREE_MESS_MAIN  
int preferences,geometryX,geometryY;
GtkWidget *Awin;
char *Apath;
#else /* XTREE_MESS_MAIN */
extern int preferences,geometryX,geometryY;
extern GtkWidget *Awin;
extern char *Apath;
#endif /* XTREE_MESS_MAIN */

void read_defaults(void);
void save_defaults(void);
char * 
 override_txt(char *new_file,char *old_file);
GtkWidget *
 shortcut_menu (GtkWidget * parent, char *txt, gpointer func, gpointer data);
void 
 cb_toggle_preferences (GtkWidget * widget, gpointer data);
void cb_custom_SCK(GtkWidget * item, GtkWidget * ctree);
void cb_default_SCK(GtkWidget * item, GtkWidget * ctree );
void show_cat (char *message);


