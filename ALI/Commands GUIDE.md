# 🐠 Aquarium Simulator — Commands Guide

Everything happens inside the **`ALI`** folder.

---

## 0. Go to the project folder
```bash
cd ~/gtk-proj/ALI
```

## 1. Build (compile) the project
```bash
make
```
Run this **every time you change a `.c` or `.h` file**. It recompiles only what changed and links the `aquarium` binary.

For a clean, full rebuild:
```bash
make clean    # delete all .o files and the binary
make          # build from scratch
```

## 2. Run the aquarium
```bash
./run_aquarium.sh
```
👉 This is your **usual launch command**. It strips out the snap environment variables that break GTK, sets the cursor theme, then starts the app.

Alternative launcher:
```bash
./run_aquarium_clean.sh
```

If you get *"permission denied"*, make them runnable once:
```bash
chmod +x run_aquarium.sh run_aquarium_clean.sh
```

## 3. ⭐ Edit → Run cycle (use this during the exam)
After you change `aquarium.c` (or any `.c` / `.h` file), run these **two commands**:
```bash
make
./run_aquarium.sh
```
- **`make`** → recompiles your changed code and rebuilds the app.
- **`./run_aquarium.sh`** → launches the new version.

Or chain them so the app only runs **if the build succeeded**:
```bash
make && ./run_aquarium.sh
```
The `&&` means *"only launch if `make` had no errors"* — if compilation fails, the app won't start and you'll know to fix the code first.

If you're not already in the folder:
```bash
cd ~/gtk-proj/ALI
make && ./run_aquarium.sh
```

## 4. Difference between the two run scripts
They are ~95% identical, but:

| | `run_aquarium.sh` *(the one you use)* | `run_aquarium_clean.sh` |
|---|---|---|
| Unsets snap variables | Yes | Yes — **plus** `SNAP_*` (wildcard) |
| Sets `LC_ALL=C.UTF-8` | ✅ Yes | ❌ No |
| Sets `DISPLAY=:0` (fallback) | ✅ Yes | ❌ No |
| Sets cursor theme/size | Yes | Yes |

**Why `run_aquarium.sh` is the better default:**
- `LC_ALL=C.UTF-8` → forces a UTF-8 locale, so **emojis and special characters render correctly** (your emoji button icons 🐠➕).
- `DISPLAY=:0` → if `DISPLAY` isn't set, it falls back to `:0` so the app knows which screen to draw on.

The "clean" one only wins if you hit a stubborn snap-library conflict that its extra `SNAP_*` cleanup fixes.

## 5. Fixing `cannot open display :0`
Run this from **Windows PowerShell/CMD** (NOT inside WSL), only when WSLg has crashed and no window will open:
```powershell
wsl --shutdown
```
Then reopen your WSL terminal and run the app again.

---

## ⚡ Quick Reference

| Command | What it does |
|---|---|
| `cd ~/gtk-proj/ALI` | Go to the project |
| `make` | Compile after any code change |
| `make clean` | Delete build files (fresh rebuild) |
| `./run_aquarium.sh` | **Run the app** (usual command) |
| `make && ./run_aquarium.sh` | Build, then run only if build succeeded |
| `chmod +x *.sh` | Make scripts runnable (one-time) |
| `wsl --shutdown` *(in Windows)* | Fix "cannot open display" |

> Day-to-day you only need two: **`make`** then **`./run_aquarium.sh`**.
