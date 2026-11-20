#pragma once
#include <QMetaType>
#include <QString>
#include <chrono>

// Model data exchanged through IRobotModel, independent of ROS message types.
struct TelemetrySnapshot {
  double batteryPercent{0.0};
  double batteryVoltage{0.0};
  double cpuTemperature{0.0};
  QString health;
  QString mode;
  double dummyValue{0.0};
  double uptimeSec{0.0};
  quint64 sequence{0};
  std::chrono::steady_clock::time_point receivedAt;
};
Q_DECLARE_METATYPE(TelemetrySnapshot)
