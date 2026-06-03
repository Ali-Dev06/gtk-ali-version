#include "structures.h"
#include <gdk/gdk.h>

// Add this at the top of structures.c
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GtkCssProvider *global_css_provider = NULL;

void ProgramStart(){
    gtk_init(NULL, NULL);
    global_css_provider = gtk_css_provider_new();
}


void ProgramEnd(){
    gtk_main_quit();
    if (global_css_provider) {
        g_object_unref(global_css_provider);
    }
}

void MainStart(){
    gtk_main();
}

void apply_css(GtkWidget *widget, const char *css) {
    if (!widget || !css) return;
    
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;
    
    if (!gtk_css_provider_load_from_data(provider, css, -1, &error)) {
        g_printerr("CSS Error: %s\n", error->message);
        g_error_free(error);
    } else {
        GtkStyleContext *context = gtk_widget_get_style_context(widget);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), 
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_object_unref(provider);
}

void refresh_widget(GtkWidget *widget) {
    if (widget) {
        gtk_widget_queue_draw(widget);
    }
}

/* ---------- Window Functions ---------- */

window* create_window() {
    window *wn = malloc(sizeof(window));
    if (!wn) exit(1);
    
    wn->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    wn->title = NULL;
    wn->icon_path = NULL;
    wn->bg_image = NULL;
    wn->bg_color = NULL;
    wn->width = 400;
    wn->height = 300;
    wn->resizable = TRUE;
    wn->modal = FALSE;
    wn->opacity = 1.0;
    wn->fullscreen = FALSE;
    wn->keep_below = FALSE;
    wn->type_win = GTK_WINDOW_TOPLEVEL;
    wn->drawing_area = NULL;
    wn->css_provider = NULL;
    
    return wn;
}

void update_window(window *wn) {
    if (!wn || !wn->win) return;
    
    if (wn->title) {
        gtk_window_set_title(GTK_WINDOW(wn->win), wn->title);
    }
    
    gtk_window_set_default_size(GTK_WINDOW(wn->win), wn->width, wn->height);
    gtk_window_set_resizable(GTK_WINDOW(wn->win), wn->resizable);
    gtk_window_set_modal(GTK_WINDOW(wn->win), wn->modal);
    gtk_widget_set_opacity(wn->win, wn->opacity);
    
    if (wn->bg_color) {
        char css[256];
        snprintf(css, sizeof(css), "window { background-color: %s; }", wn->bg_color);
        if (!wn->css_provider) wn->css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(wn->css_provider, css, -1, NULL);
        GtkStyleContext *context = gtk_widget_get_style_context(wn->win);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(wn->css_provider), 
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    
    if (wn->icon_path) {
        gtk_window_set_icon_from_file(GTK_WINDOW(wn->win), wn->icon_path, NULL);
    }
    
    if (wn->fullscreen) {
        gtk_window_fullscreen(GTK_WINDOW(wn->win));
    }
}

void display_window(window *wn) {
    if (wn && wn->win) {
        update_window(wn);
        gtk_widget_show_all(wn->win);
    }
}

void close_window(window *wn) {
    if (wn && wn->win) {
        g_signal_connect(wn->win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }
}

/* ---------- Header Bar Functions ---------- */

header_bar* create_header_bar() {
    header_bar *hdb = malloc(sizeof(header_bar));
    if (!hdb) exit(1);
    
    hdb->header_bar = gtk_header_bar_new();
    hdb->title = NULL;
    hdb->sub_title = NULL;
    hdb->title_style = NULL;
    hdb->icon_path = NULL;
    hdb->width = -1;
    hdb->height = -1;
    hdb->close_button_show = FALSE;
    hdb->decoration_layout = NULL;
    
    GtkWidget *title_label = gtk_label_new("");
    gtk_header_bar_set_custom_title(GTK_HEADER_BAR(hdb->header_bar), title_label);
    
    return hdb;
}

void update_header_bar(header_bar *hdb) {
    if (!hdb || !hdb->header_bar) return;
    
    GtkWidget *title_widget = gtk_header_bar_get_custom_title(GTK_HEADER_BAR(hdb->header_bar));
    
    if (hdb->title && title_widget && GTK_IS_LABEL(title_widget)) {
        gtk_label_set_text(GTK_LABEL(title_widget), hdb->title);
        
        if (hdb->title_style) {
            PangoAttrList *attrs = pango_attr_list_new();
            if (strstr(hdb->title_style, "bold")) {
                pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
            }
            if (strstr(hdb->title_style, "italic")) {
                pango_attr_list_insert(attrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
            }
            if (strstr(hdb->title_style, "underline")) {
                pango_attr_list_insert(attrs, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
            }
            gtk_label_set_attributes(GTK_LABEL(title_widget), attrs);
            pango_attr_list_unref(attrs);
        }
    }
    
    if (hdb->sub_title) {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hdb->header_bar), hdb->sub_title);
    }
    
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hdb->header_bar), hdb->close_button_show);
    
    if (hdb->width > 0 && hdb->height > 0) {
        gtk_widget_set_size_request(hdb->header_bar, hdb->width, hdb->height);
    }
}

