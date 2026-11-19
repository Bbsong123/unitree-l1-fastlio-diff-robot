#!/usr/bin/env python3
"""在 FAST-LIO 的 camera_init 中执行单个车体位置目标。"""
import argparse
import math
import sys
import time

from geometry_msgs.msg import Twist
import rospy
from tf.transformations import euler_from_quaternion
import tf2_ros


def finite(value):
    number = float(value)
    if not math.isfinite(number):
        raise argparse.ArgumentTypeError('参数必须是有限数值')
    return number


def base_pose(buffer):
    # 查询定位 TF 与安装 TF 的合成结果；不生成第二套坐标原点。
    pose = buffer.lookup_transform('camera_init', 'base_footprint', rospy.Time(0))
    stamp = pose.header.stamp
    if stamp.is_zero() or not -0.05 <= (rospy.Time.now() - stamp).to_sec() <= 0.40:
        raise RuntimeError('车体 TF 已过期')
    pos, rot = pose.transform.translation, pose.transform.rotation
    quat = (rot.x, rot.y, rot.z, rot.w)
    if not all(map(math.isfinite, (pos.x, pos.y, *quat))) or not 0.9 <= math.hypot(*quat) <= 1.1:
        raise RuntimeError('车体 TF 数值无效')
    return pos.x, pos.y, euler_from_quaternion(quat)[2]


def follow(buffer, publish, goal):
    target_yaw = None if goal.yaw is None else math.radians(goal.yaw % 360)
    deadline = time.monotonic() + goal.timeout
    settled_since = None
    while not rospy.is_shutdown():
        now = time.monotonic()
        if now >= deadline:
            raise RuntimeError('目标超时')
        pos_x, pos_y, yaw = base_pose(buffer)
        if target_yaw is None:
            target_yaw = yaw
        delta_x, delta_y = goal.x - pos_x, goal.y - pos_y
        distance = math.hypot(delta_x, delta_y)
        yaw_error = math.atan2(math.sin(target_yaw - yaw), math.cos(target_yaw - yaw))
        command = Twist()
        if distance > 0.03:
            # 世界坐标中的目标方位减去车头航向，得到车体前/左速度，单位 m/s。
            bearing = math.atan2(delta_y, delta_x) - yaw
            speed = min(0.12, max(0.055, 0.65 * distance))
            command.linear.x = speed * math.cos(bearing)
            command.linear.y = speed * math.sin(bearing)
        if abs(yaw_error) > math.radians(7):
            command.angular.z = math.copysign(min(0.35, abs(yaw_error)), yaw_error)

        if distance <= 0.03 and abs(yaw_error) <= math.radians(7):
            if settled_since is None:
                settled_since = now
            elif now - settled_since >= 0.6:
                print('已到达: x={:.3f} m y={:.3f} m'.format(pos_x, pos_y), flush=True)
                return
        else:
            settled_since = None
        # 这里只发期望速度；C++ 是唯一向底盘发 /cmd_vel 的节点。
        publish(command)
        time.sleep(0.05)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('x', type=finite, help='camera_init 下的 x，单位米')
    parser.add_argument('y', type=finite, help='camera_init 下的 y，单位米')
    parser.add_argument('yaw', nargs='?', type=finite, help='camera_init 下的航向，单位度')
    parser.add_argument('--timeout', type=finite, default=15, help='目标最长执行时间，单位秒')
    goal = parser.parse_args(rospy.myargv()[1:])
    if goal.timeout <= 0:
        parser.error('--timeout 必须为正数')
    rospy.init_node('l1_pose_goal', anonymous=True)
    buffer = tf2_ros.Buffer()
    listener = tf2_ros.TransformListener(buffer)
    target = rospy.Publisher('/target_cmd_vel', Twist, queue_size=1)
    try:
        # 仅给当前新建的 TF 监听器接收首帧的时间，不等待传感器稳定。
        buffer.lookup_transform('camera_init', 'base_footprint', rospy.Time(0), rospy.Duration(1.0))
        follow(buffer, target.publish, goal)
        return 0
    except (tf2_ros.TransformException, RuntimeError) as error:
        print('停止定点控制：{}'.format(error), file=sys.stderr, flush=True)
        return 1
    finally:
        if not rospy.is_shutdown():
            target.publish(Twist())
            time.sleep(0.05)
        listener.unregister()


if __name__ == '__main__':
    sys.exit(main())
