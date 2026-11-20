#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <rclcpp/rclcpp.hpp>
#include <qt_robot_interfaces/msg/telemetry.hpp>
#include <qt_robot_interfaces/srv/robot_command.hpp>

// Adaptee: native ROS API. This class has no Qt or ViewModel dependencies.
// Only Ros2RobotAdapter uses it; it is not part of the public Model interface.
class Ros2RobotNode final : public rclcpp::Node {
public:
  using Telemetry = qt_robot_interfaces::msg::Telemetry;
  using Command = qt_robot_interfaces::srv::RobotCommand;
  struct Callbacks {
    std::function<void(const Telemetry &)> telemetry;
    std::function<void(bool)> commandAvailability;
    std::function<void(std::uint64_t, const Command::Response &)> commandReply;
    std::function<void(const std::string &)> error;
  };

  explicit Ros2RobotNode(Callbacks callbacks);
  bool commandAvailable() const;
  std::int64_t sendCommand(std::uint64_t operationId, const std::string &command);
  void forgetRequest(std::int64_t requestId);
  void clearPendingRequests();

private:
  Callbacks callbacks_;
  rclcpp::Subscription<Telemetry>::SharedPtr subscription_;
  rclcpp::Client<Command>::SharedPtr client_;
  rclcpp::TimerBase::SharedPtr availabilityTimer_;
};
