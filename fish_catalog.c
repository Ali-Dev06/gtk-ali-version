#include "fish_catalog.h"
#include <string.h>
#include <unistd.h>
#include <libgen.h>

static const PresetFish g_presets[] = {
    {"Clownfish", "fish.png", TYPE_PREY, DIET_OMNIVORE, 82.0f, 2.4f},
    {"Blue Tang", "TropicalFish01.png", TYPE_PREY, DIET_HERBIVORE, 80.0f, 2.6f},
    {"Royal Gramma", "TropicalFish02.png", TYPE_PREY, DIET_OMNIVORE, 80.0f, 2.5f},
    {"Yellow Tang", "TropicalFish03.png", TYPE_PREY, DIET_HERBIVORE, 80.0f, 2.7f},
    {"Copperband", "TropicalFish04.png", TYPE_PREY, DIET_OMNIVORE, 84.0f, 2.3f},
    {"Mandarin", "TropicalFish05.png", TYPE_PREY, DIET_CARNIVORE, 80.0f, 2.2f},
    {"Angelfish", "TropicalFish06.png", TYPE_PREY, DIET_OMNIVORE, 86.0f, 2.4f},
    {"Butterflyfish", "TropicalFish07.png", TYPE_PREY, DIET_OMNIVORE, 80.0f, 2.5f},
    {"Surgeonfish", "TropicalFish08.png", TYPE_PREY, DIET_HERBIVORE, 82.0f, 2.8f},
    {"Damselfish", "TropicalFish09.png", TYPE_PREY, DIET_OMNIVORE, 80.0f, 3.0f},
    {"Wrasse", "TropicalFish10.png", TYPE_PREY, DIET_CARNIVORE, 80.0f, 2.9f},
    {"Parrotfish", "TropicalFish11.png", TYPE_PREY, DIET_HERBIVORE, 88.0f, 2.2f},
    {"Lionfish", "TropicalFish12.png", TYPE_PREDATOR, DIET_CARNIVORE, 92.0f, 2.0f},
    {"Barracuda", "TropicalFish13.png", TYPE_PREDATOR, DIET_CARNIVORE, 96.0f, 3.2f},
    {"Moray", "TropicalFish14.png", TYPE_PREDATOR, DIET_CARNIVORE, 90.0f, 1.8f},
    {"Reef Shark", "TropicalFish15.png", TYPE_PREDATOR, DIET_CARNIVORE, 100.0f, 2.8f},
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
    static const char *candidates[] = {
        "images",
        "test4/images",
        "../images",
        "../test4/images",
        NULL
    };

    for (int i = 0; candidates[i]; ++i)
    {
        char probe[512];
        snprintf(probe, sizeof(probe), "%s/fish.png", candidates[i]);
        if (g_file_test(probe, G_FILE_TEST_EXISTS))
        {
            g_strlcpy(out_dir, candidates[i], out_size);
            return;
        }
    }

    g_strlcpy(out_dir, "images", out_size);
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
