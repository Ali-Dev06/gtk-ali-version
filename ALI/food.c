#include "aquarium.h"

static float rand_range(float minv, float maxv)
{
    return minv + ((float)rand() / (float)RAND_MAX) * (maxv - minv);
}

void spawn_food_pellets(int count, float spawn_x, float spawn_y)
{
    if (!g_aquarium)
        return;

    for (int i = 0; i < count; ++i)
    {
        int slot = -1;
        for (int j = 0; j < MAX_FOOD; ++j)
        {
            if (!g_aquarium->env.food_sources[j].is_alive)
            {
                slot = j;
                break;
            }
        }
        if (slot < 0)
            continue;

        Food *pellet = &g_aquarium->env.food_sources[slot];
        pellet->x = spawn_x + rand_range(-24.0f, 24.0f);
        pellet->y = spawn_y + rand_range(-28.0f, 28.0f);
        pellet->vx = rand_range(-0.8f, 0.8f);
        pellet->vy = rand_range(0.8f, 1.8f);
        pellet->energy = 25.0f;
        pellet->age = 0.0f;
        pellet->lifetime = 10.0f + rand_range(0.0f, 5.0f);
        pellet->is_alive = 1;
        g_aquarium->env.food_count = (g_aquarium->env.food_count < MAX_FOOD) ? g_aquarium->env.food_count + 1 : MAX_FOOD;
    }
}

void update_food_physics(float dt)
{
    if (!g_aquarium)
        return;

    for (int i = 0; i < MAX_FOOD; ++i)
    {
        Food *pellet = &g_aquarium->env.food_sources[i];
        if (!pellet->is_alive)
            continue;

        pellet->age += dt;
        pellet->vy += 0.22f * dt * 60.0f;
        pellet->vx *= 0.985f;
        pellet->x += pellet->vx * dt * 60.0f;
        pellet->y += pellet->vy * dt * 60.0f;

        if (pellet->y > g_aquarium->height + 20.0f || pellet->age > pellet->lifetime)
        {
            pellet->is_alive = 0;
        }
    }
}

Food *find_nearest_food(Fish *fish)
{
    if (!fish || !g_aquarium)
        return NULL;

    Food *nearest = NULL;
    float nearest_dist = 9999.0f;

    for (int i = 0; i < MAX_FOOD; ++i)
    {
        Food *pellet = &g_aquarium->env.food_sources[i];
        if (!pellet->is_alive)
            continue;

        float dx = pellet->x - fish->x;
        float dy = pellet->y - fish->y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < nearest_dist)
        {
            nearest_dist = dist;
            nearest = pellet;
        }
    }
    return nearest;
}

void consume_food(Fish *fish)
{
    if (!fish || !g_aquarium)
        return;

    Food *nearest = find_nearest_food(fish);
    if (!nearest)
        return;

    float dx = nearest->x - fish->x;
    float dy = nearest->y - fish->y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < fish->size * 1.25f)
    {
        fish->hunger = fmaxf(0.0f, fish->hunger - 22.0f);
        fish->energy = fminf(100.0f, fish->energy + 18.0f);
        fish->health = fminf(100.0f, fish->health + 4.0f);
        nearest->is_alive = 0;
        g_aquarium->total_eats++;
    }
}
