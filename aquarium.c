// aquarium.c pas JSON
#include "aquarium.h"
#include "fish_catalog.h"

Aquarium *g_aquarium = NULL;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//fonctions Cairo aux, deja declaree dans structures.c
void cairo_round_rectangle(cairo_t *cr, double x, double y, double w, double h, double r);
void cairo_ellipse(cairo_t *cr, double x, double y, double rx, double ry);

double get_current_time(void){
    return g_get_real_time() / 1000000.0;
}

int count_alive_fish(void){
    int count = 0;
    Fish *current = g_aquarium->fish_list;
    while (current){
        if (current->is_alive)
            count++;
        current = current->next;
    }
    return count;
}

//Calculer la distance (avec repliement entre deux poissons)
float wrap_distance(Fish *a, Fish *b){
    float dx = a->x - b->x;
    float dy = a->y - b->y;

    if (dx > g_aquarium->width / 2){
        dx = dx - g_aquarium->width;//Si la distance horizontal > g_aquarium->width / 2, on considere que les poissons sont plus proches en traversant le bord de l'aquarium
    }
    else if (dx < -g_aquarium->width / 2){
        dx = dx + g_aquarium->width;//Si la distance horizontale est plus petite que la moitié négative de la largeur de l'aquarium, on considère également que les poissons sont plus proches en traversant le bord de l'aquarium dans l'autre direction.
    }

    if (dy > g_aquarium->height / 2){
        dy = dy - g_aquarium->height;// Si la distance verticale est plus grande que la moitié de la hauteur de l'aquarium, on considère que les poissons sont plus proches en traversant le bord de l'aquarium.
    }
    else if (dy < -g_aquarium->height / 2){
        dy = dy + g_aquarium->height;// Si la distance verticale est plus petite que la moitié négative de la hauteur de l'aquarium, on considère également que les poissons sont plus proches en traversant le bord de l'aquarium dans l'autre direction.
    }

    return sqrt(dx * dx + dy * dy);// La distance réelle entre les deux poissons est calculée en utilisant le théorème de Pythagore, en tenant compte des ajustements pour le repliement.
}

//Trouver le predateur le plus proche en tenant compte du contournement
Fish *find_nearest_predator(Fish *prey){
    Fish *nearest = NULL;
    float nearest_dist = 200;//Distance maximale de recherche

    Fish *current = g_aquarium->fish_list;
    while (current){
        if (current->is_alive && current->type == TYPE_PREDATOR && current != prey){
            float dist = wrap_distance(prey, current);
            if (dist < nearest_dist)
            {
                nearest_dist = dist;
                nearest = current;
            }
        }
        current = current->next;
    }
    return nearest;
}

