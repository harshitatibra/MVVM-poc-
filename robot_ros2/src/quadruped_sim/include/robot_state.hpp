#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

struct CommandReply {
  bool accepted;
  std::string message;
  std::string mode;
};

// Independent of ROS so simulation behavior can be tested without middleware.
class RobotState {
public:
  void advance(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0)
      throw std::invalid_argument("Elapsed time must be finite and nonnegative");
    uptime_ += seconds;
    battery_ = std::max(0.0, battery_ - seconds * 0.02);
  }

  CommandReply command(const std::string &name) {
    if (name == "ping") return {true, "Pong from simulated quadruped", mode_};
    if (name == "stand" || name == "sit") {
      mode_ = name == "stand" ? "STANDING" : "SITTING";
      return {true, "Simulator is now " + mode_, mode_};
    }
    if (name == "low_battery") {
      battery_ = 15.0;
      return {true, "Simulated low battery enabled", mode_};
    }
    if (name == "reset") {
      battery_ = 100.0;
      mode_ = "SITTING";
      return {true, "Battery and posture reset", mode_};
    }
    return {false, "Unknown command: " + name, mode_};
  }

  double battery() const { return battery_; }
  double voltage() const { return 21.0 + battery_ * 0.042; }
  double temperature() const { return 45.0 + 5.0 * std::sin(uptime_ / 8.0); }
  double dummy() const { return 50.0 + 50.0 * std::sin(uptime_); }
  double uptime() const { return uptime_; }
  std::string health() const { return battery_ < 20.0 ? "LOW BATTERY" : "OK"; }
  const std::string &mode() const { return mode_; }

private:
  double battery_{100.0};
  double uptime_{0.0};
  std::string mode_{"SITTING"};
};
