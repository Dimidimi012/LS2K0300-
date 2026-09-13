/*********************************************************************************************************************
 * LS2K0300 智能车 —— 直道/弯道基础巡线 主程序
 *
 * 平台       LS2K0300（逐飞 2K300 核心板 + 逐飞主板，Linux）
 * 硬件       后轮双电机差速（DRV8701E：PWM+DIR） + 双正交编码器 + UVC 摄像头
 * 方案       传统图像处理巡线：灰度 -> 大津二值化 -> 逐行找边线 -> 中线偏差 -> PD 方向环
 *           + 编码器速度闭环（10ms PIT 定时器）
 *
 * 参数标定   全部集中在 user/config.hpp
 * 编译部署   cd out && ./../user/build.sh  （或按 user/build.sh 说明）
 ********************************************************************************************************************/
#include "zf_common_headfile.hpp"
#include "config.hpp"
#include "image_process.hpp"
#include "motor.hpp"
#include "encoder.hpp"
#include "pid.hpp"

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstring>

// ====================== 设备对象 ======================
zf_device_uvc uvc_dev;          // UVC 摄像头
zf_driver_pit pit_timer;        // PIT 周期定时器（10ms）

// ====================== 全局控制状态 ======================
static line_info_t line_info;   // 单帧巡线结果
static float steer_out   = 0.0f;    // 方向环输出（差速量，占空比单位）
static float last_error  = 0.0f;    // 上一帧偏差
static float base_duty   = 0.0f;    // 基础占空比（速度环输出或开环设定）
static int   lost_frames = 0;       // 连续双边丢线帧数

// 速度环
static pid_ctrl_t speed_pid;
static const bool speed_loop_enabled = (SPEED_CLOSED_LOOP == 1);

// ====================== PIT 10ms 回调：编码器测速 + 速度闭环 ======================
static void pit_callback(void)
{
    encoder_periodic_10ms();

    if (speed_loop_enabled)
    {
        float spd = encoder_get_avg_speed();
        base_duty = pid_compute(&speed_pid, SPEED_TARGET_MPS, spd);
        if (base_duty < 0.0f)                 base_duty = 0.0f;
        if (base_duty > MOTOR_DUTY_MAX)       base_duty = MOTOR_DUTY_MAX;
    }
}

// ====================== 退出清理 ======================
static void car_cleanup(void)
{
    pit_timer.stop();
    motor_stop();
    printf("car cleanup done\n");
}

static void sigint_handler(int)
{
    printf("\nSIGINT received, stopping car\n");
    exit(0);
}

// ====================== 逐飞助手 TCP 调试（可选） ======================
#if TCP_DEBUG
static zf_driver_tcp_client tcp_client_dev;
static uint8 image_copy[UVC_HEIGHT][UVC_WIDTH];
static uint8 x1_boundary[UVC_HEIGHT], x2_boundary[UVC_HEIGHT], x3_boundary[UVC_HEIGHT];

static uint32 tcp_send_wrap(const uint8 *buf, uint32 len)
{
    return tcp_client_dev.send_data(buf, len);
}
static uint32 tcp_read_wrap(uint8 *buf, uint32 len)
{
    return tcp_client_dev.read_data(buf, len);
}
#endif

// ====================== 主函数 ======================
int main(int argc, char *argv[])
{
        // ========== 开环测试模式：./LS2K0300_SmartCar test ==========
    // 关摄像头/巡线，固定双轮 20%，对照 读数符号 vs 物理转向 判定方向映射
    if (argc > 1 && strcmp(argv[1], "test") == 0)
    {
        encoder_init();
        motor_init();
        printf("OPEN-LOOP TEST: motor_set_duty(20, 20)\n");
        printf("t   Lspd(m/s)  Rspd(m/s)\n");
        motor_set_duty(20, 20);
        for (int i = 0; i < 150; i++)
        {
            usleep(10000);
            encoder_periodic_10ms();
            if (i % 10 == 0)
                printf("%3d  %7.3f  %7.3f\n", i,
                       (double)encoder_get_left_speed(),
                       (double)encoder_get_right_speed());
        }
        motor_stop();
        printf("done\n");
        return 0;
    }

    // ---------- 1. 摄像头初始化（UVC 支持热插拔，失败重试） ----------
    int init_try = 0;
    while (uvc_dev.init(UVC_PATH) < 0)
    {
        printf("uvc init failed, retry %d\n", ++init_try);
        if (init_try >= 5)
        {
            printf("uvc init give up\n");
            return -1;
        }
        system_delay_ms(500);
    }
    printf("uvc init ok (%dx%d)\n", UVC_WIDTH, UVC_HEIGHT);

    // ---------- 2. 编码器 / 电机 / 速度环 ----------
    encoder_init();
    motor_init();

    if (speed_loop_enabled)
    {
        pid_init(&speed_pid, SPEED_KP, SPEED_KI, SPEED_KD, SPEED_OUT_LIMIT);
        printf("speed loop enabled, target=%.2f m/s\n", SPEED_TARGET_MPS);
    }
    else
    {
        base_duty = OPEN_LOOP_DUTY;
        printf("speed loop disabled, open loop duty=%d\n", OPEN_LOOP_DUTY);
    }

    // ---------- 3. 定时器（10ms：编码器+速度环）与退出清理 ----------
    atexit(car_cleanup);
    signal(SIGINT, sigint_handler);
    pit_timer.init_ms(10, pit_callback);

    // ---------- 4. 逐飞助手调试链路（可选） ----------
#if TCP_DEBUG
    if (tcp_client_dev.init(TCP_SERVER_IP, TCP_PORT) == 0)
    {
        printf("tcp client ok\n");
        seekfree_assistant_interface_init(tcp_send_wrap, tcp_read_wrap);
        seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_copy[0], UVC_WIDTH, UVC_HEIGHT);
        seekfree_assistant_camera_boundary_config(X_BOUNDARY, UVC_HEIGHT,
                                                  x1_boundary, x2_boundary, x3_boundary,
                                                  NULL, NULL, NULL);
    }
    else
    {
        printf("tcp client fail\n");
    }
