#include "encoder.hpp"
#include "zf_common_headfile.hpp"

// 编码器设备对象（正交编码器）
zf_driver_encoder encoder_quad_1(ZF_ENCODER_QUAD_1);
zf_driver_encoder encoder_quad_2(ZF_ENCODER_QUAD_2);

static float left_speed  = 0.0f;
static float right_speed = 0.0f;

// 每 10ms 的脉冲增量 -> 速度(m/s)
// v = delta_pulse * wheel_circumference / (pulse_per_rev * dt) / gear_ratio
static inline float pulse_to_speed(int16 delta_pulse, float dt)
{
    return (float)delta_pulse * WHEEL_CIRCUMFERENCE
         / (PULSE_PER_REV * dt)
         / GEAR_RATIO;
}

void encoder_init(void)
{
    encoder_quad_1.clear_count();
    encoder_quad_2.clear_count();
    left_speed  = 0.0f;
    right_speed = 0.0f;
}

void encoder_periodic_10ms(void)
{
    const float dt = 0.01f;

    // 每周期读取后清零，get_count() 即本周期增量（带符号）
    int16 l_pulse = encoder_quad_1.get_count();
    int16 r_pulse = encoder_quad_2.get_count();
    encoder_quad_1.clear_count();
    encoder_quad_2.clear_count();

    left_speed  = pulse_to_speed((int16)(ENCODER_DIR_L * l_pulse), dt);
    right_speed = pulse_to_speed((int16)(ENCODER_DIR_R * r_pulse), dt);
}

float encoder_get_left_speed(void)
{
    return left_speed;
}

float encoder_get_right_speed(void)
{
    return right_speed;
}

float encoder_get_avg_speed(void)
{
    return (left_speed + right_speed) * 0.5f;
}
