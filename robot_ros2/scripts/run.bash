#!/usr/bin/env bash
set -eo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/environment.bash"
if [[ ! -f "$ROBOT_ROOT/install/quadruped_sim/lib/quadruped_sim/simulator" ]]; then
  echo 'Build this copied workspace first: bash scripts/build.bash' >&2
  exit 1
fi
exec ros2 run quadruped_sim simulator "$@"
