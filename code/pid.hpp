/*********************************************************************************************************************
 * PID 控制器（位置式 + 抗积分饱和）
 * 用于方向环（位置式）与速度环（可选用位置式或增量式，此处统一位置式）
 ********************************************************************************************************************/
#ifndef __PID_HPP__
#define __PID_HPP__

#include "zf_common_typedef.hpp"

typedef struct
{
    float kp;               // 比例系数
    float ki;               // 积分系数
    float kd;               // 微分系数

    float target;           // 目标值
    float feedback;         // 反馈值
    float output;           // 输出值

    float last_error;       // 上一次误差
    float integral;         // 积分累积

    float out_limit;        // 输出限幅（绝对值，<=0 不限幅）
    float integral_limit;   // 积分限幅（绝对值，<=0 与 out_limit 相同）
} pid_ctrl_t;

void  pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float out_limit);
void  pid_reset(pid_ctrl_t *pid);
void  pid_set_target(pid_ctrl_t *pid, float target);
void  pid_set_feedback(pid_ctrl_t *pid, float feedback);
float pid_update(pid_ctrl_t *pid);

// 一步到位计算：返回控制输出
static inline float pid_compute(pid_ctrl_t *pid, float target, float feedback)
{
    pid_set_target(pid, target);
    pid_set_feedback(pid, feedback);
    return pid_update(pid);
}

#endif // __PID_HPP__
