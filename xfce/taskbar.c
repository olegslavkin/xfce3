/*
 *
 * AFECTED FILES: taskbar.c taskbar.h xfce.c xfwm.c handle.h Makefile.am 
 *                ../libs/xfcolor.c ../configure.in acconfig.h 
 *
 * TODO (NaB = Not a Bug):
 *  - clean up widget structure 
 *  - (100%)set proper tb size according to 'standalone' state
 *  - (?) various taskbar height for different xfce panel sizes
 *  - (100%) proper disabling of 'toggled' signal handling
 *  - (100%) proper checking for XFCE window itself
 *  - (90%code,0%properties) internationalization
 *  - (?) long to gint/int
 *  - (100%/changing xfcolor.c concept) reliable indicator of current desk 
 *  - (NaB) cleanup init function(s) -- make single one
 *  - (?) remove g_xfce_taskbar and (100%) other global vars
 *  - (90%) function names clean-up
 *  - (100%) constants (DEFINEs) for all commands send to xfwm
 *  - comments
 *
 */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifdef XFCE_TASKBAR

  #include "taskbar.h"

  #include <stdio.h>
  #include <unistd.h>
  #include <stdlib.h>
  #include <stdarg.h>
  #include <string.h>
  #include <glib.h>
  #include <math.h>

  #include "xfwm.h"
  #include "../xfwm/xfwm.h"
  #include "xfce.h"
  #include "configfile.h"
  #include "sendinfo.h"
  #include "xfce_cb.h"
  #include "xfce.h"
  #include "selects.h"
  #include "constant.h"
  #include "my_intl.h"
  #include "move.h"
  #include "gnome_protocol.h"
  #include "xpmext.h"
  #include "handle.h"

#define TASKBAR_CMD_WIN_LIST  "Send_WindowList"
#define TASKBAR_CMD_CHANGE_FOCUS "Focus"
#define TASKBAR_CMD_ICONIFY  "Iconify 1"
#define TASKBAR_CMD_DEICONIFY "Iconify -1"
#define TASKBAR_CMD_RAISE "Raise"

#define TASKBAR_MIN_CELL_NO 5

#define TASKBAR_SORT_BY_NAME    1       
#define TASKBAR_SORT_BY_DESK    2       
#define TASKBAR_SORT_BY_WINID   3       
#define TASKBAR_SORT_UNSORTED   4       

#define TASKBAR_TB_HEIGHT       16

       
typedef struct _taskbar_window {
  long window;
  char *name;
  int desk;
  long flags;
} taskbar_window;


typedef struct _taskbar_xfce {
  int is_active;
  GtkWidget *gtk_xfce_toplevel;
  GtkWidget *gtk_realtaskbar;
  GtkWidget *gtk_labels[TASKBAR_MIN_CELL_NO];
  GtkToggleButton *gtk_toggled_button;
  GtkWidget *gtk_proc_load_indicator;
  int gtk_pressed;
  int base_height;
  GtkWidget *model_widget;
  int gtk_task_no;
  gint gtk_timeout_handler_id;
  long curr_desk;
  int sort_order;
  GList* windows;
  GtkWidget *gtk_stand_alone;
} taskbar_xfce;



/* 
 * where all taskbar structures are stored
 */
taskbar_xfce g_xfce_taskbar;

void taskbar_add_task_widget(long id,char *wname);
void taskbar_update_task_widget(long id,char *caption);
void taskbar_remove_task_widget(long id);
void taskbar_select_task_widget(long id);
void taskbar_sych_task_widget_with_desk(taskbar_window *win);

void taskbar_set_standalone_state(GtkWidget *tb_panel,gboolean state);
gint tb_set_proc_load(gpointer data);

  #ifdef LOG_FILE
FILE *f;
  #endif

void _print_gtk_widget_hierarchy(FILE *f,GtkWidget *widget, int level)
{
  int i;
  GList *l,*cl;
  if (!widget)
    return;
  for (i=0;i<level;i++)
    fprintf(f," ");
  if (GTK_IS_CONTAINER(widget)) {
    fprintf(f,"%s\n",widget->name);
    for (i=0;i<level;i++)
      fprintf(f," ");
    fprintf(f,"{\n");
    l=cl=gtk_container_children((GtkContainer*)widget);
    while (cl!=NULL) {
      _print_gtk_widget_hierarchy(f,(GtkWidget*)cl->data,level+3);
      cl=cl->next;
    }
    g_list_free(l);
    for (i=0;i<level;i++)
      fprintf(f," ");
    fprintf(f,"}\n");
  } else {
    fprintf(f,"%s\n",widget->name);
  }

}


