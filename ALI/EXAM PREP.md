# 🎓 Exam Prep — Adding Buttons & Features

Quick, copy-paste reference for adding any button + signal function in the Aquarium project.

---

## PART 1 — The 3-Step Recipe (works for ANY button)

### Step 1 — Write the callback
Put it **above `create_aquarium_window`** in `aquarium.c`. Signature is always the same:
```c
static void on_my_action(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;          // silence "unused" warnings
    // ... do the work here ...
    update_ui_info();                               // refresh stats text
    gtk_widget_queue_draw(g_aquarium->drawing_area); // redraw the tank
    show_message("✅ Done!");                        // status-bar feedback
}
```

### Step 2 — Create the button + connect it
```c
GtkWidget *my_btn = gtk_button_new_with_label("My Button");
g_signal_connect(my_btn, "clicked", G_CALLBACK(on_my_action), NULL);
gtk_box_pack_start(GTK_BOX(PLACE), my_btn, FALSE, FALSE, 0);
```
Replace `PLACE` with **where** you want it (see Part 3).

### Step 3 — Build & run
```bash
make && ./run_aquarium.sh
```

> **Passing data into a callback:** use the last argument.
> `g_signal_connect(btn, "clicked", G_CALLBACK(on_x), GINT_TO_POINTER(3));`
> then inside: `int n = GPOINTER_TO_INT(data);`

---

## PART 2 — Engine Functions You Reuse (the toolbox)

| Need | Function |
|---|---|
| Make a fish | `create_fish(species, image_path, x, y, size, speed, type, diet, gender, facing)` |
| Add it to the tank | `add_fish(fish)` |
| Add N of a species **as a school w/ leader** | `spawn_fish_from_preset(preset, count, 0, 0, preset->type, 0)` |
| Get a species preset | `preset_fish_get(index)` · total: `preset_fish_count()` |
| New group (1st fish = leader) | `create_group("Name", leader_fish)` |
| Add fish to a group | `add_fish_to_group(group, fish)` |
| Remove fish from its group | `remove_fish_from_group(fish)` |
| Kill / remove a fish | `fish_die(fish)` or `remove_fish(fish)` |
| Count living fish | `count_alive_fish()` |
| Drop food pellets | `spawn_food_pellets(count, x, y)` |
| The selected fish | `g_aquarium->selected_fish` |
| Loop all fish | `for (Fish *f = g_aquarium->fish_list; f; f = f->next)` |
| Loop all groups | `for (Group *g = g_aquarium->group_list; g; g = g->next)` |

**Preset indices:** 0=Clownfish 1=Blue 2=Green 3=Puffer 5=Jellyfish 8=Dolphin 10=Shark
*(verify with the table in `fish_catalog.c` if unsure)*

---

## PART 3 — WHERE to put the button

### A) In the SIDEBAR
Find the sidebar block (~line 3112, after `quick_btn`) and add:
```c
GtkWidget *my_btn = gtk_button_new_with_label("My Button");
g_signal_connect(my_btn, "clicked", G_CALLBACK(on_my_action), NULL);
gtk_box_pack_start(GTK_BOX(sidebar), my_btn, FALSE, FALSE, 0);
```

### B) In the TOOLBAR
The toolbar buttons are created ~line 3144 (`add_btn`, `delete_btn`, …) and packed into a flow box. Add yours next to them the same way they are packed (copy the pattern of the line below the existing `gtk_..._pack...`/`gtk_container_add` used for the toolbar container).

### C) Keyboard shortcut (no button)
In `on_key_press` add a case:
```c
if (event->keyval == GDK_KEY_f) {   // press 'f'
    on_my_action(NULL, NULL);
    return TRUE;
}
```

---

## PART 4 — Ready-Made Features (copy, rename, done)

### 1. Add N fish of a species as a school (the classic)
```c
static void on_add_five_clownfish(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    const PresetFish *p = preset_fish_get(0);      // 0 = Clownfish
    spawn_fish_from_preset(p, 5, 0, 0, p->type, 0); // 5 fish, one leader, one school
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
    show_message("➕ Added 5 Clownfish as a school");
}
```
Button: `gtk_button_new_with_label("Add 5 Clownfish")` → connect → pack.

