# 项目进展

更新：2026-11-19

- 已建立项目文档；当前以项目父目录为唯一 Git 仓库，统一管理文档和 L1/L2 两份并行代码。
- 已克隆 `unitree-l1-fastlio-diff-robot/`，保留远端与历史；整理前 L1 版本提交为 `6ffc312`，现保留于父仓库 `history/l1`；未推送。
- `robot_nav` 保留一个 C++14 速度转发节点，转发 vx、vy、wz，并提供详细中文注释；按最新要求加入基本停车、话题配置及速度限制。
- 保持删除上层跟踪器、目标点脚本、辅助数学头文件和测试目录；未加回平滑或控制算法。
- 新增 `base_controller.yaml`：输入/输出话题、平面合速度上限、角速度上限和接收超时；launch 在节点私有命名空间加载，CMake 安装 config 目录。
- 新增 stopRobot()：启动后无有效消息、无效消息、0.5 秒接收超时时发零；20Hz 检查无输入状态，收到有效输入后限速转发。
- 保留官方三轮全向驱动和雷达定位数据流，默认型号仍为 `senior_omni`（待实物确认）；一键启动可关闭 RViz。
- 已重写仓库 README 为 NUC 基础部署和话题联调说明；父目录与代码仓库的 AGENTS.md 均写入实验代码准则和修改后详细汇报要求。
- 本轮验证：YAML 键值、C++ 私有参数读取对应、launch XML、配置安装与停车/定时入口静态检查、`git diff --check` 通过；未新增测试体系。
- 当前 Windows 环境无 ROS Noetic 或可用 WSL。用户于本日补充确认：原 L1 雷达建图数据流已在实机验证；近期速度转发修改及底盘停车尚未提供实测结果。先前数学测试对应的算法已移除，其通过结果不代表当前节点已在 ROS 运行。

## L2 并行版本（2026-11-17）

- 最初通过独立 Git 工作树和 `codex/l2-fastlio-omni` 分支建立 L2（共同基点 `4faba28`），继承 L1 工作区的底盘修改。现 L2 已归入父仓库并行目录，原提交 `d6a79fa` 保留于 `history/l2`；未推送。
- 已换入官方 SDK2 2.0.9（来源提交 `0e3c51f512e6b8ff60b8c32f160b412cb48445c2`），修正 ROS1 包相对路径并使用 C++14 构建配置；官方静态库原样保留。
- 已配置 L2 UDP 驱动与独立 L2 YAML/launch；端口脚本仅保留底盘串口映射，未新增系统网络配置脚本。L2 分支移除 SDK1 和 L1 专用 YAML/launch；原 L1 工程无删除或修改。
- SDK2 ROS 点类型与当前 FAST-LIO 注册字段一致；FAST-LIO 的全部 src/include 文件、robot_nav 和官方底盘包均与 L1 当前工作区逐文件一致，未修改点云处理或控制算法。
- L2 TF 接为 `camera_init -> body -> unilidar_lidar -> base_link`。保留 SDK2 官方 IMU->雷达外参，移除其初始 IMU 姿态 TF；L2 底盘入口不启用旧 EKF/模型 TF，改由雷达安装 TF 挂接底盘。
- 已通过：活动 launch XML/YAML/Python 语法与路径、同名包排查、点类型对照、TF/内部外参数值核对、SDK 静态库完整性、底盘脚本 `bash -n`、`git diff --check`；SDK 公共接口头文件在 MinGW C++14 下语法检查通过（启用 Windows 数学常量宏）。原 L1 文件哈希及工作区 Git 状态核对无变化。
- 未验证：NUC 上 ROS/PCL 驱动编译与 Linux 静态库链接、L2 UDP 收包、实际时间轴、TF 运行与建图效果。安装外参仍为未测量零占位。

## 分支提交与协作规则（2026-11-17）