void display_header_bar(header_bar *hdb) {
    if (hdb && hdb->header_bar) {
        update_header_bar(hdb);
        gtk_widget_show_all(hdb->header_bar);
    }
}

/* ---------- Button Functions ---------- */

button* create_button() {
    button *btn = malloc(sizeof(button));
    if (!btn) exit(1);
    
    btn->button = gtk_button_new();
    btn->label = g_strdup("Button");
    btn->bg_image = NULL;
    btn->bg_color = NULL;
    btn->label_style = NULL;
    btn->width = -1;
    btn->height = -1;
    btn->click_callback = NULL;
    btn->callback_data = NULL;
    
    gtk_button_set_label(GTK_BUTTON(btn->button), btn->label);
    
    return btn;
}

void update_button(button *btn) {
    if (!btn || !btn->button) return;
    
    if (btn->label) {
        gtk_button_set_label(GTK_BUTTON(btn->button), btn->label);
    }
    
    if (btn->width > 0 && btn->height > 0) {
        gtk_widget_set_size_request(btn->button, btn->width, btn->height);
        gtk_widget_set_hexpand(btn->button, FALSE);
        gtk_widget_set_vexpand(btn->button, FALSE);
    }
    
    if (btn->bg_color) {
        char css[256];
        snprintf(css, sizeof(css), "button { background-color: %s; }", btn->bg_color);
        apply_css(btn->button, css);
        gtk_button_set_relief(GTK_BUTTON(btn->button), GTK_RELIEF_NONE);
    }
    
    if (btn->label_style) {
        GtkWidget *label_widget = gtk_bin_get_child(GTK_BIN(btn->button));
        if (label_widget && GTK_IS_LABEL(label_widget)) {
            PangoAttrList *attrs = pango_attr_list_new();
            if (strstr(btn->label_style, "bold")) {
                pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
            }
            if (strstr(btn->label_style, "italic")) {
                pango_attr_list_insert(attrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
            }
            gtk_label_set_attributes(GTK_LABEL(label_widget), attrs);
            pango_attr_list_unref(attrs);
        }
    }
    
    if (btn->click_callback) {
        g_signal_connect(btn->button, "clicked", G_CALLBACK(btn->click_callback), btn->callback_data);
    }
}

/* ---------- Label Functions ---------- */

label* create_label() {
    label *lb = malloc(sizeof(label));
    if (!lb) exit(1);
    
    lb->label = gtk_label_new("");
    lb->text = g_strdup("Label");
    lb->color = NULL;
    lb->bg_color = NULL;
    lb->font_family = NULL;
    lb->style = NULL;
    lb->position = NULL;
    lb->width = -1;
    lb->height = -1;
    
    gtk_label_set_text(GTK_LABEL(lb->label), lb->text);
    
    return lb;
}

void update_label(label *lb) {
    if (!lb || !lb->label) return;
    
    if (lb->text) {
        gtk_label_set_text(GTK_LABEL(lb->label), lb->text);
    }
    
    if (lb->width > 0 && lb->height > 0) {
        gtk_widget_set_size_request(lb->label, lb->width, lb->height);
    }
    
    GString *css = g_string_new("label { ");
    if (lb->color) {
        g_string_append_printf(css, "color: %s; ", lb->color);
    }
    if (lb->bg_color) {
        g_string_append_printf(css, "background-color: %s; ", lb->bg_color);
    }
    if (lb->font_family) {
        g_string_append_printf(css, "font-family: '%s'; ", lb->font_family);
    }
    g_string_append(css, "}");
    
    apply_css(lb->label, css->str);
    g_string_free(css, TRUE);
    
    if (lb->style) {
        PangoAttrList *attrs = pango_attr_list_new();
        if (strstr(lb->style, "bold")) {
            pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        }
        if (strstr(lb->style, "italic")) {
            pango_attr_list_insert(attrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
        }
        if (strstr(lb->style, "underline")) {
            pango_attr_list_insert(attrs, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
        }
        gtk_label_set_attributes(GTK_LABEL(lb->label), attrs);
        pango_attr_list_unref(attrs);
    }
    
    if (lb->position) {
        if (strcmp(lb->position, "left") == 0) {
            gtk_label_set_xalign(GTK_LABEL(lb->label), 0.0);
        } else if (strcmp(lb->position, "right") == 0) {
            gtk_label_set_xalign(GTK_LABEL(lb->label), 1.0);
        } else if (strcmp(lb->position, "center") == 0) {
            gtk_label_set_xalign(GTK_LABEL(lb->label), 0.5);
        }
    }
}

/* ---------- Progress Bar Functions ---------- */

progress_bar* create_progress_bar() {
    progress_bar *pb = malloc(sizeof(progress_bar));
    if (!pb) exit(1);
    
    pb->progress = gtk_progress_bar_new();
    pb->fraction = 0.0;
    pb->text = NULL;
    pb->pulse_text = NULL;
    pb->width = -1;
    pb->height = -1;
    pb->inverted = FALSE;
    pb->orientation = GTK_ORIENTATION_HORIZONTAL;
    pb->show_text = FALSE;
    pb->is_pulsing = FALSE;
    pb->pulse_interval = 100;
    pb->pulse_timeout_id = 0;
    
    return pb;
}

static gboolean pulse_callback(gpointer data) {
    progress_bar *pb = (progress_bar*)data;
    if (pb && pb->is_pulsing && pb->progress) {
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pb->progress));
        return TRUE;
    }
    return FALSE;
}

void update_progress_bar(progress_bar *pb) {
    if (!pb || !pb->progress) return;
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb->progress), pb->fraction);
    
    if (pb->text) {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pb->progress), pb->text);
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(pb->progress), TRUE);
    }
    
    if (pb->width > 0 && pb->height > 0) {
        gtk_widget_set_size_request(pb->progress, pb->width, pb->height);
    }
    
    gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(pb->progress), pb->inverted);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(pb->progress), pb->orientation);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(pb->progress), pb->show_text);
}

