# Unitree L1 + FAST-LIO2 差速移动机器人 SLAM 建图与自主导航系统

[![ROS Noetic](https://img.shields.io/badge/ROS-Noetic-brightgreen.svg)](http://wiki.ros.org/noetic)
[![Ubuntu 20.04](https://img.shields.io/badge/Ubuntu-20.04-orange.svg)](https://releases.ubuntu.com/20.04/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![LiDAR: Unitree L1](https://img.shields.io/badge/LiDAR-Unitree%20L1-blueviolet.svg)](https://www.unitree.com/)
[![Chassis: WHEELTEC](https://img.shields.io/badge/Chassis-WHEELTEC-red.svg)](http://www.wheeltec.net/)

本项目为一个完整的机器人操作系统（ROS1 Noetic）工作空间，专为搭载 **宇树科技 (Unitree) L1 激光雷达** 与 **轮趣 (WHEELTEC) 差速 / 四驱移动底盘** 的智能移动机器人设计。

系统紧耦合集成了 **FAST-LIO2** 实时 3D 激光惯导里程计与建图算法，利用 L1 雷达内置高频 IMU 与 18 线点云实现低延迟、高精度的 3D SLAM，并在上层建立了**纯话题解耦、极简即插即用的分层运动控制套件 (`robot_nav`)**。

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
  - [3. 启动运动控制套件](#3-启动运动控制套件)
  - [4. 全系统一键启动](#4-全系统一键启动)
- [控制算法交互与测试](#控制算法交互与测试)
- [系统核心参数配置](#系统核心参数配置)
- [未来算法扩展指南 (MPC / PID / 遥控)](#未来算法扩展指南-mpc--pid--遥控)
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
4. **分层独立、纯话题解耦的运动控制套件 (`robot_nav`)**：
   - **底层守护控制节点 (`base_controller_node`)**：负责统一速度接口 (`/target_cmd_vel` $\rightarrow$ `/cmd_vel`)、基础限速、以及心跳超时看门狗（若上层算法异常崩溃超过 0.5s，底层自动下发 0 速度刹停）。
   - **算法跟踪解算节点 (`point_tracker_node`)**：纯单文件 C++ 实现，负责定点跟踪与航向对齐算法，支持原生 RViz 2D Nav Goal 点击交互。
   - **即插即用扩展机制**：未来加入 MPC、PID、轨迹跟踪或手柄遥控时，无需改动底层，仅需新建独立算法节点直接向 `/target_cmd_vel` 发布速度即可。
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
| **电源系统** | 24V 动力锂电池组 | 接入底盘与工控机稳压供电 | 闭合底盘电源总开关与硬件急停旋钮 |

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
│   ├── robot_nav/                # 纯话题解耦运动控制套件
│   │   ├── config/               # nav_params.yaml (底层限速、算法跟踪参数)
│   │   ├── launch/               # nav.launch
│   │   ├── scripts/              # send_goal.py (测试目标发布脚本)
│   │   └── src/                  # base_controller_node.cpp / point_tracker_node.cpp
│   ├── turn_on_wheeltec_robot/   # 轮趣底盘核心驱动、协议解析、URDF 与 TF 树
│   └── unilidar_sdk1/            # 宇树官方 L1 雷达 C++ SDK 与 unitree_lidar_ros 驱动
├── .gitignore                    # 忽略 catkin 编译产物与临时文件
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

### 3. 启动运动控制套件
启动底层速度守护控制器与定点跟踪算法节点：
```bash
roslaunch robot_bringup nav.launch
```

### 4. 全系统一键启动
在已完成串口绑定的工控机上，使用一条命令拉起全套系统（底盘驱动 + 雷达感知 + FAST-LIO2 + 运动控制套件 + RViz）：
```bash
roslaunch robot_bringup bringup_all.launch
```

---

## 控制算法交互与测试

### 1. 发送测试目标点
使用内置客户端脚本向机器人发布目标位置（参考系为 FAST-LIO 全局原点 `camera_init`）：

```bash
# 用法: rosrun robot_nav send_goal.py <目标X坐标(米)> <目标Y坐标(米)> [目标朝向偏航角(度)]

# 示例: 指令小车向前行进 1.0 米，朝向 0 度
rosrun robot_nav send_goal.py 1.0 0.0 0

# 示例: 指令小车前进 2.5 米，向左平移 1.2 米，偏航 90 度
rosrun robot_nav send_goal.py 2.5 1.2 90
```

### 2. 使用 RViz 2D Nav Goal 发送目标
在 RViz 界面中，直接点击顶部的 `2D Nav Goal` 按钮并在地图上拖动箭头，`point_tracker_node` 会自动接收目标点并驱动小车前往。

---

## 系统核心参数配置

### 1. 底层速度守护控制器参数
文件位置：[`src/robot_nav/config/nav_params.yaml`](src/robot_nav/config/nav_params.yaml)
```yaml
base_controller:
  input_topic: "/target_cmd_vel"     # 接收期望速度
  output_topic: "/cmd_vel"           # 输出到底层实际底盘驱动
  max_linear_speed: 0.4              # 最大线速度限制 (m/s)
  max_angular_speed: 0.8             # 最大角速度限制 (rad/s)
  watchdog_timeout: 0.5              # 看门狗超时时间 (s)，上层异常断联超时自动减速刹停
```

### 2. 定点跟踪算法参数
```yaml
point_tracker:
  odom_topic: "/Odometry"            # FAST-LIO 里程计
  target_topic: "/desire_position"   # 目标点话题
  cmd_topic: "/target_cmd_vel"       # 输出给底层守护控制器的速度
  const_linear_speed: 0.15           # 巡航直线常速 (m/s)
  const_angular_speed: 0.25          # 原地转向常速 (rad/s)
  dist_tolerance: 0.08               # 到达判定距离容差 (m)
  angle_align_thresh: 0.35           # 航向对齐阈值 (rad, 约20°)
  control_rate: 20.0                 # 算法解算频率 (Hz)
```

---

## 未来算法扩展指南 (MPC / PID / 遥控)

在新的纯话题解耦架构下，如果未来需要实现更高级的控制算法（如 MPC 轨迹跟踪）：

1. **新建独立算法节点**：
   - 直接在 `src/robot_nav/src/` 下新建 `mpc_tracker_node.cpp`；
2. **纯话题订阅与发布**：
   - 订阅 `/Odometry`（获取当前位姿）；
   - 订阅你的目标轨迹（如 `/reference_path`）；
   - 运行求解器解算后，直接发布 `geometry_msgs/Twist` 到 `/target_cmd_vel`；
3. **即插即用**：
   - 底层的 `base_controller_node` 和 `turn_on_wheeltec_robot` 完全无需做任何代码修改，自动为你提供超时安全看门狗。

---

## 常见问题排查 (FAQ)

1. **问：运行 `setup_ports.sh` 显示找不到设备？**
   - 答：请检查底盘电源开关与硬件急停旋钮是否闭合、Type-C 数据线是否支持数据传输、雷达 USB 转接板供电是否正常。可通过 `lsusb` 命令排查是否存在 `1a86:55d4` 或 `1a86:55d3` 设备。

2. **问：FAST-LIO 节点无法收到 IMU 或点云？**
   - 答：先单独启动雷达驱动 `roslaunch unitree_lidar_ros run_without_rviz.launch`，然后通过 `rostopic hz /unilidar/cloud` 和 `rostopic hz /unilidar/imu` 检查数据流（L1 串口波特率必须固定为 2000000）。

3. **问：如何修改小车直行或转向速度？**
   - 答：在 [`src/robot_nav/config/nav_params.yaml`](src/robot_nav/config/nav_params.yaml) 中直接调整 `const_linear_speed` 或 `const_angular_speed`，无需重新编译。