void show_message(const char *msg){
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// FISH MANAGEMENT

/* void load_fish_image(Fish *fish, const char *image_path)
{
    GdkPixbuf *original = NULL;

    if (image_path && g_file_test(image_path, G_FILE_TEST_EXISTS))
    {
        GError *error = NULL;
        original = gdk_pixbuf_new_from_file(image_path, &error);
        if (error)
        {
            g_printerr("Error loading image: %s\n", error->message);
            g_error_free(error);
            original = NULL;
        }
    }

    if (!original)
    {
        int size = (int)fish->size;
        original = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, size, size);

        guint32 color;
        if (fish->type == TYPE_PREDATOR)
        {
            color = 0xFF4444FF;
        }
        else if (fish->diet == DIET_HERBIVORE)
        {
            color = 0x44FF44FF;
        }
        else if (fish->diet == DIET_CARNIVORE)
        {
            color = 0xFF8844FF;
        }
        else
        {
            color = 0x4488FFFF;
        }
        gdk_pixbuf_fill(original, color);

        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
        cairo_t *cr = cairo_create(surface);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_arc(cr, size * 0.7, size * 0.35, size * 0.07, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_arc(cr, size * 0.72, size * 0.33, size * 0.04, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_destroy(cr);

        GdkPixbuf *eye = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
        gdk_pixbuf_composite(eye, original, 0, 0, size, size, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
        g_object_unref(eye);
        cairo_surface_destroy(surface);
    }

    int target_w = (int)fish->size;
    int target_h = (int)(fish->size * 0.6);
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(original, target_w, target_h, GDK_INTERP_BILINEAR);

    if (fish->image_left)
        g_object_unref(fish->image_left);
    if (fish->image_right)
        g_object_unref(fish->image_right);

    fish->image_right = scaled;
    fish->image_left = gdk_pixbuf_flip(scaled, TRUE);
    g_object_ref(fish->image_left);

    g_object_unref(original);
} */

void load_fish_image(Fish *fish, const char *image_path){
    GdkPixbuf *original = NULL;

    //chargement image personnalise si fournie
    if (image_path && g_file_test(image_path, G_FILE_TEST_EXISTS)){
        GError *error = NULL;
        original = gdk_pixbuf_new_from_file(image_path, &error);
        if (error){
            g_printerr("Error loading image: %s\n", error->message);
            g_error_free(error);
            original = NULL;
        }
    }

// Si aucune image ou chargement fail, creer une forme de poisson simple
    if (!original){
        int size = (int)fish->size;
        
        
        original = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, size, size);
        gdk_pixbuf_fill(original, 0x00000000);  // Fully transparent background
        
        
        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
        cairo_t *cr = cairo_create(surface);
        
        if (fish->type == TYPE_PREDATOR){
            cairo_set_source_rgb(cr, 0.95, 0.25, 0.25);  // Reddish predator
        }
        else if (fish->diet == DIET_HERBIVORE){
            cairo_set_source_rgb(cr, 0.25, 0.85, 0.25);  // Green herbivore
        }
        else if (fish->diet == DIET_CARNIVORE){
            cairo_set_source_rgb(cr, 0.95, 0.55, 0.15);  // Orange carnivore
        }
        else{
            cairo_set_source_rgb(cr, 0.25, 0.55, 0.95);  // Blue omnivore
        }
        
        //fish body (elongated ellipse
        cairo_translate(cr, size / 2.0, size / 2.0);
        cairo_save(cr);
        cairo_scale(cr, 1.3, 0.65);  
        cairo_arc(cr, 0, 0, size / 2.8, 0, 2 * M_PI);
        cairo_restore(cr);
        cairo_fill(cr);
        
        //tail
        cairo_move_to(cr, -size / 3.2, 0);
        cairo_line_to(cr, -size / 1.8, -size / 4.5);
        cairo_line_to(cr, -size / 1.8, size / 4.5);
        cairo_close_path(cr);
        cairo_fill(cr);
        
        //eye (white)
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_arc(cr, size / 5.5, -size / 8, size / 12, 0, 2 * M_PI);
        cairo_fill(cr);
        
        //pupil 
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_arc(cr, size / 6.2, -size / 8.5, size / 25, 0, 2 * M_PI);
        cairo_fill(cr);
        
        //fins
        cairo_set_source_rgb(cr, 0.9, 0.5, 0.2);
        cairo_move_to(cr, -size / 12, -size / 3.5);
        cairo_line_to(cr, size / 8, -size / 2.2);
        cairo_line_to(cr, size / 3.5, -size / 3.2);
        cairo_fill(cr);
        
        cairo_destroy(cr);
        
        //drawn onto the pixbuf
        GdkPixbuf *drawn_fish = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
        gdk_pixbuf_composite(drawn_fish, original, 0, 0, size, size, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
        g_object_unref(drawn_fish);
        cairo_surface_destroy(surface);
    }
    
    // Scale to fit size 
    int target_w = (int)fish->size;
    int target_h = (int)(fish->size * 0.6);
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(original, target_w, target_h, GDK_INTERP_BILINEAR);
    
    //left/right facing versions
    if (fish->image_left)
        g_object_unref(fish->image_left);
    if (fish->image_right)
        g_object_unref(fish->image_right);
    
    fish->image_right = scaled;
    fish->image_left = gdk_pixbuf_flip(scaled, TRUE);
    g_object_ref(fish->image_left);
    
    g_object_unref(original);
}

Fish *create_fish(const char *species, const char *image_path, float x, float y,
                  float size, float speed, FishType type, DietType diet,
                  Gender gender, FishDirection image_facing){
    Fish *fish = malloc(sizeof(Fish));
    memset(fish, 0, sizeof(Fish));

    fish->id = g_aquarium->next_fish_id++;
    fish->species = g_strdup(species);
    fish->image_path = image_path ? g_strdup(image_path) : NULL;
    fish->x = x;
    fish->y = y;
    fish->home_x = x;
    fish->home_y = y;
    fish->size = size;
    fish->base_size = size;
    fish->speed = speed;
    fish->base_speed = speed;
    fish->type = type;
    fish->diet = diet;
    fish->gender = gender;
    fish->image_facing = image_facing;
    fish->direction = image_facing;
    fish->is_alive = 1;
    fish->health = 100.0f;
    fish->hunger = 20.0f;
    fish->energy = 80.0f;
    fish->age = 0.0f;
    fish->lifespan = 280.0f + ((float)rand() / (float)RAND_MAX) * 80.0f;
    fish->status = STATUS_ALIVE;
    fish->life_stage = STAGE_BABY;
    fish->personality = (Personality)(rand() % 6);
    fish->generation = 0;
    fish->group = NULL;
    fish->attack_cooldown = 0.0f;
    fish->chase_exhaustion = 0.0f;
    fish->panic_timer = 0.0f;
    fish->is_leader = 0;
    fish->leader_order = -1;
    fish->current_goal = GOAL_IDLE;
    
    fish->heading = (image_facing == DIRECTION_RIGHT) ? 0.0f : M_PI;
    fish->target_heading = fish->heading;
    fish->turn_smoothness = ((float)rand() / (float)RAND_MAX) * 0.5f + 0.25f;
    fish->swim_phase = ((float)rand() / (float)RAND_MAX) * 2.0f * M_PI;
    fish->tail_phase = 0.0f;
    fish->drag = 0.12f + ((float)rand() / (float)RAND_MAX) * 0.05f;
    fish->desired_speed = speed;
    
    fish->target_x = x;
    fish->target_y = y;
    fish->wander_timer = 2.0f + ((float)rand() / (float)RAND_MAX) * 2.0f;
    
    fish->mate_timer = 0.0f;
    fish->mate_target_id = -1;
    
    fish->visibility = 0.8f;
    fish->render_scale = 1.0f;
    fish->glow = 0.6f;
    
    if (image_facing == DIRECTION_RIGHT){
        fish->vx = speed * 0.8f;
    }
    else{
        fish->vx = -speed * 0.8f;
    }
    fish->vy = ((rand() % 100) - 50) / 50.0f * speed * 0.5f;
    fish->target_vx = fish->vx;
    fish->target_vy = fish->vy;
    fish->next_grid = NULL;

    load_fish_image(fish, image_path);

    g_aquarium->total_fish_created++;
    return fish;
}

void add_fish(Fish *fish){
    fish->next = g_aquarium->fish_list;
    if (g_aquarium->fish_list){
        g_aquarium->fish_list->prev = fish;
    }
    g_aquarium->fish_list = fish;
    update_ui_info();
}

void remove_fish(Fish *fish){
    if (!fish->is_alive)
        return;

    if (fish->group){
        remove_fish_from_group(fish);
    }

    if (fish->prev){
        fish->prev->next = fish->next;// Si le poisson à supprimer n'est pas le premier de la liste, on met à jour le pointeur du poisson précédent pour qu'il pointe vers le poisson suivant, en contournant ainsi le poisson supprimé.
    }
    else{
        g_aquarium->fish_list = fish->next;
    }
    if (fish->next){
        fish->next->prev = fish->prev;
    }

    fish->is_alive = 0;
    g_aquarium->total_fish_dead++;
    update_ui_info();

    if (g_aquarium->selected_fish == fish){
        g_aquarium->selected_fish = NULL;
    }
}

// GROUP MANAGEMENT

Group *create_group(const char *name, Fish *leader){
    Group *group = malloc(sizeof(Group));
    memset(group, 0, sizeof(Group));

    group->id = g_aquarium->next_group_id++;
    group->name = g_strdup(name);
    group->leader = leader;
    group->members_head = leader;
    group->members_tail = leader;
    group->member_count = 1;
    group->center_x = leader->x;
    group->center_y = leader->y;
    group->next = NULL;

    leader->group = group;
    leader->is_leader = 1;
    leader->leader_order = 0;
    leader->group_prev = NULL;
    leader->group_next = NULL;

    group->next = g_aquarium->group_list;
    g_aquarium->group_list = group;

    update_ui_info();
    return group;
}

void add_fish_to_group(Group *group, Fish *fish){
    if (!group || !fish)
        return;

    if (fish->group)
    {
        remove_fish_from_group(fish);
    }

    fish->group_prev = group->members_tail;
    fish->group_next = NULL;
    if (group->members_tail)
    {
        group->members_tail->group_next = fish;
    }
    else
    {
        group->members_head = fish;
        fish->group_prev = NULL;
    }
    group->members_tail = fish;
    fish->group = group;
    fish->is_leader = 0;
    fish->leader_order = group->member_count;
    group->member_count++;

    update_group_formation(group);
    update_ui_info();
}

void remove_fish_from_group(Fish *fish){
    if (!fish || !fish->group)
        return;

    Group *group = fish->group;
    int was_leader = fish->is_leader;

    if (fish->group_prev){
        fish->group_prev->group_next = fish->group_next;
    }
    else{
        group->members_head = fish->group_next;
    }
    if (fish->group_next){
        fish->group_next->group_prev = fish->group_prev;
    }
    else{
        group->members_tail = fish->group_prev;
    }

    group->member_count--;
    fish->group = NULL;
    fish->is_leader = 0;
    fish->leader_order = -1;
    fish->group_prev = NULL;
    fish->group_next = NULL;

    if (was_leader && group->member_count > 0){
        promote_next_leader(group);
    }
    else{
        Fish *current = group->members_head;
        int order = 0;
        while (current){
            current->leader_order = order++;
            current = current->group_next;
        }
    }

    if (group->member_count == 0){
        delete_group(group);
    }
    else{
        update_group_formation(group);
    }

    update_ui_info();
}

void delete_group(Group *group)
{
    if (!group)
        return;

    Fish *current = group->members_head;
    while (current)
    {
        Fish *next = current->group_next;
        current->group = NULL;
        current->is_leader = 0;
        current->leader_order = -1;
        current->group_prev = NULL;
        current->group_next = NULL;
        current = next;
    }

    Group *prev = NULL;
    Group *curr = g_aquarium->group_list;
    while (curr)
    {
        if (curr == group)
        {
            if (prev)
            {
                prev->next = curr->next;
            }
            else
            {
                g_aquarium->group_list = curr->next;
            }
            g_free(group->name);
            free(group);
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    update_ui_info();
}

void promote_next_leader(Group *group)
{
    if (!group)
        return;

    Fish *new_leader = group->members_head;
    if (!new_leader)
    {
        group->leader = NULL;
        return;
    }

    Fish *current = group->members_head;
    while (current)
    {
        current->is_leader = 0;
        current->leader_order = -1;
        current = current->group_next;
    }

    group->leader = new_leader;
    new_leader->is_leader = 1;
    new_leader->leader_order = 0;

    current = new_leader->group_next;
    int order = 1;
    while (current)
    {
        current->leader_order = order++;
        current = current->group_next;
    }

    update_group_formation(group);
}

void update_group_formation(Group *group)
{
    if (!group || !group->leader)
        return;

    float sum_x = 0, sum_y = 0;
    Fish *current = group->members_head;
    while (current)
    {
        sum_x += current->x;
        sum_y += current->y;
        current = current->group_next;
    }
    group->center_x = sum_x / group->member_count;
    group->center_y = sum_y / group->member_count;

    current = group->members_head;
    if (current)
        current = current->group_next;

    int index = 1;
    while (current)
    {
        float angle = (index * 0.7) * (current->leader_order % 2 == 0 ? 1 : -1);
        float offset_x = sin(angle) * 40;
        float offset_y = cos(angle) * 30 - 15;

        float target_x = group->leader->x + offset_x;
        float target_y = group->leader->y + offset_y;

        float dx = target_x - current->x;
        float dy = target_y - current->y;

        if (fabs(dx) > g_aquarium->width / 2)
        {
            if (dx > 0)
                dx -= g_aquarium->width;
            else
                dx += g_aquarium->width;
        }
        if (fabs(dy) > g_aquarium->height / 2)
        {
            if (dy > 0)
                dy -= g_aquarium->height;
            else
                dy += g_aquarium->height;
        }

        float dist = sqrt(dx * dx + dy * dy);
        if (dist > 5)
        {
            current->target_vx = dx * 0.1;
            current->target_vy = dy * 0.1;
        }

        index++;
        current = current->group_next;
    }
}

// ============================================================================
// HEALTH AND MOVEMENT
// ============================================================================

void update_fish_health(Fish *fish)
{
    if (!fish->is_alive)
        return;

    fish->age += 0.01;
    fish->hunger += 0.01;
    fish->energy -= 0.005;

    if (fish->hunger > 80)
    {
        fish->health -= 0.5;
    }
    else if (fish->energy < 20)
    {
        fish->health -= 0.3;
    }
    else
    {
        fish->health += 0.1;
        if (fish->health > 100)
            fish->health = 100;
    }

    if (fish->health <= 0 || fish->hunger >= 100 || fish->age > 100)
    {
        fish_die(fish);
    }
}

void fish_die(Fish *fish)
{
    if (!fish->is_alive)
        return;

    fish->is_alive = 0;

    if (fish->group)
    {
        remove_fish_from_group(fish);
    }

    g_aquarium->total_fish_dead++;

    if (g_aquarium->selected_fish == fish)
    {
        g_aquarium->selected_fish = NULL;
    }
}

// Find nearest prey considering wrap-around
Fish *find_nearest_prey(Fish *predator)
{
    Fish *nearest = NULL;
    float nearest_dist = 200;

    Fish *current = g_aquarium->fish_list;
    while (current)
    {
        if (current->is_alive && current->type == TYPE_PREY && current != predator)
        {
            float dist = wrap_distance(predator, current);
            if (dist < nearest_dist && current->size < predator->size)
            {
                nearest_dist = dist;
                nearest = current;
            }
        }
        current = current->next;
    }
    return nearest;
}

void apply_boundary_wrap(Fish *fish)
{
    int margin = (int)fish->size;

    if (fish->x + margin < 0)
    {
        fish->x = g_aquarium->width + margin;
    }
    else if (fish->x - margin > g_aquarium->width)
    {
        fish->x = -margin;
    }
}

void resolve_fish_spacing(float dt)
{
    if (!g_aquarium)
        return;

    Fish *fish_a = g_aquarium->fish_list;
    while (fish_a)
    {
        if (fish_a->is_alive)
        {
            Fish *fish_b = fish_a->next;
            while (fish_b)
            {
                if (fish_b->is_alive)
                {
                    float dx = fish_b->x - fish_a->x;
                    float dy = fish_b->y - fish_a->y;
                    float dist = sqrtf(dx * dx + dy * dy) + 0.001f;
                    float min_dist = (fish_a->size + fish_b->size) * 0.55f;
                    if (dist < min_dist)
                    {
                        float push = (min_dist - dist) * 0.25f;
                        float nx = dx / dist;
                        float ny = dy / dist;
                        fish_a->x -= nx * push;
                        fish_a->y -= ny * push;
                        fish_b->x += nx * push;
                        fish_b->y += ny * push;
                        fish_a->vx -= nx * 0.02f;
                        fish_a->vy -= ny * 0.02f;
                        fish_b->vx += nx * 0.02f;
                        fish_b->vy += ny * 0.02f;
                    }
                }
                fish_b = fish_b->next;
            }
        }
        fish_a = fish_a->next;
    }
}

void check_predation(void)
{
    if (!g_aquarium)
        return;

    Fish *current = g_aquarium->fish_list;
    while (current)
    {
        Fish *next = current->next;
        if (current->is_alive && current->type == TYPE_PREDATOR && current->hunger > 30.0f)
        {
            Fish *prey = find_nearest_prey(current);
            if (prey)
            {
                float dist = wrap_distance(current, prey);
                float required_dist = (current->size + prey->size) * 0.65f;
                if (dist < required_dist)
                {
                    current->hunger = fmaxf(0.0f, current->hunger - 36.0f);
                    current->health = fminf(100.0f, current->health + 12.0f);
                    current->energy = fminf(100.0f, current->energy + 16.0f);
                    current->attack_cooldown = 1.8f;
                    current->chase_exhaustion = 0.0f;
                    g_aquarium->total_eats++;
                    remove_fish(prey);
                }
            }
        }
        current = next;
    }
}

int is_fish_visible(Fish *fish)
{
    int margin = (int)fish->size;
    return (fish->x + margin > 0 && fish->x - margin < g_aquarium->width &&
            fish->y + margin > 0 && fish->y - margin < g_aquarium->height);
}

// ============================================================================
// SIMULATION LOOP
// ============================================================================

void update_simulation(void)
{
    static long long last_time = 0;
    long long current_time = g_get_real_time();

    float dt = (current_time - last_time) / 1000000.0f;
    if (dt > 0.05f)
        dt = 0.033f;
    if (dt <= 0)
        dt = 0.016f;

    last_time = current_time;

    if (!g_aquarium->paused)
    {
        rebuild_spatial_grid();

        Fish *current = g_aquarium->fish_list;
        while (current)
        {
            Fish *next = current->next;
            if (current->is_alive)
            {
                update_fish_ecosystem(current, dt);
                if (current->is_alive)
                {
                    update_fish_movement(current, dt);
                    apply_group_behavior(current);
                }
            }
            current = next;
        }

        update_food_physics(dt);
        merge_compatible_groups();
        resolve_fish_spacing(dt);
        update_particle_system(dt);
        check_predation();

        current = g_aquarium->fish_list;
        while (current)
        {
            Fish *next = current->next;
            if (current->is_alive)
            {
                if (current->current_goal == GOAL_SEARCH_FOOD && current->hunger > 45.0f)
                {
                    Food *food = find_nearest_food(current);
                    if (food)
                    {
                        float dx = food->x - current->x;
                        float dy = food->y - current->y;
                        float dist = sqrtf(dx * dx + dy * dy);
                        current->target_x = food->x;
                        current->target_y = food->y;
                        if (dist < current->size * 1.35f)
                        {
                            consume_food(current);
                        }
                    }
                }

                if (current->type == TYPE_PREDATOR && current->hunger > 40.0f &&
                    (current->current_goal == GOAL_HUNT || current->hunger > 55.0f))
                {
                    Fish *prey = find_nearest_prey(current);
                    if (prey && current->size > prey->size * 0.85f)
                    {
                        current->target_x = prey->x;
                        current->target_y = prey->y;
                        current->current_goal = GOAL_HUNT;
                        float dx = prey->x - current->x;
                        float dy = prey->y - current->y;
                        float dist = sqrtf(dx * dx + dy * dy);
                        if (dist < (current->size + prey->size) * 0.75f && current->attack_cooldown <= 0.0f)
                        {
                            current->hunger = fmaxf(0.0f, current->hunger - 28.0f);
                            current->health = fminf(100.0f, current->health + 8.0f);
                            current->energy = fminf(100.0f, current->energy + 12.0f);
                            remove_fish(prey);
                            g_aquarium->total_eats++;
                        }
                    }
                }
            }
            current = next;
        }
    }

    gtk_widget_queue_draw(g_aquarium->drawing_area);
}

gboolean animation_tick(gpointer data)
{
    (void)data;
    if (g_aquarium && g_aquarium->is_running)
    {
        update_simulation();
    }
    return G_SOURCE_CONTINUE;
}

// ============================================================================
// DRAWING
// ============================================================================

void draw_water_background(cairo_t *cr)
{
    int w = g_aquarium->width;
    int h = g_aquarium->height;

    cairo_pattern_t *pattern = cairo_pattern_create_linear(0, 0, 0, h);
    cairo_pattern_add_color_stop_rgb(pattern, 0, 0.05, 0.15, 0.35);
    cairo_pattern_add_color_stop_rgb(pattern, 0.5, 0.03, 0.10, 0.25);
    cairo_pattern_add_color_stop_rgb(pattern, 1, 0.01, 0.05, 0.15);

    cairo_rectangle(cr, 0, 0, w, h);
    cairo_set_source(cr, pattern);
    cairo_fill(cr);
    cairo_pattern_destroy(pattern);

    cairo_set_source_rgb(cr, 0.76, 0.70, 0.50);
    cairo_rectangle(cr, 0, h - 50, w, 50);
    cairo_fill(cr);
}

void draw_food(cairo_t *cr)
{
    if (!g_aquarium)
        return;

    for (int i = 0; i < MAX_FOOD; ++i)
    {
        Food *pellet = &g_aquarium->env.food_sources[i];
        if (!pellet->is_alive)
            continue;

        float depth = 0.6f + pellet->y / (float)g_aquarium->height * 0.4f;
        if (depth > 0.95f) depth = 0.95f;
        if (depth < 0.55f) depth = 0.55f;
        cairo_set_source_rgba(cr, 0.96f, 0.80f, 0.35f, depth);
        cairo_arc(cr, pellet->x, pellet->y, 4.0f, 0, 2 * M_PI);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, 0.65f, 0.45f, 0.12f, depth * 0.8f);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, pellet->x - 1.5f, pellet->y - 2.5f);
        cairo_line_to(cr, pellet->x + 1.5f, pellet->y + 2.5f);
        cairo_stroke(cr);
    }
}

void draw_fish(cairo_t *cr, Fish *fish)
{
    if (!fish->is_alive)
        return;

    if (!is_fish_visible(fish))
        return;

    GdkPixbuf *current_image = (fish->direction == DIRECTION_RIGHT) ? fish->image_right : fish->image_left;

    // Calculate depth-based alpha: fish lower in tank (higher Y) are more visible
    float depth_alpha = 0.4f + (fish->y / g_aquarium->height) * 0.6f;
    depth_alpha = (depth_alpha < 0.4f) ? 0.4f : (depth_alpha > 1.0f) ? 1.0f : depth_alpha;

    if (current_image)
    {
        int img_w = gdk_pixbuf_get_width(current_image);
        int img_h = gdk_pixbuf_get_height(current_image);

        // Draw shadow beneath fish (depth cue) - keep this, it's subtle
        float shadow_y = fish->y + fish->size * 0.4f;
        float shadow_scale = fish->size * 1.2f;
        cairo_set_source_rgba(cr, 0, 0, 0, depth_alpha * 0.15f);  // Reduced opacity
        cairo_ellipse(cr, fish->x, shadow_y, shadow_scale * 1.2f, shadow_scale * 0.4f);
        cairo_fill(cr);

        // Selected fish highlight - keep this
        if (g_aquarium->show_selected_highlight && g_aquarium->selected_fish == fish)
        {
            cairo_set_source_rgba(cr, 1, 1, 0, 0.28);
            cairo_arc(cr, fish->x, fish->y, fish->size + 6, 0, 2 * M_PI);
            cairo_fill(cr);
        }

        if (fish->current_goal == GOAL_ESCAPE)
            cairo_set_source_rgba(cr, 0.35f, 0.75f, 1.0f, depth_alpha * 0.35f);
        else if (fish->current_goal == GOAL_HUNT)
            cairo_set_source_rgba(cr, 1.0f, 0.35f, 0.25f, depth_alpha * 0.35f);
        else if (fish->current_goal == GOAL_MATE)
            cairo_set_source_rgba(cr, 1.0f, 0.55f, 0.85f, depth_alpha * 0.30f);
        else if (fish->current_goal == GOAL_SEARCH_FOOD && fish->hunger > 60.0f)
            cairo_set_source_rgba(cr, 1.0f, 0.75f, 0.2f, depth_alpha * 0.28f);
        else if (fish->current_goal == GOAL_REST)
            cairo_set_source_rgba(cr, 0.5f, 0.9f, 0.5f, depth_alpha * 0.22f);
        else
            cairo_set_source_rgba(cr, 0, 0, 0, 0);

        if (fish->current_goal != GOAL_IDLE)
        {
            cairo_set_line_width(cr, 2.0);
            cairo_arc(cr, fish->x, fish->y, fish->size + 4, 0, 2 * M_PI);
            cairo_stroke(cr);
        }

        /* Procedural swim on a single PNG/JPG: rotate with heading, sway, tail shear, stroke scale */
        {
            float speed = sqrtf(fish->vx * fish->vx + fish->vy * fish->vy);
            float phase = fish->swim_phase;
            float tail = fish->tail_phase;
            float sway = sinf(phase * 1.8f + fish->id * 0.17f);
            float tail_wag = sinf(tail * 2.2f);
            float stroke = sinf(phase * 2.0f);

            float draw_angle = fish->heading;
            if (fish->direction == DIRECTION_LEFT)
                draw_angle -= (float)M_PI;

            float rotate_amp = 0.12f + (speed > 2.0f ? 0.06f : speed * 0.03f);
            float angle = draw_angle + sway * rotate_amp;

            float base_scale = fish->render_scale;
            float sx = base_scale * (1.0f + stroke * 0.065f);
            float sy = base_scale * (1.0f - stroke * 0.042f);
            float shear = tail_wag * 0.10f;
            if (fish->direction == DIRECTION_LEFT)
                shear = -shear;

            float bob = sinf(phase * 1.25f) * fish->size * 0.035f;
            float alpha = depth_alpha * fish->visibility;

            cairo_save(cr);
            cairo_translate(cr, fish->x, fish->y);
            cairo_rotate(cr, angle);

            cairo_matrix_t swim_m;
            cairo_matrix_init(&swim_m, sx, shear * 0.45f, shear, sy, 0.0, 0.0);
            cairo_transform(cr, &swim_m);

            gdk_cairo_set_source_pixbuf(cr, current_image, -img_w / 2.0f, -img_h / 2.0f + bob);
            cairo_paint_with_alpha(cr, alpha);
            cairo_restore(cr);

            if (speed > 0.8f)
            {
                float wake_alpha = depth_alpha * 0.12f * (speed / 4.0f);
                if (wake_alpha > 0.18f)
                    wake_alpha = 0.18f;
                cairo_set_source_rgba(cr, 0.55f, 0.82f, 1.0f, wake_alpha);
                cairo_set_line_width(cr, 1.2f);
                float behind = (fish->direction == DIRECTION_RIGHT) ? -1.0f : 1.0f;
                float tx = fish->size * 0.55f * behind;
                cairo_move_to(cr, fish->x + cosf(draw_angle) * tx, fish->y + sinf(draw_angle) * tx);
                for (int i = 1; i <= 4; i++)
                {
                    float t = (float)i * 4.0f;
                    float wag = sinf(tail + (float)i * 0.9f) * fish->size * 0.12f;
                    cairo_line_to(cr,
                                  fish->x + cosf(draw_angle) * (tx - t * behind) - sinf(draw_angle) * wag,
                                  fish->y + sinf(draw_angle) * (tx - t * behind) + cosf(draw_angle) * wag);
                }
                cairo_stroke(cr);
            }
        }

        // Leader crown indicator - keep this
        if (fish->is_leader && fish->group && fish->group->member_count > 1)
        {
            cairo_set_source_rgba(cr, 1, 0.85, 0, depth_alpha);
            cairo_move_to(cr, fish->x - 8, fish->y - img_h / 2 - 4);
            cairo_line_to(cr, fish->x, fish->y - img_h / 2 - 14);
            cairo_line_to(cr, fish->x + 8, fish->y - img_h / 2 - 4);
            cairo_fill(cr);
        }

        // Life bar (remaining lifespan) - keep this
        float bar_width = fish->size;
        float bar_height = 4;
        float bar_x = fish->x - bar_width / 2;
        float bar_y = fish->y - fish->size / 2 - 8;

        float life_fraction = 0.0f;
        if (fish->lifespan > 0.0f)
        {
            life_fraction = (fish->lifespan - fish->age) / fish->lifespan;
            if (life_fraction < 0.0f)
                life_fraction = 0.0f;
            else if (life_fraction > 1.0f)
                life_fraction = 1.0f;
        }

        cairo_set_source_rgba(cr, 0, 0, 0, 0.5 * depth_alpha);
        cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
        cairo_fill(cr);

        if (life_fraction > 0.65f)
            cairo_set_source_rgba(cr, 0.2, 0.85, 0.3, depth_alpha);
        else if (life_fraction > 0.30f)
            cairo_set_source_rgba(cr, 1.0, 0.72, 0.2, depth_alpha);
        else
            cairo_set_source_rgba(cr, 0.9, 0.25, 0.25, depth_alpha);
            
        cairo_rectangle(cr, bar_x, bar_y, bar_width * life_fraction, bar_height);
        cairo_fill(cr);

        // Health bar - keep this
        cairo_set_source_rgba(cr, 1, 1, 1, 0.85 * depth_alpha);
        cairo_rectangle(cr, bar_x, bar_y + bar_height + 1, bar_width * (fish->health / 100.0f), 2);
        cairo_fill(cr);

        // Gender indicator - keep this
        if (fish->gender == GENDER_MALE)
            cairo_set_source_rgba(cr, 0.3, 0.5, 1, depth_alpha);
        else
            cairo_set_source_rgba(cr, 1, 0.4, 0.7, depth_alpha);
            
        cairo_arc(cr, fish->x - img_w / 2 - 3, fish->y - img_h / 2 + 5, 2, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

void draw_stats_panel(cairo_t *cr)
{
    cairo_set_source_rgba(cr, 0, 0, 0, 0.75);
    cairo_rectangle(cr, 10, 10, 300, 200);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0.3, 0.8, 1);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 20, 30);
    cairo_show_text(cr, "🐠 AQUARIUM STATS");

    cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
    cairo_set_font_size(cr, 10);

    char line[128];
    int y = 50;

    cairo_move_to(cr, 20, y);
    snprintf(line, sizeof(line), "Fish: %d | Schools: %d",
             count_alive_fish(), count_groups());
    cairo_show_text(cr, line);

    y += 20;
    cairo_move_to(cr, 20, y);
    snprintf(line, sizeof(line), "Created: %d | Dead: %d | Eats: %d",
             g_aquarium->total_fish_created, g_aquarium->total_fish_dead, g_aquarium->total_eats);
    cairo_show_text(cr, line);

    y += 20;
    cairo_move_to(cr, 20, y);
    snprintf(line, sizeof(line), "Status: %s", g_aquarium->paused ? "PAUSED" : "RUNNING");
    cairo_show_text(cr, line);

    if (g_aquarium->selected_fish)
    {
        y += 25;
        cairo_set_source_rgb(cr, 0.3, 0.8, 1);
        cairo_move_to(cr, 20, y);
        snprintf(line, sizeof(line), "Selected: %s (ID:%d)",
                 g_aquarium->selected_fish->species,
                 g_aquarium->selected_fish->id);
        cairo_show_text(cr, line);

        y += 18;
        cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
        cairo_move_to(cr, 20, y);
        float life_remaining = g_aquarium->selected_fish->lifespan - g_aquarium->selected_fish->age;
        if (life_remaining < 0.0f)
            life_remaining = 0.0f;

        snprintf(line, sizeof(line), "Health: %.0f%% | Life: %.0f s | %s",
                 g_aquarium->selected_fish->health,
                 life_remaining,
                 g_aquarium->selected_fish->is_leader ? "LEADER" : "Follower");
        cairo_show_text(cr, line);

        y += 18;
        cairo_move_to(cr, 20, y);
        snprintf(line, sizeof(line), "%s | %s | %s",
                 fish_type_to_string(g_aquarium->selected_fish->type),
                 life_stage_to_string(g_aquarium->selected_fish->life_stage),
                 personality_to_string(g_aquarium->selected_fish->personality));
        cairo_show_text(cr, line);

        y += 18;
        cairo_move_to(cr, 20, y);
        snprintf(line, sizeof(line), "Goal: %s | Hunger: %.0f%%",
                 goal_to_string(g_aquarium->selected_fish->current_goal),
                 g_aquarium->selected_fish->hunger);
        cairo_show_text(cr, line);

        y += 18;
        cairo_move_to(cr, 20, y);
        const char *relation = "Solo";
        if (g_aquarium->selected_fish->group)
        {
            if (g_aquarium->selected_fish->type == TYPE_PREY)
                relation = "School (friends)";
            else
                relation = "Pack";
        }
        snprintf(line, sizeof(line), "Relation: %s | Gen: %d",
                 relation, g_aquarium->selected_fish->generation);
        cairo_show_text(cr, line);
    }
}

void draw_grid(cairo_t *cr)
{
    if (!g_aquarium->show_grid)
        return;

    cairo_set_source_rgba(cr, 1, 1, 1, 0.1);
    cairo_set_line_width(cr, 1);

    for (int x = 0; x < g_aquarium->width; x += 50)
    {
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, g_aquarium->height);
        cairo_stroke(cr);
    }

    for (int y = 0; y < g_aquarium->height; y += 50)
    {
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, g_aquarium->width, y);
        cairo_stroke(cr);
    }
}

