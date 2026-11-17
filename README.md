# L2 三轮全向底盘实验项目

部署环境：额外的 NUC，Ubuntu 20.04 / ROS1 Noetic / C++14。
独立 L2 版本，分支 `codex/l2-fastlio-omni`，与原 L1 工程并行维护。
基于当前 L1 工作区复制底盘和 FAST-LIO 实现，原 L1 文件未修改。用户确认 L1 雷达建图数据流已通过实机验证；L2 尚未实机验证。
当前只保留底盘速度通信和雷达定位数据，不包含自主运动控制。

## 数据流

```text
外部速度指令 /target_cmd_vel (Twist)
    → base_controller_node（限速、无有效输入时发零）
    → /cmd_vel
    → 官方 wheeltec_robot_node
    → 串口
    → 三轮全向底盘单片机

L2 雷达 → 网口 UDP → 官方 SDK2 → /unilidar/cloud、/unilidar/imu
    → 现有 FAST-LIO → /Odometry、地图点云
底盘单片机 → 官方驱动 → /odom、/imu、/PowerVoltage
```

速度转发只使用车体坐标下三个分量：

| 分量 | 含义 | 单位及正方向 |
| --- | --- | --- |
| `linear.x` | 前后速度 | m/s，向前为正 |
| `linear.y` | 横移速度 | m/s，向左为正 |
| `angular.z` | 旋转速度 | rad/s，逆时针为正 |

节点只增加基本限速和无有效输入时停车，不做平滑或目标跟踪：

- 启动后没收到有效指令：每 0.05 秒向输出话题发零速度。
- 消息指针为空，或 vx、vy、wz 中有 NaN / 无穷：立即发零，之后继续发零。
- 超过 `command_timeout` 没收到有效消息：发零停车，正常调度下检查延迟最多约 0.05 秒。
- 收到新的有效消息：立即按限速转发，恢复运动；零速度也属于有效消息。

`Twist` 没有时间戳，这里按本节点实际收到消息的时间计时。停车依赖节点和串口链路正常运行；单片机自身断联行为尚未实测。

单片机按已经配好三轮全向程序处理，上位机不计算三个电机的转速。官方包已支持三个速度分量。默认 `senior_omni` 暂作型号占位，还可选 `mini_omni`、`top_omni`；`car_mode` 选择 ROS 模型和 TF，不会切换单片机固件。

## 主要文件

| 路径 | 用途 |
| --- | --- |
| `src/robot_nav/src/base_controller_node.cpp` | 唯一自写速度节点，详细中文注释 |
| `src/robot_nav/launch/nav.launch` | 启动速度转发并加载 YAML |
| `src/robot_nav/config/base_controller.yaml` | 输入/输出话题、线速/角速限制和接收超时 |
| `src/robot_bringup/launch/chassis.launch` | 启动官方底盘驱动并选择车型 |
| `src/robot_bringup/launch/mapping.launch` | 启动 L2 UDP 驱动和 FAST-LIO |
| `src/robot_bringup/launch/bringup_all.launch` | 同时启动上述数据流 |
| `src/robot_bringup/config/static_tf.yaml` | 雷达到底盘的安装位姿，需实测填写 |
| `src/turn_on_wheeltec_robot/` | 官方串口驱动和车型模型 |
| `src/unilidar_sdk2/` | 官方 SDK2 2.0.9，来源和改动见 UPSTREAM.md |
| `src/fast_lio/config/unilidar_l2.yaml` | L2 输入话题、逐点时间及内部外参 |

## 速度转发配置

修改 `src/robot_nav/config/base_controller.yaml`，重启节点生效：

```yaml
input_topic: "/target_cmd_vel"
output_topic: "/cmd_vel"
max_linear_speed: 0.4    # 平面合速度上限，m/s
max_angular_speed: 0.8   # 旋转速度绝对值上限，rad/s
command_timeout: 0.5     # 未收到有效指令的超时时间，秒
```

