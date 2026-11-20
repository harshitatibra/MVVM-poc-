#include "robot_state.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <qt_robot_interfaces/msg/telemetry.hpp>
#include <qt_robot_interfaces/srv/robot_command.hpp>

class QuadrupedSimulator : public rclcpp::Node {
public:
  QuadrupedSimulator() : Node("quadruped_simulator", "/quadruped") {
    using Telemetry = qt_robot_interfaces::msg::Telemetry;
    using Command = qt_robot_interfaces::srv::RobotCommand;
    // Matching QoS on the GCS. Keep the newest sample; don't replay old telemetry.
    auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().durability_volatile();
    publisher_ = create_publisher<Telemetry>("telemetry", qos);
    service_ = create_service<Command>("command",
      [this](const std::shared_ptr<Command::Request> request,
             std::shared_ptr<Command::Response> response) {
        const auto result = state_.command(request->command);
        response->accepted = result.accepted;
        response->message = result.message;
        response->mode = result.mode;
        RCLCPP_INFO(get_logger(), "Command '%s': %s", request->command.c_str(),
                    result.message.c_str());
      });
    previousTick_ = std::chrono::steady_clock::now();
    timer_ = create_wall_timer(std::chrono::milliseconds(200), [this]() {
      const auto current = std::chrono::steady_clock::now();
      state_.advance(std::chrono::duration<double>(current - previousTick_).count());
      previousTick_ = current;
      Telemetry msg;
      msg.stamp = now();
      msg.sequence = ++sequence_;
      msg.battery_percent = static_cast<float>(state_.battery());
      msg.battery_voltage = static_cast<float>(state_.voltage());
      msg.cpu_temperature_c = static_cast<float>(state_.temperature());
      msg.health = state_.health();
      msg.mode = state_.mode();
      msg.dummy_value = static_cast<float>(state_.dummy());
      msg.uptime_sec = state_.uptime();
      publisher_->publish(msg);
    });
    RCLCPP_INFO(get_logger(), "Publishing /quadruped/telemetry at 5 Hz; command service ready");
  }
private:
  RobotState state_;
  std::uint64_t sequence_{0};
  std::chrono::steady_clock::time_point previousTick_;
  rclcpp::Publisher<qt_robot_interfaces::msg::Telemetry>::SharedPtr publisher_;
  rclcpp::Service<qt_robot_interfaces::srv::RobotCommand>::SharedPtr service_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<QuadrupedSimulator>());
  rclcpp::shutdown();
  return 0;
}
