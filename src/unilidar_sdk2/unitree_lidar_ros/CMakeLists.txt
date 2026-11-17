cmake_minimum_required(VERSION 2.8.3)
project(unitree_lidar_ros)

set(CMAKE_BUILD_TYPE "Release")
# 本项目使用 C++14；官方 SDK2 头文件接口保留原样，NUC 上仍需实际编译链接验证。
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -llz4")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -DNDEBUG -llz4") 

find_package(PCL REQUIRED QUIET)

find_package(catkin REQUIRED COMPONENTS
  tf
  roscpp
  rospy
  rosparam
  pcl_conversions
  std_msgs
  sensor_msgs
  geometry_msgs
  nav_msgs
  message_generation
  visualization_msgs
  roslib
)

catkin_package(
  INCLUDE_DIRS include
  DEPENDS PCL

  CATKIN_DEPENDS
  rosparam
  std_msgs
  nav_msgs
  geometry_msgs
  sensor_msgs
  message_runtime
  message_generation
  visualization_msgs
)

include_directories(
	include
	${catkin_INCLUDE_DIRS}
	${PCL_INCLUDE_DIRS}

  ${CMAKE_CURRENT_SOURCE_DIR}/../unitree_lidar_sdk/include
)

link_directories(
  lib
  ${PCL_LIBRARY_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../unitree_lidar_sdk/lib/${CMAKE_SYSTEM_PROCESSOR}
)

set(EXTRA_LIBS 
  ${catkin_LIBRARIES} 
  ${PCL_LIBRARIES} 
  libunilidar_sdk2.a
)

add_executable(unitree_lidar_ros_node src/unitree_lidar_ros_node.cpp)
target_link_libraries(unitree_lidar_ros_node ${EXTRA_LIBS})

# 保留官方驱动入口，同时让 catkin 安装空间能找到配置和启动文件。
install(TARGETS unitree_lidar_ros_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
install(DIRECTORY launch config rviz
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
