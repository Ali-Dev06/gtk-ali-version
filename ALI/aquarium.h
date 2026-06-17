#ifndef AQUARIUM_H
#define AQUARIUM_H

#include "structures.h"
#include <math.h>
#include <time.h>
#include <gdk/gdk.h>

// Add these function declarations
void save_aquarium_session(const char *filename);
void load_aquarium_session(const char *filename);
void on_save_session_clicked(GtkWidget *widget, gpointer data);
void on_load_session_clicked(GtkWidget *widget, gpointer data);
void on_new_session_clicked(GtkWidget *widget, gpointer data);
void show_load_dialog(void);
void show_save_dialog(void);

#define MAX_FISH 300
#define MAX_GROUPS 50
#define MAX_SPEED 7.0
#define MIN_SPEED 1.2
#define FRAME_INTERVAL 33
#define MAX_FOOD 50

typedef struct Fish Fish;
typedef struct Group Group;
typedef struct Food Food;
typedef struct SpriteAnim SpriteAnim;  /* shared sprite-sheet animation (fish_catalog.c) */

typedef enum {
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} FishDirection;

typedef enum {
    TYPE_PREY,
    TYPE_PREDATOR
} FishType;

typedef enum {
    DIET_HERBIVORE,
    DIET_CARNIVORE,
    DIET_OMNIVORE
} DietType;

typedef enum {
    GENDER_MALE,
    GENDER_FEMALE
} Gender;

typedef enum {
    GOAL_IDLE,
    GOAL_SEARCH_FOOD,
    GOAL_ESCAPE,
    GOAL_FOLLOW_LEADER,
    GOAL_HUNT,
    GOAL_REST,
    GOAL_GLIDE,
    GOAL_MATE
} Goal;

typedef enum {
    STATUS_ALIVE,
    STATUS_DEAD,
    STATUS_HUNGRY,
    STATUS_SICK
} LifeStatus;

typedef enum {
    PERSONALITY_SHY,
    PERSONALITY_AGGRESSIVE,
    PERSONALITY_SOCIAL,
    PERSONALITY_CURIOUS,
    PERSONALITY_TERRITORIAL,
    PERSONALITY_PEACEFUL
} Personality;

typedef enum {
    STAGE_BABY,
    STAGE_JUVENILE,
    STAGE_ADULT,
    STAGE_ELDERLY
} LifeStage;

// Food structure
struct Food {
    float x, y;
    float vx, vy;
    float energy;
    float age, lifetime;
    int is_alive;
};

// Fish structure
struct Fish {
    int id;
    char *species;
    char *image_path;
    GdkPixbuf *image_left;
    GdkPixbuf *image_right;
    
    float x, y;
    float vx, vy;
    float target_vx, target_vy;
    float size, base_size;
    float speed, base_speed;
    
    float heading, target_heading;
    float turn_smoothness;
    float swim_phase, tail_phase;
    float drag;
    float desired_speed;
    
    float home_x, home_y;
    float target_x, target_y;
    float wander_timer;
    
    FishType type;
    DietType diet;
    Gender gender;
    float health;
    float hunger;
    float energy;
    float age;
    float lifespan;
    LifeStatus status;
    LifeStage life_stage;
    Personality personality;
    int generation;
    int is_alive;
    
    float mate_timer;
    int mate_target_id;
    float attack_cooldown;
    float chase_exhaustion;
    float panic_timer;
    
    float visibility;
    float render_scale;
    float glow;
    
    FishDirection direction;
    FishDirection image_facing;
    struct Group *group;
    struct Fish *next_grid;
    int is_leader;
    int leader_order;
    
    Goal current_goal;

    /* Sprite-sheet animation (NULL = static image, no animation) */
    SpriteAnim *anim;     /* shared, owned by the catalog cache (do not free per-fish) */
    int anim_frame;       /* current frame index into the sheet */
    float anim_timer;     /* seconds accumulated toward the next frame */

    struct Fish *group_next;
    struct Fish *group_prev;
    struct Fish *next;
    struct Fish *prev;
};

// Group structure
struct Group {
    int id;
    char *name;
    Fish *leader;
    Fish *members_head;
    Fish *members_tail;
    int member_count;
    float center_x, center_y;
    struct Group *next;
};

// Environment structure
typedef struct {
    int width, height;
    Food food_sources[50];
    int food_count;
} Environment;

// In aquarium.h, update the Aquarium struct definition:
typedef struct {
    Fish *fish_list;
    Group *group_list;
    GQueue *delete_stack;
    GHashTable *fish_by_id;        // Add this line
    GtkWidget *drawing_area;
    GtkWidget *info_label;
    GtkWidget *group_combo;
    GtkWidget *status_bar;
    Environment env;
    int next_fish_id;
    int next_group_id;
    int width, height;
    int is_running;
    double current_time;
    float mouse_x, mouse_y;
    int guiding;            // 1 while the selected fish is being dragged by the mouse

    // Statistics
    int total_fish_created;
    int total_fish_dead;
    int total_eats;
    
    // UI State
    Fish *selected_fish;
    Group *selected_group;
    int show_help;
    int show_grid;
    int show_selected_highlight;
    int paused;
    
    // Timer
    guint animation_timer;
} Aquarium;