线速度按 `hypot(vx, vy)` 限制，超限时等比例缩小 vx、vy，保留方向。例如 `(0.3, 0.4)` 会缩为 `(0.24, 0.32)`，合速度为 0.4 m/s。角速度独立限制到 ±0.8 rad/s，使用 C++14 的 `std::min/max`。

三个数值参数必须为有限正数。接收和转发话题不能相同；默认输出 `/cmd_vel` 对应官方驱动的订阅话题，若改输出名，也要相应重映射驱动话题。

## 在 NUC 上编译

将当前修改后的仓库完整复制到 NUC，例如 `~/robot_l2_ws/`，确保该目录下直接包含 `src/`。本地修改尚未推送，单独克隆远端不会包含本次修改。

```bash
# NUC 预先安装 Ubuntu 20.04 和 ROS Noetic。
source /opt/ros/noetic/setup.bash
cd ~/robot_l2_ws
# rosdep 需已初始化，用它安装仓库声明的依赖。
rosdep install --from-paths src --ignore-src -r -y
# 当前启动链只需串口和模型发布；不启用轮式 EKF。
sudo apt-get install -y ros-noetic-serial ros-noetic-robot-state-publisher python3-yaml
catkin_make
source devel/setup.bash
```

底盘默认串口 `/dev/wheeltec_controller`，波特率 `115200`。端口脚本只映射底盘（原 PID `55d4`，仍需按实物核对）：

```bash
sudo bash src/robot_bringup/scripts/setup_ports.sh
```

L2 不需要串口赋权。请自行在系统中配置连接雷达的网卡，再核对
`src/unilidar_sdk2/unitree_lidar_ros/config/config.yaml` 的地址：

```yaml
initialize_type: 2
work_mode: 0
lidar_ip: "192.168.1.62"
lidar_port: 6101
pc_ip: "192.168.1.2"
pc_port: 6201
```

`pc_ip` 必须是系统中已配置的本机地址，雷达实际地址若已改过也要同步填写。
工程不修改网卡、路由或系统网络配置。不要同时 source L1、L2 两套工作空间：两者 ROS 包和话题同名。

底盘串口配置：`src/turn_on_wheeltec_robot/launch/include/base_serial.launch`。

## 先跑底盘数据流

两个终端均先执行 `source ~/robot_l2_ws/devel/setup.bash`，分别启动：

```bash
# 终端 1：官方底盘驱动。
roslaunch robot_bringup chassis.launch car_mode:=senior_omni
# 终端 2：最小速度转发节点。
roslaunch robot_bringup nav.launch
```

第三个终端发送向左 0.05 m/s 的指令：

```bash
rostopic pub -r 10 /target_cmd_vel geometry_msgs/Twist \
  '{linear: {x: 0.0, y: 0.05, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}'
```

按 Ctrl-C 停止持续发布后，默认约 0.5 秒超时发零；也可明确发送停止指令：

```bash
rostopic pub -1 /target_cmd_vel geometry_msgs/Twist \
  '{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}'
```

用 `rostopic echo /cmd_vel` 看转发结果，`rostopic echo /odom` 看底盘反馈。前进和旋转分别设置 `linear.x`、`angular.z`，也可组合三个分量。可直接发 `/cmd_vel` 调试官方驱动，此时绕过本节点的限速和超时停车。

## 雷达与全系统启动

```bash
# 单独启动雷达定位；无显示器的 NUC 关闭 RViz。
roslaunch robot_bringup mapping.launch open_rviz:=false
# 或一键启动底盘 + 速度转发 + 雷达定位。
roslaunch robot_bringup bringup_all.launch car_mode:=senior_omni open_rviz:=false
```

检查数据：`rostopic hz /unilidar/cloud`、`rostopic hz /unilidar/imu`、`rostopic echo /Odometry`。

## 点云格式与 FAST-LIO

SDK2 的官方 PCL/PointCloud2 输出字段与当前 FAST-LIO 的宇树点类型一致：

