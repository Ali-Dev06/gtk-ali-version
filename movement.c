#include "aquarium.h"

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

static float normalize_angle(float angle)
{
    while (angle > M_PI)
        angle -= 2.0f * M_PI;
    while (angle < -M_PI)
        angle += 2.0f * M_PI;
    return angle;
}

static float smooth_angle(float current, float target, float amount)
{
    float delta = normalize_angle(target - current);
    return normalize_angle(current + delta * amount);
}

void update_fish_movement(Fish *fish, float dt)
{
    if (!fish->is_alive || !g_aquarium)
        return;

    float dt_scale = dt * 60.0f;
    fish->swim_phase += dt * (1.6f + fish->base_speed * 0.2f);

    if (fish->wander_timer <= 0.0f)
    {
        fish->wander_timer = 3.0f + rand_range(0.0f, 3.5f);
        fish->target_x = fish->home_x + rand_range(-220.0f, 220.0f);
        fish->target_y = fish->home_y + rand_range(-130.0f, 130.0f);
    }
    fish->wander_timer -= dt;
    float current_goal_speed = fish->base_speed;

    if (fish->current_goal == GOAL_REST)
    {
        current_goal_speed *= 0.35f;
    }
    else if (fish->current_goal == GOAL_ESCAPE)
    {
        current_goal_speed *= 1.55f;
    }
    else if (fish->current_goal == GOAL_HUNT)
    {
        current_goal_speed *= 1.3f;
    }
    else if (fish->current_goal == GOAL_SEARCH_FOOD)
    {
        current_goal_speed *= 1.2f;
    }
    else if (fish->current_goal == GOAL_MATE)
    {
        current_goal_speed *= 0.9f;
    }
    else if (fish->current_goal == GOAL_GLIDE)
    {
        current_goal_speed *= 0.55f;
    }
    else if (fish->current_goal == GOAL_MATE)
    {
        current_goal_speed *= 0.85f;
    }

    switch (fish->personality)
    {
        case PERSONALITY_SHY:
            current_goal_speed *= 0.88f;
            break;
        case PERSONALITY_AGGRESSIVE:
            current_goal_speed *= 1.12f;
            break;
        case PERSONALITY_SOCIAL:
            current_goal_speed *= 1.0f;
            break;
        case PERSONALITY_CURIOUS:
            current_goal_speed *= 1.05f;
            break;
        case PERSONALITY_TERRITORIAL:
            current_goal_speed *= 0.92f;
            break;
        case PERSONALITY_PEACEFUL:
        default:
            current_goal_speed *= 0.96f;
            break;
    }

    if (fish->life_stage == STAGE_BABY)
    {
        current_goal_speed *= 1.08f;
    }
    else if (fish->life_stage == STAGE_ELDERLY)
    {
        current_goal_speed *= 0.78f;
    }

    if (fish->health < 30.0f)
    {
        current_goal_speed *= 0.65f;
    }

    current_goal_speed = clampf(current_goal_speed, 0.6f, 6.0f);
    fish->desired_speed = current_goal_speed;

    float desired_x = fish->target_x;
    float desired_y = fish->target_y;

    if (fish->group && fish->group->leader && fish->group->leader != fish)
    {
        Fish *leader = fish->group->leader;
        float follow_distance = 18.0f + fish->leader_order * 9.0f;
        float dx = leader->x - fish->x;
        float dy = leader->y - fish->y;
        if (fabsf(dx) > g_aquarium->width / 2.0f)
            dx = (dx > 0) ? dx - g_aquarium->width : dx + g_aquarium->width;
        if (fabsf(dy) > g_aquarium->height / 2.0f)
            dy = (dy > 0) ? dy - g_aquarium->height : dy + g_aquarium->height;
        float dist = sqrtf(dx * dx + dy * dy) + 1.0f;
        if (dist > follow_distance)
        {
            desired_x = leader->x - cosf(leader->heading) * follow_distance;
            desired_y = leader->y - sinf(leader->heading) * follow_distance;
        }
    }

    float dx = desired_x - fish->x;
    float dy = desired_y - fish->y;
    if (fabsf(dx) > g_aquarium->width / 2.0f)
        dx = (dx > 0) ? dx - g_aquarium->width : dx + g_aquarium->width;
    if (fabsf(dy) > g_aquarium->height / 2.0f)
        dy = (dy > 0) ? dy - g_aquarium->height : dy + g_aquarium->height;

    float dist = sqrtf(dx * dx + dy * dy) + 1.0f;
    float desired_heading = atan2f(dy, dx);

    if (dist < 30.0f)
    {
        desired_heading = fish->heading + sinf(fish->swim_phase) * 0.25f;
    }

    fish->target_heading = desired_heading;
    fish->heading = smooth_angle(fish->heading, desired_heading, clampf(0.12f + fish->turn_smoothness * 0.1f, 0.08f, 0.26f));

    float desired_vx = cosf(fish->heading) * fish->desired_speed;
    float desired_vy = sinf(fish->heading) * fish->desired_speed;

    /* Side-to-side body glide perpendicular to heading */
    float wobble = sinf(fish->swim_phase) * 0.18f * fish->desired_speed;
    desired_vx += -sinf(fish->heading) * wobble;
    desired_vy += cosf(fish->heading) * wobble;

    if (fish->current_goal == GOAL_ESCAPE)
    {
        desired_vx *= -1.0f;
        desired_vy *= -1.0f;
    }

    if (fish->current_goal == GOAL_REST)
    {
        desired_vx *= 0.68f;
        desired_vy *= 0.55f;
    }

    float accel = 0.85f + (1.0f - fish->hunger / 100.0f) * 0.55f + (fish->current_goal == GOAL_HUNT ? 0.25f : 0.0f);
    fish->vx += (desired_vx - fish->vx) * accel * dt;
    fish->vy += (desired_vy - fish->vy) * accel * dt;

    float drag = 0.12f + fish->drag + (fish->current_goal == GOAL_REST ? 0.06f : 0.0f);
    fish->vx *= (1.0f - drag * dt);
    fish->vy *= (1.0f - drag * dt);

    float speed = sqrtf(fish->vx * fish->vx + fish->vy * fish->vy);
    float max_speed = fish->desired_speed * 1.15f;
    if (speed > max_speed)
    {
        fish->vx = fish->vx / speed * max_speed;
        fish->vy = fish->vy / speed * max_speed;
    }

    float wall_margin = fish->size * 0.70f;
    float wall_repulsion = 0.28f;

    if (fish->y < wall_margin)
    {
        fish->vy += wall_repulsion * (wall_margin - fish->y) / wall_margin;
    }
    else if (fish->y > g_aquarium->height - wall_margin)
    {
        fish->vy -= wall_repulsion * (fish->y - (g_aquarium->height - wall_margin)) / wall_margin;
    }

    fish->x += fish->vx * dt_scale;
    fish->y += fish->vy * dt_scale;

    apply_boundary_wrap(fish);

    if (speed > 0.25f)
    {
        fish->heading = smooth_angle(fish->heading, atan2f(fish->vy, fish->vx), 0.18f);
        if (cosf(fish->heading) >= 0.0f)
        {
            fish->direction = DIRECTION_RIGHT;
            fish->image_facing = DIRECTION_RIGHT;
        }
        else
        {
            fish->direction = DIRECTION_LEFT;
            fish->image_facing = DIRECTION_LEFT;
        }
    }

    fish->tail_phase += dt * (2.8f + speed * 0.55f);
    fish->glow = 0.55f + (sin(fish->swim_phase * 0.8f) + 1.0f) * 0.2f;

    if (fish->health < 25.0f)
    {
        fish->vy += 0.25f * dt_scale;
        fish->visibility = 0.55f;
    }
    else
    {
        fish->visibility = clampf(0.72f + (1.0f - fish->y / (float)g_aquarium->height) * 0.22f, 0.45f, 1.0f);
    }

    fish->render_scale = clampf(1.0f + fish->y / (float)g_aquarium->height * 0.12f - (fish->life_stage == STAGE_BABY ? 0.08f : 0.0f), 0.82f, 1.20f);
}
