# LS2K0300 智能车 —— 直道/弯道基础巡线工程

基于逐飞 LS2K0300（龙芯 2K300）官方开源库的传统图像处理巡线示例。
**后轮双电机差速转向 + UVC 摄像头 + 双正交编码器速度闭环。**

---

## 一、目录结构

```
LS2K0300_SmartCar/
├── user/
│   ├── main.cpp            # 主程序：采图->巡线->方向环->差速输出
│   ├── config.hpp          # ★ 全部可调参数（实车标定只需改这里）
│   ├── CMakeLists.txt      # 编译配置（龙芯交叉编译 + OpenCV + TFLM/ncnn）
│   ├── build.sh            # 一键编译 + scp 上传板子
│   └── cross.cmake         # 交叉编译工具链配置
├── code/
│   ├── image_process.*     # 图像处理：灰度->大津二值化->逐行找边线->中线偏差
│   ├── pid.*               # PID 控制器（位置式 + 抗积分饱和）
│   ├── motor.*             # 双电机封装（DIR+PWM -> 占空比）
│   └── encoder.*           # 正交编码器测速（PIT 10ms 周期采样）
├── model/                  # TFLite 模型（官方库 headfile 依赖，基础巡线暂不使用）
├── libraries/              # 逐飞官方库（zf_common/zf_device/zf_driver/zf_components）
├── out/                    # 编译输出（build.sh 自动清理重建）
└── tools/                  # 本机语法检查辅助工具（可忽略）
```

---

## 二、硬件接线（逐飞主板默认）

| 功能 | 设备节点 | 说明 |
|------|----------|------|
| 左电机 PWM | `/dev/zf_pwm_motor_1` | GPIO86 |
| 左电机 DIR  | `/dev/zf_gpio_motor_1` | GPIO73 |
| 右电机 PWM | `/dev/zf_pwm_motor_2` | GPIO87 |
| 右电机 DIR  | `/dev/zf_gpio_motor_2` | GPIO76 |
| 左编码器 | `/dev/zf_encoder_quad_1` | 正交 |
| 右编码器 | `/dev/zf_encoder_quad_2` | 正交 |
| 摄像头 | `/dev/video0` | UVC，160x120@180fps（库默认） |

> 若摄像头分辨率不是 160x120，需同步修改 `libraries/zf_device/zf_device_uvc.hpp` 中的 `UVC_WIDTH/UVC_HEIGHT` 与 `config.hpp` 的 `IMG_W/IMG_H`。

---

## 三、实车标定步骤（按顺序！）

> 标定前：车轮架空或低速测试，人站在车旁随时准备拍停（串口 Ctrl+C 或断电）。

### 1. 电机方向
把 `SPEED_CLOSED_LOOP` 置 0（开环），临时把方向环关掉（见下方"单独测试电机"），
`OPEN_LOOP_DUTY` 给 20，看两个轮子是否都**前进**：
- 某轮反转 → 对应 `MOTOR_DIR_L` / `MOTOR_DIR_R` 取反。

### 2. 编码器方向
开环让车前进，串口打印 `Lspd/Rspd` 应为**正值**：
- 某侧为负 → 对应 `ENCODER_DIR_L` / `ENCODER_DIR_R` 取反。
- 速度数值明显偏大/偏小 → 校 `PULSE_PER_REV`（编码器线数）与 `GEAR_RATIO`（齿比）。

### 3. 图像与偏差方向
`TCP_DEBUG` 置 1，开逐飞助手看灰度图+边线：
- 赛道应为白色、背景为深色；若相反 → `LINE_IS_WHITE` 置 0。
- 二值化噪声大 → 调 `FIXED_THRESHOLD`（或保持大津自适应 `USE_OTSU=1`）。
- 边线是否贴合赛道左右边缘、中线是否居中。

### 4. 转向方向
`STEER_DIR`：
- 车头偏左（赛道偏右）时，期望**右转**（左轮快右轮慢）。
- 实测转向反了 → `STEER_DIR` 取反。

### 5. 方向环调参（KP -> KD）
1. `STEER_KP` 从 0.3 开始，低速（目标 0.3 m/s）跑直线弯道，逐步加到转弯跟得上、不摆头；
2. 出现震荡 → 加 `STEER_KD`（0.2~0.6），或减小 KP；
3. `STEER_OUT_LIMIT` 限制最大差速，防止急弯甩尾。

### 6. 速度闭环
`SPEED_CLOSED_LOOP` 置 1，`SPEED_TARGET_MPS` 从 0.3 逐步加到 0.6、0.8……
- 加速无力/上不去 → 加 `SPEED_KI`、`SPEED_KP`；
- 速度波动大 → 适当减小 `SPEED_KI`。

---

## 四、编译与部署

在**Linux 编译机**（装有龙芯工具链 + OpenCV 的机器，如板子本身或 WSL）上：

```bash
cd LS2K0300_SmartCar/out
../user/build.sh
```

脚本会自动：清理 out -> cmake -> make -> `scp` 上传整个工程到板子。
上传前修改 `user/build.sh` 中的 `REMOTE_IP`（板子 IP）、`REMOTE_USER`、`REMOTE_PATH`。

板上运行：

```bash
cd /home/root/LS2K0300_SmartCar/out
./LS2K0300_SmartCar
```

> 需要 root 权限访问 /dev 设备节点；`Ctrl+C` 会自动停车（SIGINT 清理）。

---

## 五、常见问题

| 现象 | 检查 |
|------|------|
| 摄像头初始化失败 | 检查 `/dev/video0` 是否存在、权限、`UVC_PATH` |
| 电机不动 | 检查设备节点权限、`MOTOR_DUTY_MAX` 是否太小、开环占空比是否给够 |
| 速度显示异常 | 编码器方向、齿比、`PULSE_PER_REV` |
| 转弯方向反 | `STEER_DIR` / `MOTOR_DIR_*` |
| 丢线停车 | 图像 ROI、阈值、赛道外干扰，调 `ROI_ROW_*` 与阈值 |
| 串口无打印 | `DEBUG_PRINT=1`，确认串口波特率 115200 |

---

## 六、后续扩展方向

- 十字/斑马线/环岛识别：扩展 `image_process` 状态机（参考 20th_smart_car 元素处理思路）
- 红色标识牌 AI 识别：启用 `model/` 中 TFLite 模型 + `classify` 模块
- 逐飞助手调参：打开 `TCP_DEBUG=1` 实时看图像与边线
