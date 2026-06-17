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

/* Non-blocking notifications: update the bottom status bar instead of popping
 * a modal "OK" dialog for every little event. */
void show_message(const char *msg){
    if (g_aquarium && g_aquarium->status_bar && GTK_IS_LABEL(g_aquarium->status_bar))
        gtk_label_set_text(GTK_LABEL(g_aquarium->status_bar), msg);
    else
        g_message("%s", msg); /* before the UI exists, just log it */
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
    
    fish->image_right = scaled;                        /* owns 1 ref */
    fish->image_left = gdk_pixbuf_flip(scaled, TRUE);  /* owns 1 ref (no extra ref!) */

    g_object_unref(original);
}

/* Build fish->image_right/left from the current sprite-sheet frame.
 * Frames are scaled to the fish's base size (matching the static-image path). */
static void apply_fish_anim_frame(Fish *fish)
{
    if (!fish || !fish->anim)
        return;
    GdkPixbuf *frame = sprite_anim_frame(fish->anim, fish->anim_frame);
    if (!frame)
        return;

    int tw = (int)fish->base_size;
    int th = (int)(fish->base_size * 0.6f);
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(frame, tw, th, GDK_INTERP_BILINEAR);
    if (!scaled)
        return;

    if (fish->image_left)  g_object_unref(fish->image_left);
    if (fish->image_right) g_object_unref(fish->image_right);

    fish->image_right = scaled;                        /* owns 1 ref */
    fish->image_left  = gdk_pixbuf_flip(scaled, TRUE); /* owns 1 ref */
}

/* Advance the animation clock; switch to the next frame when due.
 * Animation cadence scales gently with swimming speed. */
