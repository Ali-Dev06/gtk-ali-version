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

# Run the aquarium binary
exec "$(dirname "$0")/aquarium" "$@"
