#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
@file static_tf_broadcaster.py
@brief 从 static_tf.yaml 动态加载雷达相对底盘的外参并广播 Static TF
"""

import rospy
import tf2_ros
import geometry_msgs.msg
import tf.transformations

def main():
    rospy.init_node('static_tf_broadcaster', anonymous=False)

    parent_frame = rospy.get_param('~parent_frame', 'base_link')
    child_frame = rospy.get_param('~child_frame', 'unilidar_lidar')

    x = float(rospy.get_param('~translation/x', 0.0))
    y = float(rospy.get_param('~translation/y', 0.0))
    z = float(rospy.get_param('~translation/z', 0.0))

    roll = float(rospy.get_param('~rotation/roll', 0.0))
    pitch = float(rospy.get_param('~rotation/pitch', 0.0))
    yaw = float(rospy.get_param('~rotation/yaw', 0.0))

    broadcaster = tf2_ros.StaticTransformBroadcaster()

    static_transform = geometry_msgs.msg.TransformStamped()
    static_transform.header.stamp = rospy.Time.now()
    static_transform.header.frame_id = parent_frame
    static_transform.child_frame_id = child_frame

    static_transform.transform.translation.x = x
    static_transform.transform.translation.y = y
    static_transform.transform.translation.z = z

    q = tf.transformations.quaternion_from_euler(roll, pitch, yaw)
    static_transform.transform.rotation.x = q[0]
    static_transform.transform.rotation.y = q[1]
    static_transform.transform.rotation.z = q[2]
    static_transform.transform.rotation.w = q[3]

    broadcaster.sendTransform(static_transform)
    rospy.loginfo(f"[Static TF] 广播成功: {parent_frame} -> {child_frame} "
                  f"(x={x:.3f}, y={y:.3f}, z={z:.3f}, yaw={yaw:.3f})")

    rospy.spin()

if __name__ == '__main__':
    try:
        main()
    except rospy.ROSInterruptException:
        pass