static void advance_fish_animation(Fish *fish, float dt)
{
    if (!fish || !fish->anim)
        return;
    int count = sprite_anim_frame_count(fish->anim);
    if (count <= 1)
        return;

    float speed = sqrtf(fish->vx * fish->vx + fish->vy * fish->vy);
    float fps = 8.0f + speed * 1.6f;   /* faster swimming -> faster animation */
    if (fps > 22.0f) fps = 22.0f;

    fish->anim_timer += dt;
    if (fish->anim_timer >= 1.0f / fps)
    {
        fish->anim_timer = 0.0f;
        fish->anim_frame = (fish->anim_frame + 1) % count;
        apply_fish_anim_frame(fish);
    }
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

    /* Attach a sprite-sheet animation if one exists for this image/species.
     * Otherwise fall back to the static image (or procedural sprite). */
    fish->anim = sprite_anim_for_image(image_path);
    fish->anim_timer = 0.0f;
    if (fish->anim)
    {
        fish->anim_frame = rand() % sprite_anim_frame_count(fish->anim);
        apply_fish_anim_frame(fish);
    }
    else
    {
        fish->anim_frame = 0;
        load_fish_image(fish, image_path);
    }

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

    spawn_death_effect(fish->x, fish->y);

    if (fish->group){
        remove_fish_from_group(fish);
    }

    /* Only mark as dead. sweep_dead_fish() (end of each frame) safely unlinks
     * and frees it -- unlinking/freeing here would be unsafe mid-iteration. */
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

    spawn_death_effect(fish->x, fish->y);
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

/* Drive the grabbed fish directly toward the mouse cursor, smoothly, bypassing
 * the normal AI so it reliably follows the pointer regardless of group/wander. */
static void guide_fish_to_mouse(Fish *fish, float dt)
{
    float tx = g_aquarium->mouse_x;
    float ty = g_aquarium->mouse_y;

    /* Smooth follow: move a fraction of the remaining distance each tick. */
    float k = 0.22f;
    float dx = tx - fish->x;
    float dy = ty - fish->y;

    fish->vx = dx * k;          /* used for heading + animation speed */
    fish->vy = dy * k;
    fish->x += fish->vx;
    fish->y += fish->vy;

    /* Keep inside the tank. */
    if (fish->x < 0) fish->x = 0;
    if (fish->y < 0) fish->y = 0;
    if (fish->x > g_aquarium->width)  fish->x = g_aquarium->width;
    if (fish->y > g_aquarium->height) fish->y = g_aquarium->height;

    /* Face the direction of travel + keep the swim/tail animation alive. */
    float speed = sqrtf(fish->vx * fish->vx + fish->vy * fish->vy);
    if (speed > 0.5f)
    {
        fish->heading = atan2f(fish->vy, fish->vx);
        fish->direction = (cosf(fish->heading) >= 0.0f) ? DIRECTION_RIGHT : DIRECTION_LEFT;
        fish->image_facing = fish->direction;
    }
    fish->swim_phase += dt * (2.0f + speed * 0.2f);
    fish->tail_phase += dt * (3.0f + speed * 0.5f);
    fish->wander_timer = 5.0f; /* don't let it pick a random target while held */
}

/* Fully release one fish (struct + strings + image pixbufs). The sprite
 * animation (fish->anim) is shared/cached, so it is NOT freed here. */
static void destroy_fish(Fish *fish)
{
    if (!fish)
        return;
    if (fish->species)     g_free(fish->species);
    if (fish->image_path)  g_free(fish->image_path);
    if (fish->image_left)  g_object_unref(fish->image_left);
    if (fish->image_right) g_object_unref(fish->image_right);
    free(fish);
}

/* Remove every dead fish (is_alive == 0) from the global list and free it.
 * Run once per frame at a safe point (no other pass is iterating the list),
 * which is why death only *marks* fish and never frees mid-iteration. */
static void sweep_dead_fish(void)
{
    Fish *cur = g_aquarium->fish_list;
    while (cur)
    {
        Fish *next = cur->next;
        if (!cur->is_alive)
        {
            if (cur->prev) cur->prev->next = cur->next;
            else           g_aquarium->fish_list = cur->next;
            if (cur->next) cur->next->prev = cur->prev;
            if (g_aquarium->selected_fish == cur) g_aquarium->selected_fish = NULL;
            destroy_fish(cur);
        }
        cur = next;
    }
}

/* ---- Death "pop" effects: a short burst shown where a fish dies ---- */
#define MAX_DEATH_FX 64
typedef struct { float x, y, age, life; int active; } DeathFx;
static DeathFx g_death_fx[MAX_DEATH_FX];

void spawn_death_effect(float x, float y)
{
    for (int i = 0; i < MAX_DEATH_FX; ++i)
    {
        if (!g_death_fx[i].active)
        {
            g_death_fx[i].x = x;
            g_death_fx[i].y = y;
            g_death_fx[i].age = 0.0f;
            g_death_fx[i].life = 0.75f; /* seconds */
            g_death_fx[i].active = 1;
            return;
        }
    }
}

void update_death_effects(float dt)
{
    for (int i = 0; i < MAX_DEATH_FX; ++i)
        if (g_death_fx[i].active && (g_death_fx[i].age += dt) >= g_death_fx[i].life)
            g_death_fx[i].active = 0;
}

void draw_death_effects(cairo_t *cr)
{
    for (int i = 0; i < MAX_DEATH_FX; ++i)
    {
        if (!g_death_fx[i].active) continue;
        float t = g_death_fx[i].age / g_death_fx[i].life; /* 0..1 */
        float x = g_death_fx[i].x, y = g_death_fx[i].y;
        float a = 1.0f - t;

        /* expanding fading ring */
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, a * 0.7);
        cairo_set_line_width(cr, 2.5f * (1.0f - t) + 0.5f);
        cairo_arc(cr, x, y, 6.0f + t * 32.0f, 0, 2 * M_PI);
        cairo_stroke(cr);

        /* radiating "pop" spokes */
        cairo_set_source_rgba(cr, 0.8, 0.95, 1.0, a * 0.85);
        cairo_set_line_width(cr, 2.0f);
        for (int k = 0; k < 8; ++k)
        {
            float ang = (float)k * (float)(M_PI / 4.0);
            float r0 = 4.0f + t * 16.0f;
            float r1 = r0 + 9.0f * (1.0f - t);
            cairo_move_to(cr, x + cosf(ang) * r0, y + sinf(ang) * r0);
            cairo_line_to(cr, x + cosf(ang) * r1, y + sinf(ang) * r1);
        }
        cairo_stroke(cr);

        /* a few bubbles drifting up */
        cairo_set_source_rgba(cr, 0.85, 0.97, 1.0, a * 0.55);
        for (int k = 0; k < 3; ++k)
        {
            float bx = x + (float)(k - 1) * 7.0f;
            float by = y - t * 26.0f - (float)k * 4.0f;
            float br = 2.6f - t * 1.2f;
            if (br > 0.3f)
            {
                cairo_arc(cr, bx, by, br, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }
    }
}

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
                    if (g_aquarium->guiding && current == g_aquarium->selected_fish)
                    {
                        guide_fish_to_mouse(current, dt); /* mouse-controlled */
                    }
                    else
                    {
                        update_fish_movement(current, dt);
                        apply_group_behavior(current);
                    }
                    advance_fish_animation(current, dt);
                }
            }
            current = next;
        }

        update_food_physics(dt);
        merge_compatible_groups();
        resolve_fish_spacing(dt);
        update_particle_system(dt);
        update_death_effects(dt);
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

    sweep_dead_fish(); /* unlink + free everything that died this frame */

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

