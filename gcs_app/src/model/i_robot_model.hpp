#pragma once
#include "telemetry_snapshot.hpp"
#include <QObject>

// Adapter pattern target interface in the Model layer.
// The ViewModel knows this contract, never a ROS node or generated ROS type.
// Call methods on the Qt GUI thread; receive signals with queued connections.
class IRobotModel : public QObject {
  Q_OBJECT
public:
  using QObject::QObject;
  ~IRobotModel() override = default;
  virtual bool sendCommand(quint64 id, const QString &command) = 0;
  // Forget a local pending response. This does NOT cancel work at the robot.
  virtual void forgetRequest(quint64 id) = 0;
Q_SIGNALS:
  void telemetryReceived(TelemetrySnapshot snapshot);
  void commandAvailabilityChanged(bool available);
  void commandFinished(quint64 id, bool accepted, QString message);
  void connectionError(QString message);
};
