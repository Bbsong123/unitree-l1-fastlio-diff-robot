# 讨论与学习记录

按日期和主题记录项目下的全部讨论、学习、方法探索与决策，精简保留要点。

## 2026-11-17｜探索由 L1 切换到 L2（迁移计划，未实施）

- 用户意向：可能更换为宇树 L2，先核对依赖、串口脚本和组织结构，尚未决定实际迁移或通信方式。本轮只读代码及官方资料、追加讨论；不改运行代码，不更新项目进展。
- 本地核对：当前 `src/unilidar_sdk1/` 包含 L1 SDK 与 `unitree_lidar_ros`；`robot_bringup/mapping.launch` 经 `fast_lio/mapping_unilidar_l1.launch` 同时启动驱动和定位。`setup_ports.sh` 同时处理底盘 PID 55d4 与雷达 PID 55d3，不能因更换雷达删除整个脚本。Git 已有此前未提交修改，本轮不覆盖。
- 官方资料核对：L2 使用 [unilidar_sdk2](https://github.com/unitreerobotics/unilidar_sdk2)，同时提供 ROS1/ROS2；支持 UDP 与串口，常用标准 3D+IMU 模式下 work_mode 分别为 0/8。建议先按 UDP 接入，仍待用户及实物条件确认，无需因 L2 同时迁移 ROS2。
- 官方 [L2 ROS1 配置](https://github.com/unitreerobotics/unilidar_sdk2/blob/main/unitree_lidar_ros/src/unitree_lidar_ros/config/config.yaml)：initialize_type=2、work_mode=0；雷达 IP 192.168.1.62:6101、主机 192.168.1.2:6201 是官方配置值，实物可能改过，需核对。主机键名改为 pc_ip/pc_port，串口键名为 serial_port/baudrate，官方串口波特率 4000000。不能直接复制 L1 YAML；UDP 不需要雷达串口赋权，底盘赋权仍需保留。若选串口，需重新确认 L2 USB 标识，不能沿用 L1 PID。
- 依赖方案（建议）：替换 SDK1 的头文件、静态库和 ROS1 驱动为 SDK2；SDK2 库名为 libunilidar_sdk2.a、命名空间为 unilidar_sdk2。官方 ROS1 包仍叫 unitree_lidar_ros，robot_bringup 的该依赖名通常无需改；同一 catkin 源码空间不能同时启用两代同名包。ROS Noetic、PCL、Eigen、FAST-LIO、底盘 serial 依赖继续保留，按新包 CMake/package.xml 对照安装，不引入自行编写协议层。
- 编译约束：官方 [L2 ROS1 CMake](https://github.com/unitreerobotics/unilidar_sdk2/blob/main/unitree_lidar_ros/src/unitree_lidar_ros/CMakeLists.txt) 使用 C++17，与本项目统一 C++14 有冲突。计划先核对头文件并在 NUC 以 C++14 编译和链接 x86_64 官方库；是否兼容尚未验证。不能仅改编译选项就宣称支持；若有确实依赖 C++17 的接口或二进制兼容问题，记录具体错误，再讨论版本或最小兼容修改，不擅自改项目标准。
- 结构方案（建议）：维持当前单一 catkin 工作空间与现有业务包；用 `src/unilidar_sdk2/unitree_lidar_sdk/` 和 `src/unilidar_sdk2/unitree_lidar_ros/` 替代 SDK1，保持现有扁平组织。官方 ROS1 包原在 `unitree_lidar_ros/src/unitree_lidar_ros/`，扁平整理后必须相应修正 CMake 中到 SDK 的 include/lib 相对路径；不把官方 ROS1/ROS2 两个工作空间和 build/devel 一起塞入主工作空间。若需保留 L1 回退源码，放主 src 外或用 CATKIN_IGNORE，不能留下同名活动包；先不改仓库名称、不引入通用多雷达框架。
- 具体计划：新增 `fast_lio/config/unilidar_l2.yaml`、`fast_lio/launch/mapping_unilidar_l2.launch`；更新 `robot_bringup/launch/mapping.launch` 指向 L2，保留现有一键启动接口；调整端口脚本、安装 TF 与 README 部署说明。L2 驱动可保留 /unilidar/cloud、/unilidar/imu 和现有 frame 名，底盘及 robot_nav 无需随雷达迁移改逻辑。L1 配置可暂留作回退，标明不启用。
- FAST-LIO 兼容性判断：本地已有 lidar_type=5 和 x/y/z/intensity/ring(uint16)/time(float) 解析，与官方 [SDK2 点类型](https://github.com/unitreerobotics/unilidar_sdk2/blob/main/unitree_lidar_sdk/include/unitree_lidar_sdk_pcl.h) 字段匹配，故建议先复用，尚不代表定位已兼容。官方 [Point-LIO L2 配置](https://github.com/unitreerobotics/point_lio_unilidar/blob/main/config/unilidar_l2.yaml) 同样用 scan_line=18、timestamp_unit=0，不能因为换代自行猜改线数。要实测点相对时间、排序、帧时长、点云与 IMU 时间轴；Point-LIO 参数不能整份当 FAST-LIO 配置使用。
- 时间与 TF 注意：官方 [ROS1 驱动头文件](https://github.com/unitreerobotics/unilidar_sdk2/blob/main/unitree_lidar_ros/src/unitree_lidar_ros/include/unitree_lidar_ros.h) 中 IMU stamp 取 ros::Time::now()，点云 stamp 取 cloud.stamp；use_system_timestamp=true 并不能免除实测时间偏差。驱动还发布 IMU→雷达 TF，与本项目 base_link→同一雷达 frame 可能产生双父节点，需保留唯一 TF 发布关系，优先用最小修改关闭驱动中冲突的发布。L2 官方内部平移数值与当前配置一致，也仍需按算法外参定义核对方向；底盘→雷达安装外参必须重新测量，不能照搬零占位。
- 验证顺序：①确认实物接线、供电、通信模式、NUC 网卡/IP 与固件版本；②独立验证 SDK2 ROS1 驱动的 C++14 编译及点云/IMU输出；③检查字段、时间轴、单位、frame 与唯一 TF；④接 FAST-LIO，静止初始化后低速移动建图，观察 /Odometry、点云、畸变及 NUC 负载；⑤再用一键启动检查底盘和雷达并行数据流。当前 Windows 无 ROS/实物，以上运行验证均未完成。
- 备选：若现有 FAST-LIO 适配失败且修复范围明显增大，再评估官方 [point_lio_unilidar](https://github.com/unitreerobotics/point_lio_unilidar) 的 L2 路径；当前不同时更换雷达与定位算法，以便分辨问题来源。

## 2026-11-17｜初始设想整理

- 来源：此前聊天[终于考完试了](chatgpt-conversation://6afb1db8-98ac-83ec-b12c-a299f7030857)，本日整理。
- 用户设想：近期约一个月参与全向轮底盘开发，涉及上位机与单片机通信、底层驱动、定位导航、跨部门路径规划对接和性能优化；长期边学边做，可能频繁调整方案。
- 用户诉求：保留讨论历史和代码修改历史，组织方式保持简单。
- 初始方法：聊天探索，Markdown 沉淀，Git 留下文件改动；按具体问题组织聊天，目录按实际需要创建。
- 取舍：仅靠聊天不便查找结论，当前也无需复杂任务系统；资料和实验增多后再调整组织方式。
- 助手建议（未定案）：资料与接口梳理 → 驱动及通信闭环 → 定位导航 → 对接与优化。先做最小实验，验证后推进；通过条件应包含可复现的通信与反馈、运动和停止机制验证、约定场景下的导航及性能复测。
- 待了解：实物与已有代码、轮型及尺寸、电机编码器、单片机驱动器、上位机环境、通信协议、传感器、部门分工及验收指标。
- 未决定：通信方式、软件框架、导航方案、运动学模型。UART、CAN、ROS 2 等仅在此前建议中出现，均未采纳；没有相关技术验证结果。

## 2026-11-17｜调整记录方式

- 用户要求：说明尽量精简；`PROGRESS.md` 只放客观项目进展；项目下的讨论、学习和方法探索全部放入讨论记录。
- 已调整：原决策记录改名为 `DISCUSSION.md`，纳入讨论、学习、探索及决策；同步更新入口和协作规则。
- 记录方式：按日期和主题追加，保留问题、要点、理由、结论及未解决事项；新证据推翻旧方案时保留改变原因。

## 2026-11-17｜由四驱差速迁移到三轮全向

- 用户提供旧项目 `https://github.com/Bbsong123/unitree-l1-fastlio-diff-robot.git`，要求克隆、改为三轮全向控制并同步文档；明确统一 C++14，去掉 `std::clamp`。
- 用户说明：更换为照片所示全向底盘，认为单片机已写好对应程序。按此条件实现上位机；控制板固件、串口协议、具体型号尚未实测，不能把用户判断记为验证结果。
- 代码核对：官方 `turn_on_wheeltec_robot` 有 mini/senior/top_omni 三轮模型，串口已收发 vx/vy/wz。当前启动链的 car_mode 选择模型与 TF，不会下发切换单片机车型；原自写转发器丢弃 vy，跟踪器采用先转后走的差速逻辑。
- 实施决定：保留官方串口协议，补全 vy；平面向量统一限速；将世界坐标位置误差旋转到车体后进行比例控制，朝向独立跟踪，距离与朝向共同判定到达。保留 const_* 参数名但语义改为速度上限，删除 angle_align_thresh。
- 修复伴随问题：YAML 原根键与私有参数读取位置不符，原 std::clamp 不兼容 C++14；现改为节点名对应命名空间与 std::min/max。增加定位过期/断流停车并取消目标，恢复需重新发目标。
- 默认型号 senior_omni 是助手暂定值，未由照片确认；实际可切换 mini_omni / senior_omni / top_omni。新底盘 USB 标识与供电参数也待确认。
- 坐标系：FAST-LIO 输出 body 为雷达 IMU 系，并非底盘中心。增加 body_to_base_x/y/yaw 平面外参，默认零仅作占位，必须与 base_link→雷达 TF 和雷达内部外参保持一致。倾斜安装需后续完整三维转换。
- 验证边界：独立 C++14 控制数学测试及静态检查已通过，ROS 集成测试已编写但本机无 ROS/可用 WSL，未运行。跟踪器只做定点控制，没有规划或避障；实车运动方向、停车和通信仍待验证。
- 克隆首试受沙箱网络错误阻断；按用户授权提升执行权限后克隆成功。未提交、推送或改写 Git 历史。

## 2026-11-17｜按实验用途精简，只保留基础数据流

- 用户明确：部署在额外的 NUC，全部代码为实验性质，简单与可读性优先；当前不需要完整工程体系、任何看门狗或上层控制算法，要求详细中文注释，并在每次代码修改后详细说明实际改动。
- 此要求取代前一轮助手增加全向跟踪、看门狗、增益配置和测试体系的方案。前一轮记录保留作为历史，不再代表当前实现。
- 决定：保留原 robot_nav 包和 nav.launch 入口以减少改名，仅保留一个直观的 C++14 速度回调；逐条转发车体 vx、vy、wz，不做限速或平滑。官方串口驱动处理通信，单片机处理三轮运动学。
- 实施：删除 point_tracker_node.cpp、send_goal.py、nav_params.yaml、omni_control.h 和 test/，移除对应构建、参数加载与依赖。雷达/FAST-LIO 仅输出数据，不参与底盘控制。官方包附带的其他功能不作批量重写，当前启动入口不启用其上层控制功能。
- 无看门狗后的明确行为：停止输入消息不会由转发器补发零速度；实验操作以显式零速度指令结束，单片机断联策略尚未验证。
- 编写规则写入父目录 AGENTS.md，并在代码仓库新增同样的实验准则，方便代码独立复制到 NUC 后继续遵循；README 改为 NUC 部署及基础数据流说明。
- 验证：静态配置、转发字段、删除项和引用检查通过；未编写新测试体系，当前机器不能执行 ROS 编译或联调。

## 2026-11-17｜明确增加基础停车和 YAML 配置

- 用户本轮明确要求：base_controller_node 增加停车函数，没有有效 Twist 时向 /cmd_vel 发零；YAML 配置输入话题、转发话题、速度与角速度限制。这取代上一轮“无自动停车和限速”的实现，继续保持简单代码和详细中文注释。
- 实施：stopRobot() 发布全零 Twist；只验证底盘使用的 vx/vy/wz 为有限数值，空指针或无效数值立即停车；启动后没有有效输入也定时发零。有效消息恢复后立即限速转发，零速度属于有效输入。
- “没有收到”需时间界限：增加 command_timeout，默认 0.5 秒；使用墙钟时间按 20Hz 检查，避免正常消息间隔引起停车。Twist 无消息时间戳，只能依据本节点接收时间判断。
- 限速含义：max_linear_speed 为平面合速度上限，等比例缩小 vx/vy 保留方向；max_angular_speed 独立限制 wz。用 C++14 min/max，未恢复上层算法、测试目录或通用框架。
- 新增 config/base_controller.yaml，在 launch 节点内部加载到私有命名空间，CMake 安装配置目录。更新 README、父目录和代码仓库规则，使当前允许的基础功能与最新要求一致。
- 静态配置与差异检查通过；当前 Windows 无 ROS，尚未在 NUC 编译运行或进行实车停车验证。节点退出或串口断链后的单片机行为仍由其固件决定，未验证。


## 2026-11-17｜按用户确认建立 L2 并行分支，不改 L1

- 用户纠正并确认：现有 L1 雷达建图数据流已实机验证，L1 代码必须保持原样；另建文件夹，仿照当前项目实现 L2，以原仓库独立分支维护，不作为替代 L1 的迭代。本条取代上轮“修改原工作空间迁移”的建议；用户确认的是雷达建图链路，不自动扩展为近期底盘控制修改也已实测。
- 用户确定 L2 网口接入，系统网卡由用户自行配置；工程只保留 SDK 使用的 IP/端口参数，不新增网络配置脚本。重点核对点类型、升级 SDK2 及启动接口，世界坐标根来自 FAST-LIO，安装 TF 改为雷达→底盘。
- Git 实施：从原仓库 HEAD 4faba28 创建独立工作树和 codex/l2-fastlio-omni 分支，放入 unitree-l2-fastlio-omni-robot/；再复制 L1 当前未提交工作区（包括底盘简化修改），不撤走或覆盖 L1 的改动。原 main 保持原样；新分支未提交、未推送。跨 C/E 磁盘直接 git worktree move 失败（Improper link），改为受检查的复制、git worktree repair 和重新挂接，已验证两目录分别对应 main/L2 分支；旧空白工作树清理。
- SDK 来源：实际 git clone 官方 https://github.com/unitreerobotics/unilidar_sdk2，固定提交 0e3c51f512e6b8ff60b8c32f160b412cb48445c2，头文件版本 2.0.9；采用官方 SDK、预编译静态库及 ROS1 包，保留许可证与来源记录。未采用 ROS2 或另换定位算法。首次默认沙箱联网失败，提升执行后下载成功。
- 点格式源码核对：SDK2 内部 PointUnitree 的 ring 为 uint32，但 unitree_lidar_sdk_pcl.h 输出 PointCloud2 的 ring 为 uint16，x/y/z/intensity/time 为 float。与当前 preprocess.h 注册类型逐项一致；time 是相对 cloud.stamp 的秒，原 unilidar_handler 乘 1000 存入 curvature 的逻辑可直接沿用。结论是格式无需修改，不代表 L2 运行效果已经验证；全部 FAST-LIO src/include 保持字节一致。
- 保留 lidar_type=5、timestamp_unit=0、官方聚合扫描数18，以及原定位参数。独立新增 L2 YAML/launch，L2 分支删除 L1 专用配置与 SDK1，避免误启动；原 L1 文件保留。SDK2 包扁平整理，与 SDK 的相对路径改为 ../unitree_lidar_sdk，库为 libunilidar_sdk2.a；ROS 包名仍为 unitree_lidar_ros，不可在同一 ROS 环境同时启用 L1/L2。
- 内部外参以 SDK2 ROS1 驱动源码为准：IMU→雷达平移 [0.007698,0.014655,-0.00667]m、单位旋转，与 FAST-LIO p_IMU=R*p_lidar+T 相同，保留。只去掉驱动的 imu_initial→imu 姿态 TF，将 imu_frame 配为 body，让 FAST-LIO 的 camera_init→body 唯一给出世界位姿，再保留驱动 body→unilidar_lidar，最后发布 unilidar_lidar→base_link。此实现修正上轮“可能关闭内部 TF”的探索方向，不凭空重估雷达内部外参。
- 安装外参现表示“底盘原点在雷达系中的位置与姿态”，单位米/弧度，仍为未测量零占位。由旧底盘→雷达外参换方向时应使用 R'=R^T、t'=-R^T*t，不能只换父子名或只对欧拉角取负。
- 为落实 FAST-LIO 世界根，L2 chassis.launch 直接启动官方 base_serial.launch 和三轮固定关节 URDF；不再调用带旧轮式 EKF/额外 TF 的整套底盘 launch。base_footprint→base_link 零变换反向为 base_link→base_footprint；底盘 /odom、/imu、/PowerVoltage 反馈及速度协议保留，轮式 /odom 不作为 TF 世界根。未改官方底盘包和 robot_nav；模型和底盘板载 IMU 的安装占位继承原状态。
- C++14 探索：官方 SDK/ROS CMake 使用 C++17，现本地配置为 C++14，头文件与静态库保持官方内容。SDK 公共接口头文件的 MinGW C++14 语法初查因 Windows 未启用 M_PI 失败；检查命令增加 _USE_MATH_DEFINES 后通过，未为 Windows 检查而改 SDK 源码。这仅验证公共头文件语法，不包含 ROS/PCL 编译或 Linux 库链接。
- 验证：原 L1 文件哈希及 Git 工作状态无变化；FAST-LIO src/include、robot_nav、官方底盘包逐文件相同；活动 launch XML、YAML、Python 语法、引用路径、唯一 ROS 包名、点字段类型、UDP 参数、内部外参一致性、TF 方向及官方库复制完整性检查通过。底盘脚本 bash -n 首次因 MSYS 沙箱权限无法启动，提升执行后语法通过；git diff --check 通过。未建立测试目录或运行实机流程。
- 剩余事项：NUC 编译链接验证 C++14/ROS/PCL 与官方二进制库兼容性；用户配置系统网卡并同步驱动地址，测量安装外参；实机确认点云/IMU时序、唯一 TF 和定位效果。时间戳沿用官方实现（点云 cloud.stamp、IMU 接收时刻，use_system_timestamp=true），未新增补偿算法。使用 L2 README 的独立工作空间命令，不同时 source L1 与 L2。


## 2026-11-17｜分别提交 L1/L2，并增加提交询问规则

- 用户明确授权本次提交：L1 保存于 main（6ffc312），L2 保存于 codex/l2-fastlio-omni（d6a79fa）；未合并分支、未改写历史、未推送。
- 用户新要求：今后只要有代码功能改动，完成改动和必要检查后必须主动询问是否提交版本管理；已明确授权时不重复询问，提交不等于授权推送。已写入项目全局 AGENTS.md。
- 本次只保存已有代码版本，另更新 L2 README 的提交状态及全局文档；未增加运行功能。提交前通过差异检查（L2 导入的官方 SDK 文件未纳入新增代码空白检查），提交后两个代码工作区干净。NUC 编译与实机验证边界沿用先前记录。


## 2026-11-17｜用户确认父目录统一版本管理

- 问题：用户希望在 Codex 打开项目父目录，同时查看代码、规范与进度的版本。此前父目录空仓库与代码子仓库分离，且聊天关联残留已不存在的 C 盘工作树，造成查看入口混乱。
- 用户明确授权：父目录作为统一 Git 仓库，L1/L2 两个目录并行保留，现有提交和完整历史都保留，整理后直接提交。本次无需重复询问提交，不授权推送。
- 决定：两个硬件版本以普通目录并存，统一 main 保存完整项目状态；以后从统一 main 创建 codex/ 功能分支。此决定替代“L1/L2 分别作为独立工作树及长期硬件分支”的旧组织方式；旧记录和旧提交保留。
- 历史处理：原仓库 4 个提交原样导入父仓库，保留 history/l1=6ffc312、history/l2=d6a79fa。统一 main 的新目录树以两旧版本提交为父节点创建合并提交；不使用过滤历史或重新生成旧提交，因此原 ID 不变。原 origin 和远端跟踪引用保留。
- 防止丢失：先记录两个版本的全部文件哈希及旧提交清单，创建并验证完整 bundle，再封存原 Git 元数据和 L2 链接到父仓库 .git 内。仅代码目录内的 .git 管理关系退出，代码文件保持原位；备份不加入源码。
- 实施中的处理：PowerShell Move-Item 首次已搬完 Git 元数据，但因源目录 Hidden 属性无法移除空目录而报错；后续确认归档 HEAD 正确、源目录为空后才移除空壳，未删除历史。核对脚本初次遇到 Windows 默认 GBK 解码 Git 中文路径失败，改为显式 UTF-8 后通过。
- 文档调整：父目录及两版本 README/AGENTS 说明统一 Git 根、旧历史入口和后续分支用法；两个版本的 NUC catkin 工作空间仍需分别部署。补充父目录忽略规则及脚本 LF 约定，未修改运行代码或控制功能。
- 验证：L1 191/L2 177 个文件的路径集合与迁移前相同；除各自 README/AGENTS 外，全部文件原始哈希一致。两个子目录 Git 根均解析为父目录；所有旧提交对象、历史分支、bundle 和父仓库对象完整性通过。未进行新 ROS 编译或实机测试，此次只整理版本管理。

## 2026-11-18｜NUC BCrobot_ws 与本机 L1/L2 对照及合并建议

- 问题：确定 NUC 最近使用的启动链，判断是否把本机旧代码合入部署。以现有源码、生成产物和历史日志区分部署、构建和实测；文档中旧的“L2 尚未接线”不能代表后续日志状态。本次只调查，未授权合并或部署。
- NUC 主入口是 `bcrobot_bringup`。L1：`bcrobot_l1_driver` → `/unilidar/cloud`、`/unilidar/imu` → FAST-LIO `/Odometry`（`body`）；L2：UDP 集成或显式串口入口 → SDK2 → `unitree_l2_converter` 将点云转成 `/livox/lidar` → FAST-LIVO2（视觉关闭）→ `/bcrobot/lio/odom`（`aft_mapped`）。两者由 `base_pose.py` 应用各自估计外参，发布启动局部坐标系下的 `/bcrobot/base_pose`；`/odom` 另保留为轮式反馈，不混用。
- NUC 运动链：终端目标 → `pose_goal.py` → `/bcrobot/position_velocity`（`TwistStamped`）→ `velocity_guard.py` → `/cmd_vel`（`Twist`）→ WHEELTEC 串口；集成入口默认禁用自动运动。`keyboard.py` 直接发 `/cmd_vel`，不能与启用的目标控制同时使用。本机 `robot_nav` 只转发 `/target_cmd_vel`（`Twist`）并限速、超时持续发零，与现有看门狗接口和发零行为不同；并跑会竞争同一个输出。
- 文件比对（常规文件 SHA-256；文本统一换行，生成文件不作合并依据）：对本机 L1，FAST-LIO 同路径 43 个相同、2 个不同（CMake 生成目标依赖、launch 驱动包名）；SDK1 同路径 45 个相同、5 个不同（包名、启动引用和串口默认值）；WHEELTEC 同路径 76 个相同、2 个不同（编译标准、整包 launch 的车型默认值），官方串口源文件一致。NUC 多出的地图、车型导航参数和备份不是当前最小入口依赖。NUC L1/底盘使用独立 `/dev/serial/by-id`，底盘入口不拉起本机 L1 的整套模型/EKF；两端安装参数与 TF 不能直接互换。
- 本机 L2 是 C++14 的 SDK2 UDP→FAST-LIO 直连候选；NUC 另用 `fast_ws` 的 SDK2、转换器、FAST-LIVO2。对齐 SDK2 目录后，25 个文件相同、4 个内容不同（两处 CMake、驱动参数和 TF 头文件）；NUC 外置 SDK2/FAST-LIVO2 的构建配置为 C++17。不要把外部依赖全量带入，或用本机驱动/TF 直接覆盖 NUC 链路。
- 建议一：先对 NUC 已用源码做可追溯快照，从父仓库 `main` 派生 `codex/nuc-deployment-baseline`，保持 `BCrobot_ws` 和本机两个历史目录原样。纳入 `bcrobot_bringup`、L1 的少量实车部署差异、必要入口及复现说明；单独记录 `fast_ws` 实际所需包、依赖、源码来源/指纹和 C++17 边界。排除 `build/`、`devel/`、日志、`.bak`、无关导航参数和构建缓存。快照后再单独评审 L1 的依赖补丁及硬件参数是否回填，不改写 `history/l1` 或 `history/l2`。
- 建议二：不默认迁入旧 C++ 速度节点；只有上游确实需要 `/target_cmd_vel` 时，才单独定义手动/外部速度与自动目标的互斥，保证实际运行只有一个 `/cmd_vel` 发布者，不恢复已移除的旧跟踪器。L2 暂保留两个定位方案，关闭运动、分别复测点云/IMU、时间戳、安装坐标、静置漂移和连续性；查清跳变后再决定是否替换，不能以有输出或旧 L1 停车记录作为验收。

## 2026-11-18｜L1 对照：底层复用与控制层差异

- 用户要求暂只看 L1，确认底层是否一致、NUC 是否只是用 Python 改写本机 `robot_nav`。沿用前次文件比对：FAST-LIO 算法与 SDK1、WHEELTEC 的核心驱动源码一致，整包并非完全相同；部署差异集中在包名、串口、CMake 和 launch，NUC 底盘入口只拉起串口驱动。
- NUC 的 `bcrobot_bringup` 整合启停入口；Python 看门狗与本机速度转发节点职责部分重叠，但输入为位置控制器的 `TwistStamped`，不是本机的外部 `Twist` 接口。NUC 另有车体位姿换算、目标点闭环和键盘联调，不是等价改写或单纯精简。后续若只同步 L1，保留共用底层，按文件核对部署差异；本次仅澄清，未合并。

## 2026-11-18｜用户确认直接覆盖本机 L1 启动层

- 用户确认：以 NUC 目前的 L1 覆写本机 L1，作为版本迭代，而非另保一个部署目录；旧 `robot_nav` 保留不默认启动，其共用 `/cmd_vel` 的冲突只说明，不加仲裁；不修改 L2 和 NUC。该做法替代先建独立部署目录的建议，旧版本继续由 Git 历史保留。
- 取舍：运行所需入口与 Python 实现以 NUC 文件为准，保留新发现的 `l1_goal.launch`，排除 L2 专用脚本和无关旧导航资产；本机构建仍用 C++14，不回退到 NUC 厂商包的 C++11。仅适配 L1 包的依赖和安装清单、独立试验日志路径等工作空间差异。键盘与对齐脚本用于联调；位姿换算、目标控制和看门狗会参与运行，不只是测试。
- 并发边界：旧转发器、键盘和启用运动的看门狗都可写 `/cmd_vel`；旧转发器无输入时的周期零速度也会干扰自动控制。当前仅使用一种运动入口；没有变更上述策略，也未验证实车失联停车。

## 2026-11-18｜重新规划 L1：启动与控制分包

- 用户反馈：同步后职责堆在 `bcrobot_bringup`，倾向恢复独立 `robot_nav`，不需要独立看门狗；本轮讨论方案，不改代码。
- 建议：`bcrobot_bringup` 只启动 SDK1、FAST-LIO 和底盘；`robot_nav` 管车体位姿换算（含安装配置）和目标点控制。保留两个分开启动的入口，默认只开底层时不发运动指令；键盘、对齐、终端目标输入留作手动调用的联调工具，旧一键前进脚本不纳入默认流程。
- 建议：取消独立 `velocity_guard.py` 及中间速度话题，目标节点直接向 `/cmd_vel` 发布限幅后的 `Twist`；保留失去有效定位、到位或正常退出时的基本发零。旧 C++ 转发器暂留但不默认启动，不与目标或键盘并跑，不新增仲裁。失去进程、SSH 或串口时不能保证软件发零，主控是否自行停车仍未验证，不能用软件替代硬件急停。
- 待确认：是否继续保留目标点闭环和交互终端，以及是否按上述结构改本机 L1；不涉及 L2 或 NUC 部署。

## 2026-11-18｜L1 方案修订：保留 C++ 速度中间层

- 用户确认：`bcrobot_bringup` 只管底层启动；保留 `robot_nav` 的隔离和转发，键盘与定点控制用 Python 并入 `robot_nav`。本轮只评审，不实施。现有 `base_controller_node` 与 WHEELTEC `wheeltec_robot_node` 都是 C++。
- 修订建议：Python 手动或定点控制二选一，只向 `/target_cmd_vel` 发 `Twist`；现有 C++ 节点继续限幅、转发和无有效指令发零，是项目内唯一运动链的 `/cmd_vel` 发布者。取消独立 Python 看门狗及其专用话题，保留 C++ 原有超时停车，不让上层绕过中间层。
- 计划：把定位换算及 L1 安装参数、定点控制、键盘与终端/校准工具纳入 `robot_nav`。定点算法先不重写，只适配速度接口、去除对看门狗的依赖，保留定位失效/正常结束发零。底层和转发分开启动，上层脚本手动选择，不加仲裁。旧短程试验脚本依赖旧链路，不纳入新默认流程；是否改造留待后续。
- 待用户评审方案后再改本机 L1；L2 和 NUC 部署不在范围内。

## 2026-11-18｜L1 安装外参与启动原点

- 用户询问旧外参 YAML 和 NUC 位姿代码为何复杂。核对 `history/l1` 与 `4faba28`：旧安装外参在 `robot_bringup/config/static_tf.yaml`，为底盘到雷达的零值占位；旧 `robot_nav/config/nav_params.yaml` 管话题与控制参数，不管安装外参。
- 固定安装关系只需一份 4×4 刚体变换；NUC 的 `l1_mount.yaml` 已用 xyz 和 yaw 表达 IMU 在底盘中的固定关系，不能直接与旧底盘到雷达的 YAML 或 FAST-LIO 内部雷达—IMU 外参互换。启动时车头对齐和原点归零是另一道换算；检查原始点云/IMU、等待及跳变锁定是联调附加逻辑，不是外参所需。
- 建议：安装外参保留单一 YAML，代码生成矩阵；位姿节点只订阅 `/Odometry`，做帧与基本有效性检查、换算车体位姿，按是否需要启动相对目标决定是否归零。定点侧保留定位过期判断，C++ 转发逻辑不变；尚未修改或验证该精简方案。

## 2026-11-18｜L1 坐标直接沿用 camera_init

- 用户明确：世界根用 FAST-LIO 的 `camera_init`，不再归零；人工等待定位稳定后才运行算法，不需要 NUC 的原始点云/IMU 联调检查。源码确认 FAST-LIO 同时发布 `/Odometry` 和动态 `camera_init → body`，其中 `body` 是 IMU 坐标系。
- 建议：只新增一条 YAML 配置的 `body → base_footprint` 静态 TF，使用现有 IMU 安装参数的逆变换；定点控制直接查询合成后的车体 TF，不再另设 `bcrobot_start` 或维护 `base_pose.py` 的重复位姿链。目标按 `camera_init` 绝对坐标解释；轮式 `/odom` 不参与该 TF 树，避免给车体重复父帧。
- 人工等待只覆盖启动；建议定点节点只用有效且新鲜的定位，丢失后不再输出运动指令，由现有 C++ 中间层超时发零。不增加独立看门狗、等待窗口或跳变锁定。仅记录方案，尚未修改运行代码。

## 2026-11-18｜执行 L1 分包与 NUC 独立部署

- 用户确认实施前述拆分及 TF 方案，并更正授权同时更新 NUC，允许清理旧编译缓存与日志；L2 仍不改。
- 因旧 `BCrobot_ws` 也提供 L2 的启动入口，选择部署至独立 `BCrobot_L1_ws`，从零构建，不清理共享缓存或日志。运行时 source 新工作区，不叠加旧版或 L2。
- 保留定点的基本速度计算、到位和超时行为，不引入额外联调逻辑；人工只启动一种上层控制。旧对齐和前进试验脚本依赖旧位姿/控制接口，未继续移植。
- 验证过程：离线 TF 初查时默认 Buffer 尝试访问 ROS Master，改用离线 `debug=False` 重测通过，应用启动逻辑不变。实际串口、车体方向及闭环效果仍待现场验证。
- 用户确认入口改名为 `l1_drive.launch` 并提交此次 L1 迭代；不保留旧入口别名，不推送。

## 2026-11-18｜L1 底层与速度转发一键启动

- 用户确认不再分开启动底层和转发：`l1_drive.launch` 直接包含仅启动 C++ 转发的 `robot_nav/nav.launch`，避免重复维护节点及参数。上层算法仍手动选择；不改速度限幅、超时发零和话题配置。

## 2026-11-19｜L1 更换 Livox MID360

- 用户要求从当前 L1 的 `src` 派生独立 `BCrobot_MID360_ws`，最小改动复用底盘和导航。最初因 SSH 不通计划用 U 盘拷贝并手动编译，随后用户确认直接复用 L1 安装外参，并授权尝试从 SSH 部署。撤掉初稿的零值外参 YAML，改用原 `l1_mount.yaml`。
- 选择固定版本的官方 Livox-SDK2 和 livox_ros_driver2，将 SDK 源码放在 catkin 的 `src` 外，由驱动直接编译静态链接。这样复用现有 Noetic 依赖，无需在 NUC 全局安装 Livox。Windows 的临时路径干扰了首次编译器探测；改用普通临时目录复测后发现 SDK 两个头文件依赖隐式的定宽整数声明，已补标准头文件，转到目标 NUC 验证。
- FAST-LIO 本地已有 MID360 参数和被禁用的 Livox 路径。恢复 `CustomMsg`，沿用纳秒到毫秒的逐点时间和原去畸变，保留定位 TF 与控制链。内部雷达至 IMU 外参采用上游 MID360 参数；IMU 至底盘的安装关系按用户确认原样沿用 L1 参数，其物理适用性待实车核对。
- SSH 恢复后确认目标目录尚不存在，以新工作区独立部署。NUC 有线口当前无载波和 IPv4 地址，驱动 JSON 暂用官方示例地址。实际主机与雷达 IP、收包、时间轴、建图及底盘联动待接线后验证。
