#!/bin/bash
# Wrapper script to run aquarium with clean environment
# Unsets snap-related variables that cause libc conflicts

unset GTK_PATH GTK_EXE_PREFIX GTK_IM_MODULE_FILE LOCPATH GIO_MODULE_DIR \
      GSETTINGS_SCHEMA_DIR XDG_DATA_HOME SNAP SNAP_*

# Execute the binary
exec "$(dirname "$0")/aquarium" "$@"
