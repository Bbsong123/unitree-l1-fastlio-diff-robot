cmake_minimum_required(VERSION 3.0.2)
project(robot_nav)

## 编译选项设置 (统一 C++14)
add_compile_options(-std=c++14 -O3 -Wall)

## 查找依赖包
find_package(catkin REQUIRED COMPONENTS
  roscpp
  geometry_msgs
  nav_msgs
  std_msgs
  tf
)

## Catkin 导出设置
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ${PROJECT_NAME}_lib
  CATKIN_DEPENDS roscpp geometry_msgs nav_msgs std_msgs tf
)

## 头文件路径
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

## 声明算法与控制库
add_library(${PROJECT_NAME}_lib
  src/base_controller.cpp
  src/point_navigator.cpp
)
target_link_libraries(${PROJECT_NAME}_lib
  ${catkin_LIBRARIES}
)

## 构建核心导航节点
add_executable(robot_nav_node src/robot_nav_node.cpp)
target_link_libraries(robot_nav_node
  ${PROJECT_NAME}_lib
  ${catkin_LIBRARIES}
)

## 安装规则
install(TARGETS robot_nav_node ${PROJECT_NAME}_lib
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
)

install(DIRECTORY launch config
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
