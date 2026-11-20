# Source this from Bash. All project paths are relative to THIS copied folder.
GCS_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
GCS_INTERFACES_WS="$GCS_ROOT/ros2/interfaces_ws"
POC_ROS_DISTRO="${POC_ROS_DISTRO:-${ROS_DISTRO:-jazzy}}"
POC_ROS_PREFIX="${POC_ROS_PREFIX:-/opt/ros/$POC_ROS_DISTRO}"
if [[ -n "${ROS_DISTRO:-}" && "$ROS_DISTRO" != "$POC_ROS_DISTRO" ]]; then
  echo "A different ROS distro is already sourced. Open a fresh terminal." >&2
  return 1
fi
if [[ ! -f "$POC_ROS_PREFIX/setup.bash" ]]; then
  echo "ROS setup not found: $POC_ROS_PREFIX/setup.bash. Install ROS 2 Jazzy first." >&2
  return 1
fi
source "$POC_ROS_PREFIX/setup.bash" || return 1
export ROS_DOMAIN_ID="${POC_DOMAIN_ID:-42}"
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
export ROS_AUTOMATIC_DISCOVERY_RANGE="${POC_DISCOVERY_RANGE:-SUBNET}"
unset ROS_LOCALHOST_ONLY
if [[ "${GCS_SOURCE_OVERLAY:-1}" == 1 && -f "$GCS_INTERFACES_WS/install/local_setup.bash" ]]; then
  source "$GCS_INTERFACES_WS/install/local_setup.bash" || return 1
fi