void taskbar_xfwm_init()
{
#ifdef LOG_FILE
  f=fopen("/tmp/xfce_ms.log","a");
  fprintf(f,"--------------------------------------\n");
//  fclose(f);
#endif
  g_xfce_taskbar.windows=NULL;

  // g_xfce_taskbar.gtk_timeout_handler_id=gtk_timeout_add(300,tb_set_proc_load,NULL);

  if (current_config.wm)
    sendinfo (fd_internal_pipe,TASKBAR_CMD_WIN_LIST, 0);
}


gint taskbar_glist_comp_find_window(gconstpointer *a, gconstpointer *b)
{
  if (!a)
    return -1;
  return(((taskbar_window*)a)->window==(long)b) ? 0 : -1;
}

gint taskbar_glist_comp_sort_name(gconstpointer *a, gconstpointer *b)
{
  char *sa,*sb;
  sa=((taskbar_window*)a)->name;
  sb=((taskbar_window*)b)->name;
  return sa==NULL ? 1 : sb==NULL ? 1 : strcasecmp(sa,sb);
}

gint taskbar_glist_comp_sort_winid(gconstpointer *a, gconstpointer *b)
{
  long la,lb;
  la=((taskbar_window*)a)->window;
  lb=((taskbar_window*)b)->window;
  return la==lb ? 0 : la<lb ? -1 : 1;
}

gint taskbar_glist_comp_sort_desk(gconstpointer *a, gconstpointer *b)
{
  long la,lb;
  la=((taskbar_window*)a)->desk;
  lb=((taskbar_window*)b)->desk;
  return la==lb ? taskbar_glist_comp_sort_winid(a,b) : la<lb ? -1 : 1;
}

taskbar_window *taskbar_find_window(long id)
{
  GList *l;
  l=g_list_find_custom(g_xfce_taskbar.windows,(gpointer)id,(GCompareFunc)taskbar_glist_comp_find_window);
  return l ? (taskbar_window*) l->data : NULL;
}

void taskbar_set_active(long type)
{
  taskbar_window *tbw;
  tbw=taskbar_find_window(type);
  if (!tbw)
    return;
  if (current_config.wm) {
    sendinfo (fd_internal_pipe,TASKBAR_CMD_CHANGE_FOCUS, tbw->window);
    sendinfo (fd_internal_pipe,TASKBAR_CMD_DEICONIFY, tbw->window);
    sendinfo (fd_internal_pipe,TASKBAR_CMD_RAISE, tbw->window);
    tbw->flags&=!ICONIFIED;
  }
}

void taskbar_toggle_iconify(long type)
{
  taskbar_window *tbw;
  tbw=taskbar_find_window(type);
  if (!tbw)
    return;
  if (current_config.wm) {
    if (tbw->flags&ICONIFIED) {
      sendinfo (fd_internal_pipe,TASKBAR_CMD_CHANGE_FOCUS, tbw->window);
      sendinfo (fd_internal_pipe,TASKBAR_CMD_DEICONIFY, tbw->window);
      sendinfo (fd_internal_pipe,TASKBAR_CMD_RAISE, tbw->window);
      tbw->flags&=!ICONIFIED;
    } else {
      sendinfo (fd_internal_pipe,TASKBAR_CMD_ICONIFY, tbw->window);
      tbw->flags|=ICONIFIED;
    }
  }
}

GtkWidget* taskbar_get_widget(long id)
{
  GtkWidget *w;
  char sid[64];

  sprintf(sid,"task%ld",id);
  w=g_xfce_taskbar.gtk_realtaskbar;
  return(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),sid);
}

void taskbar_adapt_widgets_to_tasks_order()
{
  taskbar_window *cwin;    
  GList *clist;
  GtkWidget *w;
  GtkBox  *box;
  int cpos=0;

  box=GTK_BOX(g_xfce_taskbar.gtk_realtaskbar);
  clist=g_list_first(g_xfce_taskbar.windows);
  while (clist) {
    cwin=(taskbar_window*)clist->data;
    w=taskbar_get_widget(cwin->window);
    if (w) {
      gtk_box_reorder_child(box,w,cpos++);    
    }
    clist=g_list_next(clist);
  }
}

