#include "robot_nav/base_controller.h"

namespace robot_nav {

void BaseController::init(ros::NodeHandle& nh, const std::string& cmd_vel_topic) {
    cmd_vel_topic_ = cmd_vel_topic.empty() ? "/cmd_vel" : cmd_vel_topic;
    cmd_vel_pub_ = nh.advertise<geometry_msgs::Twist>(cmd_vel_topic_, 10);
    ROS_INFO("[BaseController] 底层控制接口就绪，发布话题: %s", cmd_vel_topic_.c_str());
}

void BaseController::send_velocity(double linear_x, double angular_z) {
    geometry_msgs::Twist twist;
    twist.linear.x = linear_x;
    twist.linear.y = 0.0;
    twist.linear.z = 0.0;
    twist.angular.x = 0.0;
    twist.angular.y = 0.0;
    twist.angular.z = angular_z;
    cmd_vel_pub_.publish(twist);
}

void BaseController::stop() {
    send_velocity(0.0, 0.0);
}

void BaseController::emergency_stop() {
    send_velocity(0.0, 0.0);
    ROS_WARN("[BaseController] 执行急停指令，已发送 0 速度制动");
}

} // namespace robot_nav