| 字段 | ROS 输出类型 | 含义 |
| --- | --- | --- |
| x/y/z | float32 | 雷达系坐标，米 |
| intensity | float32 | 回波强度 |
| ring | uint16 | 官方 ROS 点类型的扫描标识 |
| time | float32 | 相对点云起始 stamp 的时间，秒 |

SDK2 内部原始 `PointUnitree.ring` 为 uint32，但经官方 PCL 转换后输出 uint16；FAST-LIO 订阅的是 PointCloud2。
保留 `lidar_type: 5`、`timestamp_unit: 0`，原 `unilidar_handler` 将秒乘 1000 存到 curvature，后续去畸变按毫秒使用。
`cloud_scan_num: 18` 沿用官方组帧默认；算法配置的 `scan_line: 18` 沿用官方 L2 示例，不代表物理 18 线雷达。
本分支未改动任何 FAST-LIO C++ 源码，只将 L1 YAML/launch 换成 L2 入口；其余定位参数沿用基线。

时间戳沿用官方实现：点云使用 SDK 的 cloud.stamp，IMU 使用 ROS 接收时间；默认开启 use_system_timestamp。
实机需核对两路时序，未额外添加时间补偿或同步算法。

## TF 与安装外参

```text
camera_init                 FAST-LIO 世界根
  └─ body                   FAST-LIO 动态位姿；就是雷达内置 IMU 系
      └─ unilidar_lidar      SDK2 官方内部外参
          └─ base_link      static_tf.yaml：雷达→底盘
              ├─ base_footprint
              ├─ gyro_link
              └─ 三轮模型固定关节
```

SDK2 原本还发布 imu_initial→imu 姿态 TF；本分支移除这一条，避免与 FAST-LIO 争夺 body 的父节点。
驱动仍发布 IMU→雷达外参，配置 `imu_frame: body`，平移为 `[0.007698, 0.014655, -0.00667]` 米，旋转为单位阵。
FAST-LIO 的 `mapping/extrinsic_T/R` 保持相同数值和方向：`p_IMU = R * p_lidar + T`。

`src/robot_bringup/config/static_tf.yaml` 填底盘原点在雷达系中的位置和姿态，零值仅是未测量占位。
例如雷达轴向与底盘一致，且位于底盘上方 0.3 米，配置 z=-0.3。
若已有旧的底盘→雷达矩阵，先求完整逆变换 `R'=Rᵀ, t'=-Rᵀt`，再填入，不能只交换 frame 名。

L2 版 chassis.launch 直接调用官方串口节点及三轮 URDF，保留 /odom、/imu、/PowerVoltage；不启动旧 robot_pose_ekf 或旧模型 launch 的 TF。
原 base_footprint→base_link 零变换改为 base_link→base_footprint，保证 base_link 只由雷达挂接。
底盘板载 IMU 的零安装偏移沿用原占位，未做测量。/odom 仍是轮速积分反馈，其 odom_combined 坐标仅保留在消息中，不作为本系统 TF 根。
FAST-LIO 输出仅供观察，不参与底盘控制。

## 验证状态与分支

- L1 雷达建图数据流：用户确认已在实机验证，原 L1 工程保持原样；该结论不自动代表近期速度转发修改也已实测。
- L2：完成源码格式和外参核对、静态配置检查、底盘脚本 bash -n；SDK 公共接口头文件通过 MinGW C++14 语法检查（启用 Windows 数学常量宏），不等同于 ROS 驱动编译通过；当前 Windows 无 ROS Noetic，尚未在 NUC 编译链接 SDK2、接网口或建图。
- 官方 SDK2 CMake 原为 C++17，本分支按项目约定配置 C++14；ROS/PCL 与预编译库的实际兼容性仍须 NUC 编译确认。
- 本目录是原 Git 仓库的独立工作树，分支 `codex/l2-fastlio-omni`；保留共同历史，不合并到原 main。当前 L2 版本在该分支独立提交保存，未推送；包含从 L1 工作区继承的底盘改动。
- SDK2 固定来源提交和本地补丁见 `src/unilidar_sdk2/UPSTREAM.md`。没有引入 Point-LIO、新控制算法、网络设置脚本或测试框架。
