#!/usr/bin/env python3
"""发布 MID360 IMU 坐标系 body 到底盘中心的固定安装关系。"""
import math

from geometry_msgs.msg import TransformStamped
import rospy
import tf2_ros


def main():
    rospy.init_node('mid360_mount_tf')
    # YAML 给出 IMU 在车体坐标系中的位置（米）和偏航（弧度），只含 yaw。
    imu_xyz = rospy.get_param('~imu_in_base_xyz')
    imu_yaw = rospy.get_param('~imu_in_base_yaw')
    if len(imu_xyz) != 3 or not all(map(math.isfinite, (*imu_xyz, imu_yaw))):
        raise ValueError('安装外参必须是三个有限位置分量和一个有限航向角')
    imu_x, imu_y, imu_z = imu_xyz
    cos_yaw, sin_yaw = math.cos(imu_yaw), math.sin(imu_yaw)

    mount = TransformStamped()
    mount.header.stamp = rospy.Time.now()
    mount.header.frame_id = 'body'
    mount.child_frame_id = 'base_footprint'
    # FAST-LIO 已发布 camera_init → body；这里取逆 T：平移 -R^T t，旋转 -yaw。
    mount.transform.translation.x = -cos_yaw * imu_x - sin_yaw * imu_y
    mount.transform.translation.y = sin_yaw * imu_x - cos_yaw * imu_y
    mount.transform.translation.z = -imu_z
    mount.transform.rotation.z = -math.sin(imu_yaw / 2)
    mount.transform.rotation.w = math.cos(imu_yaw / 2)
    broadcaster = tf2_ros.StaticTransformBroadcaster()
    broadcaster.sendTransform(mount)
    rospy.spin()


if __name__ == '__main__':
    main()
