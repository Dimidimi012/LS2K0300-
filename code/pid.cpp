#include "pid.hpp"
#include <cmath>

static float pid_limit(float value, float max_abs)
{
    if (max_abs <= 0.0f) return value;
    if (value >  max_abs) return  max_abs;
    if (value < -max_abs) return -max_abs;
    return value;
}

void pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float out_limit)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_limit = out_limit;
    // 积分限幅 = out_limit / ki：让积分项单独就能把输出推到限幅值。
    // 若直接把 integral 限成 out_limit，则 I 项最大只有 ki*out_limit（远小于限幅），
    // 输出会被永久卡死在小值（如 0.15*40=6），带负载时动力严重不足。
    pid->integral_limit = (out_limit > 0.0f) ? ((ki > 0.0f) ? out_limit / ki : 0.0f) : 0.0f;

    pid->target = 0.0f;
    pid->feedback = 0.0f;
    pid->output = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
}

void pid_reset(pid_ctrl_t *pid)
{
    pid->output = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
}

void pid_set_target(pid_ctrl_t *pid, float target)
{
    pid->target = target;
}

void pid_set_feedback(pid_ctrl_t *pid, float feedback)
{
    pid->feedback = feedback;
}

float pid_update(pid_ctrl_t *pid)
{
    float error = pid->target - pid->feedback;

    // 比例项
    float P = pid->kp * error;

    // 积分项（带抗饱和：输出未达到限幅时才累加）
    if (fabsf(pid->output) < pid->out_limit)
    {
        pid->integral += error;
    }
    pid->integral = pid_limit(pid->integral, pid->integral_limit);
    float I = pid->ki * pid->integral;

    // 微分项（误差变化量）
    float D = pid->kd * (error - pid->last_error);

    // 总输出 + 限幅
    float out = pid_limit(P + I + D, pid->out_limit);

    pid->last_error = error;
    pid->output = out;
    return out;
}
