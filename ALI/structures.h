#ifndef STRUCTURES_H
#define STRUCTURES_H

#include<gtk/gtk.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

void ProgramStart();
void ProgramEnd();
void MainStart();

// Global CSS provider for consistent styling
extern GtkCssProvider *global_css_provider;

// Helper function to apply CSS to a widget
void apply_css(GtkWidget *widget, const char *css);

// Helper function to update widget properties
void refresh_widget(GtkWidget *widget);

/*
--------- Gestion des fenetres ---------
*/
typedef struct win {
    GtkWidget *win;
    char *title;
    char *icon_path;
    char *bg_image;
    char *bg_color;
    int width;
    int height;
    gboolean resizable;
    gboolean modal;
    double opacity;
    gboolean fullscreen;
    gboolean keep_below;
    GtkWindowType type_win;
    GtkWidget *drawing_area;
    GtkCssProvider *css_provider;
} window;

window* create_window();
void update_window(window *wn);
void display_window(window *wn);
void close_window(window *wn);

/*
--------- Gestion des header bars ---------
*/
typedef struct hdb{
    GtkWidget *header_bar;
    char *title;
    char *sub_title;
    char *title_style;
    char *icon_path;
    int width;
    int height;
    gboolean close_button_show;
    char *decoration_layout;
} header_bar;

header_bar* create_header_bar();
void update_header_bar(header_bar *hdb);
void display_header_bar(header_bar *hdb);
void cairo_ellipse(cairo_t *cr, double x, double y, double rx, double ry);
/*
---------- Gestion Buttons ----------
*/
typedef struct btn
{
    GtkWidget *button;
    char *label;
    char *bg_image;
    char *bg_color;
    char *label_style;
    int width;
    int height;
    void (*click_callback)(GtkWidget*, gpointer);
    void *callback_data;
} button;

button* create_button();
void update_button(button *btn);

/*
---------- Gestion Labels ----------
*/
typedef struct lb{
    GtkWidget *label;
    char *text;
    char *color;
    char *bg_color;
    char *font_family;
    char *style;
    char *position;
    int width;
    int height;
} label;

label* create_label();
void update_label(label *lb);

/*
---------- Gestion Progress Bars ----------
*/
typedef struct prog{
    GtkWidget *progress;
    double fraction;
    char *text;
    char *pulse_text;
    int width;
    int height;
    gboolean inverted;
    GtkOrientation orientation;
    gboolean show_text;
    gboolean is_pulsing;
    guint pulse_interval;
    guint pulse_timeout_id;
} progress_bar;

progress_bar* create_progress_bar();
void update_progress_bar(progress_bar *pb);
void start_progress_pulse(progress_bar *pb);
void stop_progress_pulse(progress_bar *pb);
void set_progress_value(progress_bar *pb, double value);

/*
---------- Gestion Spinner ----------
*/
typedef struct spin {
    GtkWidget *container;
    GtkWidget *spinner;
    GtkWidget *label;
    char *text;
    int width;
    int height;
    gboolean active;
    gboolean spinning;
} spinner;

spinner* create_spinner();
void update_spinner(spinner *spin);
void start_spinner(spinner *spin);
void stop_spinner(spinner *spin);
GtkWidget* get_spinner_widget(spinner *spin);

/*
---------- Gestion Image ----------
*/
typedef struct im{
    GtkWidget *image;
    char *file_name;
    char *animation_file;
    char *icon_name;
    char *position;
    char *border_color;
    int width;
    int height;
    int border_width;
    int border_radius;
    int pixel_size;
    void (*click_callback)(GtkWidget*, gpointer);
    void *callback_data;
} image;

image* create_image();
void update_image(image *im);

/*
---------- Gestion Entry ----------
*/
typedef struct entry {
    GtkWidget *entry;
    char *text;
    char *placeholder_text;
    char *bg_color;
    char *color;
    char *font_family;
    char *label_style;
    int width;
    int height;
    int border_width;
    int margin_top, margin_bottom, margin_left, margin_right;
    gboolean visibility;
    gboolean editable;
    gboolean has_frame;
    void (*changed_callback)(GtkWidget*, gpointer);
    void *callback_data;
} entry;

entry* create_entry();
void update_entry(entry *en);
char* get_entry_text(entry *en);
void set_entry_text(entry *en, const char *text);

