void 
cb_open_trash (GtkWidget * item, GtkCTree *ctree);
void
cb_new_window (GtkWidget * widget, GtkCTree * ctree);
void
cb_select (GtkWidget * item, GtkCTree * ctree);
void
cb_unselect (GtkWidget * widget, GtkCTree * ctree);
/* FIXME: all cb_functions should used ctree, since
 * cfg variable in ctree contains all application
 * information*/
void
cb_diff (GtkWidget * widget, gpointer data);
void
cb_empty_trash (GtkWidget * widget, GtkCTree * ctree);
void
cb_delete (GtkWidget * widget, GtkCTree * ctree);
void
cb_find (GtkWidget * item, GtkWidget * ctree);
void
cb_about (GtkWidget * item, GtkWidget * ctree);
void
cb_new_subdir (GtkWidget * item, GtkWidget * ctree);
void
cb_new_file (GtkWidget * item, GtkWidget * ctree);
void
cb_duplicate (GtkWidget * item, GtkCTree * ctree);
void
cb_rename (GtkWidget * item, GtkCTree * ctree);
void
cb_open_with (GtkWidget * item, GtkCTree * ctree);
void
cb_props (GtkWidget * item, GtkCTree * ctree);
void
on_destroy (GtkWidget * top,  GtkCTree * ctree);
void
cb_destroy (GtkWidget * top, GtkCTree * ctree);
void 
cb_quit (GtkWidget * top,  GtkCTree * ctree);
void
cb_term (GtkWidget * item, GtkWidget * ctree);
/* FIXME: same as above */
void
cb_exec (GtkWidget * top, gpointer data);
/* at xtree_du.c: */
void 
cb_du (GtkWidget * item, GtkCTree * ctree);

	