void taskbar_sort(int new_order)
{
  g_xfce_taskbar.sort_order=new_order;
  switch (new_order) {
  case TASKBAR_SORT_BY_DESK:
    g_xfce_taskbar.windows=g_list_sort(g_xfce_taskbar.windows,(GCompareFunc)taskbar_glist_comp_sort_desk);
    break;
  case TASKBAR_SORT_BY_NAME:
    g_xfce_taskbar.windows=g_list_sort(g_xfce_taskbar.windows,(GCompareFunc)taskbar_glist_comp_sort_name);
    break;
  case TASKBAR_SORT_BY_WINID:
    g_xfce_taskbar.windows=g_list_sort(g_xfce_taskbar.windows,(GCompareFunc)taskbar_glist_comp_sort_winid);
    break;
  case TASKBAR_SORT_UNSORTED:
  default:
    return;
    ;
  } //switch
  taskbar_adapt_widgets_to_tasks_order();
}

/* 
 * check for events interested for taskbar
 */
void taskbar_check_events(unsigned long type,unsigned long *body)
{
  taskbar_window *tbw;
  GList *clist;
  int desk_diff;

  switch (type) {
  case XFCE_M_CONFIGURE_WINDOW:
#ifdef LOG_FILE
    fprintf(f,"CONFIGURE_WINDOW: %ld\n",body[0]);
#endif  
  case XFCE_M_ADD_WINDOW:
    if (!(tbw=taskbar_find_window(body[0]))) {
#ifdef LOG_FILE
      fprintf(f,"ADD_WINDOW: %ld\n",body[0]);
#endif      
      tbw=(taskbar_window*)malloc(sizeof(taskbar_window));
      if (!tbw) {
#ifdef LOG_FILE
        fprintf(f,"Cannot malloc!\n");
#endif        
        return; /* TODO: error */
      }
      tbw->window=body[0];
      tbw->name=NULL;
      tbw->desk=-1;
      g_xfce_taskbar.windows=g_list_append(g_xfce_taskbar.windows,tbw);
      taskbar_add_task_widget(tbw->window,tbw->name);
    }
    desk_diff=tbw->desk!=body[7];
    tbw->desk=body[7];
    tbw->flags=body[8];
    taskbar_sych_task_widget_with_desk(tbw);
    if (desk_diff)
      taskbar_sort(g_xfce_taskbar.sort_order);

    break;
  case XFCE_M_NEW_DESK:
#ifdef LOG_FILE
    fprintf(f,"NEW_DESK: %ld\n",body[0]);
#endif    
    g_xfce_taskbar.curr_desk=body[0];
    clist=g_list_first(g_xfce_taskbar.windows);
    while (clist) {
      taskbar_sych_task_widget_with_desk((taskbar_window*) clist->data);
      clist=g_list_next(clist);
    }
    break;
  case XFCE_M_DESTROY_WINDOW:
#ifdef LOG_FILE
    fprintf(f,"DESTROY_WINDOW: %ld\n",body[0]);
#endif    
    if ((tbw=taskbar_find_window(body[0]))) {
      taskbar_remove_task_widget(tbw->window);
      g_xfce_taskbar.windows=g_list_remove(g_xfce_taskbar.windows,tbw);
      if (tbw->name)
        free(tbw->name);
      free(tbw);
    }
    break;
  case XFCE_M_FOCUS_CHANGE:
#ifdef LOG_FILE
    fprintf(f,"FOCUS_CHNAGE: %ld\n",body[0]);
#endif    
    if ((tbw=taskbar_find_window(body[0]))) {
      taskbar_select_task_widget(tbw->window);
    }
    break;
  case XFCE_M_ICON_NAME:
#ifdef LOG_FILE
    fprintf(f,"ICON NAME: %ld %s\n",body[0],(char*)(&body[3]));
#endif   
    if ((tbw=taskbar_find_window(body[0]))) {
      if (tbw->name)
        free(tbw->name);
      tbw->name=strdup((char*)(&body[3])); 
      taskbar_update_task_widget(tbw->window,tbw->name);
      taskbar_sort(g_xfce_taskbar.sort_order);
    } else
      return; /* TODO: error */
    break;


  default:
    ;
  } /* switch */
#ifdef LOG_FILE
  fflush(f);
#endif
}


/*
 * User Interace stuff
 */

void gtk_set_bg_color(GtkWidget *w,GtkStateType type,int color)
{
  GtkStyle *style,*os;

  os=gtk_widget_get_style(w);
  style=gtk_style_copy(os);

  if (style->bg_pixmap[type]) {
    gdk_pixmap_unref(style->bg_pixmap[type]);
    style->bg_pixmap[type]=NULL;
  }
  style->rc_style=NULL;
  style->bg[type].red=(color&0x00ff0000)>>8;
  style->bg[type].green=(color&0x0000ff00);
  style->bg[type].blue=(color&0x000000ff)<<8;
  gtk_widget_set_style(w,style);
  gtk_style_unref(style);
}