/* A single tapered, swaying seaweed blade rooted at (bx, by). */
static void draw_seaweed_blade(cairo_t *cr, double bx, double by, double height,
                               double phase, double t, double r, double g, double b)
{
    int seg = 9;
    double px = bx, py = by;
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    for (int i = 1; i <= seg; ++i)
    {
        double f = (double)i / seg;
        double sway = sin(t * 1.1 + phase + f * 2.6) * 16.0 * f;
        double x = bx + sway;
        double y = by - height * f;
        cairo_set_line_width(cr, 9.0 * (1.0 - f) + 2.0);
        cairo_set_source_rgba(cr, r, g, b, 0.6);
        cairo_move_to(cr, px, py);
        cairo_line_to(cr, x, y);
        cairo_stroke(cr);
        px = x; py = y;
    }
}

/* Decorative scenery drawn behind the fish: light rays, rocks, seaweed,
 * a gradient sandy bed and pebbles. */
void draw_decor(cairo_t *cr)
{
    if (!g_aquarium) return;
    int w = g_aquarium->width;
    int h = g_aquarium->height;
    double t = g_get_real_time() / 1000000.0;

    /* Soft light rays slanting down from the surface */
    for (int i = 0; i < 5; ++i)
    {
        double bx = w * (0.10 + i * 0.20);
        double sway = sin(t * 0.12 + i * 1.3) * 26.0;
        double tw = 34.0 + i * 6.0;
        cairo_pattern_t *ray = cairo_pattern_create_linear(0, 0, 0, h * 0.9);
        cairo_pattern_add_color_stop_rgba(ray, 0.0, 0.55, 0.85, 1.0, 0.07);
        cairo_pattern_add_color_stop_rgba(ray, 1.0, 0.55, 0.85, 1.0, 0.0);
        cairo_set_source(cr, ray);
        cairo_move_to(cr, bx - tw * 0.5, 0);
        cairo_line_to(cr, bx + tw * 0.5, 0);
        cairo_line_to(cr, bx + tw + sway, h * 0.9);
        cairo_line_to(cr, bx - tw + sway, h * 0.9);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_pattern_destroy(ray);
    }

    double sand_y = h - 52;

    /* Rocks resting on the bed */
    cairo_set_source_rgb(cr, 0.26, 0.28, 0.32);
    cairo_ellipse(cr, w * 0.16, sand_y + 8, 70, 34); cairo_fill(cr);
    cairo_ellipse(cr, w * 0.82, sand_y + 10, 90, 40); cairo_fill(cr);
    cairo_set_source_rgb(cr, 0.34, 0.36, 0.40);
    cairo_ellipse(cr, w * 0.21, sand_y + 2, 38, 22); cairo_fill(cr);
    cairo_ellipse(cr, w * 0.77, sand_y + 4, 50, 24); cairo_fill(cr);

    /* Swaying seaweed clusters */
    double bases[]  = {0.07, 0.10, 0.13, 0.69, 0.72, 0.90, 0.93};
    double greens[3][3] = {{0.16,0.50,0.24},{0.12,0.42,0.20},{0.20,0.56,0.28}};
    for (int i = 0; i < 7; ++i)
    {
        double bx = w * bases[i];
        double height = 90.0 + (i % 3) * 35.0 + sin(i * 1.7) * 15.0;
        int c = i % 3;
        draw_seaweed_blade(cr, bx, sand_y, height, i * 1.1, t,
                           greens[c][0], greens[c][1], greens[c][2]);
    }

    /* Gradient sandy bed with a gently wavy top edge (covers the plant roots) */
    cairo_pattern_t *sand = cairo_pattern_create_linear(0, sand_y, 0, h);
    cairo_pattern_add_color_stop_rgb(sand, 0.0, 0.80, 0.72, 0.50);
    cairo_pattern_add_color_stop_rgb(sand, 1.0, 0.58, 0.50, 0.33);
    cairo_set_source(cr, sand);
    cairo_move_to(cr, 0, sand_y);
    for (int x = 0; x <= w; x += 36)
        cairo_line_to(cr, x, sand_y + sin(x * 0.025 + 1.0) * 4.0);
    cairo_line_to(cr, w, h);
    cairo_line_to(cr, 0, h);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_pattern_destroy(sand);

    /* Scattered pebbles (deterministic positions so they don't flicker) */
    int ww = (w > 1) ? w : 1;
    for (int i = 0; i < 26; ++i)
    {
        double px = fmod(i * 137.0 + 20.0, (double)ww);
        double py = sand_y + 10.0 + fmod(i * 53.0, 34.0);
        double rad = 2.0 + fmod(i * 7.0, 4.0);
        double shade = 0.5 + fmod(i * 11.0, 5.0) / 20.0;
        cairo_set_source_rgba(cr, shade * 0.7, shade * 0.62, shade * 0.45, 0.85);
        cairo_arc(cr, px, py, rad, 0, 2 * M_PI);
        cairo_fill(cr);
    }
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
        /* Selection feedback: the chosen fish ALWAYS gets a clear pulsing
         * marker; with the Highlight toggle on, its whole group is ringed too. */
        int is_selected = (g_aquarium->selected_fish == fish);
        int is_group_hl = (g_aquarium->show_selected_highlight &&
                           g_aquarium->selected_fish &&
                           g_aquarium->selected_fish->group &&
                           fish->group == g_aquarium->selected_fish->group);
        if (is_selected || is_group_hl)
        {
            double tt = g_get_real_time() / 1000000.0;
            float pulse = 0.5f + 0.5f * sinf((float)tt * 4.0f);
            float ring_r = fish->size + 8.0f + (is_selected ? pulse * 6.0f : 2.0f);

            /* colour: bright yellow for the selected fish, cyan for group mates */
            float rr = is_selected ? 1.0f : 0.35f;
            float gg = is_selected ? 0.90f : 0.80f;
            float bb = is_selected ? 0.10f : 1.0f;

            /* soft glow */
            cairo_set_source_rgba(cr, rr, gg, bb, is_selected ? 0.18 : 0.10);
            cairo_arc(cr, fish->x, fish->y, ring_r, 0, 2 * M_PI);
            cairo_fill(cr);

            /* crisp ring */
            cairo_set_source_rgba(cr, rr, gg, bb, is_selected ? 0.90 : 0.50);
            cairo_set_line_width(cr, is_selected ? 2.6 : 1.6);
            cairo_arc(cr, fish->x, fish->y, ring_r, 0, 2 * M_PI);
            cairo_stroke(cr);

            /* bobbing downward arrow above the selected fish */
            if (is_selected)
            {
                float my = fish->y - ring_r - 12.0f - pulse * 4.0f;
                cairo_set_source_rgba(cr, 1.0, 0.9, 0.1, 0.95);
                cairo_move_to(cr, fish->x - 8.0f, my);
                cairo_line_to(cr, fish->x + 8.0f, my);
                cairo_line_to(cr, fish->x, my + 12.0f);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
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
    draw_decor(cr);
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

    draw_death_effects(cr); /* "pop" bursts where fish died, on top of fish */

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

/* Styled "View Group" panel (created in create_aquarium_window, hidden until used) */
static GtkWidget *g_group_panel = NULL;
static GtkWidget *g_group_panel_img = NULL;
static GtkWidget *g_group_panel_label = NULL;

static void on_group_panel_close(GtkWidget *btn, gpointer data)
{
    (void)btn; (void)data;
    if (g_group_panel)
        gtk_widget_hide(g_group_panel);
}

void on_view_group_clicked(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    if (!g_aquarium->selected_fish || !g_aquarium->selected_fish->group)
    {
        show_message("Select a fish that belongs to a group first.");
        return;
    }

    Group *group = g_aquarium->selected_fish->group;
    Fish *leader = group->leader;

    /* Thumbnail of the group's main species (the leader's image). */
    if (g_group_panel_img)
    {
        GdkPixbuf *pb = NULL;
        if (leader && leader->image_path)
            pb = gdk_pixbuf_new_from_file_at_scale(leader->image_path, 84, 84, TRUE, NULL);
        if (pb)
        {
            gtk_image_set_from_pixbuf(GTK_IMAGE(g_group_panel_img), pb);
            g_object_unref(pb);
        }
        else
        {
            gtk_image_set_from_icon_name(GTK_IMAGE(g_group_panel_img), "image-missing", GTK_ICON_SIZE_DIALOG);
        }
    }

    /* Count how many fish are male/female and gather a short member preview. */
    int males = 0, females = 0;
    char preview[256];
    preview[0] = '\0';
    Fish *cur = group->members_head;
    int shown = 0;
    while (cur)
    {
        if (cur->gender == GENDER_MALE) males++; else females++;
        if (shown < 6)
        {
            char tmp[48];
            g_snprintf(tmp, sizeof(tmp), "%s%s%s", shown ? ", " : "",
                       cur->species ? cur->species : "?",
                       cur->is_leader ? " 👑" : "");
            g_strlcat(preview, tmp, sizeof(preview));
            shown++;
        }
        cur = cur->group_next;
    }
    if (shown < group->member_count)
        g_strlcat(preview, " …", sizeof(preview));

    char *ename = g_markup_escape_text(group->name ? group->name : "Group", -1);
    char *espec = g_markup_escape_text(leader && leader->species ? leader->species : "?", -1);
    char *eprev = g_markup_escape_text(preview, -1);
    const char *typ = (leader && leader->type == TYPE_PREDATOR) ? "🦈 Predator" : "🐟 Prey";

    char markup[1024];
    g_snprintf(markup, sizeof(markup),
        "<span size='x-large' weight='bold' foreground='#d7ecff'>👥 %s</span>\n"
        "<span foreground='#a9cde8'>Main species: <b>%s</b>   •   %s   •   ID %d</span>\n"
        "<span foreground='#a9cde8'>Members: <b>%d</b>   ( ♂ %d  /  ♀ %d )   •   Leader: <b>%s</b> #%d</span>\n"
        "<span foreground='#8fb3cf' size='small'>%s</span>",
        ename, espec, typ, group->id,
        group->member_count, males, females,
        espec, leader ? leader->id : -1, eprev);

    if (g_group_panel_label)
        gtk_label_set_markup(GTK_LABEL(g_group_panel_label), markup);

    g_free(ename); g_free(espec); g_free(eprev);

    if (g_group_panel)
        gtk_widget_show(g_group_panel);
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

        /* Grab the fish: while the button stays held, it follows the mouse. */
        g_aquarium->mouse_x = event->x;
        g_aquarium->mouse_y = event->y;
        g_aquarium->guiding = (closest != NULL);
        if (closest)
            show_message("🐟 Guiding fish — move the mouse, release to let go");
    }
    return FALSE;
}

/* Track the pointer; the guided fish follows g_aquarium->mouse_x/y each tick. */
gboolean on_mouse_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    (void)widget;
    (void)data;
    g_aquarium->mouse_x = (float)event->x;
    g_aquarium->mouse_y = (float)event->y;
    return FALSE;
}

/* Release the grabbed fish. */
gboolean on_mouse_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    (void)widget;
    (void)data;
    if (event->button == 1)
        g_aquarium->guiding = 0;
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
            char *vx_str = extract_tag_content(div, "vx", fish_end);
            char *vy_str = extract_tag_content(div, "vy", fish_end);
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

                // Restore exact swim direction/speed it had when saved
                if (vx_str) new_fish->vx = atof(vx_str);
                if (vy_str) new_fish->vy = atof(vy_str);

                // Restore ecosystem state
                if (personality_str) new_fish->personality = atoi(personality_str);
                if (life_stage_str) new_fish->life_stage = atoi(life_stage_str);
                if (generation_str) new_fish->generation = atoi(generation_str);
                if (lifespan_str) new_fish->lifespan = atof(lifespan_str);
                if (base_size_str) new_fish->base_size = atof(base_size_str);
                if (base_speed_str) new_fish->base_speed = atof(base_speed_str);
                
                add_fish(new_fish);
            }
            
            free(species); free(x_str); free(y_str); free(vx_str); free(vy_str); free(size_str); free(speed_str);
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
            char *members_str = extract_tag_content(div, "members", group_end);

            if (group_name && leader_id_str) {
                int leader_id = atoi(leader_id_str);
                Fish *leader = g_aquarium->fish_list;
                while (leader && leader->id != leader_id) leader = leader->next;

                if (leader) {
                    Group *group = create_group(group_name, leader);

                    /* Re-link every saved member back into the group.
                     * The <members> list is comma-separated fish ids and includes
                     * the leader, so we skip the leader (already added above). */
                    if (group && members_str) {
                        char *token = strtok(members_str, ",");
                        while (token) {
                            int member_id = atoi(token);
                            if (member_id != leader_id) {
                                Fish *member = g_aquarium->fish_list;
                                while (member && member->id != member_id) member = member->next;
                                if (member) add_fish_to_group(group, member);
                            }
                            token = strtok(NULL, ",");
                        }
                    }
                }
            }

            free(group_name); free(leader_id_str); free(members_str);
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
    g_aquarium->show_help = 0;   /* hidden until the Help button (or 'H') is pressed */
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

    spawn_fish_from_preset(preset_fish_get(0), 2, 0, 0, TYPE_PREY, 0);   /* Clownfish */
    spawn_fish_from_preset(preset_fish_get(1), 2, 0, 0, TYPE_PREY, 0);   /* Blue Fish */
    spawn_fish_from_preset(preset_fish_get(2), 2, 0, 0, TYPE_PREY, 0);   /* Green Fish */
    spawn_fish_from_preset(preset_fish_get(5), 1, 0, 0, TYPE_PREY, 0);   /* Jellyfish */
    spawn_fish_from_preset(preset_fish_get(3), 1, 0, 0, TYPE_PREY, 0);   /* Puffer Fish */
    spawn_fish_from_preset(preset_fish_get(8), 1, 0, 0, TYPE_PREDATOR, 1); /* Dolphin */
    spawn_fish_from_preset(preset_fish_get(10), 1, 0, 0, TYPE_PREDATOR, 1); /* Shark */

    spawn_food_pellets(12, g_aquarium->width * 0.5f, 80.0f);
    show_message("Default reef loaded: schools, predators, and food.");
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
}

