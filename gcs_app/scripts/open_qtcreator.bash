#!/usr/bin/env bash
set -eo pipefail
gcs_script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
bash "$gcs_script_dir/prepare_ros.bash"
source "$gcs_script_dir/environment.bash"

# Qt Creator inherits ROS include/library/runtime paths from this process.
gcs_creator="${QT_CREATOR_BIN:-}"
if [[ -z "$gcs_creator" ]]; then
  gcs_creator="$(command -v qtcreator || true)"
fi
if [[ -z "$gcs_creator" && -x "$HOME/Qt/Tools/QtCreator/bin/qtcreator" ]]; then
  gcs_creator="$HOME/Qt/Tools/QtCreator/bin/qtcreator"
fi
if [[ -z "$gcs_creator" ]] || ! command -v "$gcs_creator" >/dev/null; then
  echo 'Set QT_CREATOR_BIN to the Qt Creator executable, then run this script again.' >&2
  exit 1
fi
exec "$gcs_creator" "$GCS_ROOT/CMakeLists.txt"
