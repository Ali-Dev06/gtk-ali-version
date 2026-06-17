// button_manager.c
#include <gmodule.h>
#include "button_manager.h"
#include "aquarium.h"
#include "fish_catalog.h"
#include <stdlib.h>
#include <string.h>

/* ── Internal state ── */
static GtkWidget       *g_panel_box       = NULL;
static GtkWidget       *g_highlight_toggle = NULL;
static DynamicButton   *g_btn_list_head   = NULL;
static DynamicButton   *g_btn_list_tail   = NULL;

/* ── Action metadata ── */
const char *action_display_name(DynamicButtonAction a)
{
    static const char *names[BTN_ACTION_COUNT] = {
        "Add Fish from Library",
        "Add Prey Fish (N)",
        "Add Predators (N)",
        "Open New Aquarium (clear + N fish)",
        "Clear & Add New Fish",
        "Clear All Fish",
        "Feed Fish (12 pellets)",
        "Spawn Food Pellets (N)",
        "Pause / Resume",
        "Toggle Grid",
        "Toggle Fish Highlight",
        "Load Default Reef",
        "Add Mixed School (prey + predators)",
        "Save Session (HTML)",
        "Load Session (HTML)",
        "Kill All Predators",
        "Kill All Prey",
        "Boost All Fish Speed",
        "Reset All Fish Health",
        "Add Specific Species (N)",
        "Custom C Function (by name)",
        "Custom Script/Commands"
    };
    if (a < 0 || a >= BTN_ACTION_COUNT) return "Unknown";
    return names[a];
}

gboolean action_needs_count(DynamicButtonAction a)
{
    switch (a) {
        case BTN_ACTION_ADD_LIBRARY:
        case BTN_ACTION_ADD_PREY:
        case BTN_ACTION_ADD_PREDATORS:
        case BTN_ACTION_NEW_AQUARIUM:
        case BTN_ACTION_CLEAR_AND_ADD:
        case BTN_ACTION_SPAWN_FOOD_N:
        case BTN_ACTION_MIXED_SCHOOL:
        case BTN_ACTION_ADD_SPECIES:
            return TRUE;
        default:
            return FALSE;
    }
}

gboolean action_needs_count2(DynamicButtonAction a)
{
    return (a == BTN_ACTION_MIXED_SCHOOL);
}

gboolean action_needs_preset(DynamicButtonAction a)
{
    switch (a) {
        case BTN_ACTION_ADD_LIBRARY:
        case BTN_ACTION_NEW_AQUARIUM:
        case BTN_ACTION_CLEAR_AND_ADD:
        case BTN_ACTION_ADD_SPECIES:
            return TRUE;
        default:
            return FALSE;
    }
}

static gboolean action_needs_custom_func(DynamicButtonAction a)
{
    return (a == BTN_ACTION_CUSTOM_C_FUNC);
}

static gboolean action_needs_custom_script(DynamicButtonAction a)
{
    return (a == BTN_ACTION_CUSTOM_SCRIPT);
}

/* ── Initialise ── */
void init_dynamic_buttons(GtkWidget *panel_box, GtkWidget *highlight_toggle)
{
    g_panel_box       = panel_box;
    g_highlight_toggle = highlight_toggle;
}

