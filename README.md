# 三轮全向底盘实验项目

部署环境：额外的 NUC，Ubuntu 20.04 / ROS1 Noetic / C++14。
当前只打通底盘速度通信和雷达定位数据，不包含自主运动控制。

## 数据流

```text
外部速度指令 /target_cmd_vel (Twist)
    → base_controller_node（限速、无有效输入时发零）
    → /cmd_vel
    → 官方 wheeltec_robot_node
    → 串口
    → 三轮全向底盘单片机

L1 雷达 → 点云和 IMU → FAST-LIO → /Odometry、地图点云
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
| `src/robot_bringup/launch/mapping.launch` | 启动 L1 和 FAST-LIO |
| `src/robot_bringup/launch/bringup_all.launch` | 同时启动上述数据流 |
| `src/robot_bringup/config/static_tf.yaml` | 底盘到雷达的安装位置，需实测填写 |
| `src/turn_on_wheeltec_robot/` | 官方串口驱动和车型模型 |

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

将当前修改后的仓库完整复制到 NUC，例如 `~/robot_ws/`，确保该目录下直接包含 `src/`。本地修改尚未推送，单独克隆远端不会包含本次修改。

```bash
# NUC 预先安装 Ubuntu 20.04 和 ROS Noetic。
source /opt/ros/noetic/setup.bash
cd ~/robot_ws
# rosdep 需已初始化，用它安装仓库声明的依赖。
rosdep install --from-paths src --ignore-src -r -y
# 下面是底盘启动时需要的串口、滤波及模型发布组件。
sudo apt-get install -y ros-noetic-serial ros-noetic-robot-pose-ekf \
  ros-noetic-joint-state-publisher ros-noetic-robot-state-publisher python3-yaml
catkin_make
source devel/setup.bash
```

底盘默认串口 `/dev/wheeltec_controller`，波特率 `115200`；L1 默认 `/dev/unitree_lidar`，波特率 `2000000`。原端口脚本按 USB PID 识别设备，新底盘是否一致需在 NUC 上用 `lsusb` 确认。一致时运行：

```bash
sudo bash src/robot_bringup/scripts/setup_ports.sh
```

底盘串口配置：`src/turn_on_wheeltec_robot/launch/include/base_serial.launch`。

## 先跑底盘数据流

两个终端均先执行 `source ~/robot_ws/devel/setup.bash`，分别启动：

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

`static_tf.yaml` 填写 `base_link → unilidar_lidar` 的实际安装关系，目前零值不是测量结果。FAST-LIO 的 `mapping/extrinsic_T/R` 是雷达和内置 IMU 的内部外参，保持其原有配置。定位输出仅供观察，当前不会驱动底盘，也不再接收目标点或 RViz 的 2D Nav Goal。

2026-11-17：已在 Windows 完成代码、YAML 和启动配置静态检查；本机没有 ROS Noetic，NUC 上的编译、串口与实车数据流尚未验证。
