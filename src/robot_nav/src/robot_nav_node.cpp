/**
 * @file robot_nav_node.cpp
 * @brief robot_nav (Nav) 全局导航中枢节点
 * 
 * 架构说明：
 * 1. 底层控制层 (BaseController)：负责 Twist 速度下发、停止、急停制动。
 * 2. 高层算法层 (PointNavigator)：负责全局位姿与目标点的误差解算 (v, w)。
 * 3. 通信与胶水：监听里程计、目标点、外部急停话题；预留 RViz 2D Nav Goal 接口（默认已注释）。
 */

#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Bool.h>
#include <tf/tf.h>

#include "robot_nav/base_controller.h"
#include "robot_nav/point_navigator.h"

namespace robot_nav {

class RobotNavNode {
public:
    RobotNavNode(ros::NodeHandle& nh, ros::NodeHandle& pnh)
        : nh_(nh), pnh_(pnh), has_odom_(false), has_target_(false), is_emergency_stopped_(false)
    {
        // 1. 读取参数配置
        load_parameters();

        // 2. 初始化底层控制模块
        base_controller_.init(nh_, cmd_vel_topic_);

        // 3. 初始化高层解算算法参数
        navigator_.set_params(nav_params_);

        // 4. 话题订阅
        // FAST-LIO 全局里程计
        odom_sub_ = nh_.subscribe(odom_topic_, 10, &RobotNavNode::odom_callback, this);

        // 目标点话题
        target_sub_ = nh_.subscribe(desire_position_topic_, 1, &RobotNavNode::target_callback, this);

        // 外部急停话题 (发布 true 立即停车，发布 false 解除)
        estop_sub_ = nh_.subscribe("/robot_nav/emergency_stop", 1, &RobotNavNode::estop_callback, this);

        // ==============================================================================
        // 【预留接口：RViz 2D Nav Goal 点击订阅】
        // 如后续需在 RViz 中通过鼠标点击设定目标点，取消下方注释即可：
        // rviz_goal_sub_ = nh_.subscribe("/move_base_simple/goal", 1, &RobotNavNode::target_callback, this);
        // ==============================================================================

        // 5. 定时控制循环
        control_timer_ = nh_.createTimer(
            ros::Duration(1.0 / control_rate_),
            &RobotNavNode::control_loop, this
        );

        ROS_INFO("==================================================================");
        ROS_INFO(" [robot_nav] 导航中枢节点已启动!");
        ROS_INFO(" - 里程计监听话题: %s", odom_topic_.c_str());
        ROS_INFO(" - 目标点监听话题: %s", desire_position_topic_.c_str());
        ROS_INFO(" - 底层输出控制话题: %s", cmd_vel_topic_.c_str());
        ROS_INFO(" - 常速线速度: %.2f m/s, 常速角速度: %.2f rad/s", 
                 nav_params_.const_linear_speed, nav_params_.const_angular_speed);
        ROS_INFO(" - 到达距离容差: %.3f m, 控制频率: %.1f Hz", 
                 nav_params_.dist_tolerance, control_rate_);
        ROS_INFO("==================================================================");
    }

private:
    void load_parameters() {
        pnh_.param<std::string>("odom_topic", odom_topic_, "/Odometry");
        pnh_.param<std::string>("desire_position_topic", desire_position_topic_, "/desire_position");
        pnh_.param<std::string>("cmd_vel_topic", cmd_vel_topic_, "/cmd_vel");

        pnh_.param<double>("const_linear_speed", nav_params_.const_linear_speed, 0.15);
        pnh_.param<double>("const_angular_speed", nav_params_.const_angular_speed, 0.25);
        pnh_.param<double>("dist_tolerance", nav_params_.dist_tolerance, 0.08);
        pnh_.param<double>("angle_align_thresh", nav_params_.angle_align_thresh, 0.35);
        pnh_.param<double>("control_rate", control_rate_, 20.0);
    }

