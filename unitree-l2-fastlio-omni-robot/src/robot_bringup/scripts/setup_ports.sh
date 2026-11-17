#!/bin/bash
# L2 雷达使用网口，不需要串口映射或赋权；网卡由用户在系统设置中配置。
# 此脚本仅保留原底盘控制板的 USB 识别与 /dev/wheeltec_controller 映射。
# PID 55d4 来自原工程，换底盘控制板时仍须用 lsusb 核对。
if [ "$EUID" -ne 0 ]; then
  echo "请使用 sudo bash src/robot_bringup/scripts/setup_ports.sh"
  exit 1
fi

CHASSIS_FOUND=false
for dev in /dev/ttyACM* /dev/ttyUSB*; do
  if [ -e "$dev" ] && [ ! -L "$dev" ]; then
    PID=$(udevadm info -q property -n "$dev" | grep "ID_MODEL_ID" | cut -d'=' -f2)
    if [ "$PID" == "55d4" ]; then
      ln -sf "$dev" /dev/wheeltec_controller
      chmod 777 "$dev" /dev/wheeltec_controller
      echo "[底盘] $dev -> /dev/wheeltec_controller（沿用原工程赋权方式）"
      CHASSIS_FOUND=true
    fi
  fi
done
if [ "$CHASSIS_FOUND" = false ]; then
  echo "[警告] 未检测到底盘设备（PID: 55d4），请检查连线、电源和 USB 标识。"
fi
