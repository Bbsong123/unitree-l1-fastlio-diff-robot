#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
@file send_goal.py
@brief 向 robot_nav 发送测试目标点
用法:
  rosrun robot_nav send_goal.py [x] [y] [yaw_deg]
示例 (向前 1 米):
  rosrun robot_nav send_goal.py 1.0 0.0 0
"""

import sys
import math
import rospy
from geometry_msgs.msg import PoseStamped
import tf.transformations

def main():
    rospy.init_node('send_goal_client', anonymous=True)
    pub = rospy.Publisher('/desire_position', PoseStamped, queue_size=1, latch=True)

    # 默认目标点：向前 1.0 米
    target_x = 1.0
    target_y = 0.0
    yaw_deg = 0.0

    if len(sys.argv) >= 3:
        target_x = float(sys.argv[1])
        target_y = float(sys.argv[2])
    if len(sys.argv) >= 4:
        yaw_deg = float(sys.argv[3])

    rospy.sleep(0.5)

    goal = PoseStamped()
    goal.header.stamp = rospy.Time.now()
    goal.header.frame_id = "camera_init"

    goal.pose.position.x = target_x
    goal.pose.position.y = target_y
    goal.pose.position.z = 0.0

    yaw_rad = math.radians(yaw_deg)
    q = tf.transformations.quaternion_from_euler(0, 0, yaw_rad)
    goal.pose.orientation.x = q[0]
    goal.pose.orientation.y = q[1]
    goal.pose.orientation.z = q[2]
    goal.pose.orientation.w = q[3]

    pub.publish(goal)
    rospy.loginfo(f">>> 成功发布目标点 -> X: {target_x:.2f} m, Y: {target_y:.2f} m, Yaw: {yaw_deg:.1f}°")
    rospy.sleep(0.5)

if __name__ == '__main__':
    try:
        main()
    except rospy.ROSInterruptException:
        pass
