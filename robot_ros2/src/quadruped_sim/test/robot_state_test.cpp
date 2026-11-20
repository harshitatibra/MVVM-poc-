#include "robot_state.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

void check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}

int main() {
  RobotState robot;
  check(robot.mode() == "SITTING" && robot.battery() == 100.0, "Initial state");
  check(robot.command("stand").accepted && robot.mode() == "STANDING", "Stand");
  check(!robot.command("walk").accepted && robot.mode() == "STANDING", "Reject unknown command");
  check(robot.command("sit").accepted && robot.mode() == "SITTING", "Sit");
  robot.advance(10.0);
  check(robot.battery() < 100.0 && robot.uptime() == 10.0, "Elapsed time");
  const double battery = robot.battery();
  check(robot.command("ping").accepted && robot.battery() == battery, "Ping is read only");
  robot.command("low_battery");
  check(robot.health() == "LOW BATTERY", "Warning state");
  robot.advance(100000.0);
  check(robot.battery() == 0.0 && robot.voltage() == 21.0, "Battery floor");
  check(robot.dummy() >= 0.0 && robot.dummy() <= 100.0, "Dummy bounds");
  robot.command("reset");
  check(robot.health() == "OK" && robot.battery() == 100.0, "Reset warning");
  for (double invalid : {-1.0, std::numeric_limits<double>::quiet_NaN()}) {
    bool rejected = false;
    try { robot.advance(invalid); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "Invalid elapsed time must be rejected");
  }
  std::cout << "PASS: state transitions, rejection, telemetry bounds, reset, invalid time\n";
}
