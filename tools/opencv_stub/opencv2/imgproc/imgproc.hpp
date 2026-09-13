// OpenCV stub —— imgproc，仅供本机语法检查
#pragma once
#include <cstddef>

namespace cv {

// 基础类型
class Mat
{
public:
    Mat() : rows(0), cols(0), data(nullptr) {}
    Mat(int rows_, int cols_, int /*type*/, void *data_ = nullptr)
        : rows(rows_), cols(cols_), data(data_) {}
    Mat(const Mat &) = default;
    Mat &operator=(const Mat &) = default;
    ~Mat() {}

    bool empty() const { return rows <= 0 || cols <= 0; }
    void release() { rows = 0; cols = 0; data = nullptr; }
    Mat clone() const { return *this; }

    template <typename T>
    T *ptr(int y = 0) { return reinterpret_cast<T *>(static_cast<char *>(data) + y * cols * sizeof(T)); }
    template <typename T>
    const T *ptr(int y = 0) const { return reinterpret_cast<const T *>(static_cast<const char *>(data) + y * cols * sizeof(T)); }

    int rows;
    int cols;
    void *data;
};

struct Size
{
    Size() : width(0), height(0) {}
    Size(int w, int h) : width(w), height(h) {}
    int width;
    int height;
};

struct Point
{
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
    int x;
    int y;
};

struct Scalar
{
    Scalar() : v0(0), v1(0), v2(0) {}
    Scalar(double a, double b = 0, double c = 0) : v0(a), v1(b), v2(c) {}
    double v0, v1, v2;
};

// 摄像头句柄（uvc.hpp 成员使用，仅供语法检查）
class VideoCapture
{
public:
    VideoCapture() {}
    VideoCapture(const char *) {}
    ~VideoCapture() {}
    bool isOpened() const { return false; }
    void release() {}
};

// 枚举常量（值来自真实 OpenCV，仅用于语法检查）
enum { COLOR_BGR2GRAY = 6 };
enum { THRESH_BINARY = 0, THRESH_OTSU = 8 };

// 函数声明
void cvtColor(const Mat &src, Mat &dst, int code);
void GaussianBlur(const Mat &src, Mat &dst, Size ksize, double sigmaX);
double threshold(const Mat &src, Mat &dst, double thresh, double maxval, int type);
void bitwise_not(const Mat &src, Mat &dst);
void circle(Mat &img, Point center, int radius, const Scalar &color, int thickness);

} // namespace cv
