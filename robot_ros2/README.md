# Standalone ROS 2 robot workspace

Copy this entire `robot_ros2` folder to the robot laptop. It has no dependency on the GCS folder or Qt. It simulates a quadruped and communicates directly with the GCS through ROS 2/DDS.

## 1. Dependencies

For Ubuntu 24.04 with ROS 2 Jazzy already installed:

```bash
sudo apt update
sudo apt install build-essential cmake python3-colcon-common-extensions \
  ros-jazzy-rclcpp ros-jazzy-rclpy ros-jazzy-rmw-fastrtps-cpp \
  ros-jazzy-rosidl-default-generators ros-jazzy-ament-cmake-test
```

## 2. Build and run

Assuming the folder was copied to `~/robot_ros2`:

```bash
cd ~/robot_ros2
export POC_ROS_DISTRO=jazzy
bash scripts/build.bash
bash scripts/run.bash
```

Keep the terminal running. You should see:

```text
Publishing /quadruped/telemetry at 5 Hz; command service ready
```

On later runs, use `bash scripts/run.bash`. It sources both ROS and this workspace automatically. To use ROS CLI tools yourself, initialize each new terminal:

```bash
cd ~/robot_ros2
export POC_ROS_DISTRO=jazzy
source scripts/environment.bash
ros2 node list
```

You can also start the simulator directly after sourcing: `ros2 run quadruped_sim simulator`.

## 3. Folder structure

```text
robot_ros2/
  src/
    qt_robot_interfaces/
      msg/Telemetry.msg       Battery, health, temperature, posture, dummy data
      srv/RobotCommand.srv    Command request and acceptance response
      CMakeLists.txt
      package.xml
    quadruped_sim/
      include/robot_state.hpp Simulation state and command behavior
      src/main.cpp            ROS publisher, service, and 5 Hz timer
      test/                   Simulation behavior tests
      CMakeLists.txt
      package.xml
  scripts/
    environment.bash          Sources Jazzy and sets DDS discovery
    build.bash                Builds this workspace with colcon
    run.bash                  Starts the simulator
    check_ros.py              Tests ROS telemetry and command/state feedback
```

The `qt_robot_interfaces` package name is retained for compatibility. It contains ROS message definitions and does not require Qt. A matching source copy is included in the GCS's own folder. Update both copies when changing the wire contract.

## 4. ROS interfaces

| Name | Kind | Type |
| --- | --- | --- |
| `/quadruped/telemetry` | Topic, 5 Hz | `qt_robot_interfaces/msg/Telemetry` |
| `/quadruped/command` | Service | `qt_robot_interfaces/srv/RobotCommand` |

Telemetry QoS is reliable, volatile, and keep-last-1. The GCS uses the same QoS. Commands use the standard ROS service QoS.

| Command | Effect |
| --- | --- |
| `ping` | Returns Pong |
| `stand` | Posture becomes STANDING |
| `sit` | Posture becomes SITTING |
| `low_battery` | Battery becomes 15%; health reports LOW BATTERY |
| `reset` | Battery becomes 100%; posture becomes SITTING |
| Other values | Rejected without changing state |

Battery percentage is 0..100. Uptime and sequence advance continuously; reset changes battery/posture only. All data and commands are simulated.

## 5. Wi-Fi settings

Both laptops must use ROS 2 Jazzy and matching settings. These scripts default to domain 42, `rmw_fastrtps_cpp`, and subnet discovery. Ensure router client isolation is disabled and peer DDS traffic is allowed by the network/firewall.

On the robot, after sourcing the environment:

```bash
ros2 multicast receive
```

Run `ros2 multicast send` on the GCS laptop from its sourced environment. Stop the receive command with Ctrl+C before reusing that terminal. Repeat in the other direction if needed.

If multicast is blocked but unicast is allowed, configure the GCS's real Wi-Fi IP before launching the robot:

```bash
export POC_DISCOVERY_RANGE=LOCALHOST
export ROS_STATIC_PEERS='192.168.1.11'  # Example: replace with GCS laptop IP.
bash scripts/run.bash
```

Set the robot's IP as a static peer on the GCS side as well. Restart both programs. These settings do not bypass router client isolation or blocked UDP traffic.

## 6. Verification

With the simulator running, use another terminal:

```bash
cd ~/robot_ros2
source scripts/environment.bash
python3 scripts/check_ros.py
```

This checks local ROS communication and command effects. Cross-laptop communication is verified by receiving telemetry and commands in the GCS.

Run the simulation unit test after building:

```bash
source scripts/environment.bash
colcon test --packages-select quadruped_sim --event-handlers console_direct+
colcon test-result --verbose
```

The simulation logic test previously passed on Ubuntu and Windows. A full ROS build and two-laptop test still require the installed ROS/Qt SDKs on the target laptops.

Transfer source files only. `build/`, `install/`, and `log/` are generated on the destination laptop and are excluded from the standalone ZIP.
