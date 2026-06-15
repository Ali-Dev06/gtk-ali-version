#!/bin/bash
# Wrapper script to run aquarium with clean environment, avoiding snap library conflicts

# Unset all snap-related variables that interfere with library loading
unset GTK_PATH
unset GTK_EXE_PREFIX
unset GTK_IM_MODULE_FILE
unset LOCPATH
unset GIO_MODULE_DIR
unset GSETTINGS_SCHEMA_DIR
unset XDG_DATA_HOME
unset SNAP

# Keep only essential environment variables
export LC_ALL=C.UTF-8
export DISPLAY="${DISPLAY:-:0}"

# Force the X11 (Xwayland) backend: WSLg's Wayland backend crashes the window
# with a protocol error (Wayland error 71) when it is maximized.
export GDK_BACKEND=x11

# Xwayland under WSLg has no default cursor theme, so the pointer goes invisible
# over the window. Point it at an installed theme.
export XCURSOR_THEME=Adwaita
export XCURSOR_SIZE=24

# Run the aquarium binary
exec "$(dirname "$0")/aquarium" "$@"
