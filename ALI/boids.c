#include "aquarium.h"
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GRID_CELL_SIZE 70.0f

static Fish **grid_heads = NULL;
static int grid_cols = 0;
static int grid_rows = 0;

static float rand_range(float minv, float maxv)
{
    return minv + ((float)rand() / (float)RAND_MAX) * (maxv - minv);
}

static int grid_index(int col, int row)
{
    return row * grid_cols + col;
}

void rebuild_spatial_grid(void)
{
    if (!g_aquarium)
        return;

    int cols = (int)(g_aquarium->width / GRID_CELL_SIZE) + 1;
    int rows = (int)(g_aquarium->height / GRID_CELL_SIZE) + 1;

    if (cols != grid_cols || rows != grid_rows || !grid_heads)
    {
        free(grid_heads);
        grid_heads = (Fish **)calloc(cols * rows, sizeof(Fish *));
        grid_cols = cols;
        grid_rows = rows;
    }
    else
    {
        for (int i = 0; i < grid_cols * grid_rows; ++i)
            grid_heads[i] = NULL;
    }

    Fish *current = g_aquarium->fish_list;
    while (current)
    {
        if (current->is_alive)
        {
            int col = (int)(current->x / GRID_CELL_SIZE);
            int row = (int)(current->y / GRID_CELL_SIZE);
            col = col < 0 ? 0 : (col >= grid_cols ? grid_cols - 1 : col);
            row = row < 0 ? 0 : (row >= grid_rows ? grid_rows - 1 : row);
            int index = grid_index(col, row);
            current->next_grid = grid_heads[index];
            grid_heads[index] = current;
        }
        current = current->next;
    }
}

static Fish *lookup_neighbor(Fish *fish, int neighbor_index)
{
    (void)neighbor_index;
    return NULL;
}

