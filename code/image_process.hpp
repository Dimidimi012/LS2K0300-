/*********************************************************************************************************************
 * 传统图像处理巡线模块
 * 流程：BGR -> 灰度 -> 高斯滤波 -> 大津/固定阈值二值化 -> 近端种子定位 ->
 *       逐行窗口找最长白色段(赛道) -> 左右边线/中线 -> 近端加权偏差
 * 说明：输入为 UVC 摄像头 get_frame_mjpg() 得到的 BGR 图像（尺寸须与 config.hpp 中 IMG_W/IMG_H 一致）。
 ********************************************************************************************************************/
#ifndef __IMAGE_PROCESS_HPP__
#define __IMAGE_PROCESS_HPP__

#include <opencv2/opencv.hpp>
#include "config.hpp"

// 单帧巡线结果
typedef struct
{
    int16 left_line[IMG_H];     // 每行左边线 x（-1 表示该行左丢线）
    int16 right_line[IMG_H];    // 每行右边线 x
    int16 mid_line[IMG_H];      // 每行中线 x
    uint8 left_lost[IMG_H];     // 左丢线标志
    uint8 right_lost[IMG_H];    // 右丢线标志
    int   top_row;              // 有效行最上端（再往上双边丢线）
    int   bot_row;              // 有效行最下端（ROI_ROW_BOTTOM）
    float error;                // 偏差（像素）：中线相对图像中心，右偏为正
    uint8 lost;                 // 1=本帧双边丢线（error 无效）
} line_info_t;

// 执行一帧巡线。返回 0=成功（error 有效），-1=失败（双边丢线）
int image_process(const cv::Mat &bgr, line_info_t *line);

// 将结果画到彩色图上（调试用，可选）
void image_process_draw(cv::Mat &bgr, const line_info_t *line);

#endif // __IMAGE_PROCESS_HPP__
