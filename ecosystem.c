#include "aquarium.h"
#include "fish_catalog.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static float rand_range(float min_value, float max_value)
{
    return min_value + ((float)rand() / (float)RAND_MAX) * (max_value - min_value);
}

void update_fish_ecosystem(Fish *fish, float dt)
{
    if (!fish->is_alive || !g_aquarium)
        return;

    fish->age += dt;
    fish->hunger = clampf(fish->hunger + dt * 0.5f, 0.0f, 100.0f);
    fish->energy = clampf(fish->energy - dt * 0.32f, 0.0f, 100.0f);

    if (fish->hunger > 60.0f)
    {
        fish->health -= dt * 0.45f;
        fish->status = STATUS_HUNGRY;
    }
    else if (fish->energy < 25.0f)
    {
        fish->health -= dt * 0.35f;
        fish->status = STATUS_SICK;
    }
    else
    {
        fish->health = clampf(fish->health + dt * 0.22f, 0.0f, 100.0f);
        fish->status = STATUS_ALIVE;
    }

    fish->attack_cooldown = fmaxf(0.0f, fish->attack_cooldown - dt);
    fish->panic_timer = fmaxf(0.0f, fish->panic_timer - dt);

    Fish *near_predator = (fish->type == TYPE_PREY) ? find_nearest_predator(fish) : NULL;
    float predator_dist = near_predator ? wrap_distance(fish, near_predator) : 999.0f;

    if (fish->type == TYPE_PREY && near_predator && predator_dist < 130.0f)
    {
        fish->current_goal = GOAL_ESCAPE;
        fish->panic_timer = 2.5f;
        float dx = fish->x - near_predator->x;
        float dy = fish->y - near_predator->y;
        float d = sqrtf(dx * dx + dy * dy) + 0.001f;
        fish->target_x = fish->x + (dx / d) * 180.0f;
        fish->target_y = fish->y + (dy / d) * 180.0f;
    }
    else if (fish->hunger > 85.0f)
    {
        fish->current_goal = GOAL_SEARCH_FOOD;
    }
    else if (fish->health < 25.0f || fish->energy < 20.0f)
    {
        fish->current_goal = GOAL_REST;
    }
    else if (fish->type == TYPE_PREDATOR && fish->hunger > 50.0f && fish->attack_cooldown <= 0.0f)
    {
        Fish *prey = find_nearest_prey(fish);
        if (prey)
        {
            fish->current_goal = GOAL_HUNT;
            fish->target_x = prey->x;
            fish->target_y = prey->y;
        }
    }
    else if (fish->type == TYPE_PREY && fish->hunger > 55.0f)
    {
        fish->current_goal = GOAL_SEARCH_FOOD;
    }
    else if (fish->type == TYPE_PREY && fish->life_stage >= STAGE_ADULT &&
             fish->hunger < 50.0f && fish->health > 60.0f && fish->mate_target_id < 0)
    {
        Fish *mate = find_nearest_mate(fish);
        if (mate)
        {
            fish->current_goal = GOAL_MATE;
            fish->mate_target_id = mate->id;
            fish->mate_timer = 10.0f;
            mate->mate_target_id = fish->id;
            mate->mate_timer = 10.0f;
            mate->current_goal = GOAL_MATE;
            fish->target_x = mate->x;
            fish->target_y = mate->y;
        }
    }
    else if (fish->energy < 35.0f && fish->hunger < 40.0f)
    {
        fish->current_goal = GOAL_GLIDE;
    }
    else if (fish->group && fish->group->leader && fish->group->leader != fish &&
             fish->life_stage == STAGE_BABY)
    {
        fish->current_goal = GOAL_FOLLOW_LEADER;
    }
    else if (fish->group && fish->group->leader)
    {
        fish->current_goal = GOAL_IDLE;
    }

    if (fish->current_goal == GOAL_MATE && fish->mate_target_id >= 0)
    {
        Fish *partner = g_aquarium->fish_list;
        while (partner)
        {
            if (partner->id == fish->mate_target_id && partner->is_alive)
            {
                fish->target_x = partner->x;
                fish->target_y = partner->y;
                break;
            }
            partner = partner->next;
        }
    }

    if (fish->age < 25.0f)
    {
        fish->life_stage = STAGE_BABY;
        fish->size = fish->base_size * 0.70f;
        fish->speed = fish->base_speed * 1.08f;
    }
    else if (fish->age < 70.0f)
    {
        fish->life_stage = STAGE_JUVENILE;
        fish->size = fish->base_size * 0.86f;
        fish->speed = fish->base_speed * 1.0f;
    }
    else if (fish->age < fish->lifespan * 0.72f)
    {
        fish->life_stage = STAGE_ADULT;
        fish->size = fish->base_size * 1.0f;
        fish->speed = fish->base_speed * 0.98f;
    }
    else
    {
        fish->life_stage = STAGE_ELDERLY;
        fish->size = fish->base_size * 1.08f;
        fish->speed = fish->base_speed * 0.78f;
        fish->health -= dt * 0.25f;
    }

    if (fish->health <= 0.0f || fish->age >= fish->lifespan)
    {
        fish_die(fish);
        return;
    }

    fish->mate_timer = fmaxf(0.0f, fish->mate_timer - dt);

    if (fish->mate_target_id >= 0 && fish->mate_timer > 0.0f)
    {
        Fish *partner = g_aquarium->fish_list;
        while (partner)
        {
            if (partner->id == fish->mate_target_id && partner->is_alive)
            {
                float dx = partner->x - fish->x;
                float dy = partner->y - fish->y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 12.0f && fish->type == TYPE_PREY && partner->type == TYPE_PREY)
                {
                    spawn_baby_fish(fish, partner);
                    fish->mate_timer = 0.0f;
                    partner->mate_timer = 0.0f;
                    fish->mate_target_id = -1;
                    partner->mate_target_id = -1;
                    break;
                }
                break;
            }
            partner = partner->next;
        }
    }
}

void spawn_baby_fish(Fish *dad, Fish *mom)
{
    if (!dad || !mom || !g_aquarium)
        return;

    float spawn_x = (dad->x + mom->x) * 0.5f + rand_range(-12.0f, 12.0f);
    float spawn_y = (dad->y + mom->y) * 0.5f + rand_range(-12.0f, 12.0f);

    Fish *baby = create_fish(dad->species, dad->image_path, spawn_x, spawn_y, dad->size * 0.58f, dad->speed * 1.05f,
                             dad->type, dad->diet, (rand() % 2 == 0) ? GENDER_MALE : GENDER_FEMALE, DIRECTION_RIGHT);
    baby->generation = (dad->generation > mom->generation ? dad->generation : mom->generation) + 1;
    baby->age = 0.0f;
    baby->lifespan = 110.0f + rand_range(0.0f, 30.0f);
    baby->hunger = 15.0f;
    baby->energy = 70.0f;
    baby->personality = dad->personality;
    baby->current_goal = GOAL_FOLLOW_LEADER;
    baby->home_x = spawn_x;
    baby->home_y = spawn_y;
    baby->base_speed = baby->speed;
    baby->base_size = baby->size;

    add_fish(baby);

    if (dad->group || mom->group)
    {
        Group *target_group = dad->group ? dad->group : mom->group;
        if (target_group)
        {
            add_fish_to_group(target_group, baby);
        }
    }
}

// merge_compatible_groups() is defined in boids.c
extern void merge_compatible_groups(void);