### 2. Add ONE fish of a species
```c
static void on_add_one_shark(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    const PresetFish *p = preset_fish_get(10);     // 10 = Shark
    spawn_fish_from_preset(p, 1, 0, 0, p->type, 0);
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
    show_message("➕ Added a Shark");
}
```

### 3. Feed all fish (drop food across the tank)
```c
static void on_feed_all(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    for (int i = 0; i < 12; i++) {
        float x = rand() % g_aquarium->width;
        float y = rand() % (g_aquarium->height / 2);  // near the top
        spawn_food_pellets(1, x, y);
    }
    show_message("🍽️ Fed all fish");
}
```

### 4. Delete the selected fish
```c
static void on_kill_selected(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    if (!g_aquarium->selected_fish) { show_message("⚠️ No fish selected"); return; }
    fish_die(g_aquarium->selected_fish);   // shows the death pop effect too
    g_aquarium->selected_fish = NULL;
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
    show_message("🗑 Fish removed");
}
```

### 5. Delete ALL fish of one species
```c
static void on_delete_all_clownfish(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    int n = 0;
    for (Fish *f = g_aquarium->fish_list; f; f = f->next) {
        if (f->is_alive && g_strcmp0(f->species, "Clownfish") == 0) {
            fish_die(f); n++;            // safe: only marks dead, swept end of frame
        }
    }
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
    char m[64]; g_snprintf(m, sizeof(m), "🗑 Removed %d Clownfish", n);
    show_message(m);
}
```

### 6. Count fish of a species (show a stat)
```c
static void on_count_clownfish(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    int n = 0;
    for (Fish *f = g_aquarium->fish_list; f; f = f->next)
        if (f->is_alive && g_strcmp0(f->species, "Clownfish") == 0) n++;
    char m[64]; g_snprintf(m, sizeof(m), "🐠 Clownfish in tank: %d", n);
    show_message(m);
}
```

### 7. Pause / resume the simulation
```c
static void on_toggle_pause(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    g_aquarium->paused = !g_aquarium->paused;
    show_message(g_aquarium->paused ? "⏸ Paused" : "▶️ Running");
}
```

### 8. Put EVERY fish into one big group
```c
static void on_group_all(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    Fish *leader = NULL;
    Group *all = NULL;
    for (Fish *f = g_aquarium->fish_list; f; f = f->next) {
        if (!f->is_alive) continue;
        if (!leader) { leader = f; all = create_group("All Fish", leader); }
        else if (all) add_fish_to_group(all, f);
    }
    update_ui_info();
    gtk_widget_queue_draw(g_aquarium->drawing_area);
    show_message("👥 Grouped all fish together");
}
```

### 9. Speed up / slow down all fish
```c
static void on_speed_up_all(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    for (Fish *f = g_aquarium->fish_list; f; f = f->next) {
        f->base_speed *= 1.25f;        // use 0.8f to slow down
        f->speed = f->base_speed;
    }
    show_message("⚡ Fish sped up");
}
```

### 10. Heal all fish (reset health/hunger)
```c
static void on_heal_all(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    for (Fish *f = g_aquarium->fish_list; f; f = f->next) {
        if (!f->is_alive) continue;
        f->health = 100.0f; f->hunger = 0.0f; f->energy = 100.0f;
    }
    show_message("💚 All fish healed");
}
```

---

## PART 5 — Exam Survival Checklist

1. **Callback** above `create_aquarium_window`, marked `static`, signature `(GtkWidget *btn, gpointer data)`.
2. **`(void)btn; (void)data;`** at the top to avoid warnings.
3. **Create + connect + pack** the button (3 lines) in the sidebar/toolbar.
4. Always end an action with **`update_ui_info();`** and **`gtk_widget_queue_draw(g_aquarium->drawing_area);`** if you changed fish/visuals.
5. **`make && ./run_aquarium.sh`** — if it doesn't rebuild, run `make clean && make`.
6. Modifying fish in a loop? Use **`fish_die(f)`** (marks dead, swept safely) — never `free()` mid-loop.
7. Compare species with **`g_strcmp0(f->species, "Name") == 0`**.