void start_progress_pulse(progress_bar *pb) {
    if (!pb) return;
    pb->is_pulsing = TRUE;
    if (pb->pulse_timeout_id) {
        g_source_remove(pb->pulse_timeout_id);
    }
    pb->pulse_timeout_id = g_timeout_add(pb->pulse_interval, pulse_callback, pb);
}

void stop_progress_pulse(progress_bar *pb) {
    if (!pb) return;
    pb->is_pulsing = FALSE;
    if (pb->pulse_timeout_id) {
        g_source_remove(pb->pulse_timeout_id);
        pb->pulse_timeout_id = 0;
    }
}

void set_progress_value(progress_bar *pb, double value) {
    if (!pb) return;
    pb->fraction = value < 0 ? 0 : (value > 1 ? 1 : value);
    if (pb->progress) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb->progress), pb->fraction);
    }
}

/* ---------- Spinner Functions ---------- */

spinner* create_spinner() {
    spinner *spin = malloc(sizeof(spinner));
    if (!spin) exit(1);
    
    spin->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    spin->spinner = gtk_spinner_new();
    spin->label = gtk_label_new("");
    spin->text = NULL;
    spin->width = -1;
    spin->height = -1;
    spin->active = FALSE;
    spin->spinning = FALSE;
    
    gtk_box_pack_start(GTK_BOX(spin->container), spin->spinner, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(spin->container), spin->label, FALSE, FALSE, 0);
    gtk_widget_show_all(spin->container);
    
    return spin;
}

void update_spinner(spinner *spin) {
    if (!spin) return;
    
    if (spin->text) {
        gtk_label_set_text(GTK_LABEL(spin->label), spin->text);
    }
    
    if (spin->width > 0 && spin->height > 0) {
        gtk_widget_set_size_request(spin->container, spin->width, spin->height);
    }
    
    if (spin->active && !spin->spinning) {
        start_spinner(spin);
    } else if (!spin->active && spin->spinning) {
        stop_spinner(spin);
    }
}

void start_spinner(spinner *spin) {
    if (!spin || !spin->spinner) return;
    spin->active = TRUE;
    spin->spinning = TRUE;
    gtk_spinner_start(GTK_SPINNER(spin->spinner));
}

void stop_spinner(spinner *spin) {
    if (!spin || !spin->spinner) return;
    spin->active = FALSE;
    spin->spinning = FALSE;
    gtk_spinner_stop(GTK_SPINNER(spin->spinner));
}

GtkWidget* get_spinner_widget(spinner *spin) {
    return spin ? spin->container : NULL;
}

/* ---------- Image Functions ---------- */

image* create_image() {
    image *im = malloc(sizeof(image));
    if (!im) exit(1);
    
    im->image = gtk_image_new();
    im->file_name = NULL;
    im->animation_file = NULL;
    im->icon_name = NULL;
    im->position = NULL;
    im->border_color = NULL;
    im->width = -1;
    im->height = -1;
    im->border_width = 0;
    im->border_radius = 0;
    im->pixel_size = -1;
    im->click_callback = NULL;
    im->callback_data = NULL;
    
    return im;
}

