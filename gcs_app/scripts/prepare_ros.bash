#!/usr/bin/env bash
set -eo pipefail
# Only build the interface package bundled with this GCS. No robot folder is used.
GCS_SOURCE_OVERLAY=0 source "$(dirname -- "${BASH_SOURCE[0]}")/environment.bash"
if ! command -v colcon >/dev/null; then
  echo 'Install python3-colcon-common-extensions before preparing the GCS.' >&2
  exit 1
fi
cd "$GCS_INTERFACES_WS"
colcon build --symlink-install --base-paths src --packages-select qt_robot_interfaces \
  --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF
echo "GCS ROS interfaces are ready in $GCS_INTERFACES_WS/install"
