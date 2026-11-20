#include "robot_view_model.hpp"
#include "model/ros2/ros2_robot_adapter.hpp"
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QVariant>
#include <iostream>
#include <vector>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char **argv) {
  try {
    rclcpp::init(argc, argv);
    // ROS consumes its own flags; pass only application arguments to Qt.
    const auto arguments = rclcpp::remove_ros_arguments(argc, argv);
    std::vector<QByteArray> storage;
    for (const auto &argument : arguments) storage.push_back(QByteArray::fromStdString(argument));
    std::vector<char *> qtArgv;
    for (auto &argument : storage) qtArgv.push_back(argument.data());
    qtArgv.push_back(nullptr);
    int qtArgc = static_cast<int>(storage.size());
    QGuiApplication app(qtArgc, qtArgv.data());
    QCoreApplication::setApplicationName("Quadruped GCS");
    QQuickStyle::setStyle("Basic");
    int result = 1;
    {
      // Composition root: choose the adapter here, outside the ViewModel.
      Ros2RobotAdapter robotModel;
      RobotViewModel viewModel(&robotModel);
      QQmlApplicationEngine engine;
      engine.setInitialProperties({{"viewModel", QVariant::fromValue(&viewModel)}});
      engine.loadFromModule("Quadruped.Gcs", "Main");
      if (engine.rootObjects().isEmpty()) {
        rclcpp::shutdown();
        return 1;
      }
      QTimer shutdownPoll;
      QObject::connect(&shutdownPoll, &QTimer::timeout, &app, [&app]() {
        if (!rclcpp::ok()) app.quit();
      });
      shutdownPoll.start(200);
      // Optional end-to-end check: load QML, receive telemetry, send Ping,
      // and exit successfully only after the robot's service response.
      QTimer smokePoll;
      QElapsedTimer smokeDeadline;
      bool smokeSent = false;
      if (app.arguments().contains("--smoke-test")) {
        smokeDeadline.start();
        QObject::connect(&smokePoll, &QTimer::timeout, &app, [&]() {
          if (smokeDeadline.elapsed() > 15000) {
            std::cerr << "FAIL: no telemetry/command round trip within 15 seconds\n";
            app.exit(2);
          } else if (!smokeSent && viewModel.canSendCommand()) {
            smokeSent = true;
            viewModel.sendCommand("ping");
          }
        });
        QObject::connect(&viewModel, &RobotViewModel::commandChanged, &app, [&]() {
          if (!smokeSent || viewModel.commandPending()) return;
          const bool passed = viewModel.commandStatus().startsWith("Accepted:");
          std::cout << (passed ? "PASS: " : "FAIL: ")
                    << viewModel.commandStatus().toStdString() << '\n';
          app.exit(passed ? 0 : 3);
        });
        smokePoll.start(100);
      }
      robotModel.start();
      result = app.exec();
    } // Destroy QML, ViewModel, then stop/join the ROS worker before shutdown.
    rclcpp::shutdown();
    return result;
  } catch (const std::exception &e) {
    std::cerr << "GCS startup failed: " << e.what() << '\n';
    if (rclcpp::ok()) rclcpp::shutdown();
    return 1;
  }
}