void update_image(image *im) {
    if (!im || !im->image) return;
    
    if (im->file_name) {
        GError *error = NULL;
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(im->file_name, &error);
        if (pixbuf) {
            if (im->width > 0 && im->height > 0) {
                GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, im->width, im->height, 
                                                            GDK_INTERP_BILINEAR);
                gtk_image_set_from_pixbuf(GTK_IMAGE(im->image), scaled);
                g_object_unref(scaled);
            } else {
                gtk_image_set_from_pixbuf(GTK_IMAGE(im->image), pixbuf);
            }
            g_object_unref(pixbuf);
        }
    }
    
    if (im->width > 0 && im->height > 0) {
        gtk_widget_set_size_request(im->image, im->width, im->height);
    }
    
    if (im->border_width > 0 || im->border_radius > 0 || im->border_color) {
        GString *css = g_string_new("image { ");
        if (im->border_width > 0) {
            g_string_append_printf(css, "border-width: %dpx; border-style: solid; ", im->border_width);
        }
        if (im->border_radius > 0) {
            g_string_append_printf(css, "border-radius: %dpx; ", im->border_radius);
        }
        if (im->border_color) {
            g_string_append_printf(css, "border-color: %s; ", im->border_color);
        }
        g_string_append(css, "}");
        apply_css(im->image, css->str);
        g_string_free(css, TRUE);
    }
    
    if (im->position) {
        if (strcmp(im->position, "left") == 0) {
            gtk_widget_set_halign(im->image, GTK_ALIGN_START);
        } else if (strcmp(im->position, "right") == 0) {
            gtk_widget_set_halign(im->image, GTK_ALIGN_END);
        } else if (strcmp(im->position, "center") == 0) {
            gtk_widget_set_halign(im->image, GTK_ALIGN_CENTER);
        }
    }
    
    if (im->click_callback) {
        g_signal_connect(im->image, "button-press-event", G_CALLBACK(im->click_callback), im->callback_data);
    }
}

/* ---------- Entry Functions ---------- */

entry* create_entry() {
    entry *en = malloc(sizeof(entry));
    if (!en) exit(1);
    
    en->entry = gtk_entry_new();
    en->text = NULL;
    en->placeholder_text = NULL;
    en->bg_color = NULL;
    en->color = NULL;
    en->font_family = NULL;
    en->label_style = NULL;
    en->width = -1;
    en->height = -1;
    en->border_width = 0;
    en->margin_top = en->margin_bottom = en->margin_left = en->margin_right = 0;
    en->visibility = TRUE;
    en->editable = TRUE;
    en->has_frame = TRUE;
    en->changed_callback = NULL;
    en->callback_data = NULL;
    
    return en;
}

void update_entry(entry *en) {
    if (!en || !en->entry) return;
    
    if (en->text) {
        gtk_entry_set_text(GTK_ENTRY(en->entry), en->text);
    }
    
    if (en->placeholder_text) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(en->entry), en->placeholder_text);
    }
    
    if (en->width > 0 && en->height > 0) {
        gtk_widget_set_size_request(en->entry, en->width, en->height);
    }
    
    gtk_entry_set_visibility(GTK_ENTRY(en->entry), en->visibility);
    gtk_editable_set_editable(GTK_EDITABLE(en->entry), en->editable);
    gtk_entry_set_has_frame(GTK_ENTRY(en->entry), en->has_frame);
    
    GString *css = g_string_new("entry { ");
    if (en->bg_color) {
        g_string_append_printf(css, "background-color: %s; ", en->bg_color);
    }
    if (en->color) {
        g_string_append_printf(css, "color: %s; ", en->color);
    }
    if (en->font_family) {
        g_string_append_printf(css, "font-family: '%s'; ", en->font_family);
    }
    if (en->border_width > 0) {
        g_string_append_printf(css, "border-width: %dpx; border-style: solid; ", en->border_width);
    }
    if (en->margin_top > 0 || en->margin_bottom > 0 || en->margin_left > 0 || en->margin_right > 0) {
        g_string_append_printf(css, "margin-top: %dpx; margin-bottom: %dpx; ", 
                               en->margin_top, en->margin_bottom);
        g_string_append_printf(css, "margin-left: %dpx; margin-right: %dpx; ", 
                               en->margin_left, en->margin_right);
    }
    g_string_append(css, "}");
    apply_css(en->entry, css->str);
    g_string_free(css, TRUE);
    
    if (en->changed_callback) {
        g_signal_connect(en->entry, "changed", G_CALLBACK(en->changed_callback), en->callback_data);
    }
}

char* get_entry_text(entry *en) {
    if (!en || !en->entry) return NULL;
    return (char*)gtk_entry_get_text(GTK_ENTRY(en->entry));
}

void set_entry_text(entry *en, const char *text) {
    if (!en || !en->entry) return;
    gtk_entry_set_text(GTK_ENTRY(en->entry), text);
}

/* ---------- Radio Button Functions ---------- */

typedef struct RadioGroupNode {
    char *group_id;
    GSList *group;
    struct RadioGroupNode *next;
} RadioGroupNode;

static RadioGroupNode *radio_groups = NULL;