int gtk_get_bg_color(GtkWidget *w,GtkStateType type)
{
  GtkStyle *style;
  int r;

  style=gtk_widget_get_style (w);
  r=0;
  r=r|((style->bg[type].red&0xff00)<<8);
  r=r|(style->bg[type].green&0xff00);
  r=r|((style->bg[type].blue&0xff00)>>8);
  return r;
}


void taskbar_remove_labels()
{
  int i;

  for (i=0;(i<TASKBAR_MIN_CELL_NO) && (g_xfce_taskbar.gtk_labels[i]!=NULL);i++) {
    gtk_container_remove(GTK_CONTAINER(g_xfce_taskbar.gtk_realtaskbar),g_xfce_taskbar.gtk_labels[i]);
//    gtk_widget_destroy(g_xfce_taskbar.gtk_labels[i]); // TODO check if neccessary ?
    g_xfce_taskbar.gtk_labels[i]=NULL;
  }

}

void taskbar_set_labels()
{
  int i;

  for (i=0;i<(TASKBAR_MIN_CELL_NO-g_xfce_taskbar.gtk_task_no);i++) {
    g_xfce_taskbar.gtk_labels[i]=gtk_label_new("");
    gtk_container_add(GTK_CONTAINER(g_xfce_taskbar.gtk_realtaskbar),g_xfce_taskbar.gtk_labels[i]);
    gtk_widget_show(g_xfce_taskbar.gtk_labels[i]);
  }
}


void taskbar_sych_task_widget_with_desk(taskbar_window *win)
{
  GtkWidget *button;


  button=taskbar_get_widget(win->window);
  if (!button)
    return;
  if (win->desk==g_xfce_taskbar.curr_desk) {
    gtk_widget_set_name(button,"task_active");
  } else {
    gtk_widget_set_name(button,"task");
  }
}




// int omitt_toggle_handler=FALSE;

void taskbar_on_button_task_toggled(GtkToggleButton *button,
                                    gpointer user_data)
{
  long id=(long)user_data;
  int oldPressed;

//  if(omitt_toggle_handler)
//    return;
//  omitt_toggle_handler=TRUE;
  oldPressed=g_xfce_taskbar.gtk_pressed;
  g_xfce_taskbar.gtk_pressed=FALSE;

  if (!oldPressed||gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    taskbar_set_active(id);
  } else {
    taskbar_toggle_iconify(id);   
  }
//  omitt_toggle_handler=FALSE;
}

gint taskbar_on_button_task_pressed(GtkToggleButton *button,GdkEventButton *event,
                                    gpointer user_data)
{
  g_xfce_taskbar.gtk_pressed=TRUE;
  return FALSE;
}

void taskbar_add_task_widget(long id,char *wname)
{
  GtkWidget *w;
  GtkWidget *button;
  GtkWidget *label;
  char sid[64];
  guint handler_id;

  if ((GDK_WINDOW_XWINDOW(g_xfce_taskbar.gtk_xfce_toplevel->window)==id)||
      ((g_xfce_taskbar.gtk_stand_alone)&&(GDK_WINDOW_XWINDOW(g_xfce_taskbar.gtk_stand_alone->window)==id))) {
    return;
  }

  sprintf(sid,"task%ld",id);

  w=g_xfce_taskbar.gtk_realtaskbar;
  button = gtk_toggle_button_new();
  label = gtk_label_new(wname ? wname : "(null)");
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
  gtk_container_add(GTK_CONTAINER(button),label);

  gtk_object_set_data (GTK_OBJECT (w), strdup(sid), button);
  gtk_widget_set_name(button,"task");

  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
                      GTK_SIGNAL_FUNC (taskbar_on_button_task_pressed),
                      (gpointer)id);
  handler_id=gtk_signal_connect (GTK_OBJECT (button), "toggled",
                                 GTK_SIGNAL_FUNC (taskbar_on_button_task_toggled),
                                 (gpointer)id);
  gtk_object_set_data(GTK_OBJECT(button),"handler_id",(gpointer)handler_id);

  gtk_widget_show (button);

  taskbar_remove_labels();
  gtk_box_pack_start (GTK_BOX (w), button, TRUE, TRUE, 0);
  g_xfce_taskbar.gtk_task_no++;
  taskbar_set_labels();

}

