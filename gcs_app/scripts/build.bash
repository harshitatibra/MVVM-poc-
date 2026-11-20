#!/usr/bin/env bash
set -eo pipefail
gcs_script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ -z "${QT_ROOT:-}" || ! -f "$QT_ROOT/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
  echo 'Set QT_ROOT to your Qt 6.12 kit, e.g. $HOME/Qt/6.12.0/gcc_64.' >&2
  exit 1
fi
bash "$gcs_script_dir/prepare_ros.bash"
source "$gcs_script_dir/environment.bash"
export PATH="$QT_ROOT/bin:$PATH"
export CMAKE_PREFIX_PATH="$QT_ROOT:${CMAKE_PREFIX_PATH:-}"
cmake -S "$GCS_ROOT" -B "$GCS_ROOT/build" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON "-DQt6_ROOT=$QT_ROOT"
cmake --build "$GCS_ROOT/build" --parallel "${POC_BUILD_JOBS:-2}"
echo "Build complete. Run: bash $gcs_script_dir/run.bash"
