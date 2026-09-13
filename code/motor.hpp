/*********************************************************************************************************************
 * 双电机差速驱动封装（DRV8701E：每路 1 路 DIR(GPIO) + 1 路 PWM）
 * 引脚（逐飞主板默认，设备树一致）：
 *   MOTOR1_DIR = GPIO73  (ZF_GPIO_MOTOR_1)   MOTOR1_PWM = GPIO86 (ZF_PWM_MOTOR_1)
 *   MOTOR2_DIR = GPIO76  (ZF_GPIO_MOTOR_2)   MOTOR2_PWM = GPIO87 (ZF_PWM_MOTOR_2)
 * 说明：本模块只做"占空比(百分数)->DIR+PWM"的映射，车速/差速计算在 main 中完成。
 ********************************************************************************************************************/
#ifndef __MOTOR_HPP__
#define __MOTOR_HPP__

#include "zf_common_typedef.hpp"
#include "config.hpp"

// 初始化电机（打开设备节点、读取 PWM 信息、输出零占空比）
void motor_init(void);

// 设置左右轮占空比，范围 [-MOTOR_DUTY_MAX, +MOTOR_DUTY_MAX]（百分数，正=前进 负=后退）
// 内部自动完成：符号->DIR 电平，绝对值->PWM 占空比
void motor_set_duty(int16 left_duty, int16 right_duty);

// 立即停止（双轮零占空比）
void motor_stop(void);

#endif // __MOTOR_HPP__
