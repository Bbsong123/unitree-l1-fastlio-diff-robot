#ifndef ROBOT_NAV_BASE_CONTROLLER_H_
#define ROBOT_NAV_BASE_CONTROLLER_H_

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <string>

namespace robot_nav {

/**
 * @brief 底层驱动控制基础类
 * 负责与底层小车驱动的话题通信，提供基础运动下发、制动与急停接口。
 * 保持纯粹精简，供上层算法或节点直接调用。
 */
class BaseController {
public:
    BaseController() = default;
    ~BaseController() = default;

    /**
     * @brief 初始化底层控制发布者
     * @param nh ROS 节点句柄
     * @param cmd_vel_topic 底层驱动监听的 Twist 话题名 (默认 "/cmd_vel")
     */
    void init(ros::NodeHandle& nh, const std::string& cmd_vel_topic = "/cmd_vel");

    /**
     * @brief 基础速度指令发送
     * @param linear_x 期望线速度 (m/s)
     * @param angular_z 期望角速度 (rad/s)
     */
    void send_velocity(double linear_x, double angular_z);

    /**
     * @brief 正常平稳停车
     */
    void stop();

    /**
     * @brief 紧急停车（下发 0 速度制动）
     */
    void emergency_stop();

private:
    ros::Publisher cmd_vel_pub_;
    std::string cmd_vel_topic_ = "/cmd_vel";
};

} // namespace robot_nav

#endif // ROBOT_NAV_BASE_CONTROLLER_H_
