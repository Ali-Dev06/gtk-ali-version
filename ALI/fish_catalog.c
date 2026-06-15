#include "fish_catalog.h"
#include <string.h>
#include <unistd.h>
#include <libgen.h>

/* Species drawn from the FISH/ folder. Those whose name has a matching sheet in
 * SPRITES/ (e.g. dolphin.png -> dolphin_25.png) are animated automatically. */
static const PresetFish g_presets[] = {
    /* index 0..6 : prey (all animated) */
    {"Clownfish",   "clownfish.png",   TYPE_PREY,     DIET_OMNIVORE,  130.0f, 2.5f},
    {"Blue Fish",   "blue_fish.png",   TYPE_PREY,     DIET_OMNIVORE,  125.0f, 2.7f},
    {"Green Fish",  "green_fish.png",  TYPE_PREY,     DIET_HERBIVORE, 125.0f, 2.6f},
    {"Puffer Fish", "puffer_fish.png", TYPE_PREY,     DIET_OMNIVORE,  140.0f, 2.0f},
    {"Seahorse",    "seahorse.png",    TYPE_PREY,     DIET_OMNIVORE,  120.0f, 1.8f},
    {"Jellyfish",   "jellyfish.png",   TYPE_PREY,     DIET_CARNIVORE, 125.0f, 1.6f},
    {"Sea Turtle",  "sea_turtle.png",  TYPE_PREY,     DIET_HERBIVORE, 180.0f, 1.9f},
    /* index 7..10 : predators (7,8 animated; 9,10 static only) */
    {"Swordfish",   "swordfish.png",   TYPE_PREDATOR, DIET_CARNIVORE, 195.0f, 3.2f},
    {"Dolphin",     "dolphin.png",     TYPE_PREDATOR, DIET_CARNIVORE, 205.0f, 3.0f},
    {"Orca",        "orca.png",        TYPE_PREDATOR, DIET_CARNIVORE, 240.0f, 2.6f},
    {"Shark",       "sahrk.png",       TYPE_PREDATOR, DIET_CARNIVORE, 225.0f, 2.8f},
};

static float library_clamp_size(float size)
{
    if (size < LIBRARY_MIN_FISH_SIZE)
        return LIBRARY_MIN_FISH_SIZE;
    if (size > LIBRARY_MAX_FISH_SIZE)
        return LIBRARY_MAX_FISH_SIZE;
    return size;
}

void resolve_images_directory(char *out_dir, size_t out_size)
{
    /* Fish artwork now lives in the FISH/ folder. */
    static const char *candidates[] = {
        "FISH",
        "../FISH",
        "ALI/FISH",
        "images",      /* legacy fallback */
        NULL
    };

    for (int i = 0; candidates[i]; ++i)
    {
        char probe[512];
        snprintf(probe, sizeof(probe), "%s/dolphin.png", candidates[i]);
        if (g_file_test(probe, G_FILE_TEST_EXISTS))
        {
            g_strlcpy(out_dir, candidates[i], out_size);
            return;
        }
    }

    g_strlcpy(out_dir, "FISH", out_size);
}

void resolve_sprite_directory(char *out_dir, size_t out_size)
{
    /* Animation sheets live in the SPRITES/ folder. */
    static const char *candidates[] = {
        "SPRITES",
        "../SPRITES",
        "ALI/SPRITES",
        NULL
    };

    for (int i = 0; candidates[i]; ++i)
    {
        char probe[512];
        snprintf(probe, sizeof(probe), "%s/dolphin_25.png", candidates[i]);
        if (g_file_test(probe, G_FILE_TEST_EXISTS))
        {
            g_strlcpy(out_dir, candidates[i], out_size);
            return;
        }
    }

    g_strlcpy(out_dir, "SPRITES", out_size);
}

int preset_fish_count(void)
{
    return (int)(sizeof(g_presets) / sizeof(g_presets[0]));
}

const PresetFish *preset_fish_get(int index)
{
    if (index < 0 || index >= preset_fish_count())
        return NULL;
    return &g_presets[index];
}

char *preset_fish_image_path(int index)
{
    const PresetFish *preset = preset_fish_get(index);
    if (!preset)
        return NULL;

    char dir[512];
    resolve_images_directory(dir, sizeof(dir));

    char path[768];
    snprintf(path, sizeof(path), "%s/%s", dir, preset->image_file);
    if (g_file_test(path, G_FILE_TEST_EXISTS))
        return g_strdup(path);

    char alt[768];
    snprintf(alt, sizeof(alt), "%s/%s", dir, preset->image_file);
    char *dot = strrchr(alt, '.');
    if (dot)
    {
        strcpy(dot, ".jpg");
        if (g_file_test(alt, G_FILE_TEST_EXISTS))
            return g_strdup(alt);
    }

    return g_strdup(path);
}

int count_groups(void)
{
    int n = 0;
    if (!g_aquarium)
        return 0;
    Group *g = g_aquarium->group_list;
    while (g)
    {
        n++;
        g = g->next;
    }
    return n;
}

