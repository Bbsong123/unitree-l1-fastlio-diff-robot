/**
 * 三轮全向速度转发：输入话题 -> 限速 -> 输出话题 -> 官方串口驱动。
 * 默认数据流为 /target_cmd_vel -> /cmd_vel，话题和限速从 YAML 读取。
 * 只处理车体 vx、vy、wz，不计算路径或三个电机的转速。
 */
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <algorithm>
#include <cmath>
#include <string>

ros::Publisher cmd_vel_pub;
double max_linear_speed;       // 平面合速度上限，m/s。
double max_angular_speed;      // 旋转速度绝对值上限，rad/s。
double command_timeout;        // 多久没有有效输入后停车，秒。
ros::WallTime last_valid_time; // 最近一次有效消息的接收时刻，使用实际时间。
bool has_valid_command = false;

// 统一的停车函数：默认构造的 Twist 六个速度分量全为零。
// 启动后没有输入、输入无效或输入超时时，都调用此函数。
void stopRobot() {
    cmd_vel_pub.publish(geometry_msgs::Twist());
}

void velocityCallback(const geometry_msgs::Twist::ConstPtr& msg) {
    // 有效消息：指针存在，且底盘使用的三个分量均为有限数值。
    // NaN / 正负无穷不能作为串口速度发送；其余三个非平面分量不使用。
    if (!msg || !std::isfinite(msg->linear.x) ||
        !std::isfinite(msg->linear.y) || !std::isfinite(msg->angular.z)) {
        has_valid_command = false;
        stopRobot();
        return;
    }

    geometry_msgs::Twist command;
    command.linear.x = msg->linear.x;    // 向前为正，m/s。
    command.linear.y = msg->linear.y;    // 向左为正，m/s。
    command.angular.z = msg->angular.z; // 逆时针为正，rad/s。

    // 限制平面合速度，同时等比例缩小 vx、vy，保持原来的移动方向。
    // 这样斜向运动也不会超过 max_linear_speed。
    const double speed = std::hypot(command.linear.x, command.linear.y);
    if (!std::isfinite(speed)) {
        has_valid_command = false;
        stopRobot();
        return;
    }
    if (speed > max_linear_speed) {
        const double scale = max_linear_speed / speed;
        command.linear.x *= scale;
        command.linear.y *= scale;
    }

    // 旋转速度独立限幅。使用 C++14 的 min/max，不用 std::clamp。
    command.angular.z = std::max(-max_angular_speed,
                                std::min(command.angular.z, max_angular_speed));

    // 零速度也是有效指令；超过限制的有限速度经限幅后同样有效。
    last_valid_time = ros::WallTime::now();
    has_valid_command = true;
    cmd_vel_pub.publish(command);
}

// 没有消息时订阅回调不会执行，所以用 20Hz 定时回调检查接收状态。
// 启动后未收到有效消息，或超过超时时间没有新消息时，持续发布零速度。
// 在超时之前保持上一条有效指令，不因两条正常消息之间的间隔而停车。
void checkCommand(const ros::WallTimerEvent&) {
    if (!has_valid_command ||
        (ros::WallTime::now() - last_valid_time).toSec() >= command_timeout) {
        has_valid_command = false;
        stopRobot();
    }
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "base_controller_node");
    ros::NodeHandle nh;
    ros::NodeHandle private_nh("~"); // 读取当前节点私有命名空间中的 YAML 参数。

    std::string input_topic, output_topic;
    private_nh.param<std::string>("input_topic", input_topic, "/target_cmd_vel");
    private_nh.param<std::string>("output_topic", output_topic, "/cmd_vel");
    private_nh.param("max_linear_speed", max_linear_speed, 0.4);
    private_nh.param("max_angular_speed", max_angular_speed, 0.8);
    private_nh.param("command_timeout", command_timeout, 0.5);

    // 简单检查配置，避免负限制、无效超时或同名输入输出导致自我转发。
    if (input_topic.empty() || output_topic.empty() ||
        !std::isfinite(max_linear_speed) || max_linear_speed <= 0.0 ||
        !std::isfinite(max_angular_speed) || max_angular_speed <= 0.0 ||
        !std::isfinite(command_timeout) || command_timeout <= 0.0) {
        ROS_FATAL("话题不能为空，速度限制和接收超时必须为有限正数");
        return 1;
    }
    if (nh.resolveName(input_topic) == nh.resolveName(output_topic)) {
        ROS_FATAL("接收与转发话题不能相同");
        return 1;
    }

    // 队列长度为 1，避免指令在本节点内积压；消息到达时立即限速并转发。
    cmd_vel_pub = nh.advertise<geometry_msgs::Twist>(output_topic, 1);
    ros::Subscriber velocity_sub = nh.subscribe(input_topic, 1, velocityCallback);
    ros::WallTimer timer = nh.createWallTimer(ros::WallDuration(0.05), checkCommand);

    ROS_INFO("速度转发: %s -> %s，线速上限 %.2f m/s，角速上限 %.2f rad/s，超时 %.2f s",
             input_topic.c_str(), output_topic.c_str(), max_linear_speed,
             max_angular_speed, command_timeout);
    ros::spin();
    return 0;
}
