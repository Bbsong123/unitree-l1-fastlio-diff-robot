/**
 * @file point_tracker_node.cpp
 * @brief 基于 FAST-LIO 里程计的定点跟踪控制算法节点
 * 
 * 职责说明：
 * 1. 订阅 FAST-LIO 全局里程计 (/Odometry) 获取当前位姿；
 * 2. 订阅目标点 (/desire_position 或 RViz 2D Nav Goal)；
 * 3. 几何解算两点间航向偏差与欧氏距离，实施“原地对准 + 巡航纠偏”差速控制；
 * 4. 将期望速度发布给内部总线 (/target_cmd_vel)，由底层 base_controller_node 接收与执行；
 * 5. 到位后自动发送 0 速度并释放目标点。
 */

#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <tf/tf.h>
#include <cmath>

class PointTrackerNode {
public:
    PointTrackerNode() : pnh_("~"), has_odom_(false), has_target_(false) {
        // 1. 读取参数
        pnh_.param<std::string>("odom_topic", odom_topic_, "/Odometry");
        pnh_.param<std::string>("target_topic", target_topic_, "/desire_position");
        pnh_.param<std::string>("cmd_topic", cmd_topic_, "/target_cmd_vel");

        pnh_.param<double>("const_linear_speed", const_linear_speed_, 0.15);   // 直行巡航线速度 (m/s)
        pnh_.param<double>("const_angular_speed", const_angular_speed_, 0.25); // 原地转向角速度 (rad/s)
        pnh_.param<double>("dist_tolerance", dist_tolerance_, 0.08);           // 到位距离阈值 (m)
        pnh_.param<double>("angle_align_thresh", angle_align_thresh_, 0.35);   // 航向对齐角度阈值 (rad, 约20°)
        pnh_.param<double>("control_rate", control_rate_, 20.0);              // 控制解算频率 (Hz)

        // 2. 订阅与发布
        odom_sub_ = nh_.subscribe(odom_topic_, 10, &PointTrackerNode::odomCallback, this);
        target_sub_ = nh_.subscribe(target_topic_, 1, &PointTrackerNode::targetCallback, this);
        
        // 兼容 RViz 2D Nav Goal 鼠标点击交互
        rviz_goal_sub_ = nh_.subscribe("/move_base_simple/goal", 1, &PointTrackerNode::targetCallback, this);

        cmd_pub_ = nh_.advertise<geometry_msgs::Twist>(cmd_topic_, 10);

        // 3. 定时解算主循环
        timer_ = nh_.createTimer(ros::Duration(1.0 / control_rate_), &PointTrackerNode::controlLoop, this);

        ROS_INFO("==================================================");
        ROS_INFO("[point_tracker_node] 定点跟踪控制算法节点已启动!");
        ROS_INFO(" - 里程计监听话题: %s", odom_topic_.c_str());
        ROS_INFO(" - 目标点监听话题: %s (同时支持 /move_base_simple/goal)", target_topic_.c_str());
        ROS_INFO(" - 速度输出话题: %s", cmd_topic_.c_str());
        ROS_INFO(" - 巡航线速: %.2f m/s, 转向角速: %.2f rad/s", const_linear_speed_, const_angular_speed_);
        ROS_INFO(" - 到达距离容差: %.3f m, 航向对齐阈值: %.2f rad", dist_tolerance_, angle_align_thresh_);
        ROS_INFO("==================================================");
    }

private:
    // 里程计回调：提取当前 x, y, yaw
    void odomCallback(const nav_msgs::Odometry::ConstPtr& msg) {
        current_x_ = msg->pose.pose.position.x;
        current_y_ = msg->pose.pose.position.y;

        tf::Quaternion q(
            msg->pose.pose.orientation.x,
            msg->pose.pose.orientation.y,
            msg->pose.pose.orientation.z,
            msg->pose.pose.orientation.w
        );
        current_yaw_ = tf::getYaw(q);
        has_odom_ = true;
    }

    // 目标点回调
    void targetCallback(const geometry_msgs::PoseStamped::ConstPtr& msg) {
        target_x_ = msg->pose.position.x;
        target_y_ = msg->pose.position.y;

        tf::Quaternion q(
            msg->pose.orientation.x,
            msg->pose.orientation.y,
            msg->pose.orientation.z,
            msg->pose.orientation.w
        );
        target_yaw_ = tf::getYaw(q);
        has_target_ = true;

        ROS_INFO(">>> [point_tracker] 接收到新目标点: (X: %.2f, Y: %.2f, Yaw: %.1f°)", 
                 target_x_, target_y_, target_yaw_ * 180.0 / M_PI);
    }

    // 角度正规化到 [-pi, pi]
    static double normalizeAngle(double angle) {
        while (angle > M_PI)  angle -= 2.0 * M_PI;
        while (angle < -M_PI) angle += 2.0 * M_PI;
        return angle;
    }

    // 20Hz 控制解算主循环
    void controlLoop(const ros::TimerEvent&) {
        if (!has_odom_ || !has_target_) {
            return;
        }

        // 1. 计算与目标点的欧氏距离
        double dx = target_x_ - current_x_;
        double dy = target_y_ - current_y_;
        double dist = std::hypot(dx, dy);

        // 2. 到位判定
        if (dist <= dist_tolerance_) {
            publishVelocity(0.0, 0.0);
            has_target_ = false;
            ROS_INFO("==================================================");
            ROS_INFO(">>> [point_tracker] 目标已到达! 距离误差: %.3f m", dist);
            ROS_INFO("==================================================");
            return;
        }

        // 3. 计算航向角偏差
        double target_heading = std::atan2(dy, dx);
        double angle_diff = normalizeAngle(target_heading - current_yaw_);

        // 4. 两段式运动解算
        double out_v = 0.0;
        double out_w = 0.0;

        if (std::abs(angle_diff) > angle_align_thresh_) {
            // 阶段 1：车头未对准，原地自转对准目标
            out_v = 0.0;
            out_w = (angle_diff > 0.0 ? 1.0 : -1.0) * const_angular_speed_;
        } else {
            // 阶段 2：车头已对准，以常速直线前行，叠加航向微调
            out_v = const_linear_speed_;
            out_w = 0.8 * angle_diff;
        }

        publishVelocity(out_v, out_w);
    }

    // 发送期望速度给内部话题
    void publishVelocity(double v, double w) {
        geometry_msgs::Twist twist;
        twist.linear.x = v;
        twist.angular.z = w;
        cmd_pub_.publish(twist);
    }

    ros::NodeHandle nh_;
    ros::NodeHandle pnh_;

    ros::Subscriber odom_sub_;
    ros::Subscriber target_sub_;
    ros::Subscriber rviz_goal_sub_;
    ros::Publisher cmd_pub_;
    ros::Timer timer_;

    std::string odom_topic_;
    std::string target_topic_;
    std::string cmd_topic_;

    double const_linear_speed_;
    double const_angular_speed_;
    double dist_tolerance_;
    double angle_align_thresh_;
    double control_rate_;

    double current_x_ = 0.0;
    double current_y_ = 0.0;
    double current_yaw_ = 0.0;

    double target_x_ = 0.0;
    double target_y_ = 0.0;
    double target_yaw_ = 0.0;

    bool has_odom_;
    bool has_target_;
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "point_tracker_node");
    PointTrackerNode node;
    ros::spin();
    return 0;
}
