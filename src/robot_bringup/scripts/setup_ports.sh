#!/bin/bash
# ==============================================================================
# setup_ports.sh: 智能 USB 串口识别与动态软链接绑定脚本
# 适用硬件:
#   1. WHEELTEC C30D 底盘控制板 (USB 芯片: CH9102, Product ID: 55d4)
#   2. 宇树 Unitree L1 激光雷达 (USB 芯片: CH9102, Product ID: 55d3)
# ==============================================================================

if [ "$EUID" -ne 0 ]; then
  echo "请使用 sudo 权限运行此脚本: sudo bash setup_ports.sh"
  exit 1
fi

CHASSIS_FOUND=false
LIDAR_FOUND=false

for dev in /dev/ttyACM* /dev/ttyUSB*; do
  if [ -e "$dev" ] && [ ! -L "$dev" ]; then
    PID=$(udevadm info -q property -n "$dev" | grep "ID_MODEL_ID" | cut -d'=' -f2)

    if [ "$PID" == "55d4" ]; then
      ln -sf "$dev" /dev/wheeltec_controller
      chmod 777 "$dev" /dev/wheeltec_controller
      echo "[底盘] 成功绑定: $dev -> /dev/wheeltec_controller (已赋权 777)"
      CHASSIS_FOUND=true
    elif [ "$PID" == "55d3" ]; then
      ln -sf "$dev" /dev/unitree_lidar
      chmod 777 "$dev" /dev/unitree_lidar
      echo "[雷达] 成功绑定: $dev -> /dev/unitree_lidar (已赋权 777)"
      LIDAR_FOUND=true
    fi
  fi
done

if [ "$CHASSIS_FOUND" = false ]; then
  echo "[警告] 未检测到底盘设备 (PID: 55d4)，请检查底盘 Type-C 连线与电源开关闭合！"
fi

if [ "$LIDAR_FOUND" = false ]; then
  echo "[警告] 未检测到雷达设备 (PID: 55d3)，请检查雷达 USB 转接板连接！"
fi

echo "端口映射检测与配置完成。"
