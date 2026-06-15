#ifndef FISH_CATALOG_H
#define FISH_CATALOG_H

#include "aquarium.h"

#define LIBRARY_MIN_FISH_SIZE 80.0f
#define LIBRARY_MAX_FISH_SIZE 180.0f

typedef struct {
    const char *species;
    const char *image_file;
    FishType type;
    DietType diet;
    float default_size;
    float default_speed;
} PresetFish;

int preset_fish_count(void);
const PresetFish *preset_fish_get(int index);
char *preset_fish_image_path(int index);
void resolve_images_directory(char *out_dir, size_t out_size);
int count_groups(void);

const char *goal_to_string(Goal goal);
const char *personality_to_string(Personality p);
const char *life_stage_to_string(LifeStage stage);
const char *fish_type_to_string(FishType type);

Fish *find_nearest_mate(Fish *fish);
void spawn_fish_from_preset(const PresetFish *preset, int count, float size, float speed,
                            FishType type_override, int type_use_override);

#endif
