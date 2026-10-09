/**
 * @file base_controller_node.cpp
 * @brief 底层底盘速度转发与看门狗保护节点
 * 
 * 功能极简说明：
 * 1. 订阅上层算法发布的期望速度 (/target_cmd_vel)；
 * 2. 基础速度限幅，直接转发给底盘驱动 (/cmd_vel)；
 * 3. 超时看门狗：上层若超过 watchdog_timeout 未发指令，自动发 0 刹停。
 */

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <algorithm>

class BaseControllerNode {
public:
    BaseControllerNode() : pnh_("~"), has_stopped_(true) {
        // 读取参数
        pnh_.param<std::string>("input_topic", input_topic_, "/target_cmd_vel");
        pnh_.param<std::string>("output_topic", output_topic_, "/cmd_vel");
        pnh_.param<double>("max_linear_speed", max_linear_speed_, 0.4);
        pnh_.param<double>("max_angular_speed", max_angular_speed_, 0.8);
        pnh_.param<double>("watchdog_timeout", watchdog_timeout_, 0.5);

        // 订阅与发布
        cmd_sub_ = nh_.subscribe(input_topic_, 10, &BaseControllerNode::cmdCallback, this);
        cmd_pub_ = nh_.advertise<geometry_msgs::Twist>(output_topic_, 10);

        last_cmd_time_ = ros::Time::now();

        // 看门狗定时器 (10Hz 巡检)
        watchdog_timer_ = nh_.createTimer(ros::Duration(0.1), &BaseControllerNode::watchdogCheck, this);

        ROS_INFO("[base_controller_node] 已就绪: 监听 %s -> 转发 %s (超时看门狗: %.2fs)", 
                 input_topic_.c_str(), output_topic_.c_str(), watchdog_timeout_);
    }

private:
    void cmdCallback(const geometry_msgs::Twist::ConstPtr& msg) {
        geometry_msgs::Twist out_msg;
        out_msg.linear.x = std::clamp(msg->linear.x, -max_linear_speed_, max_linear_speed_);
        out_msg.angular.z = std::clamp(msg->angular.z, -max_angular_speed_, max_angular_speed_);

        cmd_pub_.publish(out_msg);

        last_cmd_time_ = ros::Time::now();
        has_stopped_ = false;
    }

    void watchdogCheck(const ros::TimerEvent&) {
        if (!has_stopped_ && (ros::Time::now() - last_cmd_time_).toSec() > watchdog_timeout_) {
            geometry_msgs::Twist stop_msg;
            cmd_pub_.publish(stop_msg);
            has_stopped_ = true;
            ROS_WARN_THROTTLE(2.0, "[base_controller_node] 上层指令超时 (>%.2fs)，已自动发 0 刹停底盘", watchdog_timeout_);
        }
    }

    ros::NodeHandle nh_;
    ros::NodeHandle pnh_;
    ros::Subscriber cmd_sub_;
    ros::Publisher cmd_pub_;
    ros::Timer watchdog_timer_;

    std::string input_topic_;
    std::string output_topic_;
    double max_linear_speed_;
    double max_angular_speed_;
    double watchdog_timeout_;

    ros::Time last_cmd_time_;
    bool has_stopped_;
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "base_controller_node");
    BaseControllerNode node;
    ros::spin();
    return 0;
}
