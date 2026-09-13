#include "image_process.hpp"

// 在 [x0, x1] 区间内查找最长连续白色段（row 为二值化行数据，255=白）
// 返回 false 表示没有长度 >= MIN_LINE_WIDTH 的白段
static bool find_longest_segment(const uint8 *row, int x0, int x1,
                                 int *out_l, int *out_r)
{
    if (x0 < 0) x0 = 0;
    if (x1 >= IMG_W) x1 = IMG_W - 1;
    if (x0 > x1) return false;

    int best_l = -1, best_r = -1, best_len = 0;
    int x = x0;
    while (x <= x1)
    {
        if (row[x] == 255)
        {
            int s = x;
            while (x <= x1 && row[x] == 255) x++;
            int len = x - s;
            if (len > best_len)
            {
                best_len = len;
                best_l = s;
                best_r = x - 1;
            }
        }
        else
        {
            x++;
        }
    }
    if (best_len < MIN_LINE_WIDTH) return false;
    *out_l = best_l;
    *out_r = best_r;
    return true;
}

int image_process(const cv::Mat &bgr, line_info_t *line)
{
    if (bgr.empty() || bgr.cols != IMG_W || bgr.rows != IMG_H)
    {
        line->lost = 1;
        return -1;
    }

    // ---------- 1. 灰度 + 滤波 ----------
    cv::Mat gray, bin;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
    if (GAUSS_KERNEL >= 3)
    {
        cv::GaussianBlur(gray, gray, cv::Size(GAUSS_KERNEL, GAUSS_KERNEL), 0);
    }

    // ---------- 2. 二值化 ----------
    if (USE_OTSU)
    {
        cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    }
    else
    {
        cv::threshold(gray, bin, FIXED_THRESHOLD, 255, cv::THRESH_BINARY);
    }
    if (!LINE_IS_WHITE)
    {
        cv::bitwise_not(bin, bin);   // 黑色为赛道时反色，统一按白色处理
    }

    // 初始化结果
    for (int y = 0; y < IMG_H; y++)
    {
        line->left_line[y]  = -1;
        line->right_line[y] = -1;
        line->mid_line[y]   = -1;
        line->left_lost[y]  = 1;
        line->right_lost[y] = 1;
    }
    line->lost = 1;
    line->error = 0.0f;

    int bot = ROI_ROW_BOTTOM;
    int top = ROI_ROW_TOP;
    if (bot < 0) bot = 0;
    if (top < 0) top = 0;
    if (bot > IMG_H - 1) bot = IMG_H - 1;
    if (top > bot) top = bot;
    line->bot_row = bot;

    // ---------- 3. 近端种子：在近端 5 行内统计白色赛道中心 ----------
    int seed_center = -1;
    int seed_cnt = 0;
    int seed_end = bot - 5;
    if (seed_end < top) seed_end = top;
    for (int y = bot; y >= seed_end; y--)
    {
        int l, r;
        if (find_longest_segment(bin.ptr<uint8>(y), 0, IMG_W - 1, &l, &r))
        {
            seed_center += (l + r) / 2;
            seed_cnt++;
        }
    }
    if (seed_cnt == 0)
    {
        // 近端完全看不到赛道（冲出赛道 / 遮挡）
        return -1;
    }
    seed_center /= seed_cnt;

    // ---------- 4. 逐行向上找边线 ----------
    int prev_mid  = seed_center;
    int prev_wide = STD_ROAD_WIDE;
    int y = bot;
    for (; y >= top; y--)
    {
        const uint8 *row = bin.ptr<uint8>(y);

        int wl = prev_mid - SEARCH_HALF;
        int wr = prev_mid + SEARCH_HALF;
        int l, r;
        bool found = find_longest_segment(row, wl, wr, &l, &r);

        if (!found)
        {
            // 窗口内没有白段：扩大到整行再找一次（弯道/丢线恢复）
            found = find_longest_segment(row, 0, IMG_W - 1, &l, &r);
            if (!found)
            {
                break;   // 双边丢线，停止向上
            }
        }

        // 简单赛宽保护：若本行赛宽与上一行差异过大（撞上干扰白块），保留与上一行重叠较大的一侧
        int wide = r - l;
        if (prev_wide > 0 && wide > prev_wide * 2 + 8)
        {
            int mid = prev_mid;
            int left_len  = mid - l;
            int right_len = r - mid;
            if (left_len >= right_len) r = mid;
            else                       l = mid;
            if (r - l < MIN_LINE_WIDTH) { break; }
            wide = r - l;
        }

        line->left_line[y]  = (int16)l;
        line->right_line[y] = (int16)r;
        line->mid_line[y]   = (int16)((l + r) / 2);
        line->left_lost[y]  = 0;
        line->right_lost[y] = 0;

        prev_mid  = line->mid_line[y];
        prev_wide = wide;
    }
    line->top_row = y + 1;      // 有效行最上端

    if (line->top_row > line->bot_row)
    {
        return -1;              // 一行都没找到
    }

    // ---------- 5. 近端加权偏差 ----------
    int rows = ERROR_ROWS;
    int valid = line->bot_row - line->top_row + 1;
    if (rows > valid) rows = valid;
    if (rows <= 0) return -1;

    float sum_w = 0.0f, sum_d = 0.0f;
    for (int i = 0; i < rows; i++)
    {
        int yy = line->bot_row - i;
        // 近端权重线性衰减：最近端行权重 ERROR_NEAR_WEIGHT，最远端 1.0（rows=1 时权重取 1）
        float t = (rows > 1) ? (float)i / (float)(rows - 1) : 1.0f;
        float w = 1.0f + (ERROR_NEAR_WEIGHT - 1.0f) * (1.0f - t);
        sum_d += w * ((float)line->mid_line[yy] - (float)IMG_CENTER_X);
        sum_w += w;
    }
    line->error = (sum_w > 0.0f) ? (sum_d / sum_w) : 0.0f;
    line->lost = 0;
    return 0;
}

// ---------- 调试绘制：把边线/中线画到彩色图上 ----------
void image_process_draw(cv::Mat &bgr, const line_info_t *line)
{
    if (bgr.empty() || line->bot_row < 0) return;

    for (int y = line->top_row; y <= line->bot_row; y++)
    {
        if (!line->left_lost[y] && line->left_line[y] >= 0)
            cv::circle(bgr, cv::Point(line->left_line[y], y), 1, cv::Scalar(0, 0, 255), -1);
        if (!line->right_lost[y] && line->right_line[y] >= 0)
            cv::circle(bgr, cv::Point(line->right_line[y], y), 1, cv::Scalar(0, 255, 0), -1);
        if (line->mid_line[y] >= 0)
            cv::circle(bgr, cv::Point(line->mid_line[y], y), 1, cv::Scalar(255, 255, 0), -1);
    }
}
