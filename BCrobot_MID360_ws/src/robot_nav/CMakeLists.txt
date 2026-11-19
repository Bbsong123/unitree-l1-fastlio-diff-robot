cmake_minimum_required(VERSION 3.0.2)
project(robot_nav)

# 项目统一使用 C++14，不使用 C++17 接口。
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 此包只需要 ROS C++ 接口和 Twist 速度消息。
find_package(catkin REQUIRED COMPONENTS roscpp geometry_msgs)
catkin_package(CATKIN_DEPENDS roscpp geometry_msgs)
include_directories(${catkin_INCLUDE_DIRS})

# C++ 只编译速度转发节点，上层控制按需运行 Python 脚本。
add_executable(base_controller_node src/base_controller_node.cpp)
target_link_libraries(base_controller_node ${catkin_LIBRARIES})

catkin_install_python(PROGRAMS scripts/keyboard.py scripts/pose_goal.py scripts/goal_console.py
  DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

# 保留基本安装规则，便于在 NUC 上使用 catkin 的运行/安装目录。
install(TARGETS base_controller_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
install(DIRECTORY launch config
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
