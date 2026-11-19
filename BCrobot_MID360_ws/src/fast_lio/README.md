# FAST-LIO 与 Livox MID360（ROS1 Noetic）

本包沿用 L1 工作区中的 HKU-Mars FAST-LIO2 实现。MID360 启动入口为
`mapping_mid360.launch`，读取 `config/mid360.yaml`。

`lidar_type: 1` 从 `/livox/lidar` 订阅 `livox_ros_driver2/CustomMsg`，从
`/livox/imu` 订阅 `sensor_msgs/Imu`。点坐标为米，逐点时间由纳秒换算为毫秒，
供原有 IMU 去畸变使用。建图节点发布 `/Odometry`、`/cloud_registered` 和
`camera_init → body`，其中 `body` 是 IMU 坐标系。历史 PointCloud2 输入分支仍在。

仅启动定位节点可用 `roslaunch fast_lio mapping_mid360.launch rviz:=false`。
驱动和底盘联调见工作区根目录的 README。