const char *goal_to_string(Goal goal)
{
    switch (goal)
    {
        case GOAL_SEARCH_FOOD: return "Find food";
        case GOAL_ESCAPE: return "Flee predator";
        case GOAL_FOLLOW_LEADER: return "Follow school";
        case GOAL_HUNT: return "Hunt prey";
        case GOAL_REST: return "Rest";
        case GOAL_GLIDE: return "Glide";
        case GOAL_MATE: return "Mate";
        default: return "Swim";
    }
}

const char *personality_to_string(Personality p)
{
    switch (p)
    {
        case PERSONALITY_SHY: return "Shy";
        case PERSONALITY_AGGRESSIVE: return "Aggressive";
        case PERSONALITY_SOCIAL: return "Social";
        case PERSONALITY_CURIOUS: return "Curious";
        case PERSONALITY_TERRITORIAL: return "Territorial";
        default: return "Peaceful";
    }
}

const char *life_stage_to_string(LifeStage stage)
{
    switch (stage)
    {
        case STAGE_BABY: return "Baby";
        case STAGE_JUVENILE: return "Juvenile";
        case STAGE_ELDERLY: return "Elderly";
        default: return "Adult";
    }
}

const char *fish_type_to_string(FishType type)
{
    return type == TYPE_PREDATOR ? "Predator" : "Prey";
}

Fish *find_nearest_mate(Fish *fish)
{
    if (!fish || !g_aquarium || fish->type != TYPE_PREY)
        return NULL;

    Fish *best = NULL;
    float best_dist = 160.0f;

    Fish *current = g_aquarium->fish_list;
    while (current)
    {
        if (current != fish && current->is_alive && current->type == TYPE_PREY &&
            current->life_stage >= STAGE_ADULT && current->gender != fish->gender &&
            current->mate_target_id < 0 && fish->mate_target_id < 0)
        {
            if (fish->species && current->species &&
                strcmp(fish->species, current->species) == 0)
            {
                float dist = wrap_distance(fish, current);
                if (dist < best_dist && current->hunger < 55.0f && current->health > 50.0f)
                {
                    best_dist = dist;
                    best = current;
                }
            }
        }
        current = current->next;
    }
    return best;
}

void spawn_fish_from_preset(const PresetFish *preset, int count, float size, float speed,
                            FishType type_override, int type_use_override)
{
    if (!preset || !g_aquarium || count < 1)
        return;

    char *image_path = NULL;
    char dir[512];
    resolve_images_directory(dir, sizeof(dir));
    char path[768];
    snprintf(path, sizeof(path), "%s/%s", dir, preset->image_file);
    if (g_file_test(path, G_FILE_TEST_EXISTS))
        image_path = g_strdup(path);

    FishType type = type_use_override ? type_override : preset->type;
    float use_size = size > 0 ? size : preset->default_size;
    use_size = library_clamp_size(use_size);
    float use_speed = speed > 0 ? speed : preset->default_speed;

    float base_x = 120.0f + (float)(rand() % (g_aquarium->width > 240 ? g_aquarium->width - 240 : 400));
    float base_y = 120.0f + (float)(rand() % (g_aquarium->height > 240 ? g_aquarium->height - 240 : 400));

    Fish *leader = NULL;
    Group *school = NULL;

    for (int i = 0; i < count; ++i)
    {
        Gender gender = (rand() % 2 == 0) ? GENDER_MALE : GENDER_FEMALE;
        FishDirection facing = (rand() % 2 == 0) ? DIRECTION_RIGHT : DIRECTION_LEFT;
        float ox = (float)(rand() % 50 - 25) + (float)i * 10.0f;
        float oy = (float)(rand() % 40 - 20);

        Fish *new_fish = create_fish(preset->species, image_path, base_x + ox, base_y + oy,
                                     use_size, use_speed, type, preset->diet, gender, facing);
        add_fish(new_fish);

        if (!leader)
        {
            leader = new_fish;
            char group_name[128];
            snprintf(group_name, sizeof(group_name), "%s School", preset->species);
            school = create_group(group_name, leader);
            g_aquarium->selected_fish = leader;
        }
        else if (school)
        {
            add_fish_to_group(school, new_fish);
        }
    }

    if (image_path)
        g_free(image_path);
}

/* ==========================================================================
 *  Sprite-sheet animation cache
 *  Each sheet in SPRITES/ is a 5x5 grid (25 frames). We load a sheet once,
 *  slice it into 25 square frames (downscaled to a cache size), and share the
 *  result between every fish of that species.
 * ========================================================================== */

#define ANIM_GRID_COLS      5
#define ANIM_GRID_ROWS      5
#define ANIM_MAX_FRAMES     (ANIM_GRID_COLS * ANIM_GRID_ROWS)
#define ANIM_CACHE_FRAME_PX 220   /* native cache frame size (square, >= max fish size) */