void taskbar_remove_task_widget(long id)
{
  GtkWidget *w;
  GtkWidget *button;
  char sid[64];

  sprintf(sid,"task%ld",id);
  w=g_xfce_taskbar.gtk_realtaskbar;
  button=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),sid);
  if (!button)
    return;

  if ((GtkToggleButton*)button==g_xfce_taskbar.gtk_toggled_button)
    g_xfce_taskbar.gtk_toggled_button=NULL;

  taskbar_remove_labels();
  gtk_container_remove(GTK_CONTAINER(w),button);
  gtk_object_remove_data(GTK_OBJECT(w),sid);
//  gtk_widget_destroy(button);
  g_xfce_taskbar.gtk_task_no--;
  taskbar_set_labels();
}

void taskbar_update_task_widget(long id,char *wname)
{
  GtkWidget *button;
  GList *gl;
  GtkLabel *label;

  button=taskbar_get_widget(id);
  if (!button)
    return;

  // TODO:  TEMP SOLUTION 
/*
  if((wname!=NULL)&&(strncasecmp ("XFce Main Panel",wname,10)==0)) {
    taskbar_remove_task_widget(id);
    return;
  }
*/
  gl=gtk_container_children(GTK_CONTAINER(button));

  if (gl&&gl->data) {
    label=(GtkLabel*)gl->data;
    gtk_label_set_text(label,wname);
    gtk_widget_show(GTK_WIDGET(label));
  }

  /* set tool tip */
  gtk_tooltips_set_tip(gtk_tooltips_new(),button,wname,NULL);


}



void taskbar_select_task_widget(long id)
{
  GtkWidget *button;
  guint handler_id;

  button=taskbar_get_widget(id);    
  if (!button)
    return;

  handler_id=(guint)gtk_object_get_data(GTK_OBJECT(button),"handler_id");
//    omitt_toggle_handler=TRUE;
  if (g_xfce_taskbar.gtk_toggled_button) {
    handler_id=(guint)gtk_object_get_data(GTK_OBJECT(g_xfce_taskbar.gtk_toggled_button),"handler_id");
    gtk_signal_handler_block(GTK_OBJECT(g_xfce_taskbar.gtk_toggled_button),handler_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_xfce_taskbar.gtk_toggled_button),FALSE);
    gtk_signal_handler_unblock(GTK_OBJECT(g_xfce_taskbar.gtk_toggled_button),handler_id);

  }

  if (button) {
    handler_id=(guint)gtk_object_get_data(GTK_OBJECT(button),"handler_id");
    gtk_signal_handler_block(GTK_OBJECT(button),handler_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
    gtk_signal_handler_unblock(GTK_OBJECT(button),handler_id);
  }

  g_xfce_taskbar.gtk_toggled_button=(GtkToggleButton*)button;

//    omitt_toggle_handler=FALSE;
}



void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
  int i;
  float f, p, q, t;

  if ( s == 0 ) {
// achromatic (grey)
    *r = *g = *b = v;
    return;
  }

  h /= 60;// sector 0 to 5
  i = floor( h );
  f = h - i;// factorial part of h
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );

  switch ( i ) {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default:// case 5:
    *r = v;
    *g = p;
    *b = q;
    break;
  }

}



gint taskbar_set_proc_load(gpointer data)
{
  char stats_in[256];
  char *s;
  FILE *stats_f;
  int user,sys,nice,idle;
  static int o_user,o_sys,o_nice,o_idle;
  float over;
  int busy,total;
  float r,g,b;


  if (!g_xfce_taskbar.gtk_proc_load_indicator)
    return TRUE;

  stats_f=fopen("/proc/stat","r");
  if (!stats_f) {
    return TRUE;
  }
  while (fgets(stats_in,sizeof(stats_in),stats_f)!=NULL) {
    if ((s=strstr(stats_in,"cpu"))!=NULL) {
      s=s+strlen("cpu");
      sscanf(s,"%d %d %d %d",&user,&sys,&nice,&idle);
      busy=(user-o_user)+(sys-o_sys)+(nice-o_nice); 
      total=busy+(idle-o_idle);
      over=(float)busy/(float)total;
      HSVtoRGB(&r,&g,&b,0.0,over,1.0);
      gtk_set_bg_color(g_xfce_taskbar.gtk_proc_load_indicator,GTK_STATE_NORMAL,(((int)(r*0xff))<<16)|(((int)(g*0xff))<<8)|(((int)(b*0xff))<<0));
      o_user=user; o_sys=sys; o_nice=nice; o_idle=idle;
      break;
    }
  }
  fclose(stats_f);

  return TRUE;

}