#endif

    // ---------- 5. 主循环：采图 -> 巡线 -> 方向环 -> 差速输出 ----------
    uint32 frame_cnt = 0;
    while (1)
    {
        // 阻塞等待新帧
        if (uvc_dev.wait_image_refresh() < 0)
        {
            continue;
        }
        cv::Mat frame = uvc_dev.get_frame_mjpg();
        if (frame.empty())
        {
            continue;
        }

        // 图像巡线
        int ret = image_process(frame, &line_info);

        if (ret == 0)
        {
            // ---------- 方向环（PD） ----------
            lost_frames = 0;
            float error = (float)STEER_DIR * line_info.error;   // STEER_DIR：偏差方向修正
            float derror = (error - last_error) / STEER_DT_NORM;

            steer_out = STEER_KP * error + STEER_KD * derror;
            if (steer_out >  STEER_OUT_LIMIT) steer_out =  STEER_OUT_LIMIT;
            if (steer_out < -STEER_OUT_LIMIT) steer_out = -STEER_OUT_LIMIT;
            last_error = error;
        }
        else
        {
            // ---------- 双边丢线保护 ----------
            lost_frames++;
            if (lost_frames >= LOST_STOP_FRAMES)
            {
                motor_stop();
                last_error = 0.0f;
                steer_out  = 0.0f;
                printf("[LOST] no line for %d frames, stop\n", lost_frames);
            }
        }

        // ---------- 差速输出（左轮 +steer / 右轮 -steer） ----------
        if (lost_frames < LOST_STOP_FRAMES)
        {
            int16 left  = (int16)(base_duty + steer_out);
            int16 right = (int16)(base_duty - steer_out);
            if (left  >  MOTOR_DUTY_MAX) left  =  MOTOR_DUTY_MAX;
            if (left  < -MOTOR_DUTY_MAX) left  = -MOTOR_DUTY_MAX;
            if (right >  MOTOR_DUTY_MAX) right =  MOTOR_DUTY_MAX;
            if (right < -MOTOR_DUTY_MAX) right = -MOTOR_DUTY_MAX;
            motor_set_duty(left, right);
        }

#if TCP_DEBUG
        // 逐飞助手：灰度图 + 三条边线（左/中/右）
        uint8_t *gray = uvc_dev.get_gray_image_ptr();
        if (gray != NULL)
        {
            memcpy(image_copy[0], gray, UVC_WIDTH * UVC_HEIGHT);
        }
        for (int y = 0; y < UVC_HEIGHT; y++)
        {
            x1_boundary[y] = (line_info.left_lost[y] || line_info.left_line[y] < 0)
                                ? 0 : (uint8)line_info.left_line[y];
            x2_boundary[y] = (line_info.mid_line[y] < 0)
                                ? (uint8)(UVC_WIDTH / 2) : (uint8)line_info.mid_line[y];
            x3_boundary[y] = (line_info.right_lost[y] || line_info.right_line[y] < 0)
                                ? (uint8)(UVC_WIDTH - 1) : (uint8)line_info.right_line[y];
        }
        seekfree_assistant_camera_send();
#endif

#if DEBUG_PRINT
        frame_cnt++;
        if (frame_cnt % DEBUG_PRINT_FREQ == 0)
        {
            printf("err=%6.1f steer=%6.1f base=%5.1f Lspd=%5.2f Rspd=%5.2f lost=%d top=%d\n",
                   line_info.error, steer_out, base_duty,
                   (double)encoder_get_left_speed(), (double)encoder_get_right_speed(),
                   line_info.lost, line_info.top_row);
        }
#endif
    }

    return 0;
}