static void execute_custom_script(const char *script)
{
    if (!script || !g_aquarium) return;
    char *script_copy = g_strdup(script);
    char **commands = g_strsplit(script_copy, ";", -1);
    for (int i = 0; commands[i] != NULL; i++) {
        char *cmd = g_strdup(commands[i]);
        g_strstrip(cmd); // trims leading/trailing whitespace
        if (strlen(cmd) == 0) {
            g_free(cmd);
            continue;
        }

        if (g_ascii_strcasecmp(cmd, "clear") == 0 || g_ascii_strcasecmp(cmd, "clean") == 0) {
            on_clear_all_no_confirm();
        } else if (g_ascii_strncasecmp(cmd, "feed", 4) == 0) {
            int count = 12;
            sscanf(cmd + 4, "%d", &count);
            spawn_food_pellets(count, g_aquarium->width / 2.0f, 60.0f);
        } else if (g_ascii_strncasecmp(cmd, "spawn", 5) == 0) {
            int count = 1;
            char species[128] = "";
            if (sscanf(cmd + 5, "%d %127[^\n]", &count, species) >= 1) {
                g_strstrip(species);
                const PresetFish *preset = NULL;
                if (species[0]) {
                    // Try to parse species as integer index first
                    char *endptr;
                    int idx = (int)strtol(species, &endptr, 10);
                    if (*endptr == '\0') {
                        preset = preset_fish_get(idx);
                    } else {
                        // Look up by name (case-insensitive)
                        for (int j = 0; j < preset_fish_count(); j++) {
                            const PresetFish *p = preset_fish_get(j);
                            if (p && g_ascii_strcasecmp(p->species, species) == 0) {
                                preset = p;
                                break;
                            }
                        }
                    }
                }
                if (!preset) {
                    // Default fallback
                    preset = preset_fish_get(0);
                }
                if (preset) {
                    spawn_fish_from_preset(preset, count, 0, 0, preset->type, 0);
                }
            }
        } else if (g_ascii_strcasecmp(cmd, "pause") == 0) {
            g_aquarium->paused = !g_aquarium->paused;
        } else if (g_ascii_strcasecmp(cmd, "grid") == 0) {
            g_aquarium->show_grid = !g_aquarium->show_grid;
        } else if (g_ascii_strcasecmp(cmd, "highlight") == 0) {
            g_aquarium->show_selected_highlight = !g_aquarium->show_selected_highlight;
            if (g_highlight_toggle) {
                gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(g_highlight_toggle),
                    g_aquarium->show_selected_highlight);
            }
        } else if (g_ascii_strncasecmp(cmd, "speed", 5) == 0) {
            float factor = 2.0f;
            sscanf(cmd + 5, "%f", &factor);
            Fish *f = g_aquarium->fish_list;
            while (f) {
                if (f->is_alive) {
                    f->speed = fminf(f->speed * factor, MAX_SPEED);
                    f->base_speed = fminf(f->base_speed * factor, MAX_SPEED);
                }
                f = f->next;
            }
        } else if (g_ascii_strcasecmp(cmd, "health") == 0) {
            Fish *f = g_aquarium->fish_list;
            while (f) {
                if (f->is_alive) {
                    f->health = 100.0f;
                    f->hunger = 0.0f;
                    f->energy = 100.0f;
                }
                f = f->next;
            }
        }
        g_free(cmd);
    }
    g_strfreev(commands);
    g_free(script_copy);
}