void draw_help_overlay(cairo_t *cr)
{
    if (!g_aquarium->show_help)
        return;

    cairo_set_source_rgba(cr, 0, 0, 0, 0.85);
    cairo_rectangle(cr, 0, 0, g_aquarium->width, g_aquarium->height);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_font_size(cr, 12);

    const char *help[] = {
        "═══════════════════════════════════════════════════════",
        "                 AQUARIUM CONTROLS",
        "═══════════════════════════════════════════════════════",
        "",
        "FISH MANAGEMENT:",
        "   • Click any fish to select it",
        "   • 'Add Fish' - Custom tab or Library tab (preset images)",
        "   • 'Load Default' - Starter reef with schools and predators",
        "   • 'Delete Selected' - Remove selected fish",
        "   • 'Clear All' - Remove all fish and groups",
        "",
        "GROUP MANAGEMENT:",
        "   • 'Create Group' - Form new group (selected fish = leader)",
        "   • 'Add to Group' - Add selected fish to a group",
        "   • 'Remove from Group' - Remove fish from its group",
        "   • 'Delete Group' - Disband entire group",
        "   • 'View Group' - See group members",
        "",
        "KEYBOARD SHORTCUTS:",
        "   • SPACE - Pause/Resume",
        "   • G - Toggle grid",
        "   • H - Hide/Show help",
        "   • C - Clear all fish",
        "   • ESC - Clear selection",
        "",
        "SAVE/LOAD:",
        "   • 'Save HTML' - Save aquarium session as HTML file",
        "   • 'Load HTML' - Load aquarium session from HTML file",
        "",
        "═══════════════════════════════════════════════════════",
        NULL};

    int y = 50;
    for (int i = 0; help[i] != NULL; i++)
    {
        cairo_move_to(cr, 50, y);
        cairo_show_text(cr, help[i]);
        y += 22;
    }
}

gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    (void)data;
    if (!g_aquarium)
        return FALSE;

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    g_aquarium->width = alloc.width;
    g_aquarium->height = alloc.height;

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_GOOD);

    draw_water_background(cr);
    draw_background_effects(cr);
    draw_grid(cr);
    draw_food(cr);

    Fish *current = g_aquarium->fish_list;
    while (current)
    {
        if (current->is_alive)
        {
            draw_fish(cr, current);
        }
        current = current->next;
    }

    draw_stats_panel(cr);
    draw_help_overlay(cr);

    return FALSE;
}

// ============================================================================
// UI CALLBACKS
// ============================================================================

void update_ui_info(void)
{
    char info[512];
    snprintf(info, sizeof(info),
             "🐟 Fish: %d alive | 👥 Groups: %d | 🎂 Born: %d | 💀 Dead: %d | 🍽️ Eats: %d",
             count_alive_fish(),
             count_groups(),
             g_aquarium->total_fish_created,
             g_aquarium->total_fish_dead,
             g_aquarium->total_eats);

    gtk_label_set_text(GTK_LABEL(g_aquarium->info_label), info);
}

void on_fish_selected(Fish *fish)
{
    g_aquarium->selected_fish = fish;
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
}

// ============================================================================
// FISH DIALOG WITH IMAGE PICKER
// ============================================================================

static void on_preset_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    (void)box;
    GtkWidget *dialog = GTK_WIDGET(user_data);
    if (!row)
        return;
    int index = gtk_list_box_row_get_index(row);
    g_object_set_data(G_OBJECT(dialog), "preset_index", GINT_TO_POINTER(index));

    const PresetFish *preset = preset_fish_get(index);
    if (!preset)
        return;

    GtkWidget *preview = g_object_get_data(G_OBJECT(dialog), "preset_preview");
    GtkWidget *info = g_object_get_data(G_OBJECT(dialog), "preset_info");
    if (preview)
    {
        char *path = preset_fish_image_path(index);
        if (path)
        {
            GError *err = NULL;
            GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 120, 120, TRUE, &err);
            if (pb)
            {
                gtk_image_set_from_pixbuf(GTK_IMAGE(preview), pb);
                g_object_unref(pb);
            }
            else if (err)
                g_error_free(err);
            g_free(path);
        }
    }
    if (info)
    {
        char text[256];
        snprintf(text, sizeof(text), "<b>%s</b>\n%s · %s\nSuggested size %.0f (min %d) · speed %.1f",
                 preset->species, fish_type_to_string(preset->type),
                 preset->type == TYPE_PREDATOR ? "Enemy to prey" : "Schools with same species",
                 preset->default_size, (int)LIBRARY_MIN_FISH_SIZE, preset->default_speed);
        gtk_label_set_markup(GTK_LABEL(info), text);
    }

    GtkWidget *lib_size = g_object_get_data(G_OBJECT(dialog), "lib_size");
    if (lib_size)
    {
        float s = preset->default_size;
        if (s < LIBRARY_MIN_FISH_SIZE)
            s = LIBRARY_MIN_FISH_SIZE;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(lib_size), s);
    }
}

