# 全向轮底盘

边学边做、持续迭代的底盘项目。基于 ROS1 Noetic 的三轮全向实验代码，涵盖底盘串口通信、雷达定位和 L1 目标点控制联调；以简单、可读为先。

| 文件 | 用途 |
| --- | --- |
| [PROGRESS.md](PROGRESS.md) | 客观项目进展与验证结果 |
| [DISCUSSION.md](DISCUSSION.md) | 讨论、学习、方法探索和决策 |
| [AGENTS.md](AGENTS.md) | Codex 协作与记录规则 |
| Git | 父目录统一版本管理；旧 L1/L2 历史完整保留 |

按具体问题展开聊天，收尾时同步记录。换聊天后从文档和相关代码继续。

初始背景来源：[终于考完试了](chatgpt-conversation://6afb1db8-98ac-83ec-b12c-a299f7030857)。

代码与运行说明：

- [L1 工程](unitree-l1-fastlio-diff-robot/README.md)：NUC 独立部署在 `~/BCrobot_L1_ws`；底层和控制分开启动，键盘与定点共用 C++ 限速转发。
- [L2 并行版本](unitree-l2-fastlio-omni-robot/README.md)：父仓库中的并行代码目录；SDK2 UDP 接入，尚未实机验证。迁移前版本 `d6a79fa` 保留在 `history/l2`。


## 统一版本管理

在 Codex 中打开 `E:\csprogram\全向轮`。父目录的规范、进度、讨论和两个代码目录一起进入 Git；L1/L2 目录内不再有独立 `.git` 或工作树。

```text
全向轮/
├─ .git/                         唯一的 Git 元数据目录
├─ AGENTS.md / README.md
├─ PROGRESS.md / DISCUSSION.md
├─ unitree-l1-fastlio-diff-robot/  L1 工作空间
└─ unitree-l2-fastlio-omni-robot/  L2 工作空间
```

- `main`：保存文档与两份并行代码的统一状态；合入 L2 修改不会替换 L1 目录。
- `history/l1`：迁移前 L1 提交 `6ffc312`，保留原始目录布局和历史。
- `history/l2`：迁移前 L2 提交 `d6a79fa`，保留原始目录布局和历史。
- 旧历史分支仅用于追溯；今后开发从统一 `main` 创建 `codex/...` 分支，修改对应版本目录及相关文档，不直接在旧布局上继续开发。
- 旧提交哈希不变；本次统一提交以 L1、L2 两个旧提交为父节点，把两条历史连接到新的目录布局。旧版本的文件路径位于当时仓库根目录，没有改写成带子目录前缀的历史。

在父目录终端查看：

```powershell
git status
git log --graph --decorate --oneline --all
git show history/l1:README.md
git show history/l2:README.md
```

原 `origin` 地址保持不变，本次不推送。迁移恢复资料位于本机 `.git/legacy-code-history.bundle`、`.git/legacy-code-repo/` 和 `.git/legacy-l2-worktree-link.txt`；它们不是项目源码，不进入提交。正式提交历史已导入父仓库，后续正常克隆不依赖这些本地备份。

部署到 NUC 时只复制需要的 L1 或 L2 子目录作为单独的 catkin 工作空间；不要把整个父目录当成 ROS 工作空间，也不要同时 source 两套同名 ROS 包。
