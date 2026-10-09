cmake_minimum_required(VERSION 3.0.2)
project(robot_nav)

## 编译选项设置 (C++14 / O3 优化)
add_compile_options(-std=c++14 -O3 -Wall)

## 查找依赖包
find_package(catkin REQUIRED COMPONENTS
  roscpp
  geometry_msgs
  nav_msgs
  tf
)

## Catkin 导出设置
catkin_package(
  CATKIN_DEPENDS roscpp geometry_msgs nav_msgs tf
)

## 头文件路径
include_directories(
  ${catkin_INCLUDE_DIRS}
)

## 1. 底层底盘速度控制器与安全守护节点
add_executable(base_controller_node src/base_controller_node.cpp)
target_link_libraries(base_controller_node
  ${catkin_LIBRARIES}
)

## 2. 定点跟踪算法节点
add_executable(point_tracker_node src/point_tracker_node.cpp)
target_link_libraries(point_tracker_node
  ${catkin_LIBRARIES}
)

## 安装规则
install(TARGETS base_controller_node point_tracker_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY launch config scripts
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