/* Add one fish of the clicked species with its default settings. */
static void on_quick_add_species(GtkWidget *btn, gpointer data)
{
    (void)btn;
    int idx = GPOINTER_TO_INT(data);
    const PresetFish *p = preset_fish_get(idx);
    if (!p || !g_aquarium)
        return;

    /* Each species has one Quick-Add school named "<Species> School".
     * The first Quick-Added fish of a species becomes that school's leader;
     * every later Quick-Add of the same species joins it (follows the leader). */
    char group_name[128];
    g_snprintf(group_name, sizeof(group_name), "%s School", p->species);

    Group *school = NULL;
    for (Group *g = g_aquarium->group_list; g; g = g->next) {
        if (g->name && g_strcmp0(g->name, group_name) == 0 && g->member_count > 0) {
            school = g;
            break;
        }
    }

    /* Build the fish with the preset's default look/stats. */
    char *image_path = preset_fish_image_path(idx); /* malloc'd or NULL */
    float bx = 120.0f + (float)(rand() % (g_aquarium->width  > 240 ? g_aquarium->width  - 240 : 400));
    float by = 120.0f + (float)(rand() % (g_aquarium->height > 240 ? g_aquarium->height - 240 : 400));
    Gender gender = (rand() % 2 == 0) ? GENDER_MALE : GENDER_FEMALE;
    FishDirection facing = (rand() % 2 == 0) ? DIRECTION_RIGHT : DIRECTION_LEFT;

    Fish *fish = create_fish(p->species, image_path, bx, by,
                             p->default_size, p->default_speed,
                             p->type, p->diet, gender, facing);
    add_fish(fish);
    if (image_path)
        g_free(image_path);

    int joined;
    if (school) {
        add_fish_to_group(school, fish); /* follow the existing leader */
        joined = 1;
    } else {
        create_group(group_name, fish); /* first of its kind -> becomes leader */
        joined = 0;
    }
    g_aquarium->selected_fish = fish;

    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
    char m[160];
    if (joined)
        g_snprintf(m, sizeof(m), "➕ Added a %s (joined its school)", p->species);
    else
        g_snprintf(m, sizeof(m), "➕ Added a %s (new school leader)", p->species);
    show_message(m);
}

