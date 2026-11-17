# FAST-LIO：L2 输入配置（ROS1 Noetic）

保留原 L1 工程已验证的 FAST-LIO C++ 实现。本分支使用官方 SDK2 PointCloud2/IMU 输出，入口为 `launch/mapping_unilidar_l2.launch`，参数为 `config/unilidar_l2.yaml`。

- ROS 点字段仍为 x/y/z/intensity(float)、ring(uint16)、time(float，相对帧起点的秒)，不需要替换现有 `Preprocess::unilidar_handler`。
- `timestamp_unit: 0` 将点时间由秒转为内部 curvature 毫秒；保留原逐点去畸变逻辑。
- 内部外参按 SDK2 源码保留，世界 TF 仍由算法发布 camera_init→body。
- L2 的实机编译、网口数据和定位质量尚未验证；完整部署与 TF 说明见工作空间根 README。
