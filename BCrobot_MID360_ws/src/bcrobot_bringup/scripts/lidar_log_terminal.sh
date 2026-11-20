#!/usr/bin/env bash
set -e

# 雷达节点的标准输出和错误写入同一个文件。主终端只打印文件位置。
log_dir=${ROS_LOG_DIR:-${ROS_HOME:-$HOME/.ros}/log}
mkdir -p -- "$log_dir"
log_file=$(mktemp "$log_dir/mid360-sdk.XXXXXX")
printf 'MID360 SDK log: %s\n' "$log_file"

# 从 SSH 启动时，尝试使用当前用户已登录的 NUC 图形桌面。
if [[ -z ${DISPLAY:-} && -z ${WAYLAND_DISPLAY:-} ]]; then
    desktop_env=$(systemctl --user show-environment 2>/dev/null) || desktop_env=
    export DISPLAY=$(sed -n 's/^DISPLAY=//p' <<< "$desktop_env")
    export XAUTHORITY=$(sed -n 's/^XAUTHORITY=//p' <<< "$desktop_env")
    export WAYLAND_DISPLAY=$(sed -n 's/^WAYLAND_DISPLAY=//p' <<< "$desktop_env")
fi

# 关闭终端会向查看脚本发送 HUP。转发 SIGINT 给本次驱动，走 ROS 正常退出流程。
# 驱动先退出时，tail 通过 --pid 自动结束，终端随之关闭。
if [[ -n ${DISPLAY:-}${WAYLAND_DISPLAY:-} ]] && command -v gnome-terminal >/dev/null; then
    gnome-terminal --window --title="MID360 SDK" -- bash -c '
        driver_pid=$1
        trap "kill -INT $driver_pid 2>/dev/null || true; exit 0" HUP INT TERM
        # Bash 在 wait 期间可立即执行信号处理，tail 同时跟踪驱动是否退出。
        tail --pid="$driver_pid" -n +1 -f -- "$2" &
        wait "$!"
    ' lidar-log-viewer "$$" "$log_file" ||
        printf 'MID360 SDK: 日志窗口未能打开，请读取上面的文件。\n' >&2
fi

# 替换当前进程，使 roslaunch 仍直接管理驱动的信号和退出状态。
exec stdbuf -oL -eL "$@" >> "$log_file" 2>&1