GtkWidget *create_fish_dialog(void)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Fish",
                                                    NULL, GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Add", GTK_RESPONSE_ACCEPT, NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 520, 680);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 8);

    GtkWidget *custom_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), custom_page, gtk_label_new("Custom"));

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);

    int row = 0;

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>🐟 Create New Fish</span>");
    gtk_grid_attach(GTK_GRID(grid), title, 0, row++, 2, 1);

    GtkWidget *species_label = gtk_label_new("Species Name:");
    gtk_widget_set_halign(species_label, GTK_ALIGN_START);
    GtkWidget *species_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(species_entry), "e.g., Tropical Fish");
    gtk_entry_set_text(GTK_ENTRY(species_entry), "Tropical Fish");
    gtk_grid_attach(GTK_GRID(grid), species_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), species_entry, 1, row, 1, 1);
    row++;

    GtkWidget *size_label = gtk_label_new("Size (20-180 pixels):");
    gtk_widget_set_halign(size_label, GTK_ALIGN_START);
    GtkWidget *size_spin = gtk_spin_button_new_with_range(20, 180, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(size_spin), 30);
    gtk_grid_attach(GTK_GRID(grid), size_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), size_spin, 1, row, 1, 1);
    row++;

    GtkWidget *count_label = gtk_label_new("Number to create:");
    gtk_widget_set_halign(count_label, GTK_ALIGN_START);
    GtkWidget *count_spin = gtk_spin_button_new_with_range(1, 30, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(count_spin), 5);
    gtk_grid_attach(GTK_GRID(grid), count_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), count_spin, 1, row, 1, 1);
    row++;

    GtkWidget *speed_label = gtk_label_new("Speed (1-5):");
    gtk_widget_set_halign(speed_label, GTK_ALIGN_START);
    GtkWidget *speed_spin = gtk_spin_button_new_with_range(1, 5, 0.5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(speed_spin), 2.5);
    gtk_grid_attach(GTK_GRID(grid), speed_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), speed_spin, 1, row, 1, 1);
    row++;

    GtkWidget *type_label = gtk_label_new("Fish Type:");
    gtk_widget_set_halign(type_label, GTK_ALIGN_START);
    GtkWidget *type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "🐟 Prey");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "🦈 Predator");
    gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), type_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), type_combo, 1, row, 1, 1);
    row++;

    GtkWidget *diet_label = gtk_label_new("Diet:");
    gtk_widget_set_halign(diet_label, GTK_ALIGN_START);
    GtkWidget *diet_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diet_combo), "🌿 Herbivore");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diet_combo), "🥩 Carnivore");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diet_combo), "🍽️ Omnivore");
    gtk_combo_box_set_active(GTK_COMBO_BOX(diet_combo), 2);
    gtk_grid_attach(GTK_GRID(grid), diet_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), diet_combo, 1, row, 1, 1);
    row++;

    GtkWidget *gender_label = gtk_label_new("Gender:");
    gtk_widget_set_halign(gender_label, GTK_ALIGN_START);
    GtkWidget *gender_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "♂ Male");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "♀ Female");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gender_combo), rand() % 2);
    gtk_grid_attach(GTK_GRID(grid), gender_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gender_combo, 1, row, 1, 1);
    row++;

    GtkWidget *facing_label = gtk_label_new("Image Facing:");
    gtk_widget_set_halign(facing_label, GTK_ALIGN_START);
    GtkWidget *facing_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(facing_combo), "→ Right");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(facing_combo), "← Left");
    gtk_combo_box_set_active(GTK_COMBO_BOX(facing_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), facing_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), facing_combo, 1, row, 1, 1);
    row++;

    GtkWidget *image_label = gtk_label_new("Custom Image (optional):");
    gtk_widget_set_halign(image_label, GTK_ALIGN_START);
    GtkWidget *image_button = gtk_file_chooser_button_new("Select PNG Image", GTK_FILE_CHOOSER_ACTION_OPEN);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Fish images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.PNG");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(image_button), filter);

    gtk_grid_attach(GTK_GRID(grid), image_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), image_button, 1, row, 1, 1);
    row++;

    GtkWidget *preview_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(preview_label), "<span size='small' foreground='#7f8c8d'>PNG/JPG with transparency work best</span>");
    gtk_grid_attach(GTK_GRID(grid), preview_label, 0, row, 2, 1);

    gtk_box_pack_start(GTK_BOX(custom_page), grid, TRUE, TRUE, 0);

    GtkWidget *library_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(library_page), 12);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), library_page, gtk_label_new("Library"));

    GtkWidget *lib_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lib_title), "<span weight='bold'>Default fish from images folder</span>");
    gtk_widget_set_halign(lib_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(library_page), lib_title, FALSE, FALSE, 0);

    GtkWidget *lib_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(library_page), lib_hbox, TRUE, TRUE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll, 260, 320);
    gtk_box_pack_start(GTK_BOX(lib_hbox), scroll, TRUE, TRUE, 0);

    GtkWidget *list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(scroll), list);

    for (int i = 0; i < preset_fish_count(); ++i)
    {
        const PresetFish *preset = preset_fish_get(i);
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(row_box), 6);

        GtkWidget *thumb = gtk_image_new();
        char *path = preset_fish_image_path(i);
        if (path)
        {
            GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 48, 48, TRUE, NULL);
            if (pb)
            {
                gtk_image_set_from_pixbuf(GTK_IMAGE(thumb), pb);
                g_object_unref(pb);
            }
            g_free(path);
        }

        GtkWidget *labels = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        char title[128];
        snprintf(title, sizeof(title), "<b>%s</b>", preset->species);
        GtkWidget *name_lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(name_lbl), title);
        gtk_widget_set_halign(name_lbl, GTK_ALIGN_START);

        char sub[128];
        snprintf(sub, sizeof(sub), "%s · %s", fish_type_to_string(preset->type),
                 preset->type == TYPE_PREDATOR ? "Predator" : "Prey / schools");
        GtkWidget *sub_lbl = gtk_label_new(sub);
        gtk_widget_set_halign(sub_lbl, GTK_ALIGN_START);

        gtk_box_pack_start(GTK_BOX(row_box), thumb, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row_box), labels, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(labels), name_lbl, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(labels), sub_lbl, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(row), row_box);
        gtk_list_box_insert(GTK_LIST_BOX(list), row, -1);
    }

    GtkWidget *detail_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(lib_hbox), detail_box, FALSE, FALSE, 0);

    GtkWidget *preset_preview = gtk_image_new();
    gtk_widget_set_size_request(preset_preview, 120, 120);
    gtk_box_pack_start(GTK_BOX(detail_box), preset_preview, FALSE, FALSE, 0);

    GtkWidget *preset_info = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(preset_info), TRUE);
    gtk_widget_set_size_request(preset_info, 180, -1);
    gtk_box_pack_start(GTK_BOX(detail_box), preset_info, FALSE, FALSE, 0);

    GtkWidget *lib_count_label = gtk_label_new("Number to add:");
    gtk_widget_set_halign(lib_count_label, GTK_ALIGN_START);
    GtkWidget *lib_count_spin = gtk_spin_button_new_with_range(1, 30, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lib_count_spin), 5);
    gtk_box_pack_start(GTK_BOX(detail_box), lib_count_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(detail_box), lib_count_spin, FALSE, FALSE, 0);

    GtkWidget *lib_size_label = gtk_label_new("Size (80-180 px):");
    gtk_widget_set_halign(lib_size_label, GTK_ALIGN_START);
    GtkWidget *lib_size_spin = gtk_spin_button_new_with_range(LIBRARY_MIN_FISH_SIZE, LIBRARY_MAX_FISH_SIZE, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lib_size_spin), LIBRARY_MIN_FISH_SIZE);
    gtk_box_pack_start(GTK_BOX(detail_box), lib_size_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(detail_box), lib_size_spin, FALSE, FALSE, 0);

    GtkWidget *lib_type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lib_type_combo), "Use catalog type");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lib_type_combo), "Force Prey");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lib_type_combo), "Force Predator");
    gtk_combo_box_set_active(GTK_COMBO_BOX(lib_type_combo), 0);
    gtk_box_pack_start(GTK_BOX(detail_box), lib_type_combo, FALSE, FALSE, 0);

    g_signal_connect(list, "row-selected", G_CALLBACK(on_preset_row_selected), dialog);
    gtk_list_box_select_row(GTK_LIST_BOX(list), gtk_list_box_get_row_at_index(GTK_LIST_BOX(list), 0));

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), notebook);
    gtk_widget_show_all(dialog);

    g_object_set_data(G_OBJECT(dialog), "notebook", notebook);
    g_object_set_data(G_OBJECT(dialog), "species", species_entry);
    g_object_set_data(G_OBJECT(dialog), "size", size_spin);
    g_object_set_data(G_OBJECT(dialog), "count", count_spin);
    g_object_set_data(G_OBJECT(dialog), "speed", speed_spin);
    g_object_set_data(G_OBJECT(dialog), "type", type_combo);
    g_object_set_data(G_OBJECT(dialog), "diet", diet_combo);
    g_object_set_data(G_OBJECT(dialog), "gender", gender_combo);
    g_object_set_data(G_OBJECT(dialog), "facing", facing_combo);
    g_object_set_data(G_OBJECT(dialog), "image", image_button);
    g_object_set_data(G_OBJECT(dialog), "preset_index", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(dialog), "preset_preview", preset_preview);
    g_object_set_data(G_OBJECT(dialog), "preset_info", preset_info);
    g_object_set_data(G_OBJECT(dialog), "lib_count", lib_count_spin);
    g_object_set_data(G_OBJECT(dialog), "lib_size", lib_size_spin);
    g_object_set_data(G_OBJECT(dialog), "lib_type", lib_type_combo);

    g_signal_connect(dialog, "response", G_CALLBACK(on_fish_dialog_response), NULL);

    return dialog;
}

