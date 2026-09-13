#!/bin/bash
# LS2K0300_SmartCar 语法检查（WSL g++ -fsyntax-only，OpenCV 用 stub）
# 用法：wsl -e bash /mnt/c/.../out/syntax_check.sh
set -u

ROOT="/mnt/c/Users/dimidimi/Desktop/LS2K0300_SmartCar"
LIB="$ROOT/libraries/zf_components"

INCS=""
for d in \
  "$ROOT/tools/opencv_stub" \
  "$ROOT/user" "$ROOT/code" "$ROOT/model" \
  "$ROOT/libraries/zf_common" \
  "$ROOT/libraries/zf_device" \
  "$ROOT/libraries/zf_driver" \
  "$LIB/seekfree_assistant" \
  "$LIB/tflm" \
  "$LIB/tflm/third_party/flatbuffers/include" \
  "$LIB/tflm/third_party/gemmlowp" \
  "$LIB/tflm/third_party/gemmlowp/fixedpoint" \
  "$LIB/tflm/third_party/ruy" \
  "$LIB/tflm/third_party/ruy/ruy" \
  "$LIB/tflm/tensorflow/lite/micro" \
  "$LIB/tflm/tensorflow/lite/micro/kernels" \
  "$LIB/tflm/tensorflow/lite/kernels/internal" \
  "$LIB/tflm/tensorflow/lite/core/api" \
  "$LIB/tflm/tensorflow/lite/core/c" \
  "$LIB/tflm/tensorflow/lite/schema" \
  "$LIB/ncnn/include/ncnn"
do
  INCS="$INCS -I $d"
done

SRCS="$ROOT/user/main.cpp $ROOT/code/pid.cpp $ROOT/code/motor.cpp $ROOT/code/encoder.cpp $ROOT/code/image_process.cpp"

echo "==== g++ -fsyntax-only 语法检查开始 ===="
g++ -fsyntax-only -std=c++17 -Wall -Wno-unused-variable -Wno-unused-but-set-variable $INCS $SRCS 2>&1
echo "==== 语法检查结束，退出码: $? ===="
