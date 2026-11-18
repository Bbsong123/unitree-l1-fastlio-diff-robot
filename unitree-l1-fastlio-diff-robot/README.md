# L1 三轮全向底盘联调

本目录是独立的 ROS1 Noetic / C++14 catkin 工作空间，由父目录统一管理。L1 和 L2 不同时 source 或运行。

## 数据流

- SDK1 → 点云与 IMU → FAST-LIO：动态发布 `camera_init → body`，其中 `body` 是 IMU 坐标系。
- `bcrobot_bringup` 同时启动 WHEELTEC 串口驱动和 `body → base_footprint` 静态 TF。安装参数在 `src/bcrobot_bringup/config/l1_mount.yaml`，填写 IMU 在底盘坐标系中的 xyz（米）和 yaw（弧度），节点取逆后发布。当前值为 NUC 安装估计值。
- `robot_nav` 的键盘、定点或外部算法任选其一 → `/target_cmd_vel`（车体坐标 Twist）→ C++ `base_controller_node` 限速转发 → `/cmd_vel` → WHEELTEC。没有独立 Python 看门狗；C++ 保留原有超时发零。
- `/odom` 保留为轮式反馈，不参与定位 TF。定点控制直接读 `camera_init → base_footprint`，无第二原点。查询不到新鲜、有效 TF 时停止运行。

## 编译与启动

独立 Ubuntu / ROS Noetic 工作空间中，安装声明的依赖及底盘所需的 `ros-noetic-serial`，然后执行：

```bash
source /opt/ros/noetic/setup.bash
rosdep install --from-paths src --ignore-src -r -y
catkin_make -j2
source devel/setup.bash
```

每个终端都加载同一个 L1 工作空间，按顺序启动：

```bash
# 终端一：只开底层，不发运动指令
roslaunch bcrobot_bringup l1_drive.launch
# 终端二：开 C++ 限速转发
roslaunch robot_nav nav.launch
```

默认使用 NUC 的雷达和底盘 `/dev/serial/by-id`；其他机器先核对设备。底盘可用 `port:=...` 覆盖，L1 雷达路径在 SDK1 `unitree_lidar_ros/config/config.yaml`。如需查看点云可传 `rviz:=true`。

确认定位已稳定、车体 TF 正确、现场有可用的硬件急停后，第三终端选一种上层控制：

```bash
rosrun robot_nav keyboard.py
# 或使用定点输入（不要与键盘或外部速度指令同时发布）
rosrun robot_nav goal_console.py
```

`pose_goal.py x y [yaw]` 也可执行单个目标。坐标是 `camera_init` 下的绝对坐标（米），航向为度，逆时针为正；省略航向保持当前车头朝向。它不做避障，不重置原点，不订阅原始点云、IMU 或轮式里程计。节点失效或串口断连时，软件超时不能替代硬件急停。