static GSList* get_or_create_radio_group(const char *group_id, GtkWidget *first_button) {
    RadioGroupNode *current = radio_groups;
    while (current) {
        if (strcmp(current->group_id, group_id) == 0) {
            return current->group;
        }
        current = current->next;
    }
    
    RadioGroupNode *new_group = malloc(sizeof(RadioGroupNode));
    new_group->group_id = g_strdup(group_id);
    new_group->group = first_button ? gtk_radio_button_get_group(GTK_RADIO_BUTTON(first_button)) : NULL;
    new_group->next = radio_groups;
    radio_groups = new_group;
    
    return new_group->group;
}

static void add_to_radio_group(const char *group_id, GtkWidget *button) {
    RadioGroupNode *current = radio_groups;
    while (current) {
        if (strcmp(current->group_id, group_id) == 0) {
            if (current->group) {
                gtk_radio_button_set_group(GTK_RADIO_BUTTON(button), current->group);
                current->group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
            }
            return;
        }
        current = current->next;
    }
}

radio_button* create_radio_button(const char *group_id, const char *label) {
    radio_button *rdb = malloc(sizeof(radio_button));
    if (!rdb) exit(1);
    
    rdb->label = g_strdup(label);
    rdb->group_id = g_strdup(group_id);
    rdb->color = NULL;
    rdb->bg_color = NULL;
    rdb->width = -1;
    rdb->height = -1;
    rdb->position = NULL;
    rdb->active = FALSE;
    rdb->mode = TRUE;
    rdb->toggled_callback = NULL;
    rdb->callback_data = NULL;
    
    RadioGroupNode *existing = radio_groups;
    while (existing) {
        if (strcmp(existing->group_id, group_id) == 0) {
            break;
        }
        existing = existing->next;
    }
    
    if (!existing || !existing->group) {
        rdb->radio_button = gtk_radio_button_new_with_label(NULL, label);
        get_or_create_radio_group(group_id, rdb->radio_button);
    } else {
        rdb->radio_button = gtk_radio_button_new_with_label(existing->group, label);
        existing->group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(rdb->radio_button));
    }
    
    return rdb;
}

// Add to structures.c
void cairo_ellipse(cairo_t *cr, double x, double y, double rx, double ry) {
    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_scale(cr, 1.0, ry / rx);
    cairo_arc(cr, 0, 0, rx, 0, 2 * M_PI);
    cairo_restore(cr);
}

void update_radio_button(radio_button *rdb) {
    if (!rdb || !rdb->radio_button) return;
    
    if (rdb->label) {
        gtk_button_set_label(GTK_BUTTON(rdb->radio_button), rdb->label);
    }
    
    if (rdb->width > 0 && rdb->height > 0) {
        gtk_widget_set_size_request(rdb->radio_button, rdb->width, rdb->height);
    }
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rdb->radio_button), rdb->active);
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(rdb->radio_button), rdb->mode);
    
    if (rdb->position) {
        if (strcmp(rdb->position, "left") == 0) {
            gtk_widget_set_halign(rdb->radio_button, GTK_ALIGN_START);
        } else if (strcmp(rdb->position, "right") == 0) {
            gtk_widget_set_halign(rdb->radio_button, GTK_ALIGN_END);
        } else if (strcmp(rdb->position, "center") == 0) {
            gtk_widget_set_halign(rdb->radio_button, GTK_ALIGN_CENTER);
        }
    }
    
    GString *css = g_string_new("radio { ");
    if (rdb->color) {
        g_string_append_printf(css, "color: %s; ", rdb->color);
    }
    if (rdb->bg_color) {
        g_string_append_printf(css, "background-color: %s; ", rdb->bg_color);
    }
    g_string_append(css, "}");
    apply_css(rdb->radio_button, css->str);
    g_string_free(css, TRUE);
    
    if (rdb->toggled_callback) {
        g_signal_connect(rdb->radio_button, "toggled", 
                        G_CALLBACK(rdb->toggled_callback), rdb->callback_data);
    }
}

void set_radio_button_active(radio_button *rdb, gboolean active) {
    if (!rdb) return;
    rdb->active = active;
    if (rdb->radio_button) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rdb->radio_button), active);
    }
}

gboolean is_radio_button_active(radio_button *rdb) {
    if (!rdb || !rdb->radio_button) return FALSE;
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdb->radio_button));
}

/* ---------- Check Button Functions ---------- */

check_button* create_check_button() {
    check_button *chb = malloc(sizeof(check_button));
    if (!chb) exit(1);
    
    chb->checkbutton = gtk_check_button_new();
    chb->label = NULL;
    chb->color = NULL;
    chb->bg_color = NULL;
    chb->font_family = NULL;
    chb->label_style = NULL;
    chb->width = -1;
    chb->height = -1;
    chb->position = NULL;
    chb->active = FALSE;
    chb->mode = TRUE;
    chb->inconsistent = FALSE;
    chb->draw_indicator = TRUE;
    chb->toggled_callback = NULL;
    chb->callback_data = NULL;
    
    return chb;
}

