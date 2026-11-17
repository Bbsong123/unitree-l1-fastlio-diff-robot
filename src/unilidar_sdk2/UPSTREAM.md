# SDK2 来源与本地集成差异

- 官方仓库：https://github.com/unitreerobotics/unilidar_sdk2
- 固定来源提交：`0e3c51f512e6b8ff60b8c32f160b412cb48445c2`，SDK 版本 `2.0.9`。
- 保留官方 SDK 头文件、x86_64/aarch64 静态库、示例和许可证；不引入 ROS2 工作空间。
- 将官方 `unitree_lidar_ros/src/unitree_lidar_ros/` 整理为当前 `unitree_lidar_ros/`，修正其 CMake 中到 SDK 的相对路径，补充节点和配置安装规则。
- 两处 CMake 改用项目要求的 C++14。官方预编译静态库未修改；NUC 上的 ROS/PCL 编译与库链接兼容性待验证。
- ROS1 驱动 `include/unitree_lidar_ros.h` 仅移除 `imu_initial -> imu` 的姿态 TF；消息解析、时间戳、点云转换及官方 IMU->雷达内部外参保持不变。配置将 `imu_frame` 命名为 `body`，由 FAST-LIO 发布其世界位姿。
- ROS1 `config/config.yaml` 改为本项目 UDP 配置，保留原点云和 IMU 话题名；系统网卡设置由用户自行完成。
- SDK 源码内部 `PointUnitree.ring` 是 uint32；官方 PCL/ROS 输出转换为 uint16，当前 FAST-LIO 消费的是后者，不直接读取 SDK 的 C++ 原始结构体。