void taskbar_on_button_open_clicked(GtkWidget *w,gpointer data)
{
  GtkWidget *tb_panel;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_hrule;
  GtkWidget *tb_standalone;
  GtkWidget *tb_button_close;

  tb_panel=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),"tb_panel");
  tb_standalone=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_mcheck_standalone");

  tb_hbox1=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hbox1");
  tb_hrule=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hrule");
  tb_button_close=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_button_close");


  g_xfce_taskbar.gtk_proc_load_indicator=tb_button_close;

  gtk_widget_show_all(tb_hbox1);
  gtk_widget_show(tb_hrule);
  gtk_widget_hide(w);
  g_xfce_taskbar.is_active=TRUE;

  //TODO remove redundacy with prevois steps
  taskbar_set_standalone_state(tb_panel,GTK_CHECK_MENU_ITEM(tb_standalone)->active ? TRUE : FALSE);

}

void taskbar_on_button_close_clicked(GtkWidget *w,gpointer data)
{
  GtkWidget *tb_panel;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_button_open;
  GtkWidget *tb_hrule;  
  GtkWidget *tb_standalone;  

  g_xfce_taskbar.gtk_proc_load_indicator=NULL;

  tb_panel=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),"tb_panel");
  tb_hbox1=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hbox1");
  tb_button_open=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_button_open");
  tb_hrule=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hrule");

  tb_standalone=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_mcheck_standalone");

  g_xfce_taskbar.is_active=FALSE;
  taskbar_set_standalone_state(tb_panel,FALSE);

  gtk_widget_hide_all(tb_hbox1);
  gtk_widget_hide(tb_hrule);
  gtk_widget_show(tb_button_open);

}

/*
 * pop-up menu
 */
gint taskbar_on_button_close_right_clicked(GtkWidget *widget, GdkEventButton *event)
{
  GtkMenu *menu;
  
  menu = GTK_MENU (widget);
  if ((event->type == GDK_BUTTON_PRESS)&&(event->button >= 2)) {
    gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
                    event->button, event->time);
    return TRUE;
  }
//fflush(f);
  return FALSE;
}


void taskbar_on_radio_sort_toggled(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
  int type;
  type=(int)user_data;
  if (!checkmenuitem->active)
    return;
  taskbar_sort(type);
}


