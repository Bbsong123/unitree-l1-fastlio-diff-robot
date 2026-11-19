#!/usr/bin/env python3
"""发布 MID360 IMU、底盘与 L515 的固定安装关系。"""
import math

from geometry_msgs.msg import TransformStamped
import rospy
from tf.transformations import quaternion_from_euler
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

    transforms = [mount]
    if rospy.has_param('~camera_link_frame'):
        # YAML 直接给出相机根帧在车体中的位置和姿态。驱动将彩色帧挂在相机根帧下。
        camera = TransformStamped()
        camera.header.stamp = mount.header.stamp
        camera.header.frame_id = mount.child_frame_id
        camera.child_frame_id = rospy.get_param('~camera_link_frame')
        translation = camera.transform.translation
        translation.x, translation.y, translation.z = rospy.get_param('~camera_in_base_xyz')
        rotation = camera.transform.rotation
        rotation.x, rotation.y, rotation.z, rotation.w = quaternion_from_euler(
            *rospy.get_param('~camera_in_base_rpy'))
        transforms.append(camera)

    broadcaster = tf2_ros.StaticTransformBroadcaster()
    broadcaster.sendTransform(transforms)
    rospy.spin()


if __name__ == '__main__':
    main()