void apply_group_behavior(Fish *fish)
{
    if (!fish->is_alive || !g_aquarium)
        return;

    float cohesion_x = 0.0f;
    float cohesion_y = 0.0f;
    float alignment_x = 0.0f;
    float alignment_y = 0.0f;
    float separation_x = 0.0f;
    float separation_y = 0.0f;
    int neighbor_count = 0;

    int cell_x = (int)(fish->x / GRID_CELL_SIZE);
    int cell_y = (int)(fish->y / GRID_CELL_SIZE);
    float personal_space = fish->size * 1.2f + 12.0f;

    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int col = cell_x + dx;
            int row = cell_y + dy;
            if (col < 0 || row < 0 || col >= grid_cols || row >= grid_rows)
                continue;

            Fish *neighbor = grid_heads[grid_index(col, row)];
            while (neighbor)
            {
                if (neighbor != fish && neighbor->is_alive)
                {
                    int is_predator_enemy = (fish->type == TYPE_PREY && neighbor->type == TYPE_PREDATOR);
                    int is_prey_enemy = (fish->type == TYPE_PREDATOR && neighbor->type == TYPE_PREY);
                    int is_school_friend = 0;
                    if (fish->group && neighbor->group == fish->group)
                        is_school_friend = 1;
                    else if (fish->species && neighbor->species &&
                             strcmp(fish->species, neighbor->species) == 0 &&
                             fish->type == neighbor->type)
                        is_school_friend = 1;

                    float dxn = neighbor->x - fish->x;
                    float dyn = neighbor->y - fish->y;
                    if (fabsf(dxn) > g_aquarium->width / 2.0f)
                        dxn = (dxn > 0) ? dxn - g_aquarium->width : dxn + g_aquarium->width;
                    if (fabsf(dyn) > g_aquarium->height / 2.0f)
                        dyn = (dyn > 0) ? dyn - g_aquarium->height : dyn + g_aquarium->height;

                    float dist = sqrtf(dxn * dxn + dyn * dyn) + 1.0f;
                    if (dist < fish->size * 9.0f)
                    {
                        neighbor_count++;
                        cohesion_x += neighbor->x;
                        cohesion_y += neighbor->y;
                        alignment_x += neighbor->vx;
                        alignment_y += neighbor->vy;

                        if (dist < personal_space)
                        {
                            float push = (personal_space - dist) / personal_space;
                            float sep_force = 16.0f;
                            if (is_predator_enemy)
                                sep_force = 42.0f;
                            else if (is_prey_enemy && fish->hunger > 45.0f)
                                sep_force = 8.0f;
                            else if (is_school_friend)
                                sep_force = 9.0f;
                            separation_x -= dxn / (dist + 1.0f) * push * sep_force;
                            separation_y -= dyn / (dist + 1.0f) * push * sep_force;
                        }

                        if (is_predator_enemy && dist < fish->size * 7.0f)
                        {
                            separation_x -= dxn / (dist + 1.0f) * 1.2f;
                            separation_y -= dyn / (dist + 1.0f) * 1.2f;
                        }

                        if (is_school_friend && dist < fish->size * 6.0f)
                        {
                            cohesion_x += neighbor->x * 1.15f;
                            cohesion_y += neighbor->y * 1.15f;
                        }
                    }
                }
                neighbor = neighbor->next_grid;
            }
        }
    }

    if (neighbor_count > 0)
    {
        cohesion_x /= (float)neighbor_count;
        cohesion_y /= (float)neighbor_count;
        alignment_x /= (float)neighbor_count;
        alignment_y /= (float)neighbor_count;

        float center_x = cohesion_x / (float)neighbor_count;
        float center_y = cohesion_y / (float)neighbor_count;

        float social_factor = 0.1f;
        float shy_factor = 0.0f;
        float aggressive_factor = 0.0f;
        if (fish->personality == PERSONALITY_SHY)
        {
            shy_factor = 0.12f;
        }
        else if (fish->personality == PERSONALITY_AGGRESSIVE)
        {
            aggressive_factor = 0.08f;
        }
        else if (fish->personality == PERSONALITY_SOCIAL)
        {
            social_factor = 0.16f;
        }
        else if (fish->personality == PERSONALITY_CURIOUS)
        {
            social_factor = 0.12f;
        }
        else if (fish->personality == PERSONALITY_TERRITORIAL)
        {
            social_factor = 0.09f;
        }

        float cohesion_force_x = (center_x - fish->x) * (0.05f + social_factor);
        float cohesion_force_y = (center_y - fish->y) * (0.05f + social_factor);
        float alignment_force_x = (alignment_x - fish->vx) * (0.03f + social_factor * 0.5f);
        float alignment_force_y = (alignment_y - fish->vy) * (0.03f + social_factor * 0.5f);

        fish->target_vx += cohesion_force_x + alignment_force_x + separation_x * (1.0f + shy_factor) + aggressive_factor * 0.2f;
        fish->target_vy += cohesion_force_y + alignment_force_y + separation_y * (1.0f + shy_factor) + aggressive_factor * 0.2f;
    }

    if (fish->group && fish->group->leader && fish->group->leader != fish)
    {
        Fish *leader = fish->group->leader;
        float leader_dx = leader->x - fish->x;
        float leader_dy = leader->y - fish->y;

        if (fabsf(leader_dx) > g_aquarium->width / 2.0f)
            leader_dx = (leader_dx > 0) ? leader_dx - g_aquarium->width : leader_dx + g_aquarium->width;
        if (fabsf(leader_dy) > g_aquarium->height / 2.0f)
            leader_dy = (leader_dy > 0) ? leader_dy - g_aquarium->height : leader_dy + g_aquarium->height;

        float follow_distance = 18.0f + fish->leader_order * 9.0f;
        float dist = sqrtf(leader_dx * leader_dx + leader_dy * leader_dy) + 1.0f;
        if (dist > follow_distance)
        {
            float follow_force = (dist - follow_distance) * 0.06f;
            fish->target_vx += leader_dx / dist * follow_force;
            fish->target_vy += leader_dy / dist * follow_force;
        }
        else
        {
            fish->target_vx += (-leader_dx) * 0.003f;
            fish->target_vy += (-leader_dy) * 0.003f;
        }
    }

    if (fish->current_goal == GOAL_ESCAPE)
    {
        Fish *predator = find_nearest_predator(fish);
        if (predator)
        {
            float dx = fish->x - predator->x;
            float dy = fish->y - predator->y;
            if (fabsf(dx) > g_aquarium->width / 2.0f)
                dx = (dx > 0) ? dx - g_aquarium->width : dx + g_aquarium->width;
            if (fabsf(dy) > g_aquarium->height / 2.0f)
                dy = (dy > 0) ? dy - g_aquarium->height : dy + g_aquarium->height;
            float d = sqrtf(dx * dx + dy * dy) + 1.0f;
            fish->target_vx += dx / d * 0.85f;
            fish->target_vy += dy / d * 0.85f;
        }
    }
}

