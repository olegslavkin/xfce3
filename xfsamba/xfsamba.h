/* xfsamba.h: header file for xfsamba modules.
 * copyright 2001 Edscott Wilson Garcia under GNU/GPL
 * */

/********** defines ***************/

#ifndef XFSAMBA_VERSION
#define XFSAMBA_VERSION "0.33.2"
#endif

#ifdef XFSAMBA_MAIN
#define EXTERN
#else
#define EXTERN extern
#endif


#define LOCATION_SHARES      1
#define LOCATION_SERVERS     2
#define LOCATION_WORKGROUPS  3

/*#define DBG_XFSAMBA*/

/* defines used for exiting : */
/*
#define E_CLEANUP  0
#define E_MALLOC   1
#define E_FILE     2
#define E_SEGV     3
#define dlg_inf(a)   my_show_message (a)
*/

#define HORIZONTAL  0
#define VERTICAL    1
#define HANDLEBOX   2
#define EXPAND      TRUE
#define FILL        TRUE
#define NOEXPAND    FALSE
#define NOFILL      FALSE
#define PACK        TRUE
#define ADD         FALSE
#define NOTUSED     FALSE
#define REFRESH     0
#define RELOAD      1
#define FORCERELOAD 2
#define FAILED     1
#define SUCCESS    2
#define CHALLENGED 3

/*column zero is reserved for icon, must be nonshared with text */

#define SHARE_NAME_COLUMN 1
#define SHARE_SIZE_COLUMN 2
#define SHARE_DATE_COLUMN 3
#define COMMENT_COLUMN 4
#define SHARE_COLUMNS 5

#define SERVER_NAME_COLUMN 1
#define SERVER_COMMENT_COLUMN 2
#define SERVER_COLUMNS 3

#define WG_NAME_COLUMN 1
#define WG_MASTER_COLUMN 2
#define WG_COLUMNS 3

#define WINDOW_WIDTH  400
#define XFSAMBA_MAX_STRING 255

/******************* structures ****************/


typedef struct nmb_cache
{
  char *textos[SHARE_COLUMNS];
  char visited;
  struct nmb_cache *next;

}
nmb_cache;

typedef struct smb_cache
{
  char *directory;
  GtkCTreeNode *node;
  struct smb_cache *next;

}
smb_cache;



typedef struct nmb_list
{
  unsigned char *netbios;
  unsigned char *server;
  unsigned char *password;
  char *serverIP;
  char loaded;
  struct nmb_list *next;
  struct nmb_list *previous;
  nmb_cache *shares;
  nmb_cache *servers;
  nmb_cache *workgroups;
}
nmb_list;

typedef struct nmb_history
{
  nmb_list *record;
  struct nmb_history *next;
  struct nmb_history *previous;
}
nmb_history;

typedef struct selected_struc
{
  GList *parent_node;
  GList *node;
  unsigned char *share;
  unsigned char *dirname;
  unsigned char *filename;
  unsigned char *comment;
  gboolean directory;
  gboolean file;
}
selected_struc;


/********** function prototypes ***************/

/* xfsamba prototypes : */

gboolean not_unique (void *object);
int parse_stderr (int n, void *data);
void print_diagnostics (char *message);
void print_status (char *message);
void xfsamba_abort (int why);
void SMBCleanLevel2 (void);

/** modules **/
/* xfsamba_download: */
void SMBGetFile (void);
/* xfsamba_upload: */
void SMBPutFile (void);
/* xfsamba_mkdir: */
void SMBmkdir (void);
/* xfsamba_rm: */
void SMBrm (void);
/* xfsamba_tar: */
void SMBtar (void);
/* xfsamba_nmblookup: */
void NMBLookup (GtkWidget * widget, gpointer data);
/* xfsamba_masterlookup; */
gboolean NMBmastersLookup (gpointer data);
/* xfsamba_masterresolve; */
gboolean NMBmastersResolve (nmb_list * currentN);
/* xfsamba_list; */
void SMBList (void);
/* xfsamba_smblookup: */
void SMBrefresh (unsigned char *servidor, int reload);
void SMBLookup (unsigned char *servidor, int reload);

/* mem prototypes: */
void eliminate2_cache (nmb_cache * the_cache, char *entry);
void pop_cache (nmb_cache * cache);
void smoke_nmb_cache (nmb_cache * fromC);
nmb_cache *clean_cache (nmb_cache * cache);
nmb_cache *push_nmb_cache (nmb_cache * headC, char **textos);

void smoke_history (nmb_history * fromH);
nmb_history *push_nmb_history (nmb_list * record);

nmb_list *push_nmb (char *serverIP);
nmb_list *push_nmbName (unsigned char *servidor);
void zap_nmb (nmb_list * currentN);
void reverse_smoke_nmb (nmb_list * fromN);
void smoke_nmb (nmb_list * fromN);
void clean_nmb (void);

void latin_1_unreadable (char *the_char);
void latin_1_readable (char *the_char);


/* gui prototypes: */
GtkWidget *passwd_dialog (int caso);
GtkWidget *create_smb_window (void);
void animation (gboolean state);
void node_destroy (gpointer p);

/************public variables *******************/

/* xfsamba modules: */

EXTERN selected_struc selected;
EXTERN nmb_list *thisN, *headN;
EXTERN nmb_history *thisH, *headH;
EXTERN void *fork_obj;
EXTERN int SMBResult;
EXTERN char *default_user;
EXTERN gboolean stopcleanup, nonstop;


/* global memory data, not to be jeopardized on forks: */
EXTERN unsigned char NMBpassword[XFSAMBA_MAX_STRING + 1];
EXTERN unsigned char NMBnetbios[XFSAMBA_MAX_STRING + 1];
EXTERN unsigned char NMBshare[XFSAMBA_MAX_STRING + 1];
EXTERN unsigned char NMBcommand[XFSAMBA_MAX_STRING + 1];
EXTERN char NMBserverIP[XFSAMBA_MAX_STRING + 1];



/* public variables from gui : */

EXTERN GdkPixmap *gPIX_page, *gPIX_rpage, *gPIX_dir_close, *gPIX_dir_open, *gPIX_dir_close_lnk, *gPIX_dir_open_lnk, *gPIX_comp1, *gPIX_comp2, *gPIX_wg1, *gPIX_wg2, *gPIX_reload, *gPIX_dotfile, *gPIX_delete, *gPIX_new_dir, *gPIX_rdotfile, *gPIX_view1, *gPIX_view2, *gPIX_view3, *gPIX_tar, *gPIX_print, *gPIX_help, *gPIX_ip, *gPIX_download, *gPIX_upload;

EXTERN GdkBitmap * gPIM_page, *gPIM_rpage, *gPIM_dir_close, *gPIM_dir_open, *gPIM_dir_close_lnk, *gPIM_dir_open_lnk, *gPIM_comp1, *gPIM_comp2, *gPIM_wg1, *gPIM_wg2, *gPIM_reload, *gPIM_dotfile, *gPIM_delete, *gPIM_new_dir, *gPIM_rdotfile, *gPIM_view1, *gPIM_view2, *gPIM_view3, *gPIM_tar, *gPIM_print, *gPIM_help, *gPIM_ip, *gPIM_download, *gPIM_upload;

EXTERN GtkWidget * smb_nav, *location, *locationIP, *shares, *servers, *workgroups, *diagnostics, *statusline, *sharesL, *serversL, *workgroupsL, *progress;
