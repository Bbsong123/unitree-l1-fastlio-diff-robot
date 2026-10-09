#ifndef ROBOT_NAV_POINT_NAVIGATOR_H_
#define ROBOT_NAV_POINT_NAVIGATOR_H_

#include <cmath>
#include <algorithm>

namespace robot_nav {

/**
 * @brief 平面二维位姿 (x, y, 航向角 yaw)
 */
struct Pose2D {
    double x = 0.0;
    double y = 0.0;
    double yaw = 0.0;
};

/**
 * @brief 导航控制参数配置
 */
struct NavParams {
    double const_linear_speed = 0.15;   ///< 常速直行 (m/s)
    double const_angular_speed = 0.25;  ///< 常速转向 (rad/s)
    double dist_tolerance = 0.08;       ///< 目标到位距离容差 (m)
    double angle_align_thresh = 0.35;   ///< 航向对齐阈值 (rad)
};

/**
 * @brief 高层算法与目标点解算接口类
 * 负责接收当前全局位姿与目标位姿，解算对应的目标线速度与角速度。
 * 纯算法逻辑实现，留有规范的接口便于未来升级为 Pure Pursuit、PID 或 MPC 等。
 */
class PointNavigator {
public:
    PointNavigator() = default;
    explicit PointNavigator(const NavParams& params) : params_(params) {}
    virtual ~PointNavigator() = default;

    void set_params(const NavParams& params) { params_ = params; }
    const NavParams& get_params() const { return params_; }

    /**
     * @brief 核心控制解算接口
     * @param current 当前机器人在世界系下的二维位姿
     * @param target  目标位姿
     * @param out_v   [输出] 解算得出的线速度 (m/s)
     * @param out_w   [输出] 解算得出的角速度 (rad/s)
     * @param is_reached [输出] 是否已到达目标点
     */
    virtual void compute(const Pose2D& current, 
                         const Pose2D& target, 
                         double& out_v, 
                         double& out_w, 
                         bool& is_reached);

protected:
    NavParams params_;

    /**
     * @brief 角度正规化到 [-pi, pi]
     */
    static inline double normalize_angle(double angle) {
        while (angle > M_PI)  angle -= 2.0 * M_PI;
        while (angle < -M_PI) angle += 2.0 * M_PI;
        return angle;
    }
};

} // namespace robot_nav

#endif // ROBOT_NAV_POINT_NAVIGATOR_H_