void on_fish_dialog_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    (void)data;
    if (response_id == GTK_RESPONSE_ACCEPT)
    {
        GtkNotebook *notebook = GTK_NOTEBOOK(g_object_get_data(G_OBJECT(dialog), "notebook"));
        int page = notebook ? gtk_notebook_get_current_page(notebook) : 0;

        if (page == 1)
        {
            int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "preset_index"));
            const PresetFish *preset = preset_fish_get(idx);
            if (!preset)
                return;

            int count = (int)gtk_spin_button_get_value(g_object_get_data(G_OBJECT(dialog), "lib_count"));
            int type_mode = gtk_combo_box_get_active(g_object_get_data(G_OBJECT(dialog), "lib_type"));
            FishType type_override = preset->type;
            int type_use_override = 0;
            if (type_mode == 1)
            {
                type_override = TYPE_PREY;
                type_use_override = 1;
            }
            else if (type_mode == 2)
            {
                type_override = TYPE_PREDATOR;
                type_use_override = 1;
            }

            float size = gtk_spin_button_get_value(g_object_get_data(G_OBJECT(dialog), "lib_size"));
            float speed = gtk_spin_button_get_value(g_object_get_data(G_OBJECT(dialog), "speed"));
            if (size < LIBRARY_MIN_FISH_SIZE)
                size = LIBRARY_MIN_FISH_SIZE;
            spawn_fish_from_preset(preset, count, size, speed, type_override, type_use_override);

            char msg[256];
            snprintf(msg, sizeof(msg), "Added %d %s from library", count, preset->species);
            show_message(msg);
            update_ui_info();
            gtk_widget_queue_draw(g_aquarium->drawing_area);
            return;
        }

        const char *species = gtk_entry_get_text(g_object_get_data(G_OBJECT(dialog), "species"));
        float size = gtk_spin_button_get_value(g_object_get_data(G_OBJECT(dialog), "size"));
        size = 90.0f; // Scale to internal size range
        int count = (int)gtk_spin_button_get_value(g_object_get_data(G_OBJECT(dialog), "count"));
        float speed = gtk_spin_button_get_value(g_object_get_data(G_OBJECT(dialog), "speed"));

        int type_idx = gtk_combo_box_get_active(g_object_get_data(G_OBJECT(dialog), "type"));
        FishType type = type_idx == 1 ? TYPE_PREDATOR : TYPE_PREY;

        int diet_idx = gtk_combo_box_get_active(g_object_get_data(G_OBJECT(dialog), "diet"));
        DietType diet = diet_idx == 0 ? DIET_HERBIVORE : (diet_idx == 1 ? DIET_CARNIVORE : DIET_OMNIVORE);

        int gender_idx = gtk_combo_box_get_active(g_object_get_data(G_OBJECT(dialog), "gender"));
        Gender gender = gender_idx == 0 ? GENDER_MALE : GENDER_FEMALE;

        int facing_idx = gtk_combo_box_get_active(g_object_get_data(G_OBJECT(dialog), "facing"));
        FishDirection facing = facing_idx == 0 ? DIRECTION_RIGHT : DIRECTION_LEFT;

        char *image_path = gtk_file_chooser_get_filename(g_object_get_data(G_OBJECT(dialog), "image"));

        Fish *leader = NULL;
        Group *school = NULL;

        float base_x = 100 + rand() % (g_aquarium->width - 200);
        float base_y = 100 + rand() % (g_aquarium->height - 200);

        for (int i = 0; i < count; ++i)
        {
            float offset_x = (rand() % 40 - 20) + (i * 8);
            float offset_y = (rand() % 30 - 15);
            Fish *new_fish = create_fish(species, image_path, base_x + offset_x, base_y + offset_y, size, speed, type, diet, gender, facing);
            add_fish(new_fish);

            if (!leader)
            {
                leader = new_fish;
                char group_name[128];
                snprintf(group_name, sizeof(group_name), "%s School", species);
                school = create_group(group_name, leader);
                g_aquarium->selected_fish = leader;
            }
            else if (school)
            {
                add_fish_to_group(school, new_fish);
            }
        }

        char msg[256];
        snprintf(msg, sizeof(msg), "✅ Added %d %s%s (leader ID: %d)%s",
                 count, species, count == 1 ? "" : " fish", leader ? leader->id : -1,
                 image_path ? " with custom image" : "");
        show_message(msg);

        if (image_path)
            g_free(image_path);

        gtk_widget_queue_draw(g_aquarium->drawing_area);
    }
}

// ============================================================================
// GROUP DIALOG
// ============================================================================

GtkWidget *create_group_dialog(void)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create Group",
                                                    NULL, GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Create", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);

    GtkWidget *name_label = gtk_label_new("Group Name:");
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(name_entry), "School");
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);

    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), "<span size='small'>👑 Selected fish will become the group leader</span>");
    gtk_grid_attach(GTK_GRID(grid), info_label, 0, 1, 2, 1);

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
    gtk_widget_show_all(dialog);

    g_object_set_data(G_OBJECT(dialog), "group_name", name_entry);

    g_signal_connect(dialog, "response", G_CALLBACK(on_group_dialog_response), NULL);

    return dialog;
}

void on_group_dialog_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    (void)data;
    if (response_id == GTK_RESPONSE_ACCEPT && g_aquarium->selected_fish)
    {
        const char *name = gtk_entry_get_text(g_object_get_data(G_OBJECT(dialog), "group_name"));
        create_group(name, g_aquarium->selected_fish);
        show_message("✅ Group created!");
        update_ui_info();
        gtk_widget_queue_draw(g_aquarium->drawing_area);
    }
}

// ============================================================================
// OTHER UI CALLBACKS
// ============================================================================

void on_add_fish_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    GtkWidget *dialog = create_fish_dialog();
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_feed_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    // Spawn food pellets from center-top of aquarium
    if (g_aquarium)
    {
        spawn_food_pellets(12, g_aquarium->width / 2.0f, 60.0f);
        show_message("🍽️ Spawned 12 food pellets!");
    }
}

void on_create_group_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    if (!g_aquarium->selected_fish)
    {
        show_message("Please select a fish to be the group leader first!");
        return;
    }

    if (g_aquarium->selected_fish->group)
    {
        show_message("This fish is already in a group! Remove it first.");
        return;
    }

    GtkWidget *dialog = create_group_dialog();
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_add_to_group_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    if (!g_aquarium->selected_fish)
    {
        show_message("Please select a fish first!");
        return;
    }

    if (g_aquarium->selected_fish->group)
    {
        show_message("Fish is already in a group! Remove it first.");
        return;
    }

    if (!g_aquarium->group_list)
    {
        show_message("No groups available! Create a group first.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add to Group",
                                                    NULL, GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Add", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *combo = gtk_combo_box_text_new();
    Group *group = g_aquarium->group_list;
    while (group)
    {
        char name[128];
        snprintf(name, sizeof(name), "Group %d: %s (%d fish)", group->id, group->name, group->member_count);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), name);
        group = group->next;
    }

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), combo);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        if (index >= 0)
        {
            group = g_aquarium->group_list;
            for (int i = 0; i < index && group; i++)
                group = group->next;
            if (group)
            {
                add_fish_to_group(group, g_aquarium->selected_fish);
                show_message("✅ Fish added to group!");
            }
        }
    }
    gtk_widget_destroy(dialog);
}

void on_remove_from_group_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    if (!g_aquarium->selected_fish)
    {
        show_message("Please select a fish first!");
        return;
    }

    if (!g_aquarium->selected_fish->group)
    {
        show_message("Fish is not in any group!");
        return;
    }

    remove_fish_from_group(g_aquarium->selected_fish);
    show_message("✅ Fish removed from group!");
}

void on_delete_group_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    if (!g_aquarium->group_list)
    {
        show_message("No groups to delete!");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Delete Group",
                                                    NULL, GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Delete", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *combo = gtk_combo_box_text_new();
    Group *group = g_aquarium->group_list;
    while (group)
    {
        char name[128];
        snprintf(name, sizeof(name), "Group %d: %s (%d fish)", group->id, group->name, group->member_count);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), name);
        group = group->next;
    }

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), combo);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        if (index >= 0)
        {
            group = g_aquarium->group_list;
            for (int i = 0; i < index && group; i++)
                group = group->next;
            if (group)
            {
                delete_group(group);
                show_message("✅ Group deleted!");
            }
        }
    }
    gtk_widget_destroy(dialog);
}

void on_delete_fish_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    if (!g_aquarium->selected_fish)
    {
        show_message("Please select a fish to delete!");
        return;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Delete %s (ID: %d)?",
             g_aquarium->selected_fish->species, g_aquarium->selected_fish->id);

    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", msg);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
    {
        remove_fish(g_aquarium->selected_fish);
        g_aquarium->selected_fish = NULL;
        show_message("✅ Fish deleted!");
    }
    gtk_widget_destroy(dialog);
}

void on_view_group_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    if (!g_aquarium->selected_fish || !g_aquarium->selected_fish->group)
    {
        show_message("Selected fish is not in a group!");
        return;
    }

    Group *group = g_aquarium->selected_fish->group;
    char msg[1024];
    snprintf(msg, sizeof(msg),
             "📊 GROUP INFO\n\n"
             "Name: %s (ID: %d)\n"
             "Leader: %s (ID: %d) 👑\n"
             "Total Members: %d fish\n\n"
             "━━━ MEMBERS LIST ━━━\n",
             group->name, group->id,
             group->leader->species, group->leader->id,
             group->member_count);

    char members[512] = "";
    Fish *current = group->members_head;
    int member_num = 1;
    while (current)
    {
        char member[128];
        snprintf(member, sizeof(member), "%d. %s (ID: %d) %s\n",
                 member_num++, current->species, current->id,
                 current->is_leader ? "👑 LEADER" : "");
        strcat(members, member);
        current = current->group_next;
    }
    strcat(msg, members);

    show_message(msg);
}

void on_toggle_help(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    g_aquarium->show_help = !g_aquarium->show_help;
    gtk_widget_queue_draw(g_aquarium->drawing_area);
}

void on_toggle_highlight_clicked(GtkWidget *widget, gpointer data)
{
    (void)data;
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    g_aquarium->show_selected_highlight = active ? 1 : 0;
    gtk_button_set_label(GTK_BUTTON(widget), active ? "🔆 Highlight ON" : "🔆 Highlight OFF");
    show_message(g_aquarium->show_selected_highlight ? "🔆 Fish highlighting enabled" : "🔅 Fish highlighting disabled");
    gtk_widget_queue_draw(g_aquarium->drawing_area);
}

void on_toggle_grid(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    g_aquarium->show_grid = !g_aquarium->show_grid;
    gtk_widget_queue_draw(g_aquarium->drawing_area);
}

