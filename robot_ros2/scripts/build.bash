#!/usr/bin/env bash
set -eo pipefail
ROBOT_SOURCE_OVERLAY=0 source "$(dirname -- "${BASH_SOURCE[0]}")/environment.bash"
cd "$ROBOT_ROOT"
colcon build --symlink-install --base-paths src --packages-up-to quadruped_sim \
  --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
echo 'Build complete. Run: bash scripts/run.bash'
