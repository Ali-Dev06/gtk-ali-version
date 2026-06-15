#!/bin/bash
# Wrapper script to run aquarium with clean environment
# Unsets snap-related variables that cause libc conflicts

unset GTK_PATH GTK_EXE_PREFIX GTK_IM_MODULE_FILE LOCPATH GIO_MODULE_DIR \
      GSETTINGS_SCHEMA_DIR XDG_DATA_HOME SNAP SNAP_*

# Force the X11 (Xwayland) backend: WSLg's Wayland backend crashes the window
# with a protocol error (Wayland error 71) when it is maximized.
export GDK_BACKEND=x11

# Xwayland under WSLg has no default cursor theme, so the pointer goes invisible
# over the window. Point it at an installed theme.
export XCURSOR_THEME=Adwaita
export XCURSOR_SIZE=24

# Execute the binary
exec "$(dirname "$0")/aquarium" "$@"
