#!/bin/bash
# Wrapper script to run aquarium with clean environment
# Unsets snap-related variables that cause libc conflicts

unset GTK_PATH GTK_EXE_PREFIX GTK_IM_MODULE_FILE LOCPATH GIO_MODULE_DIR \
      GSETTINGS_SCHEMA_DIR XDG_DATA_HOME SNAP SNAP_*

# Backend is chosen in code: prefer X11 (Xwayland) so the native maximize works,
# fall back to Wayland if Xwayland isn't running. (We must NOT force GDK_BACKEND=x11
# here, or the app would fail to start when Xwayland is down.)
export XCURSOR_THEME=Adwaita
export XCURSOR_SIZE=24

# Execute the binary
exec "$(dirname "$0")/aquarium" "$@"
