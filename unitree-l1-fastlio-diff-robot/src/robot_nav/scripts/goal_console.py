#!/usr/bin/env python3
"""Submit successive position goals from one terminal."""

import math
import signal
import subprocess
import sys


def parse_goal(text):
    fields = text.replace(',', ' ').replace('，', ' ').split()
    if len(fields) not in (2, 3):
        raise ValueError('请输入 x y 或 x y yaw，例如 0.30 0.00')
    try:
        values = tuple(float(field) for field in fields)
    except ValueError:
        raise ValueError('目标坐标和航向必须是有效数字') from None
    if not all(map(math.isfinite, values)):
        raise ValueError('目标坐标和航向必须是有限数字')
    return tuple(map(str, values))


def submit_goal(values):
    command = ['rosrun', 'robot_nav', 'pose_goal.py', *values]
    with subprocess.Popen(command, stdin=subprocess.DEVNULL) as goal:
        try:
            return goal.wait()
        except KeyboardInterrupt:
            goal.send_signal(signal.SIGINT)
            try:
                goal.wait(timeout=2)
            except subprocess.TimeoutExpired:
                goal.kill()
                goal.wait()
            raise


def interrupt(*_):
    raise KeyboardInterrupt


def main():
    if not sys.stdin.isatty():
        print('请从交互终端运行目标控制。通过 SSH 执行远程命令时请用 ssh -t。',
              file=sys.stderr, flush=True)
        return 1
    for sig in (signal.SIGHUP, signal.SIGTERM):
        signal.signal(sig, interrupt)
    print('输入目标 x y [yaw]。坐标为 camera_init 下的绝对坐标，单位米；yaw 单位度。')
    print('示例 0.30 0.00。输入 q 退出；运动中 Ctrl+C 停车并退出。', flush=True)
    try:
        while True:
            line = input('目标> ').strip()
            if line.lower() in {'q', 'quit', 'exit'}:
                return 0
            if not line:
                continue
            try:
                values = parse_goal(line)
            except ValueError as error:
                print(error, flush=True)
                continue
            if submit_goal(values):
                print('当前目标已结束，请查看上面的原因。', flush=True)
    except (EOFError, KeyboardInterrupt):
        print('\n已退出目标控制。', flush=True)
        return 0
    except OSError as error:
        print('无法启动目标控制：{}'.format(error), file=sys.stderr, flush=True)
        return 1


if __name__ == '__main__':
    sys.exit(main())
