cmake_minimum_required(VERSION 3.21)
project(quadruped_gcs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(qt_robot_interfaces REQUIRED)
find_package(Threads REQUIRED)
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.12)
qt_add_executable(gcs src/main.cpp
  src/model/ros2/ros2_robot_adapter.cpp src/model/ros2/ros2_robot_adapter.hpp
  src/model/ros2/ros2_robot_node.cpp src/model/ros2/ros2_robot_node.hpp)
qt_add_qml_module(gcs
  URI Quadruped.Gcs
  VERSION 1.0
  QML_FILES Main.qml MetricCard.qml
  SOURCES src/robot_view_model.hpp src/robot_view_model.cpp
          src/model/telemetry_snapshot.hpp src/model/i_robot_model.hpp)
target_include_directories(gcs PRIVATE src)
target_compile_definitions(gcs PRIVATE QT_NO_KEYWORDS)
target_link_libraries(gcs PRIVATE
  Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
  rclcpp::rclcpp ${qt_robot_interfaces_TARGETS} Threads::Threads)
install(TARGETS gcs RUNTIME DESTINATION lib/${PROJECT_NAME})
if(BUILD_TESTING)
  find_package(ament_cmake_test REQUIRED)
  find_package(Qt6 6.12 REQUIRED COMPONENTS Test)
  qt_add_executable(view_model_test test/view_model_test.cpp
    src/robot_view_model.cpp src/robot_view_model.hpp
    src/model/i_robot_model.hpp src/model/telemetry_snapshot.hpp)
  target_include_directories(view_model_test PRIVATE src)
  target_compile_definitions(view_model_test PRIVATE QT_NO_KEYWORDS)
  target_link_libraries(view_model_test PRIVATE Qt6::Core Qt6::Qml Qt6::Test Threads::Threads)
  ament_add_test(view_model_test COMMAND "$<TARGET_FILE:view_model_test>" TIMEOUT 30)
endif()
ament_package()