/* ── Execute action ── */
void execute_dynamic_button_action(DynamicButton *btn)
{
    if (!btn || !g_aquarium) return;

    int   count  = btn->param_count  > 0 ? btn->param_count  : 5;
    int   count2 = btn->param_count2 > 0 ? btn->param_count2 : 2;
    int   pidx   = btn->param_preset_idx;

    const PresetFish *preset = preset_fish_get(pidx);

    switch (btn->action) {

    case BTN_ACTION_ADD_LIBRARY:
        if (preset)
            spawn_fish_from_preset(preset, count, 0, 0, preset->type, 0);
        break;

    case BTN_ACTION_ADD_PREY:
        for (int i = 0; i < count; i++) {
            int ri = rand() % 12; /* indices 0-11 are prey in catalog */
            const PresetFish *p = preset_fish_get(ri);
            if (p) spawn_fish_from_preset(p, 1, 0, 0, TYPE_PREY, 1);
        }
        break;

    case BTN_ACTION_ADD_PREDATORS:
        for (int i = 0; i < count; i++) {
            int ri = 12 + rand() % 4; /* indices 12-15 are predators */
            const PresetFish *p = preset_fish_get(ri);
            if (p) spawn_fish_from_preset(p, 1, 0, 0, TYPE_PREDATOR, 1);
        }
        break;

    case BTN_ACTION_NEW_AQUARIUM:
        on_clear_all_no_confirm();
        if (preset)
            spawn_fish_from_preset(preset, count, 0, 0, preset->type, 0);
        else {
            /* fallback: random mix */
            spawn_fish_from_preset(preset_fish_get(0), count, 0, 0, TYPE_PREY, 0);
        }
        break;

    case BTN_ACTION_CLEAR_AND_ADD:
        on_clear_all_no_confirm();
        if (preset)
            spawn_fish_from_preset(preset, count, 0, 0, preset->type, 0);
        break;

    case BTN_ACTION_CLEAR_ALL:
        on_clear_all_no_confirm();
        break;

    case BTN_ACTION_FEED:
        spawn_food_pellets(12, g_aquarium->width / 2.0f, 60.0f);
        break;

    case BTN_ACTION_SPAWN_FOOD_N:
        spawn_food_pellets(count, g_aquarium->width / 2.0f, 60.0f);
        break;

    case BTN_ACTION_PAUSE_RESUME:
        g_aquarium->paused = !g_aquarium->paused;
        break;

    case BTN_ACTION_TOGGLE_GRID:
        g_aquarium->show_grid = !g_aquarium->show_grid;
        break;

    case BTN_ACTION_TOGGLE_HIGHLIGHT:
        g_aquarium->show_selected_highlight = !g_aquarium->show_selected_highlight;
        if (g_highlight_toggle) {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(g_highlight_toggle),
                g_aquarium->show_selected_highlight);
        }
        break;

    case BTN_ACTION_LOAD_DEFAULT:
        on_load_default_clicked(NULL, NULL);
        break;

    case BTN_ACTION_MIXED_SCHOOL:
        for (int i = 0; i < count; i++) {
            const PresetFish *p = preset_fish_get(rand() % 12);
            if (p) spawn_fish_from_preset(p, 1, 0, 0, TYPE_PREY, 1);
        }
        for (int i = 0; i < count2; i++) {
            const PresetFish *p = preset_fish_get(12 + rand() % 4);
            if (p) spawn_fish_from_preset(p, 1, 0, 0, TYPE_PREDATOR, 1);
        }
        break;

    case BTN_ACTION_SAVE_HTML:
        on_save_html_clicked(NULL, NULL);
        break;

    case BTN_ACTION_LOAD_HTML:
        on_load_html_clicked(NULL, NULL);
        break;

    case BTN_ACTION_KILL_PREDATORS: {
        Fish *f = g_aquarium->fish_list;
        while (f) {
            Fish *nxt = f->next;
            if (f->is_alive && f->type == TYPE_PREDATOR)
                remove_fish(f);
            f = nxt;
        }
        break;
    }

    case BTN_ACTION_KILL_PREY: {
        Fish *f = g_aquarium->fish_list;
        while (f) {
            Fish *nxt = f->next;
            if (f->is_alive && f->type == TYPE_PREY)
                remove_fish(f);
            f = nxt;
        }
        break;
    }

    case BTN_ACTION_BOOST_SPEED: {
        Fish *f = g_aquarium->fish_list;
        while (f) {
            if (f->is_alive) {
                f->speed      = fminf(f->speed * 2.0f, MAX_SPEED);
                f->base_speed = fminf(f->base_speed * 2.0f, MAX_SPEED);
            }
            f = f->next;
        }
        break;
    }

    case BTN_ACTION_RESET_HEALTH: {
        Fish *f = g_aquarium->fish_list;
        while (f) {
            if (f->is_alive) {
                f->health = 100.0f;
                f->hunger = 0.0f;
                f->energy = 100.0f;
            }
            f = f->next;
        }
        break;
    }

    case BTN_ACTION_ADD_SPECIES:
        if (preset)
            spawn_fish_from_preset(preset, count, 0, 0, preset->type, 0);
        break;

    case BTN_ACTION_CUSTOM_C_FUNC: {
        if (btn->custom_func_name && btn->custom_func_name[0]) {
            if (!g_module_supported()) {
                show_message("Error: Dynamic module loading is not supported!");
                break;
            }
            GModule *module = g_module_open(NULL, G_MODULE_BIND_LAZY);
            if (module) {
                void (*func)(GtkWidget*, gpointer) = NULL;
                if (g_module_symbol(module, btn->custom_func_name, (gpointer*)&func) && func) {
                    func(btn->btn_widget, NULL);
                } else {
                    char err_msg[256];
                    snprintf(err_msg, sizeof(err_msg), "Error: Function '%s' not found!\nMake sure it is compiled with -rdynamic.", btn->custom_func_name);
                    show_message(err_msg);
                }
                g_module_close(module);
            } else {
                show_message("Error: Failed to open main module!");
            }
        }
        break;
    }

    case BTN_ACTION_CUSTOM_SCRIPT:
        execute_custom_script(btn->custom_script);
        break;

    default:
        break;
    }

    update_ui_info();
    if (g_aquarium->drawing_area)
        gtk_widget_queue_draw(g_aquarium->drawing_area);
}

