#!/usr/bin/env python3
"""先让相机对准人，按 F 后再让全向底盘跟随位置。"""
import math
import os
from pathlib import Path
import select
import sys
import termios
import tty

from geometry_msgs.msg import PointStamped, Twist
import rospkg
import rospy
from tf2_geometry_msgs import do_transform_point
import tf2_ros
import yaml


def main():
    rospy.init_node('person_follow')
    config_path = Path(rospkg.RosPack().get_path('robot_nav')) / 'config' / 'person_follow.yaml'
    with config_path.open(encoding='utf-8') as config_file:
        config = yaml.safe_load(config_file)
    yaw_tolerance = math.radians(config['yaw_tolerance_deg'])
    pub = rospy.Publisher(config['command_topic'], Twist, queue_size=1)
    buffer = tf2_ros.Buffer()
    listener = tf2_ros.TransformListener(buffer)
    follow_position = False

    def on_position(target):
        # 视觉点在彩色相机系，x 向前、y 向左。直接用相机方位误差让镜头对准人。
        x, y = target.point.x, target.point.y
        yaw_error = math.atan2(y, x)
        command = Twist()
        if abs(yaw_error) > yaw_tolerance:
            command.angular.z = math.copysign(min(abs(yaw_error), config['max_angular_speed']), yaw_error)

        # 暂用相机系的水平距离作为跟随距离。忽略相机俯仰对该距离的影响。
        distance_error = math.hypot(x, y) - config['follow_distance']
        # 过近即沿远离目标的方向退开。前进仍需超过原来的距离容差。
        if follow_position and (distance_error < 0 or distance_error > config['distance_tolerance']):
            try:
                transform = buffer.lookup_transform('base_footprint', target.header.frame_id, target.header.stamp)
            except tf2_ros.TransformException:
                # 安装 TF 未接通时仍可对准人，平移速度保持为零。
                rospy.logwarn_throttle(5, '相机到 base_footprint 的 TF 不可用，暂时只跟随朝向')
            else:
                # 平移方向取变换后的位置在底盘平面的投影，前/左速度单位 m/s。
                point = do_transform_point(target, transform).point
                bearing = math.atan2(point.y, point.x)
                # 保留距离误差的正负，前进与后退都按同一速度上限限幅。
                speed = math.copysign(min(abs(distance_error), config['max_linear_speed']), distance_error)
                command.linear.x = speed * math.cos(bearing)
                command.linear.y = speed * math.sin(bearing)

        # 每个新视觉点产生一条 Twist。视觉断流时由既有底盘转发节点停车。
        pub.publish(command)

    subscriber = rospy.Subscriber(config['position_topic'], PointStamped, on_position, queue_size=1)
    print('只跟随朝向。按 F 开启位置跟随，空格退回只跟随朝向，Ctrl+C 退出。', flush=True)
    try:
        if not sys.stdin.isatty():
            rospy.spin()
            return

        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setcbreak(fd)
            while not rospy.is_shutdown():
                if select.select([fd], [], [], 0.1)[0]:
                    key = os.read(fd, 1).lower()
                    if not key:
                        break
                    if key == b'f' and not follow_position:
                        follow_position = True
                        print('已开启位置跟随。', flush=True)
                    elif key == b' ' and follow_position:
                        follow_position = False
                        print('只跟随朝向。', flush=True)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    finally:
        subscriber.unregister()
        listener.unregister()


if __name__ == '__main__':
    main()
