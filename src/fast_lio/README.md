# FAST-LIO for Unitree L1 LiDAR (ROS1 Noetic)

本算法包源自港大 HKU-Mars FAST-LIO2，经宇树官方 L1 移植与解耦优化：
1. **已完全移除 Livox 依赖**：编译不再需要 `Livox-SDK` 与 `livox_ros_driver`；
2. **支持逐点去畸变**：已内置 `lidar_type: 5` 与 `Preprocess::unilidar_handler`，利用 PointCloud2 每点 `time` 进行运动补偿；
3. **已修复 Launch XML 语法**：开箱即用，支持一键 `catkin_make` 编译。