- 按用户明确要求，L1/main 提交 `6ffc312`，L2/codex/l2-fastlio-omni 提交 `d6a79fa`；两个代码工作区均无待提交改动，未推送。
- 已在项目全局 AGENTS.md 增加功能改动完成后主动询问是否提交的规则；用户已授权提交时直接执行，推送需单独授权。


## 父目录统一 Git 仓库（2026-11-17）

- 唯一仓库根为 `E:\csprogram\全向轮`，当前统一分支 `main`；规范、入口、进度、讨论及 L1/L2 普通代码目录共同纳入版本管理，两个子目录不再是独立仓库或工作树。
- 保留旧版本分支 `history/l1`（`6ffc312`）、`history/l2`（`d6a79fa`）；原仓库全部 4 个提交已原样导入，统一提交以两个旧版本提交为父节点，不改写提交 ID。原 origin 地址和远端跟踪引用已保留，未推送。
- 原代码 Git 元数据封存在 `.git/legacy-code-repo/`，完整历史备份 `.git/legacy-code-history.bundle` 已验证，L2 旧工作树链接另行封存。上述备份不进入源码提交；正式历史已进入父仓库。
- L1 191 个文件、L2 177 个文件全部保留，逐文件哈希确认仅各自 README/AGENTS 的版本管理说明改变，运行代码及 SDK 库内容未改。补充父目录 Git 忽略与 Linux 脚本换行规则。
- 验证通过：两子目录解析出的 Git 根均为父目录、全部旧提交可通过历史分支追溯、Git bundle 完整性、父仓库对象完整性。此次无功能变化，未重复 ROS/实机检查；原有实机验证边界不变。
- 提交索引包含 374 个普通文件，无 Git 子模块链接；与旧版本逐项核对后，两个代码目录除 README/AGENTS 外的路径、文件模式和 Git blob ID 全部相同。

## NUC 部署只读核查（2026-11-18）

- 通过 VS Code 使用的 SSH 主机别名 `nuc` 检查 `/home/icral/BCrobot_ws`。工作空间和 `src` 内未发现 Git 仓库；已有 4 个 catkin 包的构建产物。检查时未见 ROS 进程、默认 master 端口未监听；最近 ROS 日志记录 L1 节点于 11:13 关停。
- 在 NUC 只解析 launch 和包路径，未启动节点：L1 集成、底盘和 L2 集成、纯定位、串口、UDP 入口均解析成功。L2 通过 `setup_l2.bash` 叠加 `fast_ws`，SDK2、点云转换、Livox 消息及 FAST-LIVO2 均来自该外部工作空间。
- NUC 历史记录：L1 点云和定位约 9.86 Hz；11:04 的 0.20 m 短程试验日志记录剩余 0.042 m、轮式里程 0.171 m、最后速度为零且 `stopped=true`。当前 `pose_goal.py` 在试验后有更新，不能用该日志验证当前 3 cm 阈值。
- NUC 历史记录：L2 曾采到点云约 11.96 Hz、IMU 约 250.88 Hz、车体位姿约 11.88 Hz；分别来自不同采样记录。日志随后记录定位跳变，无 L2 自动运动验收。本机 L2 的 SDK2→FAST-LIO 直连不等同于该验证。
- 本次未修改部署、未重编译、未实车复测；未改本地运行代码、未提交或推送。

## 本机 L1 同步 NUC 部署源码（2026-11-18）

- 从父仓库 `main` 创建 `codex/l1-nuc-sync`。以 NUC 当前 L1 为准替换本机 `robot_bringup` 为 `bcrobot_bringup`，导入两个 L1 集成入口、底盘串口入口、位姿配置及 6 个 Python 脚本；另同步工作空间根目录的独立前进试验脚本。
- 同步了 FAST-LIO 的生成目标依赖、L1 驱动引用及日志输出选项，SDK1 包名与串口路径，以及 WHEELTEC 旧整包 launch 默认车型；串口驱动源码和本机 C++14 构建设置保留。保留 `robot_nav` 不变，新入口不启动它。
- 跳过 NUC 中非当前入口依赖的旧地图、导航参数、L2 专用内容及运行产物；记录 FAST-LIO 日志目录占位，不带入已有日志。L2 代码与 NUC 部署未修改。
- 本机检查通过：6 个节点脚本及独立试验脚本的 Python 3.8 语法、包名与脚本安装清单、L1 XML/YAML、默认/目标/复用启动链及指纹比对；确认 `robot_nav` 和 L2 无改动。未在 ROS Noetic 重新编译、启动或实车复测；未提交或推送。

