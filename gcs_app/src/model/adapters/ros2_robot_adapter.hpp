#pragma once
#include "model/i_robot_model.hpp"
#include <memory>

// Object adapter: implements the Model contract by composing a Ros2RobotNode.
// Keeps ROS types, request IDs, and executor ownership behind this boundary.
class Ros2RobotAdapter final : public IRobotModel {
  Q_OBJECT
public:
  explicit Ros2RobotAdapter(QObject *parent = nullptr);
  ~Ros2RobotAdapter() override;
  void start(); // Called by main after wiring the ViewModel; once per instance.
  bool sendCommand(quint64 id, const QString &command) override;
  void forgetRequest(quint64 id) override;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
