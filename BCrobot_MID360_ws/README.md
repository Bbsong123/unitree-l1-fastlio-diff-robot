# BCrobot MID360 工作区

从 L1 部署源码 `b067bed` 派生，面向 Ubuntu 20.04 / ROS1 Noetic。用 U 盘把整个
`BCrobot_MID360_ws` 拷到 NUC 家目录，包括 `src/` 和 `vendor/`。建议拷到 NUC 的
Linux 文件系统后再编译。

## 1. 编译

在只加载 Noetic 的新终端执行。如果终端配置自动加载旧机器人工作区，
先停用该自动加载，再开新终端。

```bash
cd ~/BCrobot_MID360_ws
source /opt/ros/noetic/setup.bash
# 新环境缺依赖时执行。已装齐依赖的 NUC 可直接 catkin_make。
rosdep install --from-paths src --ignore-src -r -y
catkin_make -j2
source devel/setup.bash
```

SDK2 源码随工作区提供，由 catkin 编译并静态链接到 Livox ROS 驱动。驱动已带
ROS1 的 `package.xml`，直接用 `catkin_make`。所需系统依赖来自 Noetic/Ubuntu，
例如底盘的 `ros-noetic-serial`、驱动的 `libapr1-dev`。NUC 已装齐依赖时可以离线编译。
每个运行终端均加载 Noetic 和本工作区的 `devel/setup.bash`。

## 2. 配置网口和安装位置

- 官方示例 JSON 位于 `src/livox_ros_driver2/config/MID360_config.json`。
  示例使用 NUC 有线网口 `192.168.1.5/24`、雷达 `192.168.1.12`。把
  `MID360.host_net_info` 中四个非空 IP 改成实际 NUC 网口地址，
  把 `lidar_configs[0].ip` 改成雷达真实地址。两端同网段，端口与固件配置保持一致。
  可用 `lidar_config:=/绝对路径/你的配置.json` 加载自定义文件。
- 驱动 JSON 的 `extrinsic_parameter` 保持全零，点云保留在原生雷达坐标系。
  `src/fast_lio/config/mid360.yaml` 使用 FAST-LIO 上游的雷达至内置 IMU 外参。
- `src/bcrobot_bringup/config/sensor_mount.yaml` 保存 IMU 和 L515 相对底盘的安装参数。
  IMU 参数沿用原数值。xyz 为各传感器原点在 `base_footprint` 中的位置（X 前、Y 左、Z 上，单位米），
  yaw 为绕 +Z 的偏航，rpy 为滚转、俯仰、偏航（弧度）。L515 默认发布 `base_footprint → l515_link`，
  位置和姿态是零占位，实车平移跟随前请填入实测外参。

## 3. 先验收定位，再接底盘

```bash
roslaunch bcrobot_bringup mid360_drive.launch start_chassis:=false rviz:=true
```

雷达 SDK 的日志默认在 NUC 桌面的 **MID360 SDK** 终端单独显示。关闭该窗口会停止
雷达驱动，并让 `roslaunch` 一并关闭本次启动的节点。在主终端按 Ctrl+C 停止启动时，
日志窗口也会退出。通过 SSH 启动时，有已登录的 NUC 桌面也会弹出窗口。
无图形会话时，可在另一个 SSH 终端按主终端打印的路径用 `tail -f` 查看日志；
此时从主终端停止驱动。加 `lidar_log_window:=false` 可恢复在主终端直接显示 SDK 日志。

另开终端加载同一工作区，核对消息类型、频率、里程计延迟和 TF。

```bash
rostopic type /livox/lidar
rostopic hz /livox/lidar
rostopic hz /livox/imu
rostopic hz /Odometry
rostopic delay /Odometry
rosrun tf tf_echo camera_init base_footprint
```

`/livox/lidar` 应为 `livox_ros_driver2/CustomMsg`，驱动发布频率配置为 10 Hz。
RViz 显示的是 FAST-LIO 输出的 `/cloud_registered`。定点节点检查 TF 时间，
要求与 ROS 当前时间的差值在 -0.05 至 0.40 秒之间。先确认静止位姿稳定、移动方向
与底盘坐标一致，且 TF 时间有效。

停止上一步的 launch，再启动全套节点。底盘串口默认沿用 L1，可用 `port:=...` 覆盖。

```bash
roslaunch bcrobot_bringup mid360_drive.launch
```

键盘、定点和视觉跟随一次只运行一个。定点目标是 `camera_init` 中的绝对坐标（米），可选航向
为度。

```bash
rosrun robot_nav keyboard.py
# 或者
rosrun robot_nav goal_console.py
```

### L515 视觉跟随

在视觉终端启动现有 L515 节点，两套工作区使用同一个 `ROS_MASTER_URI`。在已加载 MID360
环境的交互式终端运行下方命令，SSH 可用 `ssh -t nuc` 打开终端。

```bash
rosrun robot_nav person_follow.py
```

启动后只转向对准人。按 `F` 加入平移跟随，按空格退回只转向，`Ctrl+C` 退出。
`src/robot_nav/config/person_follow.yaml` 设置输入输出话题、目标距离、误差和速度上限。
当前直接使用相机系的水平距离，平移方向通过安装 TF 转到底盘系。没有新视觉点时由现有底盘转发节点处理断流停车。

## 替换范围

- Unitree SDK1 和 L1 启动配置由官方 Livox-SDK2、livox_ros_driver2 和 MID360
  启动配置替代。驱动输出 `/livox/lidar`（CustomMsg）和 `/livox/imu`（Imu）。
  SDK 补上了定宽整数类型所需的标准头文件。
- FAST-LIO 恢复 Livox 原有点处理分支，使用 CustomMsg 中逐点时间去畸变，切换到
  MID360 上游参数。定位 TF 仍是 `camera_init → body → base_footprint`。
- WHEELTEC 串口包、底盘入口、`robot_nav` 原样复用。控制数据流仍是
  `/target_cmd_vel → base_controller_node → /cmd_vel → WHEELTEC`，含原限速和超时发零。

官方源码基线：[Livox-SDK2 v1.3.1](https://github.com/Livox-SDK/Livox-SDK2/tree/f5d9375f84efe2b15bc0a052d3e18482ed13adf4)、[livox_ros_driver2 1.2.4](https://github.com/Livox-SDK/livox_ros_driver2/tree/b6ff7d1c8b210a96b74e919823771e1e32249758)。
