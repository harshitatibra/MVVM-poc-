# Standalone Qt Creator GCS

Copy this **entire `gcs_app` folder** to the GCS laptop. It builds without the robot folder. Keep the included `ros2/` directory: it contains the GCS node and the interface definitions needed for native DDS communication.

The application uses Qt 6.12, QML, MVVM, and a Model-layer adapter. It is a standard Qt/CMake project. ROS 2 Jazzy remains a dependency because communication uses native ROS 2/DDS. No network bridge is used.

## 1. Prerequisites on Ubuntu 24.04

ROS 2 Jazzy and a Qt 6.12 Desktop GCC kit are already assumed installed. The Qt kit must include Qt QML, Quick, Quick Controls, and Qt Test.

```bash
sudo apt update
sudo apt install git build-essential cmake python3-colcon-common-extensions \
  ros-jazzy-rclcpp ros-jazzy-rmw-fastrtps-cpp \
  ros-jazzy-rosidl-default-generators \
  libgl1-mesa-dev libxkbcommon-dev libxkbcommon-x11-0 \
  libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
  libxcb-render-util0 libxcb-xinerama0 libxcb-xkb1
```

## 2. Open, build, and run in Qt Creator

Assuming you copied the folder to `~/gcs_app`:

```bash
cd ~/gcs_app
export POC_ROS_DISTRO=jazzy
bash scripts/open_qtcreator.bash
```

The script builds the small interface package in `ros2/interfaces_ws`, sources its generated environment, and launches Qt Creator with this folder's `CMakeLists.txt`. This initial interface build needs colcon; the GCS application itself is built by Qt Creator/CMake.

If the launcher cannot find Qt Creator, set its executable explicitly:

```bash
export QT_CREATOR_BIN="$HOME/Qt/Tools/QtCreator/bin/qtcreator"
bash scripts/open_qtcreator.bash
```

Change the path if your installer used a different location. Start a fresh Qt Creator process from this script so the IDE inherits the ROS environment.

In Qt Creator:

1. Choose your **Qt 6.12 Desktop GCC** kit and click **Configure Project**.
2. Use a build directory within this copied folder, for example `~/gcs_app/build-qtcreator`.
3. Build the project.
4. Select **gcs** as the run target, rather than `view_model_test`.
5. Press **Run**. The dashboard starts even if the robot is offline; it shows `WAITING FOR ROBOT` until telemetry arrives.
6. Run the robot simulator on the other laptop. The dashboard should discover it and show `ONLINE`.

You can subsequently open `CMakeLists.txt` from an IDE that already has the sourced environment. Launching an unsourced IDE from the desktop may fail to find `rclcpp`, the generated interfaces, or their runtime libraries. See the official [Qt Creator CMake guidance](https://doc.qt.io/qtcreator/creator-project-cmake.html).

## 3. Optional command-line build

This is an alternative to building in Qt Creator:

```bash
cd ~/gcs_app
export POC_ROS_DISTRO=jazzy
export QT_ROOT="$HOME/Qt/6.12.0/gcc_64"  # Your actual Qt kit path.
bash scripts/build.bash
bash scripts/run.bash
```

`QT_ROOT` is only needed for this command-line build; Qt Creator uses its selected kit. `scripts/run.bash` runs the executable in `build/`, so use Qt Creator's Run button if you built only in `build-qtcreator/`.

The GCS is now a normal executable, not a `ros2 run quadruped_gcs gcs` package. Its internal ROS node still participates in the ROS graph as `/quadruped/quadruped_gcs`.

## 4. Folder structure and adapter pattern

```text
gcs_app/
  CMakeLists.txt                 Open this project in Qt Creator
  qml/                          View: Main.qml and MetricCard.qml
  src/
    main.cpp                    Creates and connects the application objects
    viewmodel/                  QML properties, commands, timeouts, event log
    model/
      i_robot_model.hpp         Target interface used by the ViewModel
      telemetry_snapshot.hpp    Model data independent of ROS message types
      adapters/                 Ros2RobotAdapter implements IRobotModel
  ros2/
    ros2_robot_node.*           Native ROS adaptee; no Qt dependencies
    interfaces_ws/src/
      qt_robot_interfaces/      Local message/service definitions
  scripts/                      Environment, interface build, IDE launcher, CLI build/run
  test/                         ViewModel tests using a fake Model
```

Telemetry flows from `Ros2RobotNode` through `Ros2RobotAdapter` into the ViewModel and QML. Commands flow back through the same abstraction. The adapter owns the ROS executor worker and translates ROS messages into `TelemetrySnapshot`. The ViewModel depends on `IRobotModel`; it does not include ROS headers or the concrete adapter. Queued Qt signals deliver updates on the UI thread.

The interface package is bundled here so this folder can be copied independently. If you change `.msg` or `.srv` definitions, distribute the same changes to both laptops and rebuild both sides. The complete repository has a `tools/check_layout.py` utility to check that the copies match.

## 5. Wi-Fi and discovery

The launcher and environment script default to:

```text
ROS_DOMAIN_ID=42
RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ROS_AUTOMATIC_DISCOVERY_RANGE=SUBNET
```

Use the same settings on the robot laptop. The router must allow client-to-client traffic and DDS discovery. No robot IP is entered in the GCS for normal multicast discovery.

For CLI diagnostics, first run:

```bash
cd ~/gcs_app
export POC_ROS_DISTRO=jazzy
source scripts/environment.bash
ros2 topic echo /quadruped/telemetry --once
ros2 service call /quadruped/command qt_robot_interfaces/srv/RobotCommand "{command: 'ping'}"
```

If multicast fails but unicast works, set the robot's real Wi-Fi IP before starting the GCS:

```bash
export POC_DISCOVERY_RANGE=LOCALHOST
export ROS_STATIC_PEERS='192.168.1.10'  # Example: replace with robot laptop IP.
bash scripts/open_qtcreator.bash
```

Configure the robot similarly with the GCS laptop's IP, and restart both applications. This is native DDS static-peer discovery, documented in the [ROS discovery guide](https://docs.ros.org/en/jazzy/Tutorials/Advanced/Improved-Dynamic-Discovery.html).

## 6. Tests

After a command-line build:

```bash
source scripts/environment.bash
ctest --test-dir build --output-on-failure
```

For a Qt Creator build, substitute its build directory for `build`. ViewModel tests cover telemetry updates, stale data, command acceptance/rejection, timeouts, and late responses.

With the robot running, the real application's smoke check loads QML, receives telemetry, sends Ping, and expects an accepted response:

```bash
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  bash scripts/run.bash --smoke-test
```

Use the visible UI for the demo: Stand, Sit, Ping, Low battery, and Reset. Stopping the robot should mark telemetry stale after about 2.5 seconds and disable commands. An unanswered command times out after 3 seconds with an unknown outcome; it is not automatically retried.

## Moving the folder

Transfer source files, then build on the destination laptop. `build/`, `build-*/`, `.qt/`, `ros2/interfaces_ws/build/`, `install/`, `log/`, and Qt Creator `.user` files are generated and machine/path-specific. The provided ZIP excludes them. Reconfigure and rebuild after moving an already-built project; copying a built executable alone is not a deployment package.

Full Qt/ROS compilation and the two-laptop Wi-Fi test have not been run in the authoring environment because the SDKs are absent there. Folder isolation, setup-script behavior, interface consistency, and source references are checked separately.
