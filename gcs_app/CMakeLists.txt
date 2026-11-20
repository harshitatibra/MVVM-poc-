cmake_minimum_required(VERSION 3.21)
project(quadruped_gcs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
find_package(rclcpp QUIET)
if(NOT rclcpp_FOUND)
  message(FATAL_ERROR
    "Native DDS requires ROS 2 Jazzy. Start Qt Creator with bash scripts/open_qtcreator.bash "
    "so it inherits the ROS environment.")
endif()
find_package(qt_robot_interfaces QUIET)
if(NOT qt_robot_interfaces_FOUND)
  message(FATAL_ERROR
    "The GCS-local ROS interfaces are not prepared. Run bash scripts/open_qtcreator.bash "
    "from gcs_app, or run bash scripts/prepare_ros.bash and source scripts/environment.bash.")
endif()
find_package(Threads REQUIRED)
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.12)
# This is an ordinary Qt/CMake project, not an ament/colcon application package.
qt_add_executable(gcs src/main.cpp
  src/model/adapters/ros2_robot_adapter.cpp src/model/adapters/ros2_robot_adapter.hpp
  ros2/ros2_robot_node.cpp ros2/ros2_robot_node.hpp)
# Keep the module's public QML type names stable after moving the files to qml/.
set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/MetricCard.qml PROPERTIES QT_RESOURCE_ALIAS MetricCard.qml)
qt_add_qml_module(gcs
  URI Quadruped.Gcs
  VERSION 1.0
  QML_FILES qml/Main.qml qml/MetricCard.qml
  SOURCES src/viewmodel/robot_view_model.hpp src/viewmodel/robot_view_model.cpp
          src/model/telemetry_snapshot.hpp src/model/i_robot_model.hpp)
target_include_directories(gcs PRIVATE src src/viewmodel ros2)
target_compile_definitions(gcs PRIVATE QT_NO_KEYWORDS)
target_link_libraries(gcs PRIVATE
  Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
  rclcpp::rclcpp ${qt_robot_interfaces_TARGETS} Threads::Threads)
install(TARGETS gcs RUNTIME DESTINATION bin)
include(CTest)
if(BUILD_TESTING)
  find_package(Qt6 6.12 REQUIRED COMPONENTS Test)
  qt_add_executable(view_model_test test/view_model_test.cpp
    src/viewmodel/robot_view_model.cpp src/viewmodel/robot_view_model.hpp
    src/model/i_robot_model.hpp src/model/telemetry_snapshot.hpp)
  target_include_directories(view_model_test PRIVATE src)
  target_compile_definitions(view_model_test PRIVATE QT_NO_KEYWORDS)
  target_link_libraries(view_model_test PRIVATE Qt6::Core Qt6::Qml Qt6::Test Threads::Threads)
  add_test(NAME view_model_test COMMAND view_model_test)
  set_tests_properties(view_model_test PROPERTIES TIMEOUT 30)
endif()
