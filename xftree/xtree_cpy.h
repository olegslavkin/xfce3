/* copywrite 2001 edscott wilson garcia under GNU/GPL*/

#ifdef XTREE_CPY_MAIN  
#else /* XTREE_CPY_MAIN */
#endif /* XTREE_CPY_MAIN */

/* byte 1 */
#define RW_ERROR_MALLOC		0x01
#define RW_ERROR_OPENING_SRC	0x02
#define RW_ERROR_OPENING_TGT	0x04
#define RW_ERROR_TOO_FEW	0x08
#define RW_ERROR_TOO_MANY	0x10
#define RW_ERROR_READING_SRC	0x20
#define RW_ERROR_WRITING_TGT	0x40
#define RW_ERROR_STAT_READ_SRC	0x80
/* byte 2 */
#define RW_ERROR_STAT_READ_TGT	0x100
#define RW_ERROR_STAT_WRITE_TGT	0x200
#define RW_ERROR_MULTIPLE2FILE	0x400
#define RW_ERROR_DIR2FILE	0x800
#define RW_ERROR_ENTRY_NEW	0x1000
#define RW_ERROR_CLOSE_SRC	0x2000
#define RW_ERROR_CLOSE_TGT	0x4000
#define RW_ERROR_UNLINK_SRC	0x8000
/* byte 3 */
#define RW_ERRNO		0x10000
#define RW_ERROR_FIFO		0x20000
#define RW_ERROR_DEVICE		0x40000
#define RW_ERROR_SOCKET		0x80000
#define RW_ERROR_WRITING_SRC	0x100000
#define RW_OK			0x200000
#define RW_ERROR_WRITING_DIR 	0x400000

#define TR_COPY		1
#define TR_MOVE		2
#define TR_LINK		4
#define TR_OVERRIDE	8


GtkWidget *show_cpy(GtkWidget *parent,gboolean show,int mode);
int rw_file(char *target,char *source);
void set_show_cpy(char *target,char *source);
void set_show_cpy_bar(int item,int nitems);
char *CreateTmpList(GtkWidget *parent,GList *list,entry *t_en);
gboolean DeleteTmpList(char *tmpfile);
gboolean IndirectTransfer(GtkWidget *ctree,int mode,char *tmpfile);
gboolean DirectTransfer(GtkWidget *ctree,int mode,char *tmpfile);
void set_override(gboolean state);
gboolean on_same_device(void);