struct SpriteAnim {
    char *key;                          /* sheet path (also the cache key) */
    int frame_count;
    GdkPixbuf *frames[ANIM_MAX_FRAMES]; /* right-facing, square, ~ANIM_CACHE_FRAME_PX */
};

static GHashTable *g_anim_cache = NULL; /* char* sheet-path -> SpriteAnim* */

/* Strip directory + extension: "FISH/dolphin.png" -> "dolphin" (caller frees). */
static char *base_name_no_ext(const char *path)
{
    char *base = g_path_get_basename(path);
    char *dot = strrchr(base, '.');
    if (dot)
        *dot = '\0';
    return base;
}

static SpriteAnim *sprite_anim_load(const char *sheet_path)
{
    GError *error = NULL;
    GdkPixbuf *sheet = gdk_pixbuf_new_from_file(sheet_path, &error);
    if (!sheet)
    {
        if (error)
        {
            g_printerr("Sprite sheet load failed (%s): %s\n", sheet_path, error->message);
            g_error_free(error);
        }
        return NULL;
    }

    int sw = gdk_pixbuf_get_width(sheet);
    int sh = gdk_pixbuf_get_height(sheet);
    double fw = (double)sw / ANIM_GRID_COLS;
    double fh = (double)sh / ANIM_GRID_ROWS;

    SpriteAnim *anim = g_malloc0(sizeof(SpriteAnim));
    anim->key = g_strdup(sheet_path);
    anim->frame_count = 0;

    for (int i = 0; i < ANIM_MAX_FRAMES; ++i)
    {
        int col = i % ANIM_GRID_COLS;
        int row = i / ANIM_GRID_COLS;
        int x = (int)(col * fw);
        int y = (int)(row * fh);
        int w = (int)fw;
        int h = (int)fh;
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;
        if (w <= 0 || h <= 0)
            continue;

        GdkPixbuf *view = gdk_pixbuf_new_subpixbuf(sheet, x, y, w, h);
        /* scale_simple produces an independent copy so we can drop the sheet */
        GdkPixbuf *frame = gdk_pixbuf_scale_simple(view, ANIM_CACHE_FRAME_PX,
                                                   ANIM_CACHE_FRAME_PX, GDK_INTERP_BILINEAR);
        g_object_unref(view);
        anim->frames[anim->frame_count++] = frame;
    }

    g_object_unref(sheet);

    if (anim->frame_count == 0)
    {
        g_free(anim->key);
        g_free(anim);
        return NULL;
    }
    return anim;
}

SpriteAnim *sprite_anim_for_image(const char *image_path)
{
    if (!image_path || !image_path[0])
        return NULL;

    char *base = base_name_no_ext(image_path);
    char dir[512];
    resolve_sprite_directory(dir, sizeof(dir));

    /* Possible sheet names for this species, in priority order. */
    const char *patterns[] = { "%s/%s_25_clean.png", "%s/%s_25.png", "%s/%s_sheet.png", NULL };
    char sheet_path[768];
    sheet_path[0] = '\0';
    for (int i = 0; patterns[i]; ++i)
    {
        char probe[768];
        snprintf(probe, sizeof(probe), patterns[i], dir, base);
        if (g_file_test(probe, G_FILE_TEST_EXISTS))
        {
            g_strlcpy(sheet_path, probe, sizeof(sheet_path));
            break;
        }
    }
    g_free(base);

    if (!sheet_path[0])
        return NULL; /* species has no animation sheet -> static image */

    if (!g_anim_cache)
        g_anim_cache = g_hash_table_new(g_str_hash, g_str_equal);

    SpriteAnim *anim = g_hash_table_lookup(g_anim_cache, sheet_path);
    if (anim)
        return anim;

    anim = sprite_anim_load(sheet_path);
    if (anim)
        g_hash_table_insert(g_anim_cache, anim->key, anim); /* key owned by anim */
    return anim;
}

int sprite_anim_frame_count(const SpriteAnim *anim)
{
    return anim ? anim->frame_count : 0;
}

GdkPixbuf *sprite_anim_frame(const SpriteAnim *anim, int index)
{
    if (!anim || anim->frame_count == 0)
        return NULL;
    if (index < 0) index = 0;
    if (index >= anim->frame_count) index = anim->frame_count - 1;
    return anim->frames[index];
}

void sprite_anim_cleanup(void)
{
    if (!g_anim_cache)
        return;
    GHashTableIter it;
    gpointer k, v;
    g_hash_table_iter_init(&it, g_anim_cache);
    while (g_hash_table_iter_next(&it, &k, &v))
    {
        SpriteAnim *anim = (SpriteAnim *)v;
        for (int i = 0; i < anim->frame_count; ++i)
            if (anim->frames[i]) g_object_unref(anim->frames[i]);
        g_free(anim->key);
        g_free(anim);
    }
    g_hash_table_destroy(g_anim_cache);
    g_anim_cache = NULL;
}
