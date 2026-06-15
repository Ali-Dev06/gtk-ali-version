# Fish Sprite Animation — How It Works

A plain-English + real-code guide to how fish are animated from sprite sheets.

## In simple terms

- Each sheet = **25 little pictures** of the fish in a **5×5 grid**.
- **Load once, cut into 25 frames.** When a fish needs animating, the code opens its sheet
  (e.g. `dolphin_25.png`), slices it into the 25 frames, and keeps them in a **shared cache**
  so all dolphins reuse the same frames.
- Each fish remembers **which frame it's on** (`anim_frame`) and a little **timer** (`anim_timer`).
- Every tick the timer counts up. When enough time passes (~10–20×/sec, faster when the fish
  swims faster), it moves to the **next frame** (looping `0 → 24 → 0`) and swaps that frame into
  the fish's image.
- **Drawing just shows the current frame** — flipping through them quickly = animation.
- **No sheet? No animation.** Fish without a sheet (orca, shark) just show their static image.

> That's it: cut the sheet into 25 pictures → show them one after another → it looks alive. 🐟

---

## 1. The data: what's stored

The shared cache type (`fish_catalog.c`) — one per **species**, holds the 25 cut-out frames:

```c
#define ANIM_GRID_COLS      5
#define ANIM_GRID_ROWS      5
#define ANIM_MAX_FRAMES     (ANIM_GRID_COLS * ANIM_GRID_ROWS)   // 25
#define ANIM_CACHE_FRAME_PX 220   // each frame stored at 220x220

struct SpriteAnim {
    char *key;                          // the sheet path, e.g. "SPRITES/dolphin_25.png"
    int frame_count;                    // 25
    GdkPixbuf *frames[ANIM_MAX_FRAMES]; // the 25 sliced pictures
};

static GHashTable *g_anim_cache = NULL; // "sheet path" -> SpriteAnim*  (shared by all fish)
```

Each fish (`aquarium.h`) just stores a **pointer** to the shared cache + where it's at:

```c
SpriteAnim *anim;     // shared, NOT owned by the fish (NULL = no animation)
int   anim_frame;     // which of the 25 it's showing
float anim_timer;     // seconds counted toward the next frame
```

> **Key point:** the heavy pixel data (the 25 frames) lives **once** in `g_anim_cache`.
> 300 dolphins share the same 25 frames — they only differ by their own `anim_frame`/`anim_timer`.

---

## 2. Slicing the sheet into 25 frames (`fish_catalog.c → sprite_anim_load`)

```c
GdkPixbuf *sheet = gdk_pixbuf_new_from_file(sheet_path, &error); // load the big 2048x2048 PNG
int sw = gdk_pixbuf_get_width(sheet);
int sh = gdk_pixbuf_get_height(sheet);
double fw = sw / 5.0;   // one frame's width
double fh = sh / 5.0;   // one frame's height

for (int i = 0; i < 25; ++i) {
    int col = i % 5, row = i / 5;           // grid position of frame i
    int x = col * fw, y = row * fh;         // top-left pixel of that cell
    GdkPixbuf *view  = gdk_pixbuf_new_subpixbuf(sheet, x, y, fw, fh);              // window into the sheet
    GdkPixbuf *frame = gdk_pixbuf_scale_simple(view, 220, 220, GDK_INTERP_BILINEAR); // own copy
    g_object_unref(view);
    anim->frames[anim->frame_count++] = frame;
}
g_object_unref(sheet); // big sheet no longer needed; we kept 25 small copies
```

- `gdk_pixbuf_new_subpixbuf` = a cheap reference to a **region** (no copy).
- `gdk_pixbuf_scale_simple` = makes an **independent** 220px copy, so we can throw the giant sheet away.

---

## 3. Finding/loading a sheet for a species (`sprite_anim_for_image`)

```c
char *base = base_name_no_ext(image_path);          // "FISH/dolphin.png" -> "dolphin"
const char *patterns[] = { "%s/%s_25_clean.png", "%s/%s_25.png", "%s/%s_sheet.png", NULL };
// probe SPRITES/ for dolphin_25_clean.png, dolphin_25.png, dolphin_sheet.png ...
if (!sheet_path[0]) return NULL;                     // no sheet -> NO animation (orca, shark)

SpriteAnim *anim = g_hash_table_lookup(g_anim_cache, sheet_path);
if (anim) return anim;                               // already loaded -> reuse
anim = sprite_anim_load(sheet_path);                 // first time -> slice it now
g_hash_table_insert(g_anim_cache, anim->key, anim);  // store in cache
return anim;
```

> This is **lazy**: a sheet is only loaded the first time a fish of that species is created,
> then cached forever.

---

## 4. Attaching to a fish (`aquarium.c → create_fish`)

```c
fish->anim = sprite_anim_for_image(image_path);
fish->anim_timer = 0.0f;
if (fish->anim) {
    fish->anim_frame = rand() % sprite_anim_frame_count(fish->anim); // random start (school isn't in sync)
    apply_fish_anim_frame(fish);
} else {
    fish->anim_frame = 0;
    load_fish_image(fish, image_path);   // static fallback (orca, shark, custom images)
}
```

---

## 5. Turning a frame into the fish's picture (`apply_fish_anim_frame`)