void update_check_button(check_button *chb) {
    if (!chb || !chb->checkbutton) return;
    
    if (chb->label) {
        gtk_button_set_label(GTK_BUTTON(chb->checkbutton), chb->label);
    }
    
    if (chb->width > 0 && chb->height > 0) {
        gtk_widget_set_size_request(chb->checkbutton, chb->width, chb->height);
    }
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chb->checkbutton), chb->active);
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(chb->checkbutton), chb->mode);
    gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(chb->checkbutton), chb->inconsistent);
    
    if (chb->position) {
        if (strcmp(chb->position, "left") == 0) {
            gtk_widget_set_halign(chb->checkbutton, GTK_ALIGN_START);
        } else if (strcmp(chb->position, "right") == 0) {
            gtk_widget_set_halign(chb->checkbutton, GTK_ALIGN_END);
        } else if (strcmp(chb->position, "center") == 0) {
            gtk_widget_set_halign(chb->checkbutton, GTK_ALIGN_CENTER);
        }
    }
    
    GString *css = g_string_new("checkbutton { ");
    if (chb->bg_color) {
        g_string_append_printf(css, "background-color: %s; ", chb->bg_color);
    }
    g_string_append(css, "}");
    apply_css(chb->checkbutton, css->str);
    g_string_free(css, TRUE);
    
    if (chb->color) {
        GtkWidget *label_widget = gtk_bin_get_child(GTK_BIN(chb->checkbutton));
        if (label_widget && GTK_IS_LABEL(label_widget)) {
            char css2[256];
            snprintf(css2, sizeof(css2), "label { color: %s; }", chb->color);
            apply_css(label_widget, css2);
        }
    }
    
    if (chb->font_family) {
        GtkWidget *label_widget = gtk_bin_get_child(GTK_BIN(chb->checkbutton));
        if (label_widget && GTK_IS_LABEL(label_widget)) {
            char css2[256];
            snprintf(css2, sizeof(css2), "label { font-family: '%s'; }", chb->font_family);
            apply_css(label_widget, css2);
        }
    }
    
    if (chb->toggled_callback) {
        g_signal_connect(chb->checkbutton, "toggled", 
                        G_CALLBACK(chb->toggled_callback), chb->callback_data);
    }
}

void set_check_button_active(check_button *chb, gboolean active) {
    if (!chb) return;
    chb->active = active;
    if (chb->checkbutton) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chb->checkbutton), active);
    }
}

gboolean is_check_button_active(check_button *chb) {
    if (!chb || !chb->checkbutton) return FALSE;
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chb->checkbutton));
}

/* ---------- Menu Functions ---------- */

menu* create_horizontal_menu() {
    menu *m = malloc(sizeof(menu));
    if (!m) exit(1);
    
    m->menu_widget = gtk_menu_bar_new();
    m->orientation = GTK_ORIENTATION_HORIZONTAL;
    m->bg_color = NULL;
    m->text_color = NULL;
    m->item_spacing = 0;
    
    return m;
}

menu* create_vertical_menu() {
    menu *m = malloc(sizeof(menu));
    if (!m) exit(1);
    
    m->menu_widget = gtk_menu_new();
    m->orientation = GTK_ORIENTATION_VERTICAL;
    m->bg_color = NULL;
    m->text_color = NULL;
    m->item_spacing = 0;
    
    return m;
}

menu* create_submenu() {
    return create_vertical_menu();
}

void add_menu_item(menu *m, const char *label, void (*callback)(GtkWidget*, gpointer), void *data) {
    if (!m || !m->menu_widget) return;
    
    GtkWidget *item = gtk_menu_item_new_with_label(label);
    g_signal_connect(item, "activate", G_CALLBACK(callback), data);
    gtk_menu_shell_append(GTK_MENU_SHELL(m->menu_widget), item);
    gtk_widget_show(item);
}

void add_menu_item_with_icon(menu *m, const char *label, const char *icon_name,
                              void (*callback)(GtkWidget*, gpointer), void *data) {
    if (!m || !m->menu_widget) return;
    
    GtkWidget *item = gtk_menu_item_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
    GtkWidget *lbl = gtk_label_new(label);
    
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);
    gtk_widget_show_all(hbox);
    gtk_container_add(GTK_CONTAINER(item), hbox);
    
    g_signal_connect(item, "activate", G_CALLBACK(callback), data);
    gtk_menu_shell_append(GTK_MENU_SHELL(m->menu_widget), item);
    gtk_widget_show(item);
}

