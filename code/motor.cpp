#include "motor.hpp"
#include "zf_common_headfile.hpp"

// PWM 设备对象
zf_driver_pwm motor_pwm_1(ZF_PWM_MOTOR_1);
zf_driver_pwm motor_pwm_2(ZF_PWM_MOTOR_2);

// DIR 设备对象
zf_driver_gpio motor_dir_1(ZF_GPIO_MOTOR_1, O_RDWR);
zf_driver_gpio motor_dir_2(ZF_GPIO_MOTOR_2, O_RDWR);

static struct pwm_info motor_pwm_1_info;
static struct pwm_info motor_pwm_2_info;

// 占空比百分数 -> 设备树占空比计数值
static inline uint16 duty_percent_to_raw(int16 percent, uint32 duty_max)
{
    int32 raw = (int32)percent * (int32)duty_max / 100;
    if (raw < 0)    raw = 0;
    if (raw > (int32)duty_max) raw = (int32)duty_max;
    return (uint16)raw;
}

void motor_init(void)
{
    motor_pwm_1.get_dev_info(&motor_pwm_1_info);
    motor_pwm_2.get_dev_info(&motor_pwm_2_info);
    motor_stop();
}

// 单轮设置：duty 为百分数（正负代表方向）
static void motor_set_one(zf_driver_pwm &pwm, zf_driver_gpio &dir,
                          int16 duty, uint32 duty_max, int16 dir_sign)
{
    int16 effective = (int16)(dir_sign * duty);   // 电机安装方向修正
    if (effective >= 0)
    {
        dir.set_level(1);                          // 正转
        pwm.set_duty(duty_percent_to_raw(effective, duty_max));
    }
    else
    {
        dir.set_level(0);                          // 反转
        pwm.set_duty(duty_percent_to_raw(-effective, duty_max));
    }
}

void motor_set_duty(int16 left_duty, int16 right_duty)
{
    motor_set_one(motor_pwm_1, motor_dir_1, left_duty,  motor_pwm_1_info.duty_max, MOTOR_DIR_L);
    motor_set_one(motor_pwm_2, motor_dir_2, right_duty, motor_pwm_2_info.duty_max, MOTOR_DIR_R);
}

void motor_stop(void)
{
    motor_pwm_1.set_duty(0);
    motor_pwm_2.set_duty(0);
}