/* ── Delete a button ── */
void delete_dynamic_button(DynamicButton *btn)
{
    if (!btn) return;

    /* Unlink from list */
    if (btn->prev) btn->prev->next = btn->next;
    else           g_btn_list_head = btn->next;
    if (btn->next) btn->next->prev = btn->prev;
    else           g_btn_list_tail = btn->prev;

    /* Remove widget from panel */
    if (btn->container && g_panel_box) {
        gtk_container_remove(GTK_CONTAINER(g_panel_box), btn->container);
    }

    g_free(btn->label);
    g_free(btn->image_path);
    g_free(btn->custom_func_name);
    g_free(btn->custom_script);
    g_free(btn);
}

/* ── Callbacks ── */
static void on_dyn_btn_clicked(GtkWidget *w, gpointer data)
{
    (void)w;
    execute_dynamic_button_action((DynamicButton *)data);
}

static void on_dyn_btn_delete(GtkWidget *w, gpointer data)
{
    (void)w;
    delete_dynamic_button((DynamicButton *)data);
}

/* ── Build widget for one entry ── */
GtkWidget *build_dynamic_button_widget(DynamicButton *btn)
{
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    /* -- Main action button -- */
    GtkWidget *action_btn;

    if (btn->image_path && g_file_test(btn->image_path, G_FILE_TEST_EXISTS)) {
        /* Button with icon + label */
        action_btn = gtk_button_new();
        GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

        GError *err = NULL;
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(
                            btn->image_path, 24, 24, TRUE, &err);
        if (pb) {
            gtk_box_pack_start(GTK_BOX(inner),
                               gtk_image_new_from_pixbuf(pb), FALSE, FALSE, 0);
            g_object_unref(pb);
        } else if (err) {
            g_error_free(err);
        }

        GtkWidget *lbl = gtk_label_new(btn->label);
        gtk_box_pack_start(GTK_BOX(inner), lbl, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(action_btn), inner);
    } else {
        action_btn = gtk_button_new_with_label(btn->label);
    }

    gtk_widget_set_tooltip_text(action_btn, action_display_name(btn->action));
    g_signal_connect(action_btn, "clicked", G_CALLBACK(on_dyn_btn_clicked), btn);
    gtk_box_pack_start(GTK_BOX(hbox), action_btn, FALSE, FALSE, 0);

    /* -- Delete (✕) button -- */
    GtkWidget *del_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_tooltip_text(del_btn, "Remove this button");

    /* Style the ✕ button red */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "button { color: #e74c3c; padding: 2px 4px; }", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(del_btn),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    g_signal_connect(del_btn, "clicked", G_CALLBACK(on_dyn_btn_delete), btn);
    gtk_box_pack_start(GTK_BOX(hbox), del_btn, FALSE, FALSE, 0);

    btn->btn_widget = action_btn;
    btn->container  = hbox;

    gtk_widget_show_all(hbox);
    return hbox;
}

/* ============================================================
 *  ADD-BUTTON DIALOG
 * ============================================================ */

/* Context passed between dialog signal handlers */
typedef struct {
    GtkWidget *label_entry;
    GtkWidget *image_btn;
    GtkWidget *image_preview;
    GtkWidget *action_combo;
    GtkWidget *params_box;       /* vbox rebuilt when action changes */
    GtkWidget *count_spin;
    GtkWidget *count2_spin;
    GtkWidget *preset_combo;
    GtkWidget *custom_func_entry;
    GtkWidget *custom_script_entry;
    GtkWidget *count_row;
    GtkWidget *count2_row;
    GtkWidget *preset_row;
    GtkWidget *custom_func_row;
    GtkWidget *custom_script_row;
} AddBtnCtx;

static void rebuild_param_widgets(AddBtnCtx *ctx)
{
    int action_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->action_combo));
    DynamicButtonAction a = (DynamicButtonAction)action_idx;

    /* Show / hide rows */
    if (ctx->count_row)
        gtk_widget_set_visible(ctx->count_row,  action_needs_count(a));
    if (ctx->count2_row)
        gtk_widget_set_visible(ctx->count2_row, action_needs_count2(a));
    if (ctx->preset_row)
        gtk_widget_set_visible(ctx->preset_row, action_needs_preset(a));
    if (ctx->custom_func_row)
        gtk_widget_set_visible(ctx->custom_func_row, action_needs_custom_func(a));
    if (ctx->custom_script_row)
        gtk_widget_set_visible(ctx->custom_script_row, action_needs_custom_script(a));
}