    void odom_callback(const nav_msgs::Odometry::ConstPtr& msg) {
        current_pose_.x = msg->pose.pose.position.x;
        current_pose_.y = msg->pose.pose.position.y;

        tf::Quaternion q(
            msg->pose.pose.orientation.x,
            msg->pose.pose.orientation.y,
            msg->pose.pose.orientation.z,
            msg->pose.pose.orientation.w
        );
        current_pose_.yaw = tf::getYaw(q);
        has_odom_ = true;
    }

    void target_callback(const geometry_msgs::PoseStamped::ConstPtr& msg) {
        if (is_emergency_stopped_) {
            ROS_WARN("[robot_nav] 当前处于急停状态，忽略新目标点!");
            return;
        }

        target_pose_.x = msg->pose.position.x;
        target_pose_.y = msg->pose.position.y;

        tf::Quaternion q(
            msg->pose.orientation.x,
            msg->pose.orientation.y,
            msg->pose.orientation.z,
            msg->pose.orientation.w
        );
        target_pose_.yaw = tf::getYaw(q);
        has_target_ = true;

        ROS_INFO(">>> [robot_nav] 接收到新目标点: X = %.3f m, Y = %.3f m, Yaw = %.2f rad", 
                 target_pose_.x, target_pose_.y, target_pose_.yaw);
    }

    void estop_callback(const std_msgs::Bool::ConstPtr& msg) {
        is_emergency_stopped_ = msg->data;
        if (is_emergency_stopped_) {
            has_target_ = false;
            base_controller_.emergency_stop();
            ROS_WARN(">>> [robot_nav] 收到外部急停信号 (TRUE)，已强制停车并取消目标!");
        } else {
            ROS_INFO(">>> [robot_nav] 急停信号解除 (FALSE)，恢复待命");
        }
    }

    void control_loop(const ros::TimerEvent&) {
        // 急停状态下持续保持制动
        if (is_emergency_stopped_) {
            base_controller_.emergency_stop();
            return;
        }

        if (!has_odom_) {
            ROS_WARN_THROTTLE(5.0, "[robot_nav] 等待里程计定位数据 [%s] ...", odom_topic_.c_str());
            return;
        }

        if (!has_target_) {
            return;
        }

        // 调用高层算法解算线速度与角速度
        double v = 0.0;
        double w = 0.0;
        bool is_reached = false;
        navigator_.compute(current_pose_, target_pose_, v, w, is_reached);

        if (is_reached) {
            // 到达目的地，平稳停车并复位目标
            base_controller_.stop();
            has_target_ = false;

            double residual = std::hypot(target_pose_.x - current_pose_.x, 
                                         target_pose_.y - current_pose_.y);
            ROS_INFO("==================================================================");
            ROS_INFO(">>> [到达目的地!] 目标点 (%.3f, %.3f) 已到达, 距离残差: %.3f m", 
                     target_pose_.x, target_pose_.y, residual);
            ROS_INFO("==================================================================");
            return;
        }

        // 下发速度到底层
        base_controller_.send_velocity(v, w);
    }

    ros::NodeHandle nh_;
    ros::NodeHandle pnh_;

    // 通信对象
    ros::Subscriber odom_sub_;
    ros::Subscriber target_sub_;
    ros::Subscriber estop_sub_;
    // ros::Subscriber rviz_goal_sub_; // 预留 RViz 接口

    ros::Timer control_timer_;

    // 核心模块
    BaseController base_controller_;
    PointNavigator navigator_;

    // 配置与状态
    std::string odom_topic_;
    std::string desire_position_topic_;
    std::string cmd_vel_topic_;
    NavParams nav_params_;
    double control_rate_ = 20.0;

    Pose2D current_pose_;
    Pose2D target_pose_;
    bool has_odom_;
    bool has_target_;
    bool is_emergency_stopped_;
};

} // namespace robot_nav

int main(int argc, char** argv) {
    ros::init(argc, argv, "robot_nav_node");
    ros::NodeHandle nh;
    ros::NodeHandle pnh("~");

    robot_nav::RobotNavNode node(nh, pnh);

    ros::spin();
    return 0;
}