## L1 启动与控制分离，独立部署到 NUC（2026-11-18）

- 本机 `bcrobot_bringup` 只启动 SDK1、FAST-LIO、WHEELTEC 和 `body → base_footprint` 静态 TF；外参由安装 YAML 取逆得到。移除启动归零、原始传感器联调检查、独立 Python 看门狗和依赖旧链路的入口/试验工具。
- `robot_nav` 保留原 C++ 限速、转发、超时代码及其 YAML；键盘、定点、目标终端脚本归入该包。定点直接查 `camera_init → base_footprint`，只接受有效且新鲜的 TF；键盘和定点都向 `/target_cmd_vel` 发 Twist。
- 已部署到 NUC `/home/icral/BCrobot_L1_ws`，干净环境 `catkin_make -j2` 成功；没有沿用旧构建缓存。L2 本机代码、共享 `BCrobot_ws`（含日志与缓存）和 `fast_ws` 均未改动。
- 本机语法、包清单、XML/YAML、话题和 Git 差异检查通过。NUC 启动静态解析、可执行产物、非零 yaw 外参逆变换、TF 合成及拒绝过期 TF 检查通过。未启动底盘、雷达或运动控制，未实车复测；未提交或推送。
- L1 底层入口随后统一改名为 `l1_drive.launch`，本机及 NUC 独立 L1 工作区已同步；NUC `roslaunch --nodes` 解析出雷达、FAST-LIO、底盘和静态 TF 四个节点，未实际启动。

## L1 bringup 默认启动速度转发（2026-11-18）

- 本机 L1 `l1_drive.launch` 默认启动原有四个底层节点和 `base_controller_node`，不启动上层控制程序。已声明 `robot_nav` 运行依赖并更新启动文档。
- 本机 XML 与启动链静态展开检查通过：默认五个节点，转发仍加载原私有 YAML，关闭硬件启动参数时仍保留转发。本轮未更新 NUC 或进行实车验证；L2 未改动。

## Livox MID360 独立工作区（2026-11-19）

- 在 `codex/mid360-workspace` 创建本地 `BCrobot_MID360_ws`，以已部署的 L1 提交 `b067bed` 为代码基线。原 L1/L2 源码未改。WHEELTEC 包、底盘入口和 `robot_nav` 逐文件比对一致。
- 工作区随附 Livox-SDK2 v1.3.1 和 livox_ros_driver2 1.2.4；驱动使用 ROS1 包清单并随 catkin 静态链接 SDK2。FAST-LIO 恢复 Livox CustomMsg 输入、逐点时间和 MID360 参数。新增 `mid360_drive.launch`，原样复用 `l1_mount.yaml`；已与 NUC 的 L1 外参文件比对一致。
- SSH 恢复后通过校验归档部署至 NUC `/home/icral/BCrobot_MID360_ws`。在干净 Noetic 环境从零运行 `catkin_make -j2` 成功，默认五节点和关闭底盘且开启 RViz 的入口解析通过。四个 C++ 可执行文件无缺失动态库；构建日志在 `log/catkin_make.log`。
- 在独立 ROS master 上关闭雷达和底盘，确认 FAST-LIO 的点云与 IMU 订阅类型、里程计话题注册、复用的安装 TF 以及转发节点无输入时的零速度输出。NUC 有线端口无载波及 IPv4 地址，实际网口配置、雷达收包、定位建图和车体运动待实测。