```c
GdkPixbuf *frame = sprite_anim_frame(fish->anim, fish->anim_frame); // the 220px cache frame
int tw = (int)fish->base_size;          // scale to this fish's size
int th = (int)(fish->base_size * 0.6f); // fish are wider than tall
GdkPixbuf *scaled = gdk_pixbuf_scale_simple(frame, tw, th, GDK_INTERP_BILINEAR);

if (fish->image_left)  g_object_unref(fish->image_left);   // free the previous frame's images
if (fish->image_right) g_object_unref(fish->image_right);
fish->image_right = scaled;                         // facing right
fish->image_left  = gdk_pixbuf_flip(scaled, TRUE);  // mirrored copy facing left
```

> So `image_right`/`image_left` always hold the **current** frame, scaled to that fish.

---

## 6. Advancing frames over time (`advance_fish_animation`, called every tick)

```c
float speed = sqrtf(fish->vx*fish->vx + fish->vy*fish->vy);
float fps = 8.0f + speed * 1.6f;   // faster swimming -> faster flapping
if (fps > 22.0f) fps = 22.0f;

fish->anim_timer += dt;
if (fish->anim_timer >= 1.0f / fps) {     // enough time passed for one frame?
    fish->anim_timer = 0.0f;
    fish->anim_frame = (fish->anim_frame + 1) % count;  // next frame, looping 0->24->0
    apply_fish_anim_frame(fish);                         // rebuild image_left/right
}
```

Called once per fish each simulation step (`update_simulation`):

```c
advance_fish_animation(current, dt);
```

---

## 7. Drawing (`draw_fish`) — unchanged

```c
GdkPixbuf *current_image = (fish->direction == DIRECTION_RIGHT) ? fish->image_right : fish->image_left;
// ... painted with the existing rotate/sway transforms
```

It just blits whatever `image_right`/`image_left` currently is. Because step 6 keeps swapping
that, you see animation.

---

## 8. Cleanup (`sprite_anim_cleanup`, called at shutdown from `cleanup_aquarium`)

```c
// frees every SpriteAnim in the cache and its 25 frames
```

Per-fish: only `image_left`/`image_right` are freed with the fish (the shared 25 frames are freed
once, by `sprite_anim_cleanup`).

---

## The pipeline at a glance

```
SPRITES/dolphin_25.png  (2048x2048, 5x5)
        |  sprite_anim_load()  -> slice + scale to 25 x 220px frames
        v
   SpriteAnim { frames[25] }  ---- cached in g_anim_cache (one per species, shared) ----+
        ^                                                                               |
        |  sprite_anim_for_image()  (lazy, name-based lookup)                           |
        |                                                                               |
   create_fish() -> fish->anim ------------------------------------------------------- +
        |
        v  every tick: advance_fish_animation()
   anim_timer += dt; if due -> anim_frame = (anim_frame+1) % 25; apply_fish_anim_frame()
        |
        v
   fish->image_right / image_left  (current frame, scaled to this fish)
        |
        v  draw_fish() blits it  ->  animation on screen
```

---

## Things worth knowing / gotchas

- **Memory model:** 25 frames per species exist **once** (shared). Each fish owns only its 2
  current scaled pixbufs. That's why it scales to 300 fish.
- **CPU cost:** the only per-frame work is `gdk_pixbuf_scale_simple` of one frame
  (~10–20×/sec per fish) inside `apply_fish_anim_frame`. Cheap, but it's the one spot that does
  real work during animation.
- **No sheet → no animation:** `sprite_anim_for_image` returns `NULL`, and the fish keeps its
  static image. That's the orca/shark case — completely automatic.
- **Naming convention is the glue:** a species animates only if `SPRITES/<name>_25.png`
  (or `_25_clean.png` / `_sheet.png`) exists, where `<name>` is the FISH image's base name.
  Drop a correctly-named sheet in `SPRITES/` and it animates with **zero code changes**.
- **Grid is hard-coded 5×5.** A sheet with a different layout (e.g. 4×4) would need
  `ANIM_GRID_COLS/ROWS` changed (or made per-sheet).
- **Frames are square-scaled to 220px** in the cache, then re-scaled per fish — so very large
  fish (up to 250px) may look slightly soft. Raise `ANIM_CACHE_FRAME_PX` for crisper big fish.
- **De-sync trick:** the random `anim_frame` start in `create_fish` keeps a school from flapping
  in lockstep.

---

## File / function map

| Part | File | Function |
|------|------|----------|
| Cache type + storage | `fish_catalog.c` | `struct SpriteAnim`, `g_anim_cache` |
| Slice sheet → frames | `fish_catalog.c` | `sprite_anim_load()` |
| Find sheet for species | `fish_catalog.c` | `sprite_anim_for_image()` |
| Frame accessors | `fish_catalog.c` | `sprite_anim_frame()`, `sprite_anim_frame_count()` |
| Free cache | `fish_catalog.c` | `sprite_anim_cleanup()` |
| Per-fish fields | `aquarium.h` | `Fish.anim`, `Fish.anim_frame`, `Fish.anim_timer` |
| Attach on creation | `aquarium.c` | `create_fish()` |
| Frame → fish image | `aquarium.c` | `apply_fish_anim_frame()` |
| Advance over time | `aquarium.c` | `advance_fish_animation()` (from `update_simulation`) |
| Draw current frame | `aquarium.c` | `draw_fish()` |