static void on_action_combo_changed(GtkComboBox *combo, gpointer data)
{
    (void)combo;
    rebuild_param_widgets((AddBtnCtx *)data);
}

static void on_image_btn_file_set(GtkFileChooserButton *fcb, gpointer data)
{
    AddBtnCtx *ctx = (AddBtnCtx *)data;
    char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcb));
    if (!path) return;

    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 48, 48, TRUE, NULL);
    if (pb) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(ctx->image_preview), pb);
        g_object_unref(pb);
    }
    g_free(path);
}

void show_add_button_dialog(void)
{
    if (!g_panel_box) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Add Custom Button",
        NULL, GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "✓ Create Button", GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 480, -1);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    AddBtnCtx *ctx = g_new0(AddBtnCtx, 1);

    /* ── Section 1: Appearance ── */
    GtkWidget *sec1 = gtk_frame_new(" 1 · Button Appearance ");
    GtkWidget *grid1 = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid1), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid1), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid1), 10);
    gtk_container_add(GTK_CONTAINER(sec1), grid1);
    gtk_box_pack_start(GTK_BOX(vbox), sec1, FALSE, FALSE, 0);

    /* Label */
    gtk_grid_attach(GTK_GRID(grid1),
        gtk_label_new("Label:"), 0, 0, 1, 1);
    ctx->label_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->label_entry), "e.g., Open Reef");
    gtk_entry_set_text(GTK_ENTRY(ctx->label_entry), "My Button");
    gtk_widget_set_hexpand(ctx->label_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid1), ctx->label_entry, 1, 0, 2, 1);

    /* Image */
    gtk_grid_attach(GTK_GRID(grid1),
        gtk_label_new("Image (opt):"), 0, 1, 1, 1);

    GtkWidget *img_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    ctx->image_btn = gtk_file_chooser_button_new(
        "Select Icon", GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter *ff = gtk_file_filter_new();
    gtk_file_filter_set_name(ff, "Images");
    gtk_file_filter_add_pattern(ff, "*.png");
    gtk_file_filter_add_pattern(ff, "*.jpg");
    gtk_file_filter_add_pattern(ff, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(ctx->image_btn), ff);
    g_signal_connect(ctx->image_btn, "file-set",
        G_CALLBACK(on_image_btn_file_set), ctx);

    ctx->image_preview = gtk_image_new();
    gtk_widget_set_size_request(ctx->image_preview, 48, 48);

    gtk_box_pack_start(GTK_BOX(img_hbox), ctx->image_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(img_hbox), ctx->image_preview, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(grid1), img_hbox, 1, 1, 2, 1);

    /* ── Section 2: Action ── */
    GtkWidget *sec2 = gtk_frame_new(" 2 · Choose Action ");
    GtkWidget *grid2 = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid2), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid2), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid2), 10);
    gtk_container_add(GTK_CONTAINER(sec2), grid2);
    gtk_box_pack_start(GTK_BOX(vbox), sec2, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(grid2),
        gtk_label_new("Action:"), 0, 0, 1, 1);
    ctx->action_combo = gtk_combo_box_text_new();
    for (int i = 0; i < BTN_ACTION_COUNT; i++)
        gtk_combo_box_text_append_text(
            GTK_COMBO_BOX_TEXT(ctx->action_combo),
            action_display_name((DynamicButtonAction)i));
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->action_combo), 0);
    gtk_widget_set_hexpand(ctx->action_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid2), ctx->action_combo, 1, 0, 1, 1);

    g_signal_connect(ctx->action_combo, "changed",
        G_CALLBACK(on_action_combo_changed), ctx);

    /* ── Section 3: Parameters ── */
    GtkWidget *sec3 = gtk_frame_new(" 3 · Action Parameters ");
    GtkWidget *pgrid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pgrid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(pgrid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(pgrid), 10);
    gtk_container_add(GTK_CONTAINER(sec3), pgrid);
    gtk_box_pack_start(GTK_BOX(vbox), sec3, FALSE, FALSE, 0);

    /* Primary count row */
    ctx->count_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *count_lbl = gtk_label_new("Number of fish / pellets:");
    ctx->count_spin = gtk_spin_button_new_with_range(1, 50, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctx->count_spin), 5);
    gtk_box_pack_start(GTK_BOX(ctx->count_row), count_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx->count_row), ctx->count_spin, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(pgrid), ctx->count_row, 0, 0, 2, 1);

    /* Secondary count row (for MIXED_SCHOOL) */
    ctx->count2_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *count2_lbl = gtk_label_new("Number of predators:");
    ctx->count2_spin = gtk_spin_button_new_with_range(1, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctx->count2_spin), 2);
    gtk_box_pack_start(GTK_BOX(ctx->count2_row), count2_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx->count2_row), ctx->count2_spin, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(pgrid), ctx->count2_row, 0, 1, 2, 1);

    /* Preset species row */
    ctx->preset_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *preset_lbl = gtk_label_new("Fish species:");
    ctx->preset_combo = gtk_combo_box_text_new();
    for (int i = 0; i < preset_fish_count(); i++) {
        const PresetFish *p = preset_fish_get(i);
        if (p) gtk_combo_box_text_append_text(
                    GTK_COMBO_BOX_TEXT(ctx->preset_combo), p->species);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->preset_combo), 0);
    gtk_box_pack_start(GTK_BOX(ctx->preset_row), preset_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx->preset_row), ctx->preset_combo, TRUE, TRUE, 0);
    gtk_grid_attach(GTK_GRID(pgrid), ctx->preset_row, 0, 2, 2, 1);

    /* Custom C Function Name Row */
    ctx->custom_func_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *func_lbl = gtk_label_new("C Function Name:");
    ctx->custom_func_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->custom_func_entry), "e.g., testFonction");
    gtk_box_pack_start(GTK_BOX(ctx->custom_func_row), func_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx->custom_func_row), ctx->custom_func_entry, TRUE, TRUE, 0);
    gtk_grid_attach(GTK_GRID(pgrid), ctx->custom_func_row, 0, 3, 2, 1);

    /* Custom Script Row */
    ctx->custom_script_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *script_lbl = gtk_label_new("Script Commands:");
    ctx->custom_script_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->custom_script_entry), "e.g., clear; spawn 5 Clownfish; feed 10");
    gtk_widget_set_tooltip_text(ctx->custom_script_entry, "Commands: clear, feed <N>, spawn <N> <species>, pause, grid, highlight, speed <val>, health");
    gtk_box_pack_start(GTK_BOX(ctx->custom_script_row), script_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx->custom_script_row), ctx->custom_script_entry, TRUE, TRUE, 0);
    gtk_grid_attach(GTK_GRID(pgrid), ctx->custom_script_row, 0, 4, 2, 1);

    /* Initial visibility */
    rebuild_param_widgets(ctx);

    /* Guarantee a visible cursor over this dialog under X11/WSLg. */
    g_signal_connect(dialog, "realize", G_CALLBACK(ensure_visible_cursor), NULL);

    gtk_widget_show_all(dialog);
    /* Now re-apply visibility after show_all (which reveals everything) */
    rebuild_param_widgets(ctx);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        /* Collect values */
        const char *lbl_text =
            gtk_entry_get_text(GTK_ENTRY(ctx->label_entry));
        char *img_path =
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ctx->image_btn));
        int action_idx =
            gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->action_combo));

        DynamicButton *btn = g_new0(DynamicButton, 1);
        btn->label      = g_strdup(lbl_text[0] ? lbl_text : "Button");
        btn->image_path = img_path; /* already g_malloc'd by GTK */
        btn->action     = (DynamicButtonAction)action_idx;
        btn->param_count  =
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->count_spin));
        btn->param_count2 =
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->count2_spin));
        btn->param_preset_idx =
            gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->preset_combo));

        const char *func_text = gtk_entry_get_text(GTK_ENTRY(ctx->custom_func_entry));
        btn->custom_func_name = g_strdup(func_text);

        const char *script_text = gtk_entry_get_text(GTK_ENTRY(ctx->custom_script_entry));
        btn->custom_script = g_strdup(script_text);

        /* Append to linked list */
        btn->prev = g_btn_list_tail;
        btn->next = NULL;
        if (g_btn_list_tail) g_btn_list_tail->next = btn;
        else                 g_btn_list_head = btn;
        g_btn_list_tail = btn;

        /* Build widget and add to panel */
        GtkWidget *w = build_dynamic_button_widget(btn);
        gtk_box_pack_start(GTK_BOX(g_panel_box), w, FALSE, FALSE, 0);
    }

    g_free(ctx);
    gtk_widget_destroy(dialog);
}
