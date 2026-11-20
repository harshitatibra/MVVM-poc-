# Source this file from Bash in EVERY terminal used for this PoC.
# Override POC_ROS_DISTRO before sourcing to select jazzy instead of kilted.
POC_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
POC_ROS_DISTRO="${POC_ROS_DISTRO:-${ROS_DISTRO:-kilted}}"
if [[ -n "${ROS_DISTRO:-}" && "$ROS_DISTRO" != "$POC_ROS_DISTRO" ]]; then
  echo "A different ROS distro is already sourced. Open a fresh terminal." >&2
  return 1
fi
if [[ ! -f "/opt/ros/$POC_ROS_DISTRO/setup.bash" ]]; then
  echo "Install ROS 2 $POC_ROS_DISTRO first; see README.md." >&2
  return 1
fi
source "/opt/ros/$POC_ROS_DISTRO/setup.bash"
export ROS_DOMAIN_ID="${POC_DOMAIN_ID:-42}"
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
export ROS_AUTOMATIC_DISCOVERY_RANGE="${POC_DISCOVERY_RANGE:-SUBNET}"
unset ROS_LOCALHOST_ONLY
if [[ -f "$POC_ROOT/install/setup.bash" ]]; then
  source "$POC_ROOT/install/setup.bash"
fi