/* Pop up a gallery of every species; one click on an image adds that fish. */
static void on_quick_add_clicked(GtkWidget *btn, gpointer data)
{
    (void)btn;
    GtkWidget *parent = GTK_WIDGET(data);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "🐟 Quick Add Fish");
    gtk_window_set_default_size(GTK_WINDOW(win), 620, 470);
    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(parent));
        gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER_ON_PARENT);
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *flow = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow), GTK_SELECTION_NONE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow), 4);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow), 2);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow), 10);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow), 10);
    gtk_widget_set_margin_start(flow, 12);
    gtk_widget_set_margin_end(flow, 12);
    gtk_widget_set_margin_top(flow, 12);
    gtk_widget_set_margin_bottom(flow, 12);

    int n = preset_fish_count();
    for (int i = 0; i < n; ++i)
    {
        const PresetFish *p = preset_fish_get(i);
        if (!p) continue;

        GtkWidget *vb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        GtkWidget *img = NULL;
        char *path = preset_fish_image_path(i);
        if (path)
        {
            GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 90, 90, TRUE, NULL);
            if (pb) { img = gtk_image_new_from_pixbuf(pb); g_object_unref(pb); }
            g_free(path);
        }
        if (!img)
            img = gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);

        GtkWidget *lbl = gtk_label_new(p->species);
        gtk_box_pack_start(GTK_BOX(vb), img, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vb), lbl, FALSE, FALSE, 0);

        GtkWidget *cell = gtk_button_new();
        gtk_container_add(GTK_CONTAINER(cell), vb);
        gtk_widget_set_tooltip_text(cell, "Click to add this fish");
        g_signal_connect(cell, "clicked", G_CALLBACK(on_quick_add_species), GINT_TO_POINTER(i));
        gtk_flow_box_insert(GTK_FLOW_BOX(flow), cell, -1);
    }

    gtk_container_add(GTK_CONTAINER(scroll), flow);
    gtk_container_add(GTK_CONTAINER(win), scroll);
    gtk_widget_show_all(win); /* non-modal: stays open so you can add several */
}