/*
---------- Gestion Radio Button ----------
*/
typedef struct radbtn {
    GtkWidget *radio_button;
    char *label;
    char *group_id;
    char *color;
    char *bg_color;
    int width;
    int height;
    char *position;
    gboolean active;
    gboolean mode;
    void (*toggled_callback)(GtkWidget*, gpointer);
    void *callback_data;
} radio_button;

radio_button* create_radio_button(const char *group_id, const char *label);
void update_radio_button(radio_button *rdb);
void set_radio_button_active(radio_button *rdb, gboolean active);
gboolean is_radio_button_active(radio_button *rdb);

/*
---------- Gestion Check Button ----------
*/
typedef struct chb {
    GtkWidget *checkbutton;
    char *label;
    char *color;
    char *bg_color;
    char *font_family;
    char *label_style;
    int width;
    int height;
    char *position;
    gboolean active;
    gboolean mode;
    gboolean inconsistent;
    gboolean draw_indicator;
    void (*toggled_callback)(GtkWidget*, gpointer);
    void *callback_data;
} check_button;

check_button* create_check_button();
void update_check_button(check_button *chb);
void set_check_button_active(check_button *chb, gboolean active);
gboolean is_check_button_active(check_button *chb);


/*
---------- Gestion Box ----------
*/
typedef struct bx {
    GtkWidget *box;
    GtkOrientation orientation;
    int spacing;
    int width;
    int height;
    int border_width;
    char *bg_color;
    int margin_top, margin_bottom, margin_left, margin_right;
    gboolean homogeneous;
    gboolean expand;
    gboolean fill;
    guint padding;
} box;

box* create_box(GtkOrientation orientation, int spacing, gboolean homogeneous);
box* create_hbox(int spacing, gboolean homogeneous);
box* create_vbox(int spacing, gboolean homogeneous);
void update_box(box *bx);
void add_to_box(box *bx, GtkWidget *widget);
void add_to_box_with_properties(box *bx, GtkWidget *widget, gboolean expand, gboolean fill, guint padding);
void clear_box(box *bx);
GtkWidget* get_box_widget(box *bx);


/*
---------- Gestion Menu and Submenu ----------
*/
typedef struct menu {
    GtkWidget *menu_widget;
    GtkOrientation orientation;
    char *bg_color;
    char *text_color;
    int item_spacing;
} menu;

typedef struct menu_button {
    GtkWidget *button;
    char *label;
    char *icon_name;
    menu *main_menu;
    int width;
    int height;
    char *bg_color;
    char *text_color;
    char *position;
} menu_button;

// Menu creation functions
menu* create_horizontal_menu();
menu* create_vertical_menu();
menu* create_submenu();

// Menu item functions
void add_menu_item(menu *m, const char *label, void (*callback)(GtkWidget*, gpointer), void *data);
void add_menu_item_with_icon(menu *m, const char *label, const char *icon_name, 
                              void (*callback)(GtkWidget*, gpointer), void *data);
void add_check_menu_item(menu *m, const char *label, gboolean active,
                          void (*callback)(GtkWidget*, gpointer), void *data);
void add_radio_menu_item(menu *m, const char *label, gboolean active,
                          void (*callback)(GtkWidget*, gpointer), void *data);
void add_menu_separator(menu *m);
void add_submenu(menu *parent, const char *label, menu *submenu);
void add_submenu_with_icon(menu *parent, const char *label, const char *icon_name, menu *submenu);

// Menu button functions
menu_button* create_menu_button(const char *label);
menu_button* create_menu_button_with_icon(const char *icon_name);
void set_menu_button_menu(menu_button *mb, menu *m);
void update_menu_button(menu_button *mb);
void add_menu_button_to_box(box *bx, menu_button *mb);

// Helper functions
void free_menu(menu *m);
void free_menu_button(menu_button *mb);


/*
--------- Container Addition Functions ---------
*/
void add_to_window(window *wn, GtkWidget *widget);
void add_to_header_bar_start(header_bar *hdb, GtkWidget *widget);
void add_to_header_bar_end(header_bar *hdb, GtkWidget *widget);


// Convenience macros
#define ADD_TO_WINDOW(win, widget) add_to_window(win, (widget))
#define ADD_TO_HEADER_START(hdb, widget) add_to_header_bar_start(hdb, (widget))
#define ADD_TO_HEADER_END(hdb, widget) add_to_header_bar_end(hdb, (widget))


#endif