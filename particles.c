#include "aquarium.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BUBBLE_COUNT 24

typedef struct {
    float x, y;
    float vx, vy;
    float radius;
    float alpha;
    float phase;
} Bubble;

static Bubble bubbles[BUBBLE_COUNT];

static float rand_range(float minv, float maxv)
{
    return minv + ((float)rand() / (float)RAND_MAX) * (maxv - minv);
}

void init_particle_system(void)
{
    for (int i = 0; i < BUBBLE_COUNT; ++i)
    {
        bubbles[i].x = rand_range(40.0f, 1140.0f);
        bubbles[i].y = rand_range(40.0f, 760.0f);
        bubbles[i].vx = rand_range(-0.30f, 0.30f);
        bubbles[i].vy = rand_range(-0.25f, -0.08f);
        bubbles[i].radius = rand_range(1.8f, 5.2f);
        bubbles[i].alpha = rand_range(0.15f, 0.35f);
        bubbles[i].phase = rand_range(0.0f, 2.0f * M_PI);
    }
}

void update_particle_system(float dt)
{
    for (int i = 0; i < BUBBLE_COUNT; ++i)
    {
        Bubble *bubble = &bubbles[i];
        bubble->x += bubble->vx * dt * 60.0f;
        bubble->y += bubble->vy * dt * 60.0f;
        bubble->phase += dt * 1.8f;

        bubble->vy -= 0.02f * dt * 60.0f;
        bubble->vx += sinf(bubble->phase) * 0.01f;

        if (bubble->y < -10.0f)
        {
            bubble->y = 820.0f;
            bubble->x = rand_range(20.0f, 1180.0f);
            bubble->vx = rand_range(-0.35f, 0.35f);
            bubble->vy = rand_range(0.2f, 0.8f);
            bubble->radius = rand_range(2.0f, 5.0f);
            bubble->alpha = rand_range(0.16f, 0.32f);
        }

        if (bubble->x < -20.0f || bubble->x > 1220.0f)
        {
            bubble->x = rand_range(20.0f, 1180.0f);
            bubble->vx = rand_range(-0.35f, 0.35f);
        }
    }
}

void draw_background_effects(cairo_t *cr)
{
    for (int i = 0; i < BUBBLE_COUNT; ++i)
    {
        Bubble *bubble = &bubbles[i];
        cairo_set_source_rgba(cr, 0.9f, 0.99f, 1.0f, bubble->alpha);
        cairo_arc(cr, bubble->x, bubble->y, bubble->radius, 0, 2.0f * M_PI);
        cairo_fill(cr);
    }

    cairo_set_source_rgba(cr, 0.5f, 0.9f, 1.0f, 0.08f);
    for (int i = 0; i < 8; ++i)
    {
        double wave_x = 40.0 + i * 110.0;
        double wave_y = 120.0 + i * 18.0;
        double wave = sin((g_get_real_time() / 1000000.0) * 0.7 + i) * 12.0;
        cairo_arc(cr, wave_x, wave_y + wave, 30.0, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}