void on_clear_simulation(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                               "⚠️ Clear all fish and groups? This cannot be undone!");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
    {
        Fish *current = g_aquarium->fish_list;
        while (current)
        {
            Fish *next = current->next;
            g_free(current->species);
            if (current->image_path)
                g_free(current->image_path);
            if (current->image_left)
                g_object_unref(current->image_left);
            if (current->image_right)
                g_object_unref(current->image_right);
            g_free(current);
            current = next;
        }

        Group *group = g_aquarium->group_list;
        while (group)
        {
            Group *next = group->next;
            g_free(group->name);
            free(group);
            group = next;
        }

        g_aquarium->fish_list = NULL;
        g_aquarium->group_list = NULL;
        g_aquarium->selected_fish = NULL;
        g_aquarium->show_selected_highlight = 0;
        g_aquarium->next_fish_id = 1;
        g_aquarium->next_group_id = 1;
        g_aquarium->total_fish_created = 0;
        g_aquarium->total_fish_dead = 0;
        g_aquarium->total_eats = 0;

        show_message("✅ Simulation cleared!");
        update_ui_info();
        gtk_widget_queue_draw(g_aquarium->drawing_area);
    }
    gtk_widget_destroy(dialog);
}

gboolean on_mouse_click(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    (void)widget;
    (void)data;

    if (event->button == 1)
    {
        Fish *current = g_aquarium->fish_list;
        Fish *closest = NULL;
        float min_dist = 30;

        while (current)
        {
            if (current->is_alive)
            {
                float dx = current->x - event->x;
                float dy = current->y - event->y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    closest = current;
                }
            }
            current = current->next;
        }

        on_fish_selected(closest);
    }
    return FALSE;
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    (void)widget;
    (void)data;

    switch (event->keyval)
    {
    case GDK_KEY_space:
        g_aquarium->paused = !g_aquarium->paused;
        show_message(g_aquarium->paused ? "⏸ Paused" : "▶ Resumed");
        return TRUE;
    case GDK_KEY_g:
    case GDK_KEY_G:
        on_toggle_grid(NULL, NULL);
        return TRUE;
    case GDK_KEY_h:
    case GDK_KEY_H:
        on_toggle_help(NULL, NULL);
        return TRUE;
    case GDK_KEY_c:
    case GDK_KEY_C:
        on_clear_simulation(NULL, NULL);
        return TRUE;
    case GDK_KEY_Escape:
        g_aquarium->selected_fish = NULL;
        update_ui_info();
        gtk_widget_queue_draw(g_aquarium->drawing_area);
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

// ============================================================================
// HTML SESSION SAVE/LOAD (NO JSON - Pure HTML)
// ============================================================================

// Helper function to extract text between tags
char* extract_tag_content(const char *html, const char *tag, const char *end_boundary) {
    char search_tag[256];
    char end_tag[256];
    snprintf(search_tag, sizeof(search_tag), "<%s>", tag);
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag);
    
    char *start = strstr(html, search_tag);
    if (!start || (end_boundary && start > end_boundary)) return NULL;
    
    start += strlen(search_tag);
    char *end = strstr(start, end_tag);
    if (!end || (end_boundary && end > end_boundary)) return NULL;
    
    int len = end - start;
    char *result = malloc(len + 1);
    if (result) {
        strncpy(result, start, len);
        result[len] = '\0';
    }
    return result;
}

// Save aquarium session as HTML file
void save_aquarium_as_html(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        show_message("❌ Cannot create HTML session file!");
        return;
    }
    
    fprintf(file, "<!DOCTYPE html>\n");
    fprintf(file, "<html>\n");
    fprintf(file, "<head><meta charset=\"UTF-8\"><title>Aquarium Session</title></head>\n");
    fprintf(file, "<body>\n");
    fprintf(file, "<window title=\"Aquarium Simulator\" width=\"%d\" height=\"%d\">\n", 
            g_aquarium->width, g_aquarium->height);
    fprintf(file, "    <metadata>\n");
    fprintf(file, "        <total_fish_created>%d</total_fish_created>\n", g_aquarium->total_fish_created);
    fprintf(file, "        <total_fish_dead>%d</total_fish_dead>\n", g_aquarium->total_fish_dead);
    fprintf(file, "        <total_eats>%d</total_eats>\n", g_aquarium->total_eats);
    fprintf(file, "        <next_fish_id>%d</next_fish_id>\n", g_aquarium->next_fish_id);
    fprintf(file, "        <next_group_id>%d</next_group_id>\n", g_aquarium->next_group_id);
    fprintf(file, "        <paused>%s</paused>\n", g_aquarium->paused ? "true" : "false");
    fprintf(file, "    </metadata>\n");
    fprintf(file, "    <fish_container>\n");
    
    Fish *fish = g_aquarium->fish_list;
    while (fish) {
        if (fish->is_alive) {
            fprintf(file, "        <div id=\"fish_%d\">\n", fish->id);
            fprintf(file, "            <species>%s</species>\n", fish->species);
            fprintf(file, "            <x>%.2f</x>\n", fish->x);
            fprintf(file, "            <y>%.2f</y>\n", fish->y);
            fprintf(file, "            <vx>%.2f</vx>\n", fish->vx);
            fprintf(file, "            <vy>%.2f</vy>\n", fish->vy);
            fprintf(file, "            <size>%.0f</size>\n", fish->size);
            fprintf(file, "            <speed>%.2f</speed>\n", fish->speed);
            fprintf(file, "            <type>%d</type>\n", fish->type);
            fprintf(file, "            <diet>%d</diet>\n", fish->diet);
            fprintf(file, "            <gender>%d</gender>\n", fish->gender);
            fprintf(file, "            <health>%.1f</health>\n", fish->health);
            fprintf(file, "            <hunger>%.1f</hunger>\n", fish->hunger);
            fprintf(file, "            <energy>%.1f</energy>\n", fish->energy);
            fprintf(file, "            <age>%.1f</age>\n", fish->age);
            fprintf(file, "            <image_facing>%d</image_facing>\n", fish->image_facing);
            fprintf(file, "            <image_path>%s</image_path>\n", fish->image_path ? fish->image_path : "");
            fprintf(file, "            <group_id>%d</group_id>\n", fish->group ? fish->group->id : -1);
            fprintf(file, "            <is_leader>%s</is_leader>\n", fish->is_leader ? "true" : "false");
            // Ecosystem state fields
            fprintf(file, "            <personality>%d</personality>\n", fish->personality);
            fprintf(file, "            <life_stage>%d</life_stage>\n", fish->life_stage);
            fprintf(file, "            <generation>%d</generation>\n", fish->generation);
            fprintf(file, "            <lifespan>%.0f</lifespan>\n", fish->lifespan);
            fprintf(file, "            <base_size>%.1f</base_size>\n", fish->base_size);
            fprintf(file, "            <base_speed>%.2f</base_speed>\n", fish->base_speed);
            fprintf(file, "        </div>\n");
        }
        fish = fish->next;
    }
    
    fprintf(file, "    </fish_container>\n");
    fprintf(file, "    <group_container>\n");
    
    Group *group = g_aquarium->group_list;
    while (group) {
        fprintf(file, "        <div id=\"group_%d\">\n", group->id);
        fprintf(file, "            <group_name>%s</group_name>\n", group->name);
        fprintf(file, "            <leader_id>%d</leader_id>\n", group->leader->id);
        fprintf(file, "            <members>");
        Fish *member = group->members_head;
        int first = 1;
        while (member) {
            if (!first) fprintf(file, ",");
            fprintf(file, "%d", member->id);
            first = 0;
            member = member->group_next;
        }
        fprintf(file, "</members>\n");
        fprintf(file, "        </div>\n");
        group = group->next;
    }
    
    fprintf(file, "    </group_container>\n");
    fprintf(file, "</window>\n");
    fprintf(file, "</body>\n");
    fprintf(file, "</html>\n");
    
    fclose(file);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "✅ Session saved as HTML: %s", filename);
    show_message(msg);
}

// Load aquarium from HTML session file
void load_aquarium_from_html(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        show_message("❌ Cannot open HTML session file!");
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *html_content = malloc(file_size + 1);
    fread(html_content, 1, file_size, file);
    html_content[file_size] = '\0';
    fclose(file);
    
    on_clear_simulation(NULL, NULL);
    
    char *metadata = strstr(html_content, "<metadata>");
    char *metadata_end = strstr(html_content, "</metadata>");
    if (metadata && metadata_end) {
        char *total_created = extract_tag_content(metadata, "total_fish_created", metadata_end);
        char *total_dead = extract_tag_content(metadata, "total_fish_dead", metadata_end);
        char *total_eats_val = extract_tag_content(metadata, "total_eats", metadata_end);
        char *next_fish = extract_tag_content(metadata, "next_fish_id", metadata_end);
        char *next_group = extract_tag_content(metadata, "next_group_id", metadata_end);
        
        if (total_created) g_aquarium->total_fish_created = atoi(total_created);
        if (total_dead) g_aquarium->total_fish_dead = atoi(total_dead);
        if (total_eats_val) g_aquarium->total_eats = atoi(total_eats_val);
        if (next_fish) g_aquarium->next_fish_id = atoi(next_fish);
        if (next_group) g_aquarium->next_group_id = atoi(next_group);
        
        free(total_created); free(total_dead); free(total_eats_val); free(next_fish); free(next_group);
    }
    
    char *fish_container = strstr(html_content, "<fish_container>");
    char *fish_end = strstr(html_content, "</fish_container>");
    if (fish_container && fish_end) {
        char *div = fish_container;
        while ((div = strstr(div + 1, "<div id=\"fish_")) && div < fish_end) {
            int fish_id = 0;
            char *id_start = strstr(div, "id=\"fish_");
            if (id_start) {
                id_start += 9;
                char *id_end = strchr(id_start, '"');
                if (id_end) {
                    char id_str[32];
                    int len = id_end - id_start;
                    if (len < 32) {
                        strncpy(id_str, id_start, len);
                        id_str[len] = '\0';
                        fish_id = atoi(id_str);
                    }
                }
            }
            
            char *species = extract_tag_content(div, "species", fish_end);
            char *x_str = extract_tag_content(div, "x", fish_end);
            char *y_str = extract_tag_content(div, "y", fish_end);
            char *size_str = extract_tag_content(div, "size", fish_end);
            char *speed_str = extract_tag_content(div, "speed", fish_end);
            char *type_str = extract_tag_content(div, "type", fish_end);
            char *diet_str = extract_tag_content(div, "diet", fish_end);
            char *gender_str = extract_tag_content(div, "gender", fish_end);
            char *facing_str = extract_tag_content(div, "image_facing", fish_end);
            char *image_path = extract_tag_content(div, "image_path", fish_end);
            
            // Ecosystem state fields
            char *personality_str = extract_tag_content(div, "personality", fish_end);
            char *life_stage_str = extract_tag_content(div, "life_stage", fish_end);
            char *generation_str = extract_tag_content(div, "generation", fish_end);
            char *lifespan_str = extract_tag_content(div, "lifespan", fish_end);
            char *base_size_str = extract_tag_content(div, "base_size", fish_end);
            char *base_speed_str = extract_tag_content(div, "base_speed", fish_end);
            
            if (species && x_str && y_str) {
                Fish *new_fish = create_fish(species, image_path && image_path[0] ? image_path : NULL,
                                              atof(x_str), atof(y_str), atof(size_str), atof(speed_str),
                                              atoi(type_str), atoi(diet_str), atoi(gender_str), atoi(facing_str));
                new_fish->id = fish_id;
                
                // Restore ecosystem state
                if (personality_str) new_fish->personality = atoi(personality_str);
                if (life_stage_str) new_fish->life_stage = atoi(life_stage_str);
                if (generation_str) new_fish->generation = atoi(generation_str);
                if (lifespan_str) new_fish->lifespan = atof(lifespan_str);
                if (base_size_str) new_fish->base_size = atof(base_size_str);
                if (base_speed_str) new_fish->base_speed = atof(base_speed_str);
                
                add_fish(new_fish);
            }
            
            free(species); free(x_str); free(y_str); free(size_str); free(speed_str);
            free(type_str); free(diet_str); free(gender_str); free(facing_str); free(image_path);
            free(personality_str); free(life_stage_str); free(generation_str);
            free(lifespan_str); free(base_size_str); free(base_speed_str);
        }
    }
    
    char *group_container = strstr(html_content, "<group_container>");
    char *group_end = strstr(html_content, "</group_container>");
    if (group_container && group_end) {
        char *div = group_container;
        while ((div = strstr(div + 1, "<div id=\"group_")) && div < group_end) {
            char *group_name = extract_tag_content(div, "group_name", group_end);
            char *leader_id_str = extract_tag_content(div, "leader_id", group_end);
            
            if (group_name && leader_id_str) {
                int leader_id = atoi(leader_id_str);
                Fish *leader = g_aquarium->fish_list;
                while (leader && leader->id != leader_id) leader = leader->next;
                
                if (leader) {
                    create_group(group_name, leader);
                }
            }
            
            free(group_name); free(leader_id_str);
        }
    }
    
    free(html_content);
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "✅ Loaded %d fish from HTML session", count_alive_fish());
    show_message(msg);
}

