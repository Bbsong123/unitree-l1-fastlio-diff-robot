#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
@file emergency_stop.py
@brief 终端紧急停车触发与复位脚本
用法:
  rosrun robot_nav emergency_stop.py stop    # 触发急停
  rosrun robot_nav emergency_stop.py resume  # 解除急停
"""

import sys
import rospy
from std_msgs.msg import Bool

def main():
    rospy.init_node('emergency_stop_trigger', anonymous=True)
    pub = rospy.Publisher('/robot_nav/emergency_stop', Bool, queue_size=1, latch=True)

    action = "stop"
    if len(sys.argv) >= 2:
        action = sys.argv[1].lower()

    rospy.sleep(0.3)

    msg = Bool()
    if action in ["stop", "true", "1"]:
        msg.data = True
        pub.publish(msg)
        rospy.logwarn(">>> [急停触发] 已向 /robot_nav/emergency_stop 发送 TRUE 制动指令!")
    else:
        msg.data = False
        pub.publish(msg)
        rospy.loginfo(">>> [急停解除] 已向 /robot_nav/emergency_stop 发送 FALSE 恢复待命!")

    rospy.sleep(0.3)

if __name__ == '__main__':
    try:
        main()
    except rospy.ROSInterruptException:
        pass
