/*********************************************************************************************************************
 * 正交编码器测速模块
 * 用法：encoder_init() 初始化；在 PIT 定时器（10ms）回调里调用 encoder_periodic_10ms()；
 *       主循环通过 encoder_get_left_speed() 等获取速度（m/s）。
 ********************************************************************************************************************/
#ifndef __ENCODER_HPP__
#define __ENCODER_HPP__

#include "zf_common_typedef.hpp"
#include "config.hpp"

// 初始化编码器（清零计数）
void encoder_init(void);

// 周期调用（建议 PIT 10ms）：读取增量脉冲并换算速度
void encoder_periodic_10ms(void);

// 速度获取（m/s）
float encoder_get_left_speed(void);
float encoder_get_right_speed(void);
float encoder_get_avg_speed(void);

#endif // __ENCODER_HPP__
