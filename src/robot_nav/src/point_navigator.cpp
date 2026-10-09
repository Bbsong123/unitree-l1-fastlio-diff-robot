#include "robot_nav/point_navigator.h"

namespace robot_nav {

void PointNavigator::compute(const Pose2D& current, 
                             const Pose2D& target, 
                             double& out_v, 
                             double& out_w, 
                             bool& is_reached) 
{
    out_v = 0.0;
    out_w = 0.0;
    is_reached = false;

    // 1. 计算与目标点的欧氏距离
    double dx = target.x - current.x;
    double dy = target.y - current.y;
    double dist = std::hypot(dx, dy);

    // 2. 到位判定
    if (dist <= params_.dist_tolerance) {
        is_reached = true;
        return;
    }

    // 3. 计算目标方位角与当前航向角差值
    double target_heading = std::atan2(dy, dx);
    double angle_diff = normalize_angle(target_heading - current.yaw);

    // 4. 几何跟踪控制解算 (可按需在此扩充或重载)
    if (std::abs(angle_diff) > params_.angle_align_thresh) {
        // 航向偏差较大：原地旋转对准
        out_v = 0.0;
        out_w = (angle_diff > 0.0 ? 1.0 : -1.0) * params_.const_angular_speed;
    } else {
        // 航向已对准：以常速直行，轻微微调角速度纠偏
        out_v = params_.const_linear_speed;
        out_w = 0.8 * angle_diff;
    }
}

} // namespace robot_nav
