#!/usr/bin/env python3
# 键盘生成车体坐标速度：前向/左向 m/s，逆时针 rad/s，交由 C++ 节点限速转发。
# No key input for 0.5 seconds: continuously publish zero. Space stops immediately.
import sys
import os
import math
import select
import termios
import tty
import time
import rospy
from geometry_msgs.msg import Twist

def main():
    if not sys.stdin.isatty():
        sys.exit('Keyboard needs an interactive terminal. Connect using ssh -t.')
    rospy.init_node('bcrobot_keyboard')
    linear_speed = float(rospy.get_param('~linear_speed', 0.15))
    angular_speed = float(rospy.get_param('~angular_speed', 0.5))
    if not all(math.isfinite(v) and v > 0 for v in (linear_speed, angular_speed)):
        sys.exit('linear_speed and angular_speed must be finite positive numbers.')
    pub = rospy.Publisher('/target_cmd_vel', Twist, queue_size=1)
    keys = {'w': (1,0,0), 's': (-1,0,0), 'a': (0,1,0),
            'd': (0,-1,0), 'q': (0,0,1), 'e': (0,0,-1)}
    old = termios.tcgetattr(sys.stdin)
    last = 0.0
    command = Twist()
    moving = False
    print('senior_omni: W/S forward/back, A/D left/right, Q/E rotate; SPACE stop; Ctrl-C exit')
    print('Linear {:.2f} m/s; angular {:.2f} rad/s; key input timeout 0.5 s'.format(linear_speed, angular_speed))
    print('Set speeds: rosrun robot_nav keyboard.py _linear_speed:=0.15 _angular_speed:=0.5')
    print('Focus this terminal and use English input. Every received key is displayed.', flush=True)
    print('STOP vx=0 vy=0 wz=0', flush=True)
    try:
        tty.setcbreak(sys.stdin.fileno())
        while not rospy.is_shutdown():
            if select.select([sys.stdin], [], [], 0.05)[0]:
                key = os.read(sys.stdin.fileno(), 1).decode('ascii', errors='ignore').lower()
                if key == chr(3):
                    break
                command = Twist()
                if key in keys:
                    x,y,z = keys[key]
                    command.linear.x = x * linear_speed
                    command.linear.y = y * linear_speed
                    command.angular.z = z * angular_speed
                    last = time.monotonic()
                    moving = True
                    print("KEY {} -> vx={:.2f} vy={:.2f} wz={:.2f}; target subscribers={}".format(key.upper(), command.linear.x, command.linear.y, command.angular.z, pub.get_num_connections()), flush=True)
                else:
                    last = 0.0
                    moving = False
                    print("STOP (key={!r})".format(key), flush=True)
            if time.monotonic() - last > 0.5:
                command = Twist()
                if moving:
                    print('STOP (no key input for 0.5 s)', flush=True)
                    moving = False
            pub.publish(command)
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old)
        for _ in range(3):
            pub.publish(Twist())
            time.sleep(0.05)

if __name__ == '__main__':
    main()