GtkWidget* taskbar_create_standalone_frame(GtkWidget *to_be_added)
{
  GtkWidget *window,*frame;
  GtkWidget *hbox;
  GtkWidget *ebox;
  GtkWidget *move_pixmap;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);
  gtk_widget_set_name (window, "tb_standalone");
  gtk_window_set_title (GTK_WINDOW (window), "XFce Taskbar");
  gtk_widget_realize (window);
  /* decorations !!!!! */
  gdk_window_set_decorations (window->window, 0);//GDK_DECOR_RESIZEH);
  frame=gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(window),frame);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

  hbox=gtk_hbox_new(FALSE,1);
  gtk_container_add(GTK_CONTAINER(frame),hbox);
  ebox=gtk_event_box_new();
  create_move_button(ebox,window);
  move_pixmap = MyCreateFromPixmapData (ebox, handle);
  if (move_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_show(move_pixmap);
  gtk_widget_set_usize(move_pixmap,12,TASKBAR_TB_HEIGHT);
  gtk_container_add(GTK_CONTAINER(ebox),move_pixmap);

  gtk_box_pack_start(GTK_BOX(hbox),ebox,FALSE,FALSE,2);
  gtk_widget_show(ebox);
  gtk_widget_show(hbox);


  gtk_object_ref(GTK_OBJECT(to_be_added));
  if (to_be_added->parent)
    gtk_container_remove(GTK_CONTAINER(to_be_added->parent),to_be_added);
//  gtk_widget_unparent(to_be_added);
  gtk_container_add(GTK_CONTAINER(hbox),to_be_added);
  gtk_object_unref(GTK_OBJECT(to_be_added));


  ebox=gtk_event_box_new();
  create_move_button(ebox,window);
  move_pixmap = MyCreateFromPixmapData (ebox, handle);
  if (move_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_show(move_pixmap);
  gtk_widget_set_usize(move_pixmap,12,TASKBAR_TB_HEIGHT);
  gtk_container_add(GTK_CONTAINER(ebox),move_pixmap);
  gtk_box_pack_start(GTK_BOX(hbox),ebox,FALSE,FALSE,2);
  gtk_widget_show(ebox);

  gtk_widget_show(to_be_added);

  gnome_layer (window->window, current_config.panel_layer);

  gtk_widget_show(window);
  return window;
}


void taskbar_set_standalone_state(GtkWidget *tb_panel,gboolean state)
{
  GtkWidget *tb_parent;
  GtkWidget *tb_hrule;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_button_open;
  GtkRequisition req;
  GtkWidget *toggle;

  tb_hbox1=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hbox1");
  tb_hrule=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hrule");
  tb_button_open=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_size_model");

  toggle=gtk_toggle_button_new_with_label("A");

/*  
  toggle_style=gtk_widget_get_style(toggle);
  if(toggle_style->font) {
    base_height=toggle_style->
  } else {
    base_height=TASKBAR_TB_HEIGHT;
  }
*/  
  gtk_widget_size_request(GTK_BIN(toggle)->child,&req);
  g_xfce_taskbar.base_height=req.height;
  gtk_widget_unref(toggle);

  if (state==TRUE) {
    if (!g_xfce_taskbar.gtk_stand_alone) {
      g_xfce_taskbar.gtk_stand_alone=taskbar_create_standalone_frame(tb_panel);
      gtk_widget_hide(tb_hrule);
    }
    gtk_widget_set_usize(g_xfce_taskbar.gtk_stand_alone,gdk_screen_width(),g_xfce_taskbar.base_height+8);
    gtk_widget_set_uposition(g_xfce_taskbar.gtk_stand_alone,0,gdk_screen_height()-(g_xfce_taskbar.base_height+8));
  } else {
    if (g_xfce_taskbar.gtk_stand_alone) {
      gtk_object_ref(GTK_OBJECT(tb_panel));
      if (tb_panel->parent)
        gtk_container_remove(GTK_CONTAINER(tb_panel->parent),tb_panel);
      tb_parent=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_parent");
      gtk_container_add(GTK_CONTAINER(tb_parent),tb_panel);
      gtk_widget_show(tb_hrule);
      gtk_widget_show(tb_panel);
      gtk_object_unref(GTK_OBJECT(tb_panel));
      gtk_widget_destroy(g_xfce_taskbar.gtk_stand_alone);
      g_xfce_taskbar.gtk_stand_alone=NULL;
    }
    
/*
    gtk_widget_show(tb_button_open);
    gtk_widget_size_allocate(tb_button_open,&alloc);
    gtk_widget_hide(tb_button_open);
*/
    gtk_widget_set_usize(tb_hbox1,tb_button_open->allocation.width,g_xfce_taskbar.base_height);
    gtk_widget_set_usize(tb_hrule,tb_button_open->allocation.width,2); 
  }
}
void taskbar_on_mcheck_standalone_toggled(GtkWidget *w, gpointer user_data)
{
  GtkWidget *tb_panel;

  tb_panel=(GtkWidget*)user_data;
  if (GTK_CHECK_MENU_ITEM(w)->active) {
    taskbar_set_standalone_state(tb_panel,TRUE);
  } else {
    taskbar_set_standalone_state(tb_panel,FALSE);
  }
}

void taskbar_on_mcheck_sysload_toggled(GtkWidget *w, gpointer user_data)
{
  GtkWidget *tb_panel,*cb;

  tb_panel=(GtkWidget*)user_data;
  if (!(GTK_CHECK_MENU_ITEM(w)->active)) {
    if (g_xfce_taskbar.gtk_timeout_handler_id>=0) {
      gtk_timeout_remove(g_xfce_taskbar.gtk_timeout_handler_id);
      g_xfce_taskbar.gtk_timeout_handler_id=-1;
      cb=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_button_close");
      if (cb) {
        gtk_widget_restore_default_style(cb);
      }
    }
  } else if (g_xfce_taskbar.gtk_timeout_handler_id<0) {
    g_xfce_taskbar.gtk_timeout_handler_id=(gint)gtk_timeout_add(900,taskbar_set_proc_load,NULL);
//      gtk_set_bg_color(cb,GTK_STATE_NORMAL,gtk_get_bg_color (,GTK_STATE_NORMAL));
  }
}


/* 
 * create taskbar frame (to be added to xfce main window) 
 */
GtkWidget *taskbar_create_gxfce_with_taskbar (GtkWidget *parent,GtkWidget *size_model,GtkWidget *top_level_window)
{
  GtkWidget *tb_panel;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_button_close;
  GtkWidget *tb_real;
  GtkWidget *tb_button_open;
  GtkWidget *tb_hrule;
  GtkWidget *popup_menu;
  GtkWidget *menu_item;
  GSList    *radio_list;
  FILE *stats_f;
  int i;

  tb_panel=gtk_vbox_new(FALSE,0);
  tb_button_open=gtk_button_new();
  gtk_object_set_data(GTK_OBJECT(tb_panel),"tb_parent",(gpointer)parent);
  gtk_object_set_data(GTK_OBJECT(tb_panel),"tb_size_model",(gpointer)size_model);
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_button_open", tb_button_open);
  gtk_object_set_data (GTK_OBJECT (tb_button_open), "tb_panel", tb_panel);

  gtk_container_add(GTK_CONTAINER(tb_panel),tb_button_open);
  gtk_signal_connect(GTK_OBJECT(tb_button_open),"clicked",GTK_SIGNAL_FUNC(taskbar_on_button_open_clicked),NULL);
  gtk_widget_show(tb_panel);
  gtk_widget_show(tb_button_open);
  gtk_widget_set_usize(tb_button_open,tb_button_open->allocation.width,4);// TODO correct height

  tb_hrule=gtk_hseparator_new();
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_hrule", tb_hrule);
  gtk_container_add(GTK_CONTAINER(tb_panel),tb_hrule);

  tb_hbox1=gtk_hbox_new(FALSE,1);
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_hbox1", tb_hbox1);
  tb_button_close=gtk_button_new();
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_button_close", tb_button_close);
  gtk_object_set_data (GTK_OBJECT (tb_button_close), "tb_panel", tb_panel);

  /* pop-up menu */
  popup_menu=gtk_menu_new();
  radio_list=NULL;

  menu_item=gtk_check_menu_item_new_with_label(_("Stand alone"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_mcheck_standalone", menu_item);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),FALSE);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_mcheck_standalone_toggled),(gpointer)tb_panel);
  menu_item=gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);


  menu_item=gtk_menu_item_new_with_label(_("Sort order:"));
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);



  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  by desk"));
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_BY_DESK);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);
  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  by name"));
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_BY_NAME);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);
  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  by window ID"));
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_BY_WINID);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);
  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  unsorted"));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_UNSORTED);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);

  menu_item=gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item=gtk_check_menu_item_new_with_label(_("System load"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_mcheck_sysload", menu_item);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),FALSE);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);

   /* check if /proc/stat present */
  stats_f=fopen("/proc/stat","r");
  if (!stats_f)
    gtk_widget_set_sensitive(menu_item,FALSE);
  else 
    fclose(stats_f);


  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_mcheck_sysload_toggled),(gpointer)tb_panel);

  /* menu end */

  gtk_signal_connect_object(GTK_OBJECT(tb_button_close), "button_press_event",
                            GTK_SIGNAL_FUNC (taskbar_on_button_close_right_clicked), GTK_OBJECT(popup_menu));


  gtk_signal_connect(GTK_OBJECT(tb_button_close),"clicked",GTK_SIGNAL_FUNC(taskbar_on_button_close_clicked),NULL);

  tb_real=gtk_hbox_new(TRUE,3);
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_real", tb_real);

  gtk_box_pack_start(GTK_BOX(tb_hbox1),tb_button_close,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(tb_hbox1),tb_real,TRUE,TRUE,0);

  gtk_container_add(GTK_CONTAINER(tb_panel),tb_hbox1);
  /* init g_xfce_taskbar */
  g_xfce_taskbar.gtk_xfce_toplevel=top_level_window;
  g_xfce_taskbar.gtk_realtaskbar=tb_real;

  for (i=0;i<TASKBAR_MIN_CELL_NO;i++) {
    g_xfce_taskbar.gtk_labels[i]=NULL;
  }
  g_xfce_taskbar.gtk_task_no=0;
  g_xfce_taskbar.gtk_toggled_button=NULL;
  g_xfce_taskbar.curr_desk=0; //TODO check if -1 is better ?
  g_xfce_taskbar.sort_order=TASKBAR_SORT_UNSORTED;
  g_xfce_taskbar.model_widget=tb_button_open;
  g_xfce_taskbar.gtk_stand_alone=NULL;
  g_xfce_taskbar.gtk_pressed=FALSE;
  g_xfce_taskbar.gtk_timeout_handler_id=-1;
  g_xfce_taskbar.gtk_proc_load_indicator=NULL;
  g_xfce_taskbar.is_active=FALSE;
  taskbar_set_labels();
  return tb_panel;
}

#endif