// HTML save dialog
void save_html_session_dialog(void) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Aquarium Session (HTML)",
                          NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                          "_Cancel", GTK_RESPONSE_CANCEL,
                          "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "aquarium_session.html");
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "HTML Files");
    gtk_file_filter_add_pattern(filter, "*.html");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        save_aquarium_as_html(filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

// HTML load dialog
void load_html_session_dialog(void) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Load Aquarium Session (HTML)",
                          NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                          "_Cancel", GTK_RESPONSE_CANCEL,
                          "_Load", GTK_RESPONSE_ACCEPT, NULL);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "HTML Files");
    gtk_file_filter_add_pattern(filter, "*.html");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        on_clear_simulation(NULL, NULL);
        load_aquarium_from_html(filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_save_html_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    save_html_session_dialog();
}

void on_load_html_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    load_html_session_dialog();
}


//INITIALIZATION

void init_aquarium(int width, int height) {
    g_aquarium = malloc(sizeof(Aquarium));
    memset(g_aquarium, 0, sizeof(Aquarium));//pour eviter les valeurs indefinies, j ai  zero est une valeur sûre pour les pointeurs et les flags
    
    g_aquarium->width = width;
    g_aquarium->height = height;
    g_aquarium->is_running = 1;
    g_aquarium->next_fish_id = 1;
    g_aquarium->next_group_id = 1;
    g_aquarium->total_fish_created = 0;
    g_aquarium->total_fish_dead = 0;
    g_aquarium->total_eats = 0;
    g_aquarium->show_help = 1;
    g_aquarium->show_grid = 0;
    g_aquarium->show_selected_highlight = 0;
    g_aquarium->paused = 0;
    g_aquarium->selected_fish = NULL;
    g_aquarium->fish_list = NULL;
    g_aquarium->group_list = NULL;
    
    srand(time(NULL));
    init_particle_system();
    g_aquarium->animation_timer = g_timeout_add(FRAME_INTERVAL, animation_tick, NULL);
}

void on_load_default_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (!g_aquarium)
        return;

    on_clear_simulation(NULL, NULL);

    spawn_fish_from_preset(preset_fish_get(0), 4, 0, 0, TYPE_PREY, 0);
    spawn_fish_from_preset(preset_fish_get(1), 5, 0, 0, TYPE_PREY, 0);
    spawn_fish_from_preset(preset_fish_get(5), 4, 0, 0, TYPE_PREY, 0);
    spawn_fish_from_preset(preset_fish_get(11), 2, 0, 0, TYPE_PREDATOR, 1);
    spawn_fish_from_preset(preset_fish_get(14), 1, 0, 0, TYPE_PREDATOR, 1);

    spawn_food_pellets(12, g_aquarium->width * 0.5f, 80.0f);
    show_message("Default reef loaded: schools, predators, and food.");
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
}

void create_aquarium_window(void)
{
    ProgramStart();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);
    gtk_window_set_title(GTK_WINDOW(window), "🐠 Aquarium Simulator");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(toolbar, 10);
    gtk_widget_set_margin_end(toolbar, 10);
    gtk_widget_set_margin_top(toolbar, 5);
    gtk_widget_set_margin_bottom(toolbar, 5);

    GtkWidget *add_btn = gtk_button_new_with_label("➕ Add Fish");
    GtkWidget *delete_btn = gtk_button_new_with_label("🗑 Delete Selected");
    GtkWidget *clear_btn = gtk_button_new_with_label("🗑 Clear All");
    GtkWidget *feed_btn = gtk_button_new_with_label("🍽️ FEED FISH");
    GtkWidget *highlight_btn = gtk_toggle_button_new_with_label("🔆 Highlight OFF");
    GtkWidget *create_group_btn = gtk_button_new_with_label("👥 Create Group");
    GtkWidget *add_to_group_btn = gtk_button_new_with_label("➕ Add to Group");
    GtkWidget *remove_from_group_btn = gtk_button_new_with_label("➖ Remove from Group");
    GtkWidget *delete_group_btn = gtk_button_new_with_label("❌ Delete Group");
    GtkWidget *view_group_btn = gtk_button_new_with_label("👁️ View Group");
    GtkWidget *grid_btn = gtk_button_new_with_label("📊 Grid");
    GtkWidget *help_btn = gtk_button_new_with_label("❓ Help");
    GtkWidget *save_html_btn = gtk_button_new_with_label("💾 Save HTML");
    GtkWidget *load_html_btn = gtk_button_new_with_label("📂 Load HTML");
    //GtkWidget *load_default_btn = gtk_button_new_with_label("Load Default");

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_fish_clicked), NULL);
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_fish_clicked), NULL);
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_simulation), NULL);
    g_signal_connect(feed_btn, "clicked", G_CALLBACK(on_feed_clicked), NULL);
    g_signal_connect(highlight_btn, "clicked", G_CALLBACK(on_toggle_highlight_clicked), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(highlight_btn), FALSE);
    g_signal_connect(create_group_btn, "clicked", G_CALLBACK(on_create_group_clicked), NULL);
    g_signal_connect(add_to_group_btn, "clicked", G_CALLBACK(on_add_to_group_clicked), NULL);
    g_signal_connect(remove_from_group_btn, "clicked", G_CALLBACK(on_remove_from_group_clicked), NULL);
    g_signal_connect(delete_group_btn, "clicked", G_CALLBACK(on_delete_group_clicked), NULL);
    g_signal_connect(view_group_btn, "clicked", G_CALLBACK(on_view_group_clicked), NULL);
    g_signal_connect(grid_btn, "clicked", G_CALLBACK(on_toggle_grid), NULL);
    g_signal_connect(help_btn, "clicked", G_CALLBACK(on_toggle_help), NULL);
    g_signal_connect(save_html_btn, "clicked", G_CALLBACK(on_save_html_clicked), NULL);
    g_signal_connect(load_html_btn, "clicked", G_CALLBACK(on_load_html_clicked), NULL);
    //g_signal_connect(load_default_btn, "clicked", G_CALLBACK(on_load_default_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), add_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), delete_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), clear_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), feed_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), highlight_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(toolbar), create_group_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), add_to_group_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), remove_from_group_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), delete_group_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), view_group_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 10);
    gtk_box_pack_end(GTK_BOX(toolbar), save_html_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(toolbar), load_html_btn, FALSE, FALSE, 0);
    //gtk_box_pack_end(GTK_BOX(toolbar), load_default_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(toolbar), help_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(toolbar), grid_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);

    g_aquarium->info_label = gtk_label_new("🐠 Aquarium Simulator - Click on fish to select");
    gtk_widget_set_margin_start(g_aquarium->info_label, 10);
    gtk_widget_set_margin_end(g_aquarium->info_label, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), g_aquarium->info_label, FALSE, FALSE, 0);

    g_aquarium->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(g_aquarium->drawing_area, TRUE);
    gtk_widget_set_vexpand(g_aquarium->drawing_area, TRUE);

    g_signal_connect(g_aquarium->drawing_area, "draw", G_CALLBACK(draw_callback), NULL);
    g_signal_connect(g_aquarium->drawing_area, "button-press-event", G_CALLBACK(on_mouse_click), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    gtk_widget_add_events(g_aquarium->drawing_area, GDK_BUTTON_PRESS_MASK);
    gtk_box_pack_start(GTK_BOX(main_vbox), g_aquarium->drawing_area, TRUE, TRUE, 0);

    g_aquarium->status_bar = gtk_label_new("💡 Tips: Click on fish to select | SPACE to pause | H for help | C to clear");
    gtk_widget_set_margin_start(g_aquarium->status_bar, 10);
    gtk_widget_set_margin_end(g_aquarium->status_bar, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), g_aquarium->status_bar, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);

    MainStart();
    cleanup_aquarium();
}

void cleanup_aquarium(void) {
    if (!g_aquarium) return;
    
    if (g_aquarium->animation_timer) g_source_remove(g_aquarium->animation_timer);
    
    Fish *fish = g_aquarium->fish_list;
    while (fish) {
        Fish *next = fish->next;
        g_free(fish->species);
        if (fish->image_path) g_free(fish->image_path);
        if (fish->image_left) g_object_unref(fish->image_left);
        if (fish->image_right) g_object_unref(fish->image_right);
        g_free(fish);
        fish = next;
    }
    
    Group *group = g_aquarium->group_list;
    while (group) {
        Group *next = group->next;
        g_free(group->name);
        free(group);
        group = next;
    }
    
    free(g_aquarium);
    g_aquarium = NULL;
}