#include "robot_view_model.hpp"
#include <QDateTime>
#include <cmath>

RobotViewModel::RobotViewModel(IRobotModel *model, QObject *parent,
                             int staleMs, int commandTimeoutMs)
    : QObject(parent), model_(model), staleMs_(staleMs),
      commandTimeoutMs_(commandTimeoutMs) {
  qRegisterMetaType<TelemetrySnapshot>();
  // All ViewModel mutations happen on Qt's GUI thread, even for ROS callbacks.
  connect(model_, &IRobotModel::telemetryReceived, this,
          &RobotViewModel::receiveTelemetry, Qt::QueuedConnection);
  connect(model_, &IRobotModel::commandAvailabilityChanged, this,
    [this](bool available) {
      if (serviceAvailable_ == available) return;
      serviceAvailable_ = available;
      Q_EMIT linkChanged();
    }, Qt::QueuedConnection);
  connect(model_, &IRobotModel::commandFinished, this,
    [this](quint64 id, bool accepted, const QString &message) {
      if (!pending_ || id != requestId_) return; // Ignore late/old responses.
      finishCommand((accepted ? "Accepted: " : "Rejected: ") + message);
    }, Qt::QueuedConnection);
  connect(model_, &IRobotModel::connectionError, this,
    [this](const QString &message) { appendLog("Robot connection error: " + message); },
    Qt::QueuedConnection);
  watchdog_.setInterval(100);
  connect(&watchdog_, &QTimer::timeout, this, &RobotViewModel::refreshLink);
  watchdog_.start();
  commandTimer_.setSingleShot(true);
  connect(&commandTimer_, &QTimer::timeout, this, [this]() {
    // No automatic replay: a missing reply does not prove the command failed.
    finishCommand("Timed out: outcome unknown. Check telemetry before retrying.");
  });
  appendLog("Waiting for quadruped telemetry");
}

RobotViewModel::~RobotViewModel() {
  if (pending_) model_->forgetRequest(requestId_);
}

QString RobotViewModel::linkStatus() const {
  if (!hasTelemetry_) return "WAITING FOR ROBOT";
  if (!fresh_) return "TELEMETRY STALE";
  return serviceAvailable_ ? "ONLINE" : "TELEMETRY ONLY";
}

void RobotViewModel::receiveTelemetry(const TelemetrySnapshot &snapshot) {
  if (!std::isfinite(snapshot.batteryPercent) || snapshot.batteryPercent < 0.0 ||
      snapshot.batteryPercent > 100.0 || !std::isfinite(snapshot.batteryVoltage) ||
      !std::isfinite(snapshot.cpuTemperature) || !std::isfinite(snapshot.dummyValue) ||
      !std::isfinite(snapshot.uptimeSec) || snapshot.uptimeSec < 0.0) return;
  data_ = snapshot;
  hasTelemetry_ = true;
  Q_EMIT telemetryChanged();
  refreshLink();
}

void RobotViewModel::refreshLink() {
  const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - data_.receivedAt).count();
  const bool fresh = hasTelemetry_ && age >= 0 && age < staleMs_;
  if (fresh_ == fresh) return;
  fresh_ = fresh;
  appendLog(fresh ? "Telemetry connected" : "Telemetry stale; command buttons disabled");
  Q_EMIT linkChanged();
}

void RobotViewModel::sendCommand(const QString &command) {
  refreshLink();
  if (!canSendCommand()) {
    appendLog("Command blocked: need fresh telemetry, a service, and no pending request");
    return;
  }
  pending_ = true;
  ++requestId_;
  commandStatus_ = "Waiting for robot: " + command;
  Q_EMIT commandChanged();
  Q_EMIT linkChanged();
  appendLog("Sending: " + command);
  if (!model_->sendCommand(requestId_, command)) {
    finishCommand("Could not send: command service unavailable");
    return;
  }
  commandTimer_.start(commandTimeoutMs_);
}

void RobotViewModel::finishCommand(const QString &status) {
  if (!pending_) return;
  commandTimer_.stop();
  model_->forgetRequest(requestId_);
  pending_ = false;
  commandStatus_ = status;
  appendLog(status);
  Q_EMIT commandChanged();
  Q_EMIT linkChanged();
}

void RobotViewModel::appendLog(const QString &message) {
  eventLog_.prepend(QDateTime::currentDateTime().toString("HH:mm:ss") + "  " + message);
  while (eventLog_.size() > 50) eventLog_.removeLast();
  Q_EMIT eventLogChanged();
}
