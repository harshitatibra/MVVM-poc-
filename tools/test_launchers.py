#!/usr/bin/env python3
"""Test copied-folder launch/build wiring with fake tools; does not compile ROS/Qt."""
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FAKE_TOOL = '''#!/usr/bin/env python3
import json, os, pathlib, shlex, sys
name = pathlib.Path(sys.argv[0]).name
with open(os.environ["FAKE_TOOL_LOG"], "a") as log:
    log.write(json.dumps({"tool": name, "args": sys.argv[1:], "cwd": os.getcwd(),
        "distro": os.environ.get("ROS_DISTRO"), "domain": os.environ.get("ROS_DOMAIN_ID"),
        "rmw": os.environ.get("RMW_IMPLEMENTATION"),
        "overlay": os.environ.get("TEST_OVERLAY_PATH")}) + "\\n")
if name == "colcon":
    if os.environ.get("FAIL_INTERFACE_BUILD"):
        sys.exit(17)
    install = pathlib.Path.cwd() / "install"
    install.mkdir(parents=True, exist_ok=True)
    (install / "local_setup.bash").write_text(
        "export TEST_OVERLAY_PATH=" + shlex.quote(str(install)) + "\\n")
    if "quadruped_sim" in sys.argv:
        program = install / "quadruped_sim/lib/quadruped_sim/simulator"
        program.parent.mkdir(parents=True, exist_ok=True)
        program.touch()
elif name == "cmake" and "--build" in sys.argv:
    build = pathlib.Path(sys.argv[sys.argv.index("--build") + 1])
    build.mkdir(parents=True, exist_ok=True)
    program = build / "gcs"
    program.write_text(pathlib.Path(__file__).read_text())
    program.chmod(0o755)
'''


@unittest.skipUnless(shutil.which("bash"), "Bash required")
class PortableLaunchersTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="poc launch test ")
        self.area = Path(self.temp.name)
        self.log = self.area / "tool calls.jsonl"
        fake_bin = self.area / "fake tools"
        fake_bin.mkdir()
        for name in ("colcon", "qtcreator", "cmake", "ros2"):
            tool = fake_bin / name
            tool.write_text(FAKE_TOOL)
            tool.chmod(0o755)
        fake_ros = self.area / "ROS installation"
        fake_ros.mkdir()
        (fake_ros / "setup.bash").write_text("export ROS_DISTRO=jazzy\n")
        fake_qt = self.area / "Qt installation"
        (fake_qt / "lib/cmake/Qt6").mkdir(parents=True)
        (fake_qt / "lib/cmake/Qt6/Qt6Config.cmake").touch()
        self.env = {key: value for key, value in os.environ.items()
                    if not key.startswith(("ROS_", "RMW_", "POC_", "GCS_", "ROBOT_"))}
        self.env.update(PATH=str(fake_bin) + os.pathsep + self.env["PATH"],
                        POC_ROS_DISTRO="jazzy", POC_ROS_PREFIX=str(fake_ros),
                        QT_ROOT=str(fake_qt), QT_CREATOR_BIN=str(fake_bin / "qtcreator"),
                        FAKE_TOOL_LOG=str(self.log))

    def tearDown(self):
        self.temp.cleanup()

    def copy_project(self, name):
        # Copy just one deliverable, under a new path containing spaces.
        target = self.area / ("copied " + name)
        shutil.copytree(ROOT / name, target,
                        ignore=shutil.ignore_patterns("build", "build-*", "install", "log",
                                                     "__pycache__", ".qt", "*.user"))
        return target

    def run_script(self, project, script, *args, expected=0):
        result = subprocess.run(["bash", str(project / "scripts" / script), *args],
                                cwd=self.area, env=self.env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def test_qtcreator_gets_local_interfaces_and_environment(self):
        project = self.copy_project("gcs_app")
        self.run_script(project, "open_qtcreator.bash")
        colcon, creator = self.calls()
        interfaces = project / "ros2/interfaces_ws"
        self.assertEqual(colcon["cwd"], str(interfaces))
        self.assertEqual(creator["args"], [str(project / "CMakeLists.txt")])
        self.assertEqual(creator["overlay"], str(interfaces / "install"))
        self.assertEqual(creator["distro"], "jazzy")
        self.assertEqual(creator["domain"], "42")
        self.assertEqual(creator["rmw"], "rmw_fastrtps_cpp")

    def test_gcs_cli_build_and_run_use_only_copied_folder(self):
        project = self.copy_project("gcs_app")
        self.run_script(project, "build.bash")
        self.run_script(project, "run.bash", "--smoke-test")
        calls = self.calls()
        configure = next(call for call in calls if call["tool"] == "cmake" and "-S" in call["args"])
        self.assertEqual(configure["args"][1], str(project))
        self.assertEqual(configure["args"][3], str(project / "build"))
        self.assertEqual(calls[-1]["tool"], "gcs")
        self.assertEqual(calls[-1]["args"], ["--smoke-test"])
        self.assertEqual(calls[-1]["overlay"], str(project / "ros2/interfaces_ws/install"))

    def test_robot_build_and_run_use_only_copied_folder(self):
        project = self.copy_project("robot_ros2")
        self.run_script(project, "build.bash")
        self.run_script(project, "run.bash", "--ros-args", "--log-level", "debug")
        colcon, ros2 = self.calls()
        self.assertEqual(colcon["cwd"], str(project))
        self.assertEqual(ros2["overlay"], str(project / "install"))
        self.assertEqual(ros2["args"], ["run", "quadruped_sim", "simulator", "--ros-args",
                                        "--log-level", "debug"])
        self.assertEqual(ros2["domain"], "42")

    def test_failed_interfaces_do_not_launch_qtcreator(self):
        project = self.copy_project("gcs_app")
        self.env["FAIL_INTERFACE_BUILD"] = "1"
        self.run_script(project, "open_qtcreator.bash", expected=17)
        self.assertEqual([call["tool"] for call in self.calls()], ["colcon"])

    def test_mixed_ros_distributions_fail_before_any_build(self):
        project = self.copy_project("gcs_app")
        self.env["ROS_DISTRO"] = "humble"
        result = self.run_script(project, "open_qtcreator.bash", expected=1)
        self.assertIn("different ROS distro", result.stderr)
        self.assertEqual(self.calls(), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