extern Aquarium *g_aquarium;

// Core functions
void init_aquarium(int width, int height);
void cleanup_aquarium(void);
void update_simulation(void);
gboolean animation_tick(gpointer data);

// Movement & physics
void update_fish_movement(Fish *fish, float dt);
void apply_boundary_wrap(Fish *fish);
void rebuild_spatial_grid(void);

// Boids & schooling
void apply_group_behavior(Fish *fish);
void merge_compatible_groups(void);

// Ecosystem
void update_fish_ecosystem(Fish *fish, float dt);
void spawn_baby_fish(Fish *dad, Fish *mom);

// Food system
void spawn_food_pellets(int count, float spawn_x, float spawn_y);
void update_food_physics(float dt);
Food *find_nearest_food(Fish *fish);
void consume_food(Fish *fish);

// Particles
void init_particle_system(void);
void update_particle_system(float dt);
void draw_background_effects(cairo_t *cr);

// Death "pop" effects
void spawn_death_effect(float x, float y);
void update_death_effects(float dt);
void draw_death_effects(cairo_t *cr);

// Fish management
Fish* create_fish(const char *species, const char *image_path, float x, float y,
                  float size, float speed, FishType type, DietType diet,
                  Gender gender, FishDirection image_facing);
void add_fish(Fish *fish);
void remove_fish(Fish *fish);
void delete_last_fish(void);
Fish* get_last_fish(void);
void fish_die(Fish *fish);
int count_alive_fish(void);

// Group management
Group* create_group(const char *name, Fish *leader);
void add_fish_to_group(Group *group, Fish *fish);
void remove_fish_from_group(Fish *fish);
void delete_group(Group *group);
Group* find_group_by_id(int id);
void update_group_formation(Group *group);
void promote_next_leader(Group *group);
void set_fish_status(Fish *fish, LifeStatus status);

// Health and life
void update_fish_health(Fish *fish);
void fish_die(Fish *fish);
void update_life_status(Fish *fish);
int is_fish_visible(Fish *fish);
// Behavior
void update_fish_movement(Fish *fish, float dt);
void apply_boundary_wrap(Fish *fish);
void apply_group_behavior(Fish *fish);
void apply_random_movement(Fish *fish);
void enforce_direction(Fish *fish);

// Predator-prey
Fish* find_nearest_prey(Fish *predator);
Fish* find_nearest_predator(Fish *prey);
void handle_predation(Fish *predator, Fish *prey);
void check_predation(void);

// Drawing
gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data);
void draw_water_background(cairo_t *cr);
void draw_decor(cairo_t *cr);
void draw_fish(cairo_t *cr, Fish *fish);
void draw_stats_panel(cairo_t *cr);
void draw_help_overlay(cairo_t *cr);
void draw_grid(cairo_t *cr);
void cairo_round_rectangle(cairo_t *cr, double x, double y, double w, double h, double r);
// Add to aquarium.h
// Add to aquarium.h
void save_aquarium_as_html(const char *filename);
void load_aquarium_from_html(const char *filename);
void save_html_session_dialog(void);
void load_html_session_dialog(void);
void on_save_html_clicked(GtkWidget *widget, gpointer data);
void on_load_html_clicked(GtkWidget *widget, gpointer data);
void on_load_default_clicked(GtkWidget *widget, gpointer data);
char* extract_tag_content(const char *html, const char *tag, const char *end_boundary);
void create_aquarium_window(void);
/* Bridge for the dynamic-button module (button_manager.c expects this name). */
void on_clear_all_no_confirm(void);
/* Forces a visible pointer on a window/widget (prevents the WSLg/X11
 * invisible-cursor bug). Connect to the "realize" signal. */
void ensure_visible_cursor(GtkWidget *w, gpointer data);
// UI Callbacks
void on_add_fish_clicked(GtkWidget *widget, gpointer data);
void on_create_group_clicked(GtkWidget *widget, gpointer data);
void on_add_to_group_clicked(GtkWidget *widget, gpointer data);
void on_remove_from_group_clicked(GtkWidget *widget, gpointer data);
void on_delete_group_clicked(GtkWidget *widget, gpointer data);
void on_delete_fish_clicked(GtkWidget *widget, gpointer data);
void on_view_group_clicked(GtkWidget *widget, gpointer data);
void on_toggle_help(GtkWidget *widget, gpointer data);
void on_toggle_grid(GtkWidget *widget, gpointer data);
void on_clear_simulation(GtkWidget *widget, gpointer data);
void on_fish_selected(Fish *fish);
gboolean on_mouse_click(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);

// Dialogs
GtkWidget* create_fish_dialog(void);
GtkWidget* create_group_dialog(void);
void on_fish_dialog_response(GtkDialog *dialog, gint response_id, gpointer data);
void on_group_dialog_response(GtkDialog *dialog, gint response_id, gpointer data);

// Utilities
void update_ui_info(void);
void show_message(const char *msg);
double get_current_time(void);
float wrap_distance(Fish *a, Fish *b);

#endif