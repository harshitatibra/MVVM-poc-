#include "ros2_robot_adapter.hpp"
#include "ros2_robot_node.hpp"
#include <atomic>
#include <chrono>
#include <map>
#include <thread>
#include <utility>

struct Ros2RobotAdapter::Impl {
  std::shared_ptr<Ros2RobotNode> node;
  rclcpp::executors::SingleThreadedExecutor executor;
  std::atomic<bool> running{false};
  std::thread thread;
  // Qt-side operation ID -> ROS request ID. Accessed only on the GUI thread.
  std::map<quint64, std::int64_t> pending;
};

Ros2RobotAdapter::Ros2RobotAdapter(QObject *parent)
    : IRobotModel(parent), impl_(std::make_unique<Impl>()) {
  qRegisterMetaType<TelemetrySnapshot>();
  Ros2RobotNode::Callbacks callbacks;
  callbacks.telemetry = [this](const Ros2RobotNode::Telemetry &msg) {
    // Translate the ROS wire type into the application's Model data.
    TelemetrySnapshot data;
    data.receivedAt = std::chrono::steady_clock::now();
    data.batteryPercent = msg.battery_percent;
    data.batteryVoltage = msg.battery_voltage;
    data.cpuTemperature = msg.cpu_temperature_c;
    data.health = QString::fromStdString(msg.health);
    data.mode = QString::fromStdString(msg.mode);
    data.dummyValue = msg.dummy_value;
    data.uptimeSec = msg.uptime_sec;
    data.sequence = msg.sequence;
    Q_EMIT telemetryReceived(data);
  };
  callbacks.commandAvailability = [this](bool available) {
    Q_EMIT commandAvailabilityChanged(available);
  };
  callbacks.commandReply = [this](std::uint64_t id, const Ros2RobotNode::Command::Response &reply) {
    Q_EMIT commandFinished(id, reply.accepted, QString::fromStdString(reply.message));
  };
  callbacks.error = [this](const std::string &message) {
    Q_EMIT connectionError(QString::fromStdString(message));
  };
  impl_->node = std::make_shared<Ros2RobotNode>(std::move(callbacks));
  impl_->executor.add_node(impl_->node);
}

void Ros2RobotAdapter::start() {
  // A completed std::thread remains joinable; never overwrite it on repeat calls.
  if (impl_->thread.joinable()) return;
  impl_->running = true;
  impl_->thread = std::thread([this]() {
    try {
      // Bounded wait also handles closing the UI before the worker starts.
      while (impl_->running && rclcpp::ok())
        impl_->executor.spin_once(std::chrono::milliseconds(100));
    } catch (const std::exception &e) {
      Q_EMIT connectionError(QString::fromUtf8(e.what()));
    }
    impl_->running = false;
    Q_EMIT commandAvailabilityChanged(false);
  });
}

Ros2RobotAdapter::~Ros2RobotAdapter() {
  impl_->running = false;
  impl_->executor.cancel();
  if (impl_->thread.joinable()) impl_->thread.join();
  impl_->node->clearPendingRequests();
  impl_->executor.remove_node(impl_->node);
}

bool Ros2RobotAdapter::sendCommand(quint64 id, const QString &command) {
  if (!impl_->running || !rclcpp::ok()) return false;
  try {
    if (!impl_->node->commandAvailable()) return false;
    const auto requestId = impl_->node->sendCommand(id, command.toStdString());
    impl_->pending.emplace(id, requestId);
    return true;
  } catch (const std::exception &e) {
    Q_EMIT connectionError(QString::fromUtf8(e.what()));
    return false;
  }
}

void Ros2RobotAdapter::forgetRequest(quint64 id) {
  const auto it = impl_->pending.find(id);
  if (it == impl_->pending.end()) return;
  impl_->node->forgetRequest(it->second);
  impl_->pending.erase(it);
}
