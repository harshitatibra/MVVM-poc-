#include "ros2_robot_node.hpp"
#include <chrono>
#include <utility>

Ros2RobotNode::Ros2RobotNode(Callbacks callbacks)
    : rclcpp::Node("quadruped_gcs", "/quadruped"), callbacks_(std::move(callbacks)) {
  const auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().durability_volatile();
  subscription_ = create_subscription<Telemetry>("telemetry", qos,
    [this](Telemetry::ConstSharedPtr msg) { callbacks_.telemetry(*msg); });
  client_ = create_client<Command>("command");
  availabilityTimer_ = create_wall_timer(std::chrono::milliseconds(500),
    [this]() { callbacks_.commandAvailability(commandAvailable()); });
}

bool Ros2RobotNode::commandAvailable() const {
  return client_->service_is_ready();
}

std::int64_t Ros2RobotNode::sendCommand(std::uint64_t operationId,
                                     const std::string &command) {
  auto request = std::make_shared<Command::Request>();
  request->command = command;
  const auto pending = client_->async_send_request(request,
    [this, operationId](rclcpp::Client<Command>::SharedFuture future) {
      try {
        const auto reply = future.get(); // Already ready inside this callback.
        callbacks_.commandReply(operationId, *reply);
      } catch (const std::exception &e) {
        callbacks_.error(e.what());
      }
    });
  return pending.request_id;
}

void Ros2RobotNode::forgetRequest(std::int64_t requestId) {
  client_->remove_pending_request(requestId);
}

void Ros2RobotNode::clearPendingRequests() {
  client_->prune_pending_requests();
}
