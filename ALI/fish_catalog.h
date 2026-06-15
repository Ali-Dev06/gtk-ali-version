#ifndef FISH_CATALOG_H
#define FISH_CATALOG_H

#include "aquarium.h"

#define LIBRARY_MIN_FISH_SIZE 80.0f
#define LIBRARY_MAX_FISH_SIZE 250.0f

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
void resolve_sprite_directory(char *out_dir, size_t out_size);
int count_groups(void);

/* Sprite-sheet animation cache.
 * Given a fish image path (e.g. "FISH/dolphin.png"), find and lazily load the
 * matching animation sheet from the SPRITES folder. Returns a shared SpriteAnim*
 * (owned by the cache, never freed per-fish) or NULL if no sheet exists. */
SpriteAnim *sprite_anim_for_image(const char *image_path);
int sprite_anim_frame_count(const SpriteAnim *anim);
GdkPixbuf *sprite_anim_frame(const SpriteAnim *anim, int index); /* native cache frame, right-facing */
void sprite_anim_cleanup(void); /* free the whole cache (call at shutdown) */

const char *goal_to_string(Goal goal);
const char *personality_to_string(Personality p);
const char *life_stage_to_string(LifeStage stage);
const char *fish_type_to_string(FishType type);

Fish *find_nearest_mate(Fish *fish);
void spawn_fish_from_preset(const PresetFish *preset, int count, float size, float speed,
                            FishType type_override, int type_use_override);

#endif
