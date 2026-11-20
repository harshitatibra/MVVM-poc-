#!/usr/bin/env python3
"""Check that the two deliverables have matching contracts and local source paths."""
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]


def check_layout():
    gcs = ROOT / "gcs_app"
    robot = ROOT / "robot_ros2"
    contract_copies = [
        robot / "src/qt_robot_interfaces",
        gcs / "ros2/interfaces_ws/src/qt_robot_interfaces",
    ]
    contract_files = ["CMakeLists.txt", "package.xml", "msg/Telemetry.msg", "srv/RobotCommand.srv"]
    for name in contract_files:
        if (contract_copies[0] / name).read_bytes() != (contract_copies[1] / name).read_bytes():
            raise RuntimeError(f"Interface copies differ: {name}. Update both before distributing.")

    if (gcs / "package.xml").exists():
        raise RuntimeError("The GCS root must be a standalone Qt/CMake project")
    cmake = (gcs / "CMakeLists.txt").read_text()
    if "ament_package(" in cmake or "find_package(ament_cmake" in cmake:
        raise RuntimeError("The GCS application must not be built as an ament package")
    for name in re.findall(r"(?:src|test|qml|ros2)/[A-Za-z0-9_./-]+\.(?:cpp|hpp|qml)", cmake):
        if not (gcs / name).is_file():
            raise RuntimeError(f"Missing GCS CMake input: {name}")

    for file in list((gcs / "src").rglob("*.cpp")) + list((gcs / "src").rglob("*.hpp")) + \
                list((gcs / "ros2").glob("*.cpp")) + list((gcs / "ros2").glob("*.hpp")) + \
                list((gcs / "test").glob("*.cpp")):
        for include in re.findall(r'#include\s+"([^"]+)"', file.read_text()):
            if include.endswith(".moc"):
                continue
            roots = [file.parent, gcs / "src", gcs / "src/viewmodel", gcs / "ros2"]
            if not any((base / include).is_file() for base in roots):
                raise RuntimeError(f"Unresolved local include {include} in {file.relative_to(ROOT)}")

    boundary = list((gcs / "src/viewmodel").glob("*")) + [
        gcs / "src/model/i_robot_model.hpp", gcs / "src/model/telemetry_snapshot.hpp"]
    for file in boundary:
        if re.search(r"#include.*(?:rclcpp|qt_robot_interfaces|ros2_robot)", file.read_text()):
            raise RuntimeError(f"ROS dependency leaked into {file.relative_to(ROOT)}")
    for file in (gcs / "ros2").glob("ros2_robot_node.*"):
        if re.search(r'#include.*[<"]Q|Q_OBJECT|Q_EMIT|QObject|QString', file.read_text()):
            raise RuntimeError(f"Qt dependency leaked into {file.relative_to(ROOT)}")

    for project, forbidden in [(gcs, "robot_ros2"), (robot, "gcs_app")]:
        for directory in [project / "scripts"]:
            for file in directory.glob("*.bash"):
                if forbidden in file.read_text():
                    raise RuntimeError(f"Cross-folder dependency in {file.relative_to(ROOT)}")
    print("PASS: matching interfaces, local CMake/includes, MVVM boundaries, independent scripts")


if __name__ == "__main__":
    check_layout()
