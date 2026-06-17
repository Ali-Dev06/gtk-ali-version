#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <gtk/gtk.h>

/* ============================================================
 *  DYNAMIC BUTTON SYSTEM
 *  Allows the user to add / remove custom action buttons at
 *  runtime. Each button has a label, an optional icon image,
 *  and one action chosen from the table below.
 * ============================================================ */

/* ── All available actions ── */
typedef enum {
    BTN_ACTION_ADD_LIBRARY      = 0,  /* Add N fish from a library preset      */
    BTN_ACTION_ADD_PREY         = 1,  /* Add N random prey fish                */
    BTN_ACTION_ADD_PREDATORS    = 2,  /* Add N random predators                */
    BTN_ACTION_NEW_AQUARIUM     = 3,  /* Clear all + spawn N fish (new session)*/
    BTN_ACTION_CLEAR_AND_ADD    = 4,  /* Clear all + add N fish of preset      */
    BTN_ACTION_CLEAR_ALL        = 5,  /* Remove all fish and groups            */
    BTN_ACTION_FEED             = 6,  /* Spawn 12 food pellets                 */
    BTN_ACTION_SPAWN_FOOD_N     = 7,  /* Spawn N food pellets at top           */
    BTN_ACTION_PAUSE_RESUME     = 8,  /* Toggle simulation pause               */
    BTN_ACTION_TOGGLE_GRID      = 9,  /* Show/hide debug grid                  */
    BTN_ACTION_TOGGLE_HIGHLIGHT = 10, /* Toggle selected-fish highlight        */
    BTN_ACTION_LOAD_DEFAULT     = 11, /* Load the built-in default reef        */
    BTN_ACTION_MIXED_SCHOOL     = 12, /* Spawn N prey + M predators            */
    BTN_ACTION_SAVE_HTML        = 13, /* Open Save session dialog              */
    BTN_ACTION_LOAD_HTML        = 14, /* Open Load session dialog              */
    BTN_ACTION_KILL_PREDATORS   = 15, /* Remove all predator fish              */
    BTN_ACTION_KILL_PREY        = 16, /* Remove all prey fish                  */
    BTN_ACTION_BOOST_SPEED      = 17, /* Double speed of every living fish     */
    BTN_ACTION_RESET_HEALTH     = 18, /* Restore full health / hunger to all   */
    BTN_ACTION_ADD_SPECIES      = 19, /* Add N fish of one specific species    */
    BTN_ACTION_CUSTOM_C_FUNC    = 20, /* Call a C function dynamically by name */
    BTN_ACTION_CUSTOM_SCRIPT    = 21, /* Run a sequence of simple commands     */
    BTN_ACTION_COUNT                  /* sentinel — keep last                  */
} DynamicButtonAction;

/* ── One dynamic button entry ── */
typedef struct DynamicButton {
    char *label;           /* text shown on the button                   */
    char *image_path;      /* path to icon image (NULL = no icon)        */
    DynamicButtonAction action;

    /* parameters for the action (meaning depends on action type)        */
    int param_count;       /* primary fish / pellet count                */
    int param_count2;      /* secondary count (e.g. predators in MIXED)  */
    int param_preset_idx;  /* catalog preset index                       */

    /* Custom action fields */
    char *custom_func_name; /* Dynamic C function name to load via dlsym/GModule */
    char *custom_script;    /* Command script text to parse and execute         */

    /* GTK widgets – managed internally                                  */
    GtkWidget *btn_widget;  /* the clickable button (icon + label)       */
    GtkWidget *container;   /* hbox: [btn_widget][✕]                    */

    struct DynamicButton *next;
    struct DynamicButton *prev;
} DynamicButton;

/* ── Public API ── */

/* Call once after the panel GtkBox is created.                          */
void init_dynamic_buttons(GtkWidget *panel_box, GtkWidget *highlight_toggle);

/* Open the "Add Custom Button" dialog.                                  */
void show_add_button_dialog(void);

/* Remove a button from the panel and free it.                           */
void delete_dynamic_button(DynamicButton *btn);

/* Run the action bound to a button.                                     */
void execute_dynamic_button_action(DynamicButton *btn);

/* Build (or re-build) the GTK widget pair [button][✕] for a entry.     */
GtkWidget *build_dynamic_button_widget(DynamicButton *btn);

/* Human-readable name for an action (used in the combo-box).           */
const char *action_display_name(DynamicButtonAction a);

/* Returns TRUE if the action requires param_count.                      */
gboolean action_needs_count(DynamicButtonAction a);

/* Returns TRUE if the action requires param_count2.                     */
gboolean action_needs_count2(DynamicButtonAction a);

/* Returns TRUE if the action requires a catalog preset choice.          */
gboolean action_needs_preset(DynamicButtonAction a);

#endif /* BUTTON_MANAGER_H */