void add_check_menu_item(menu *m, const char *label, gboolean active,
                          void (*callback)(GtkWidget*, gpointer), void *data) {
    if (!m || !m->menu_widget) return;
    
    GtkWidget *item = gtk_check_menu_item_new_with_label(label);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), active);
    g_signal_connect(item, "toggled", G_CALLBACK(callback), data);
    gtk_menu_shell_append(GTK_MENU_SHELL(m->menu_widget), item);
    gtk_widget_show(item);
}

void add_radio_menu_item(menu *m, const char *label, gboolean active,
                          void (*callback)(GtkWidget*, gpointer), void *data) {
    if (!m || !m->menu_widget) return;
    
    static GSList *radio_group = NULL;
    GtkWidget *item = gtk_radio_menu_item_new_with_label(radio_group, label);
    radio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), active);
    g_signal_connect(item, "toggled", G_CALLBACK(callback), data);
    gtk_menu_shell_append(GTK_MENU_SHELL(m->menu_widget), item);
    gtk_widget_show(item);
}

void add_menu_separator(menu *m) {
    if (!m || !m->menu_widget) return;
    
    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(m->menu_widget), sep);
    gtk_widget_show(sep);
}

void add_submenu(menu *parent, const char *label, menu *submenu) {
    if (!parent || !parent->menu_widget || !submenu || !submenu->menu_widget) return;
    
    GtkWidget *item = gtk_menu_item_new_with_label(label);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu->menu_widget);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent->menu_widget), item);
    gtk_widget_show(item);
}

void add_submenu_with_icon(menu *parent, const char *label, const char *icon_name, menu *submenu) {
    if (!parent || !parent->menu_widget || !submenu || !submenu->menu_widget) return;
    
    GtkWidget *item = gtk_menu_item_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
    GtkWidget *lbl = gtk_label_new(label);
    
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);
    gtk_widget_show_all(hbox);
    gtk_container_add(GTK_CONTAINER(item), hbox);
    
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu->menu_widget);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent->menu_widget), item);
    gtk_widget_show(item);
}

menu_button* create_menu_button(const char *label) {
    menu_button *mb = malloc(sizeof(menu_button));
    if (!mb) exit(1);
    
    mb->button = gtk_button_new_with_label(label);
    mb->label = g_strdup(label);
    mb->icon_name = NULL;
    mb->main_menu = NULL;
    mb->width = -1;
    mb->height = -1;
    mb->bg_color = NULL;
    mb->text_color = NULL;
    mb->position = NULL;
    
    return mb;
}

menu_button* create_menu_button_with_icon(const char *icon_name) {
    menu_button *mb = malloc(sizeof(menu_button));
    if (!mb) exit(1);
    
    mb->button = gtk_button_new();
    mb->label = NULL;
    mb->icon_name = g_strdup(icon_name);
    mb->main_menu = NULL;
    mb->width = -1;
    mb->height = -1;
    mb->bg_color = NULL;
    mb->text_color = NULL;
    mb->position = NULL;
    
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(mb->button), icon);
    gtk_widget_show(icon);
    
    return mb;
}

void set_menu_button_menu(menu_button *mb, menu *m) {
    if (!mb || !mb->button || !m || !m->menu_widget) return;
    
    mb->main_menu = m;
    
    GtkWidget *menu_button = gtk_menu_button_new();
    
    if (GTK_IS_MENU(m->menu_widget)) {
        gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_button), m->menu_widget);
    }
    
    if (mb->label) {
        gtk_button_set_label(GTK_BUTTON(menu_button), mb->label);
    }
    if (mb->icon_name) {
        GtkWidget *icon = gtk_image_new_from_icon_name(mb->icon_name, GTK_ICON_SIZE_BUTTON);
        gtk_container_add(GTK_CONTAINER(menu_button), icon);
        gtk_widget_show(icon);
    }
    
    if (mb->bg_color) {
        char css[256];
        snprintf(css, sizeof(css), "button { background-color: %s; }", mb->bg_color);
        apply_css(menu_button, css);
    }
    
    GtkWidget *old_button = mb->button;
    mb->button = menu_button;
    gtk_widget_destroy(old_button);
}

void update_menu_button(menu_button *mb) {
    if (!mb || !mb->button) return;
    
    if (mb->width > 0 && mb->height > 0) {
        gtk_widget_set_size_request(mb->button, mb->width, mb->height);
    }
    
    GString *css = g_string_new("button { ");
    if (mb->bg_color) {
        g_string_append_printf(css, "background-color: %s; ", mb->bg_color);
    }
    if (mb->text_color) {
        g_string_append_printf(css, "color: %s; ", mb->text_color);
    }
    g_string_append(css, "}");
    apply_css(mb->button, css->str);
    g_string_free(css, TRUE);
    
    if (mb->position) {
        if (strcmp(mb->position, "left") == 0) {
            gtk_widget_set_halign(mb->button, GTK_ALIGN_START);
        } else if (strcmp(mb->position, "right") == 0) {
            gtk_widget_set_halign(mb->button, GTK_ALIGN_END);
        } else if (strcmp(mb->position, "center") == 0) {
            gtk_widget_set_halign(mb->button, GTK_ALIGN_CENTER);
        }
    }
}