void create_aquarium_window(void)
{
    ProgramStart();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 640);
    gtk_window_set_title(GTK_WINDOW(window), "🐠 Aquarium Simulator");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    /* Window content = [ left sidebar | main vertical area ], split by a
     * draggable divider (GtkPaned) so the sidebar can be resized by the user. */
    GtkWidget *outer_hbox = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(window), outer_hbox);

    /* ---- Left sidebar ---- */
    {
        GtkCssProvider *sb_css = gtk_css_provider_new();
        gtk_css_provider_load_from_data(sb_css,
            "box.sidebar {"
            "  background-image: linear-gradient(to bottom, rgba(16,34,58,0.97), rgba(9,20,36,0.97));"
            "  border-right: 1px solid rgba(96,176,236,0.45);"
            "  padding: 10px 8px;"
            "}"
            "box.sidebar button { padding: 10px 6px; font-weight: bold; }", -1, NULL);
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(sb_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(sb_css);

        GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_style_context_add_class(gtk_widget_get_style_context(sidebar), "sidebar");
        gtk_widget_set_size_request(sidebar, 70, -1); /* minimum drag width */

        GtkWidget *quick_btn = gtk_button_new_with_label("🐟\nQuick\nAdd");
        gtk_widget_set_tooltip_text(quick_btn, "Open the fish gallery to add fish quickly");
        g_signal_connect(quick_btn, "clicked", G_CALLBACK(on_quick_add_clicked), window);
        gtk_box_pack_start(GTK_BOX(sidebar), quick_btn, FALSE, FALSE, 0);

        /* pane 1: sidebar — resize=FALSE (keeps its size when window resizes),
         * shrink=FALSE (can't be dragged below its minimum). */
        gtk_paned_pack1(GTK_PANED(outer_hbox), sidebar, FALSE, FALSE);
    }

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    /* pane 2: main area — takes the remaining space. */
    gtk_paned_pack2(GTK_PANED(outer_hbox), main_vbox, TRUE, TRUE);
    gtk_paned_set_position(GTK_PANED(outer_hbox), 165); /* initial sidebar width */

    /* FlowBox toolbar: buttons wrap onto extra rows when the window is narrow,
     * so the window is free to shrink instead of being forced wide. */
    GtkWidget *toolbar = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(toolbar), GTK_SELECTION_NONE);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(toolbar), 30);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(toolbar), 4);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(toolbar), 4);
    gtk_widget_set_margin_start(toolbar, 8);
    gtk_widget_set_margin_end(toolbar, 8);
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
    GtkWidget *load_default_btn = gtk_button_new_with_label("🐠 Load Default");

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
    g_signal_connect(load_default_btn, "clicked", G_CALLBACK(on_load_default_clicked), NULL);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), add_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), delete_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), clear_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), feed_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), highlight_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), create_group_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), add_to_group_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), remove_from_group_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), delete_group_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), view_group_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), grid_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), help_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), load_default_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), load_html_btn, -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(toolbar), save_html_btn, -1);

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
    g_signal_connect(g_aquarium->drawing_area, "motion-notify-event", G_CALLBACK(on_mouse_motion), NULL);
    g_signal_connect(g_aquarium->drawing_area, "button-release-event", G_CALLBACK(on_mouse_release), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    gtk_widget_add_events(g_aquarium->drawing_area,
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK | GDK_BUTTON1_MOTION_MASK);
    gtk_box_pack_start(GTK_BOX(main_vbox), g_aquarium->drawing_area, TRUE, TRUE, 0);

    /* ---- Styled "View Group" info panel (hidden until used) ---- */
    {
        GtkCssProvider *gp_css = gtk_css_provider_new();
        gtk_css_provider_load_from_data(gp_css,
            "box.group-panel {"
            "  background-image: linear-gradient(to bottom, rgba(18,40,66,0.96), rgba(10,24,42,0.96));"
            "  border: 1px solid rgba(96,176,236,0.55);"
            "  border-radius: 12px;"
            "  padding: 10px 14px;"
            "  margin: 6px 10px;"
            "}"
            "box.group-panel image {"
            "  border: 2px solid rgba(96,176,236,0.6);"
            "  border-radius: 10px;"
            "  background-color: rgba(255,255,255,0.05);"
            "  padding: 3px;"
            "}"
            "box.group-panel button {"
            "  border-radius: 9999px; padding: 2px 8px; min-width: 0;"
            "}", -1, NULL);
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(gp_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(gp_css);

        g_group_panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
        gtk_style_context_add_class(gtk_widget_get_style_context(g_group_panel), "group-panel");

        g_group_panel_img = gtk_image_new();
        gtk_widget_set_valign(g_group_panel_img, GTK_ALIGN_CENTER);

        g_group_panel_label = gtk_label_new(NULL);
        gtk_label_set_xalign(GTK_LABEL(g_group_panel_label), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(g_group_panel_label), TRUE);
        gtk_widget_set_valign(g_group_panel_label, GTK_ALIGN_CENTER);

        GtkWidget *gp_close = gtk_button_new_with_label("✕");
        gtk_widget_set_tooltip_text(gp_close, "Close");
        gtk_widget_set_valign(gp_close, GTK_ALIGN_START);
        g_signal_connect(gp_close, "clicked", G_CALLBACK(on_group_panel_close), NULL);

        gtk_box_pack_start(GTK_BOX(g_group_panel), g_group_panel_img, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(g_group_panel), g_group_panel_label, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(g_group_panel), gp_close, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(main_vbox), g_group_panel, FALSE, FALSE, 0);
    }

    g_aquarium->status_bar = gtk_label_new("💡 Tips: Click on fish to select | SPACE to pause | H for help | C to clear");
    gtk_widget_set_margin_start(g_aquarium->status_bar, 10);
    gtk_widget_set_margin_end(g_aquarium->status_bar, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), g_aquarium->status_bar, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_widget_hide(g_group_panel); /* shown only when "View Group" is clicked */

    /* Populate a starter reef directly (no clear/confirm dialog) so animated
     * fish are visible immediately on launch. */
    spawn_fish_from_preset(preset_fish_get(0), 2, 0, 0, TYPE_PREY, 0);    /* Clownfish */
    spawn_fish_from_preset(preset_fish_get(1), 2, 0, 0, TYPE_PREY, 0);    /* Blue Fish */
    spawn_fish_from_preset(preset_fish_get(2), 2, 0, 0, TYPE_PREY, 0);    /* Green Fish */
    spawn_fish_from_preset(preset_fish_get(5), 1, 0, 0, TYPE_PREY, 0);    /* Jellyfish */
    spawn_fish_from_preset(preset_fish_get(3), 1, 0, 0, TYPE_PREY, 0);    /* Puffer Fish */
    spawn_fish_from_preset(preset_fish_get(8), 1, 0, 0, TYPE_PREDATOR, 1);/* Dolphin */
    spawn_fish_from_preset(preset_fish_get(10), 1, 0, 0, TYPE_PREDATOR, 1);/* Shark */
    spawn_food_pellets(10, g_aquarium->width * 0.5f, 80.0f);
    update_ui_info();

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

    sprite_anim_cleanup();  /* free shared sprite-sheet frame cache */

    free(g_aquarium);
    g_aquarium = NULL;
}