void merge_compatible_groups(void)
{
    if (!g_aquarium)
        return;

    Group *group = g_aquarium->group_list;
    while (group)
    {
        Group *next_group = group->next;
        Group *other = group->next;
        while (other)
        {
            if (group->leader && other->leader)
            {
                int name_match = 0;
                if (group->name && other->name && group->name[0] != '\0' && other->name[0] != '\0')
                {
                    if (strcmp(group->name, other->name) == 0)
                        name_match = 1;
                }

                int attr_match = 0;
                if (group->leader->species && other->leader->species)
                {
                    if (strcmp(group->leader->species, other->leader->species) == 0 &&
                        group->leader->type == other->leader->type &&
                        group->leader->personality == other->leader->personality)
                        attr_match = 1;
                }

                if (name_match || attr_match)
                {
                    float dx = other->leader->x - group->leader->x;
                    float dy = other->leader->y - group->leader->y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < 140.0f)
                    {
                        Group *dominant = group;
                        Group *subordinate = other;
                        if (other->leader->size > group->leader->size ||
                            (fabsf(other->leader->size - group->leader->size) < 0.1f && other->leader->id > group->leader->id))
                        {
                            dominant = other;
                            subordinate = group;
                        }

                        // First, move subordinate leader into dominant as a follower
                        if (subordinate->leader)
                        {
                            Fish *slead = subordinate->leader;
                            slead->group = dominant;
                            slead->is_leader = 0;
                            slead->leader_order = dominant->member_count;
                            // append to dominant members list
                            slead->group_prev = dominant->members_tail;
                            slead->group_next = NULL;
                            if (dominant->members_tail)
                                dominant->members_tail->group_next = slead;
                            else
                                dominant->members_head = slead;
                            dominant->members_tail = slead;
                            dominant->member_count++;
                        }

                        // Now move subordinate's other members
                        Fish *member = subordinate->members_head;
                        while (member)
                        {
                            Fish *next = member->group_next;
                            member->group = dominant;
                            member->is_leader = 0;
                            member->leader_order = dominant->member_count;
                            member->group_prev = dominant->members_tail;
                            member->group_next = NULL;
                            if (dominant->members_tail)
                                dominant->members_tail->group_next = member;
                            else
                                dominant->members_head = member;
                            dominant->members_tail = member;
                            dominant->member_count++;
                            member = next;
                        }

                        // ensure dominant leader stays leader
                        if (dominant->leader)
                        {
                            dominant->leader->is_leader = 1;
                            dominant->leader->leader_order = 0;
                        }

                        Group *prev = NULL;
                        Group *scan = g_aquarium->group_list;
                        while (scan)
                        {
                            if (scan == subordinate)
                            {
                                if (prev)
                                    prev->next = scan->next;
                                else
                                    g_aquarium->group_list = scan->next;
                                break;
                            }
                            prev = scan;
                            scan = scan->next;
                        }

                        subordinate->members_head = NULL;
                        subordinate->members_tail = NULL;
                        subordinate->member_count = 0;
                        subordinate->leader = NULL;
                        g_free(subordinate->name);
                        free(subordinate);

                        update_group_formation(dominant);
                        group = next_group;
                        break;
                    }
                }
            }
            other = other->next;
        }
        group = next_group;
    }
}
