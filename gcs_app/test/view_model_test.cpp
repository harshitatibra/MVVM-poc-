#include "viewmodel/robot_view_model.hpp"
#include <QtTest>
#include <limits>
#include <thread>

class FakeRobotModel : public IRobotModel {
public:
  bool sendsSucceed{true};
  int sendCount{0};
  quint64 sentId{0};
  quint64 forgottenId{0};
  bool sendCommand(quint64 id, const QString &) override {
    ++sendCount;
    sentId = id;
    return sendsSucceed;
  }
  void forgetRequest(quint64 id) override { forgottenId = id; }
  void telemetry(double battery = 75) {
    TelemetrySnapshot snapshot;
    snapshot.batteryPercent = battery;
    snapshot.health = "OK";
    snapshot.mode = "SITTING";
    snapshot.receivedAt = std::chrono::steady_clock::now();
    Q_EMIT telemetryReceived(snapshot);
  }
};

class ViewModelTest : public QObject {
  Q_OBJECT
private Q_SLOTS:
  void telemetryCrossesThreadsAndBecomesStale() {
    FakeRobotModel model;
    RobotViewModel vm(&model, nullptr, 300, 500);
    QVERIFY(!vm.canSendCommand());
    std::thread worker([&]() {
      model.telemetry();
      Q_EMIT model.commandAvailabilityChanged(true);
    });
    worker.join();
    QTRY_VERIFY(vm.canSendCommand());
    QCOMPARE(vm.batteryPercent(), 75.0);
    QTRY_VERIFY_WITH_TIMEOUT(!vm.telemetryFresh(), 1500);
    QVERIFY(!vm.canSendCommand());
    model.telemetry(80.0);
    QTRY_VERIFY(vm.canSendCommand());
    QCOMPARE(vm.batteryPercent(), 80.0);
  }
  void acknowledgementAndRejection() {
    FakeRobotModel model;
    RobotViewModel vm(&model);
    model.telemetry();
    Q_EMIT model.commandAvailabilityChanged(true);
    QTRY_VERIFY(vm.canSendCommand());
    vm.sendCommand("stand");
    QVERIFY(vm.commandPending());
    vm.sendCommand("sit");
    QCOMPARE(model.sendCount, 1); // At most one request in flight.
    Q_EMIT model.commandFinished(model.sentId, true, "Standing");
    QTRY_VERIFY(!vm.commandPending());
    QVERIFY(vm.commandStatus().startsWith("Accepted:"));
    QCOMPARE(model.forgottenId, model.sentId);
    vm.sendCommand("invalid");
    Q_EMIT model.commandFinished(model.sentId, false, "Unknown command");
    QTRY_VERIFY(!vm.commandPending());
    QVERIFY(vm.commandStatus().startsWith("Rejected:"));
  }
  void timeoutDiscardsLateRepliesWithoutRetry() {
    FakeRobotModel model;
    RobotViewModel vm(&model, nullptr, 5000, 100);
    model.telemetry();
    Q_EMIT model.commandAvailabilityChanged(true);
    QTRY_VERIFY(vm.canSendCommand());
    vm.sendCommand("stand");
    const auto firstId = model.sentId;
    QTRY_VERIFY(!vm.commandPending());
    QVERIFY(vm.commandStatus().contains("outcome unknown"));
    QCOMPARE(model.sendCount, 1);
    QCOMPARE(model.forgottenId, firstId);
    vm.sendCommand("sit");
    Q_EMIT model.commandFinished(firstId, true, "Late stand reply");
    Q_EMIT model.commandFinished(model.sentId, true, "Sitting");
    QTRY_VERIFY(!vm.commandPending());
    QCOMPARE(vm.commandStatus(), QString("Accepted: Sitting"));
  }
  void invalidTelemetryDoesNotEstablishConnection() {
    FakeRobotModel model;
    RobotViewModel vm(&model);
    model.telemetry(std::numeric_limits<double>::quiet_NaN());
    Q_EMIT model.commandAvailabilityChanged(true);
    QTRY_VERIFY(vm.serviceAvailable()); // Earlier queued telemetry was processed.
    QVERIFY(!vm.hasTelemetry());
    QVERIFY(!vm.canSendCommand());
  }
  void unavailableServiceAndSendFailure() {
    FakeRobotModel model;
    RobotViewModel vm(&model);
    model.telemetry();
    QTRY_VERIFY(vm.telemetryFresh());
    vm.sendCommand("ping");
    QCOMPARE(model.sendCount, 0);
    Q_EMIT model.commandAvailabilityChanged(true);
    QTRY_VERIFY(vm.canSendCommand());
    model.sendsSucceed = false;
    vm.sendCommand("ping");
    QVERIFY(!vm.commandPending());
    QVERIFY(vm.commandStatus().startsWith("Could not send:"));
  }
};
QTEST_GUILESS_MAIN(ViewModelTest)
#include "view_model_test.moc"
