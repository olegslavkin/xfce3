#ifndef __XFGOGO_H_INC
#define __XFGOGO_H_INC

typedef struct golist {
	struct golist *previous;
	char *path;
} golist;

void go_to (GtkCTree * ctree, GtkCTreeNode * root, char *path, int flags);
void cb_go_to (GtkWidget * item, GtkCTree * ctree);
void cb_go_home (GtkWidget * item, GtkCTree * ctree);
void cb_go_up (GtkWidget * item, GtkCTree * ctree);
void cb_go_back (GtkWidget * item, GtkCTree * ctree);
golist *pushgo(char *path,golist *thisgo);
golist *popgo(golist *thisgo);

#endif


