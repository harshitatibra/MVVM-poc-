#pragma once
#include "model/i_robot_model.hpp"
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class RobotViewModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Created by the C++ application")
  Q_PROPERTY(bool hasTelemetry READ hasTelemetry NOTIFY telemetryChanged)
  Q_PROPERTY(double batteryPercent READ batteryPercent NOTIFY telemetryChanged)
  Q_PROPERTY(double batteryVoltage READ batteryVoltage NOTIFY telemetryChanged)
  Q_PROPERTY(double cpuTemperature READ cpuTemperature NOTIFY telemetryChanged)
  Q_PROPERTY(QString health READ health NOTIFY telemetryChanged)
  Q_PROPERTY(QString mode READ mode NOTIFY telemetryChanged)
  Q_PROPERTY(double dummyValue READ dummyValue NOTIFY telemetryChanged)
  Q_PROPERTY(double uptimeSec READ uptimeSec NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong sequence READ sequence NOTIFY telemetryChanged)
  Q_PROPERTY(bool telemetryFresh READ telemetryFresh NOTIFY linkChanged)
  Q_PROPERTY(bool serviceAvailable READ serviceAvailable NOTIFY linkChanged)
  Q_PROPERTY(bool canSendCommand READ canSendCommand NOTIFY linkChanged)
  Q_PROPERTY(QString linkStatus READ linkStatus NOTIFY linkChanged)
  Q_PROPERTY(bool commandPending READ commandPending NOTIFY commandChanged)
  Q_PROPERTY(QString commandStatus READ commandStatus NOTIFY commandChanged)
  Q_PROPERTY(QStringList eventLog READ eventLog NOTIFY eventLogChanged)
public:
  explicit RobotViewModel(IRobotModel *model, QObject *parent = nullptr,
                          int staleMs = 2500, int commandTimeoutMs = 3000);
  ~RobotViewModel() override;
  bool hasTelemetry() const { return hasTelemetry_; }
  double batteryPercent() const { return data_.batteryPercent; }
  double batteryVoltage() const { return data_.batteryVoltage; }
  double cpuTemperature() const { return data_.cpuTemperature; }
  QString health() const { return hasTelemetry_ ? data_.health : "UNKNOWN"; }
  QString mode() const { return hasTelemetry_ ? data_.mode : "UNKNOWN"; }
  double dummyValue() const { return data_.dummyValue; }
  double uptimeSec() const { return data_.uptimeSec; }
  qulonglong sequence() const { return data_.sequence; }
  bool telemetryFresh() const { return fresh_; }
  bool serviceAvailable() const { return serviceAvailable_; }
  bool canSendCommand() const { return fresh_ && serviceAvailable_ && !pending_; }
  QString linkStatus() const;
  bool commandPending() const { return pending_; }
  QString commandStatus() const { return commandStatus_; }
  QStringList eventLog() const { return eventLog_; }
  Q_INVOKABLE void sendCommand(const QString &command);
Q_SIGNALS:
  void telemetryChanged();
  void linkChanged();
  void commandChanged();
  void eventLogChanged();
private:
  void receiveTelemetry(const TelemetrySnapshot &snapshot);
  void refreshLink();
  void finishCommand(const QString &status);
  void appendLog(const QString &message);
  IRobotModel *model_; // Injected Model contract; must outlive this ViewModel.
  TelemetrySnapshot data_;
  bool hasTelemetry_{false};
  bool fresh_{false};
  bool serviceAvailable_{false};
  bool pending_{false};
  quint64 requestId_{0};
  QString commandStatus_{"No command sent"};
  QStringList eventLog_;
  QTimer watchdog_;
  QTimer commandTimer_;
  int staleMs_;
  int commandTimeoutMs_;
};
