# Unitree L1 + FAST-LIO2 差速移动机器人 SLAM 建图与自主导航系统

[![ROS Noetic](https://img.shields.io/badge/ROS-Noetic-brightgreen.svg)](http://wiki.ros.org/noetic)
[![Ubuntu 20.04](https://img.shields.io/badge/Ubuntu-20.04-orange.svg)](https://releases.ubuntu.com/20.04/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![LiDAR: Unitree L1](https://img.shields.io/badge/LiDAR-Unitree%20L1-blueviolet.svg)](https://www.unitree.com/)
[![Chassis: WHEELTEC](https://img.shields.io/badge/Chassis-WHEELTEC-red.svg)](http://www.wheeltec.net/)

本项目为一个完整的机器人操作系统（ROS1 Noetic）工作空间，专为搭载 **宇树科技 (Unitree) L1 激光雷达** 与 **轮趣 (WHEELTEC) 差速 / 四驱移动底盘** 的智能移动机器人设计。

系统紧耦合集成了 **FAST-LIO2** 实时 3D 激光惯导里程计与建图算法，利用 L1 雷达内置高频 IMU 与 18 线点云实现低延迟、高精度的 3D SLAM，并基于全局定位反馈开发了轻量级点到点自主航向对齐与差速移动导航控制器。

---

## 目录

- [系统特性](#系统特性)
- [硬件配置与连线](#硬件配置与连线)
- [工作空间结构](#工作空间结构)
- [环境依赖与安装](#环境依赖与安装)
- [串口配置与规则绑定](#串口配置与规则绑定)
- [编译指南](#编译指南)
- [快速开始与运行](#快速开始与运行)
  - [1. 单独启动底盘驱动](#1-单独启动底盘驱动)
  - [2. 启动雷达感知与 FAST-LIO2 建图](#2-启动雷达感知与-fast-lio2-建图)
  - [3. 启动自主导航控制器](#3-启动自主导航控制器)
  - [4. 全系统一键启动](#4-全系统一键启动)
- [导航控制与交互测试](#导航控制与交互测试)
- [外参标定与核心配置参数](#外参标定与核心配置参数)
- [常见问题排查 (FAQ)](#常见问题排查-faq)

---

## 系统特性

1. **宇树 Unitree L1 雷达深度适配**：
   - 官方 C++ SDK 与 ROS1 驱动封装，支持 2,000,000 高波特率串口通讯。
   - 适配 L1 独特的 18 线点云扫描结构与内置 IMU 高频数据采集。
2. **高精度 FAST-LIO2 实时 3D 激光惯导建图**：
   - 激光与雷达内置 IMU 紧耦合滤波，无需轮式里程计先验即可实现高鲁棒性全局位姿估计。
   - 实时生成全局稠密点云地图 (`/cloud_registered`) 与连续位姿轨迹 (`/Odometry`, `/path`)。
3. **轮趣 (WHEELTEC) 底盘协议与运动学驱动**：
   - 支持独立悬挂四驱差速小车（默认配置 `senior_4wd_dl`）及多种轮式车型。
   - 标准 ROS 速度接口 (`/cmd_vel`) 控制与机器人完整 URDF / TF 模型支持。
4. **轻量闭环全局自主导航控制器 (`robot_nav`)**：
   - 纯位姿反馈的航向角差速解算，具备航向对齐阈值判断与定点减速到位容差机制。
   - 内置安全急停控制通道（`/robot_nav/emergency_stop`），支持随时快速制动与复位。
   - 支持脚本命令行发点与 RViz 2D Nav Goal 目标点无缝衔接。
5. **工程化启动与外参管理 (`robot_bringup`)**：
   - 自动检测并软链接 CH9102 芯片串口设备（区分雷达与底盘 PID）。
   - 模块化分层 launch 设计，支持各子系统独立调试与全系统一键启动。

---

## 硬件配置与连线

| 硬件模块 | 设备型号 / 芯片 | 推荐接口 / 端口映射 | 说明 |
| :--- | :--- | :--- | :--- |
| **计算平台** | 工控机 / Jetson / PC | Ubuntu 20.04 LTS (ROS Noetic) | 系统主控 |
| **激光雷达** | 宇树科技 Unitree L1 | USB 端口 (识别为 `/dev/unitree_lidar`) | 芯片 CH9102 (PID: `55d3`)，波特率 2000000 |
| **移动底盘** | 轮趣 C30D 底盘控制板 | Type-C USB (识别为 `/dev/wheeltec_controller`) | 芯片 CH9102 (PID: `55d4`)，波特率 115200 |
| **电源系统** | 24V 动力锂电池组 | 接入底盘与工控机稳压供电 | 确保开机前闭合底盘电源总开关 |

---

## 工作空间结构

```text
robot_ws/
├── src/
│   ├── fast_lio/                 # FAST-LIO2 算法包 (适配宇树 L1 点云与内置 IMU)
│   │   ├── config/               # 雷达专用建图参数 (unilidar_l1.yaml)
│   │   ├── launch/               # mapping_unilidar_l1.launch
│   │   └── src/                  # FAST-LIO 核心源文件
│   ├── robot_bringup/            # 全系统一键启动调度与系统集成包
│   │   ├── config/               # static_tf.yaml (雷达与底盘物理安装外参)
│   │   ├── launch/               # chassis.launch / mapping.launch / nav.launch / bringup_all.launch
│   │   └── scripts/              # setup_ports.sh (串口智能动态绑定) / static_tf_broadcaster.py
│   ├── robot_nav/                # 自主点到点差速导航中枢包
│   │   ├── config/               # nav_params.yaml (导航线速度、角速度、对齐容差等)
│   │   ├── include/robot_nav/    # 底层速度下发 BaseController 与高层算法 PointNavigator
│   │   ├── launch/               # nav.launch
│   │   ├── scripts/              # send_goal.py (目标发布) / emergency_stop.py (急停)
│   │   └── src/                  # robot_nav_node.cpp 主节点
│   ├── turn_on_wheeltec_robot/   # 轮趣底盘核心驱动、协议解析、URDF 与 TF 树
│   └── unilidar_sdk1/            # 宇树官方 L1 雷达 C++ SDK 与 unitree_lidar_ros 驱动
├── .gitignore                    # 忽略 catkin 编译与临时文件
└── README.md                     # 项目技术文档
```

---

## 环境依赖与安装

本项目运行在 **Ubuntu 20.04 LTS** 环境下，使用 **ROS 1 Noetic**。

### 1. 基础依赖项安装
```bash
sudo apt-get update
sudo apt-get install -y \
    ros-noetic-desktop-full \
    ros-noetic-tf2-ros \
    ros-noetic-pcl-ros \
    ros-noetic-pcl-conversions \
    ros-noetic-serial \
    libpcl-dev \
    libeigen3-dev
```

### 2. Python 依赖项
```bash
sudo apt-get install -y python3-catkin-tools python3-rospy
pip3 install pyyaml
```

---

## 串口配置与规则绑定

由于宇树 L1 雷达和轮趣 C30D 底盘均使用沁恒 CH9102 USB 转串口芯片，插入系统时通常被分配为 `/dev/ttyACM*`。本项目内置了智能端口识别脚本，根据 USB 产品 ID（Product ID）自动完成软链接绑定与赋权：

```bash
cd ~/robot_ws/src/robot_bringup/scripts
sudo bash setup_ports.sh
```

脚本将自动建立如下映射并赋权 777：
- `PID 55d4` $\rightarrow$ `/dev/wheeltec_controller` (底盘)
- `PID 55d3` $\rightarrow$ `/dev/unitree_lidar` (L1 雷达)

> **提示**：如需系统开机自动绑定，也可将 udev 规则添加至 `/etc/udev/rules.d/`。

---

## 编译指南

在工作空间根目录下执行编译：

```bash
# 1. 进入工作空间
cd ~/robot_ws

# 2. 编译工作空间
catkin_make -DCMAKE_BUILD_TYPE=Release

# 3. 刷新环境变量 (建议写入 ~/.bashrc)
source devel/setup.bash
```

---

## 快速开始与运行

### 1. 单独启动底盘驱动
验证底盘串口通信与基本控制：
```bash
roslaunch robot_bringup chassis.launch
```
> 默认车型为 `senior_4wd_dl`。如使用其他车型，可通过参数指定：`roslaunch robot_bringup chassis.launch car_mode:=mini_diff`。

### 2. 启动雷达感知与 FAST-LIO2 建图
启动 L1 雷达驱动、静态外参广播并运行 FAST-LIO2 实时建图（自带 RViz 可视化）：
```bash
roslaunch robot_bringup mapping.launch
```
此时可在 RViz 中观察到雷达输出的实时点云、IMU 姿态以及稠密全局地图 `/cloud_registered`。

### 3. 启动自主导航控制器
启动基于全局里程计的差速小车导航解算中枢：
```bash
roslaunch robot_bringup nav.launch
```

### 4. 全系统一键启动
在已完成串口绑定的工控机上，使用一条命令拉起全套系统（底盘驱动 + 雷达感知 + FAST-LIO2 + 自主导航 + RViz）：
```bash
roslaunch robot_bringup bringup_all.launch
```

---

## 导航控制与交互测试

### 1. 发送测试目标点
使用内置客户端脚本向机器人发布目标位置（参考系为 FAST-LIO 全局原点 `camera_init`）：

```bash
# 用法: rosrun robot_nav send_goal.py <目标X坐标(米)> <目标Y坐标(米)> [目标朝向偏航角(度)]

# 示例: 指令小车向前行进 1.0 米，朝向 0 度
rosrun robot_nav send_goal.py 1.0 0.0 0

# 示例: 指令小车前进 2.5 米，向左平移 1.2 米，偏航 90 度
rosrun robot_nav send_goal.py 2.5 1.2 90
```

### 2. 紧急制动与解除
在调试或意外情况下，可通过专用话题快速迫停底盘：

```bash
# 触发急停 (小车立即输出 0 速度并锁定)
rosrun robot_nav emergency_stop.py stop

# 解除急停 (小车恢复正常待命状态)
rosrun robot_nav emergency_stop.py resume
```

### 3. 使用 RViz 2D Nav Goal 发送目标
若希望直接在 RViz 界面使用鼠标点击 `2D Nav Goal` 设定目标，只需打开 `src/robot_nav/src/robot_nav_node.cpp`，取消第 49 行代码的注释并重新 `catkin_make`：
```cpp
rviz_goal_sub_ = nh_.subscribe("/move_base_simple/goal", 1, &RobotNavNode::target_callback, this);
```

---

## 外参标定与核心配置参数

### 1. 传感器与底盘物理安装外参
文件位置：[`src/robot_bringup/config/static_tf.yaml`](src/robot_bringup/config/static_tf.yaml)
定义了 L1 雷达坐标系 (`unilidar_lidar`) 相对底盘旋转中心 (`base_link`) 的三维平移与旋转偏差：
```yaml
parent_frame: "base_link"
child_frame: "unilidar_lidar"
translation:
  x: 0.0      # 前后偏移: 雷达在底盘中心前方为正，后方为负 (米)
  y: 0.0      # 左右偏移: 雷达在底盘中心左侧为正，右侧为负 (米)
  z: 0.0      # 垂直高度: 雷达中心相对于底盘平面高度 (米)
rotation:
  roll: 0.0   # 横滚角 (rad)
  pitch: 0.0  # 俯仰角 (rad)
  yaw: 0.0    # 偏航角 (rad)
```

### 2. 导航控制核心参数
文件位置：[`src/robot_nav/config/nav_params.yaml`](src/robot_nav/config/nav_params.yaml)
```yaml
odom_topic: "/Odometry"                 # FAST-LIO 发布的全局里程计话题
desire_position_topic: "/desire_position" # 目标点话题
cmd_vel_topic: "/cmd_vel"               # 底盘控制速度输出话题
const_linear_speed: 0.15                # 导航直行线速度 (m/s)
const_angular_speed: 0.25               # 导航转弯角速度 (rad/s)
dist_tolerance: 0.08                    # 到达目标点距离判定阈值 (m)
angle_align_thresh: 0.35                # 航向对齐角度阈值 (rad，约 20°)
control_rate: 20.0                      # 定时控制回路频率 (Hz)
```

### 3. FAST-LIO2 算法配置
文件位置：[`src/fast_lio/config/unilidar_l1.yaml`](src/fast_lio/config/unilidar_l1.yaml)
- `lid_topic: "/unilidar/cloud"`
- `imu_topic: "/unilidar/imu"`
- `scan_line: 18` (宇树 L1 为 18 线)
- `extrinsic_est_en: false` (L1 内部点云与 IMU 外参已出厂固化标定)

---

## 常见问题排查 (FAQ)

1. **问：运行 `setup_ports.sh` 显示找不到设备？**
   - 答：请检查底盘电源开关是否处于开启状态、Type-C 数据线是否支持数据传输（而非纯充电线），以及雷达 USB 转接板供电是否正常。可通过 `lsusb` 命令排查是否存在 `1a86:55d4` 或 `1a86:55d3` 设备。

2. **问：编译报错 `fatal error: pcl/point_cloud.h: No such file or directory`？**
   - 答：缺少 PCL 库，请执行 `sudo apt install libpcl-dev ros-noetic-pcl-ros ros-noetic-pcl-conversions`。

3. **问：FAST-LIO 节点无法收到 IMU 或点云？**
   - 答：先单独启动雷达驱动 `roslaunch unitree_lidar_ros run_without_rviz.launch`，然后通过 `rostopic hz /unilidar/cloud` 和 `rostopic hz /unilidar/imu` 检查数据流是否正常（L1 串口波特率必须固定为 2000000）。

4. **问：小车执行导航时出现大幅左右摇摆或航向对不准？**
   - 答：在 [`src/robot_nav/config/nav_params.yaml`](src/robot_nav/config/nav_params.yaml) 中适度降低 `const_angular_speed`（如调为 0.18），或适当放大 `angle_align_thresh`。

---

## 许可证 (License)

本项目采用 [MIT License](LICENSE) 开源许可证。所引用的 FAST-LIO 算法组件遵循其原开源协议 (BSD-3-Clause)，宇树 SDK 与轮趣驱动遵循其各自分发许可。