void add_menu_button_to_box(box *bx, menu_button *mb) {
    if (!bx || !mb) return;
    add_to_box(bx, mb->button);
}

void free_menu(menu *m) {
    if (m) {
        if (m->menu_widget) {
            gtk_widget_destroy(m->menu_widget);
        }
        if (m->bg_color) g_free(m->bg_color);
        if (m->text_color) g_free(m->text_color);
        free(m);
    }
}

void free_menu_button(menu_button *mb) {
    if (mb) {
        if (mb->button) gtk_widget_destroy(mb->button);
        if (mb->label) g_free(mb->label);
        if (mb->icon_name) g_free(mb->icon_name);
        if (mb->bg_color) g_free(mb->bg_color);
        if (mb->text_color) g_free(mb->text_color);
        if (mb->position) g_free(mb->position);
        free(mb);
    }
}

/* ---------- Box Functions ---------- */

box* create_box(GtkOrientation orientation, int spacing, gboolean homogeneous) {
    box *bx = malloc(sizeof(box));
    if (!bx) exit(1);
    
    bx->box = gtk_box_new(orientation, spacing);
    bx->orientation = orientation;
    bx->spacing = spacing;
    bx->width = -1;
    bx->height = -1;
    bx->border_width = 0;
    bx->bg_color = NULL;
    bx->margin_top = bx->margin_bottom = bx->margin_left = bx->margin_right = 0;
    bx->homogeneous = homogeneous;
    bx->expand = FALSE;
    bx->fill = TRUE;
    bx->padding = 0;
    
    gtk_box_set_homogeneous(GTK_BOX(bx->box), homogeneous);
    
    return bx;
}

box* create_hbox(int spacing, gboolean homogeneous) {
    return create_box(GTK_ORIENTATION_HORIZONTAL, spacing, homogeneous);
}

box* create_vbox(int spacing, gboolean homogeneous) {
    return create_box(GTK_ORIENTATION_VERTICAL, spacing, homogeneous);
}

void update_box(box *bx) {
    if (!bx || !bx->box) return;
    
    if (bx->width > 0 && bx->height > 0) {
        gtk_widget_set_size_request(bx->box, bx->width, bx->height);
    }
    
    gtk_container_set_border_width(GTK_CONTAINER(bx->box), bx->border_width);
    gtk_box_set_spacing(GTK_BOX(bx->box), bx->spacing);
    gtk_box_set_homogeneous(GTK_BOX(bx->box), bx->homogeneous);
    
    if (bx->bg_color) {
        char css[256];
        snprintf(css, sizeof(css), "box { background-color: %s; }", bx->bg_color);
        apply_css(bx->box, css);
    }
    
    if (bx->margin_top > 0 || bx->margin_bottom > 0 || bx->margin_left > 0 || bx->margin_right > 0) {
        char css[256];
        snprintf(css, sizeof(css), 
                 "box { margin-top: %dpx; margin-bottom: %dpx; margin-left: %dpx; margin-right: %dpx; }",
                 bx->margin_top, bx->margin_bottom, bx->margin_left, bx->margin_right);
        apply_css(bx->box, css);
    }
}

void add_to_box(box *bx, GtkWidget *widget) {
    if (!bx || !bx->box || !widget) return;
    gtk_box_pack_start(GTK_BOX(bx->box), widget, bx->expand, bx->fill, bx->padding);
    gtk_widget_show(widget);
}

void add_to_box_with_properties(box *bx, GtkWidget *widget, gboolean expand, gboolean fill, guint padding) {
    if (!bx || !bx->box || !widget) return;
    gtk_box_pack_start(GTK_BOX(bx->box), widget, expand, fill, padding);
    gtk_widget_show(widget);
}

void clear_box(box *bx) {
    if (!bx || !bx->box) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(bx->box));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

GtkWidget* get_box_widget(box *bx) {
    return bx ? bx->box : NULL;
}

/* ---------- Container Addition Functions ---------- */

void add_to_window(window *wn, GtkWidget *widget) {
    if (wn && wn->win && widget) {
        gtk_container_add(GTK_CONTAINER(wn->win), widget);
    }
}

void add_to_header_bar_start(header_bar *hdb, GtkWidget *widget) {
    if (hdb && hdb->header_bar && widget) {
        gtk_header_bar_pack_start(GTK_HEADER_BAR(hdb->header_bar), widget);
    }
}

void add_to_header_bar_end(header_bar *hdb, GtkWidget *widget) {
    if (hdb && hdb->header_bar && widget) {
        gtk_header_bar_pack_end(GTK_HEADER_BAR(hdb->header_bar), widget);
    }
}


