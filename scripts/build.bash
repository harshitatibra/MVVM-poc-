#!/usr/bin/env bash
set -eo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/environment.bash"
cd "$POC_ROOT"
case "${1:-}" in
  robot)
    colcon build --symlink-install --packages-up-to quadruped_sim \
      --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
    ;;
  gcs|all)
    if [[ -z "${QT_ROOT:-}" || ! -f "$QT_ROOT/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
      echo 'Set QT_ROOT to your Qt 6.12 kit, e.g. $HOME/Qt/6.12.0/gcc_64.' >&2
      exit 1
    fi
    export PATH="$QT_ROOT/bin:$PATH"
    export CMAKE_PREFIX_PATH="$QT_ROOT:${CMAKE_PREFIX_PATH:-}"
    if [[ "$1" == "all" ]]; then packages=(quadruped_sim quadruped_gcs); else packages=(quadruped_gcs); fi
    colcon build --symlink-install --packages-up-to "${packages[@]}" \
      --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON \
      "-DQt6_ROOT=$QT_ROOT"
    ;;
  *) echo "Usage: bash scripts/build.bash robot|gcs|all" >&2; exit 1 ;;
esac
echo "Build complete. Source scripts/environment.bash again before running."
